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
#include <string.h>

#if WIN32
#   include <windows.h>
#else
#   include <sys/time.h>
#endif

#ifndef thread_local
#   if _MSC_VER
#       define thread_local __declspec(thread)
#   endif
#endif

namespace fibers11
{

static thread_local fiberset_t* g_current_fiberset = 0;

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

void dispatch_fiber_t(intptr_t p)
{
    auto fiber = reinterpret_cast<fiber_t*>(p);
    fiber->state = fiber_t::FIBER_RUNNING;
    fiber->callable();
    fiber->state = fiber_t::FIBER_FINISHED;
    boost::context::jump_fcontext(&g_current_fiberset->current->fiber_context, g_current_fiberset->scheduler_context, 1, false);
}

fiber_t::fiber_t(std::function<void()> callable_, bool preserve_fpu_, size_t stack_size_, std::shared_ptr<uint8_t> stack_)
    : callable(callable_)
    , preserve_fpu(preserve_fpu_)
    , stack_size(stack_size_)
    , stack(stack_)
{
    if (!stack)
    {
        stack = std::shared_ptr<uint8_t>(new uint8_t[stack_size], [](uint8_t* p) {
            delete[] p;
        });
    }
    memset(stack.get(), 0, stack_size);
    fiber_context = boost::context::make_fcontext(stack.get() + stack_size, stack_size, &dispatch_fiber_t);
}

void fiber_t::terminate()
{
    state = fiber_t::FIBER_TERMINATING;
}

void fiberset_t::add(fiber_t& fiber)
{
    active_fibers.push_back(&fiber);
}

size_t fiberset_t::schedule()
{
    g_current_fiberset = this;
    size_t min_sleep = 1;
    for (auto it = active_fibers.begin(); it != active_fibers.end();)
    {
        current = (*it);
        // Coroutine executed once already and yielded. Here we check if coroutine is still sleeping.
        size_t time_slept = get_tick_count() - current->last_yield;
        if (time_slept >= current->sleep_time)
        {
            // Coroutine is past it's sleep time therefore we resume execution.
            if (current->state == fiber_t::FIBER_FINISHED ||
                boost::context::jump_fcontext(&scheduler_context, current->fiber_context, reinterpret_cast<intptr_t>(current), false))
            {
                // Fiber finished execution or was terminated
                it = active_fibers.erase(it);
                current = 0;
            }
            else
                ++it;
        }
        else
        {
            min_sleep = std::min(min_sleep, time_slept);
            ++it;
        }
    }
    g_current_fiberset = 0;
    return min_sleep;
}

size_t fiberset_t::count()
{
    return active_fibers.size();
}

void fiberset_t::run()
{
    while (count())
    {
        sleep_ms(schedule());
    }
}

bool yield(size_t sleep, boost::context::fcontext_t into)
{
    auto current = g_current_fiberset->current;
    current->sleep_time = sleep;
    current->last_yield = get_tick_count();
    boost::context::jump_fcontext(&current->fiber_context, into, 0, current->preserve_fpu);
    return current->state == fiber_t::FIBER_RUNNING;
}

bool yield(size_t sleep)
{
    return yield(sleep, g_current_fiberset->scheduler_context);
}

bool yield(fiber_t& into)
{
    return yield(0, into.fiber_context);
}

}
