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
#pragma once

#include <functional>
#include <memory>
#include <list>
#include <setjmp.h>


namespace fibers
{
    const size_t default_stack_size = 64 * 1024;

    struct fiber_t
    {
        enum
        {
            FIBER_CREATED,
            FIBER_RUNNING,
            FIBER_FINISHED
        } state = FIBER_CREATED;

        jmp_buf ctx;
        std::function<void()> fnc;
        size_t last_yield = 0;
        int32_t sleep_time = 0;
        size_t stack_size = 0;
        std::shared_ptr<void> stack = 0;
        void* sp = 0;
    };

    struct fiberset_t
    {
        std::list<fiber_t> active_fibers;
        fiber_t* current = 0;
        jmp_buf ctx_main;

        // This function operates on global fiberset.
        // Creates a stackful fiber which will execute `callable`. Stack size can be specified, defaults ot 64K. Custom stack may
        // be passed to the function. Correct stack size must still be provided.
        // Callable may be c/c++ function pointer, c++ lambda with captured context or result of std::bind().
        fiber_t* create(std::function<void()> callable, size_t stack_size = default_stack_size, std::shared_ptr<void> stack=0);

        // Returns number of active fibers.
        size_t num_active();

        // Checks active fibers and runs them if they are not sleeping. This is done once. Returns number of milliseconds
        // left until next earliest fiber.
        size_t schedule();

        // Terminates fiber. Warning: this does not do any stack unwinding. Resources may leak. Use only if you know what you are doing.
        void terminate(fiber_t* fiber);

        // Executes fibers until all they all terminate.
        void run();
    };

    // Yields execution. It will resume no sooner than after `sleep` ms.
    void yield(int32_t sleep=0);
}

