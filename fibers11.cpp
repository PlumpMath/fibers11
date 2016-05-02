/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Rokas Kupstys
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "fibers11.h"
#include <assert.h>
#include <string.h>

#if WIN32
#   include <windows.h>
#else
#   include <sys/time.h>
#   include <sys/auxv.h>
#endif


extern "C"
{
#if __arm__
    int __fibers_setjmp(jmp_buf&, uint32_t);
    void __fibers_longjmp(jmp_buf&, int32_t, uint32_t);
#   define __setjmp  __fibers_setjmp
#   define __longjmp __fibers_longjmp
#else
    int __fibers_setjmp(jmp_buf&);
    void __fibers_longjmp(jmp_buf&, int32_t);
#   define __setjmp(ctx, hwcap)       __fibers_setjmp(ctx)
#   define __longjmp(ctx, arg, hwcap) __fibers_longjmp(ctx, arg)
#endif
    void __fibers_setsp(void* old_sp, void* new_sp)
    #if __GNUC__ && __i386__        // Using cdecl on 32 bit architecture messes up esp after return
        __attribute__((fastcall))   // from __fibers_set_sp() because caller removes passed arguments from the stack.
    #endif                          // However by that time stack is already switched.
    ;
}

namespace fibers
{

static thread_local fiberset_t* g_current_fiberset = 0;
bool
#if __GNUC__
__attribute__((optimize("-O0")))
#endif
is_stack_growing_down()
{
    int32_t a = 0;
    auto fn = [](int32_t* p_a)
    {
        int32_t b = 0;
        return (p_a > &b);
    };
    return fn(&a);
}

static auto hwcap =
#if __linux__
    getauxval(AT_HWCAP)
#else
    0
#endif
;

static bool stack_grows_down = is_stack_growing_down();

size_t get_tick_count()
{
#ifdef WIN32
    return GetTickCount();
#else
    timeval now = { 0 };
    gettimeofday(&now, 0);
    return (size_t)(now.tv_sec * 1000 + now.tv_usec / 1000);
#endif
}

void sleep_ms(size_t ms)
{
#ifdef WIN32
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}

fiber_t* fiberset_t::create(std::function<void()> callable, size_t stack_size, std::shared_ptr<void> stack)
{
    active_fibers.push_back(fiber_t());
    fiber_t& fiber = active_fibers.back();
    fiber.fnc = callable;
    if (stack)
        fiber.stack = stack;
    else
    {
        fiber.stack = std::shared_ptr<void>(new char[stack_size], [](void* p) {
            delete[] (char*)p;
        });
    }
    fiber.stack_size = stack_size;
    memset(fiber.stack.get(), 0, stack_size);
    return &fiber;
}

size_t fiberset_t::schedule()
{
    g_current_fiberset = this;
    size_t min_sleep = 1;
    for (auto it = active_fibers.begin(); it != active_fibers.end();)
    {
        current = &(*it);
        switch(current->state)
        {
            case fiber_t::FIBER_CREATED:
            {
                // This is coroutine entry point
                if (__setjmp(ctx_main, hwcap) == 0)
                {
                    // Set new stack frame
                    auto stack_size = current->stack_size;
                    void* p = current->stack.get();
                    assert(p != 0);

                    // Remove 32 bytes from each end of stack
                    stack_size -= 64;
                    p = (void*)((size_t)p + 32);

                    // Align stack to 16 byte boundary
                    p = (void*)((size_t)p & ((~size_t(0)) << 4));
                    if (stack_grows_down)
                        p = (void*)((size_t)p + stack_size);               // only needed when stack grows down

                    // Now that coroutine stack is set we execute actual coroutine. This works
                    // because `current` is global variable and is not affected by stack switch.
                    current->state = fiber_t::FIBER_RUNNING;
                    __fibers_setsp(&current->sp, p);                       // Back up stack pointer as  local variables will be overwritten
                                                                           // When another coroutine gets started.
                    g_current_fiberset->current->fnc();                    // using g_current_fiberset because __fibers_set_sp()
                                                                           // invalidates local stack and accessing `current` looks
                                                                           // up `this` pointer on local stack.
                    __fibers_setsp(0, g_current_fiberset->current->sp);    // Restore backed up stack pointer

                    // This position is reached after coroutine terminates.
                    // `current` set to 0 signals for coroutine removal.
                    current->state = fiber_t::FIBER_FINISHED;
                    // Coroutine terminated, stack is freed and it is removed from list of active coroutines.
                    it = active_fibers.erase(it);
                    current = 0;
                }
                else
                    ++it;
                // This position is reached when started coroutine yields for the first time or
                // coroutine terminates.
                break;
            }
            case fiber_t::FIBER_RUNNING:
            {
                // Coroutine executed once already and yielded. Here we check if coroutine is still sleeping.
                size_t time_slept = get_tick_count() - current->last_yield;
                if (time_slept >= current->sleep_time)
                {
                    // Coroutine is past it's sleep time therefore we resume execution.
                    if (__setjmp(ctx_main, hwcap) == 0)
                        __longjmp(current->ctx, 1, hwcap);   // Value 1 indicates that we return from main loop
                }
                else
                    min_sleep = std::min(min_sleep, time_slept);
                ++it;
                break;
            }
            case fiber_t::FIBER_FINISHED:
            {
                // Executed when fiber was terminated
                it = active_fibers.erase(it);
                current = 0;
                break;
            }
            default:
                assert(0);
        }
    }
    g_current_fiberset = 0;
    return min_sleep;
}

size_t fiberset_t::num_active()
{
    return active_fibers.size();
}

void fiberset_t::run()
{
    while (num_active())
    {
        sleep_ms(schedule());
    }
}

void fiberset_t::terminate(fiber_t* fiber)
{
    fiber->state = fiber_t::FIBER_FINISHED;
}

void yield(int32_t sleep)
{
    auto current = g_current_fiberset->current;
    current->sleep_time = sleep;
    current->last_yield = get_tick_count();
    if (__setjmp(current->ctx, hwcap) == 0)                  // setjmp result 0 indicates that we set context therefore
        __longjmp(g_current_fiberset->ctx_main, 1, hwcap);   // we jump to main loop. result 1 indicates we return from
                                                             // main loop therefore we return into coroutine
}

}
