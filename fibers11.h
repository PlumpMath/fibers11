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
#include <boost/context/all.hpp>

namespace fibers11
{
    const size_t default_stack_size = 64 * 1024;

    struct fiber_t
    {
        enum
        {
            FIBER_CREATED,
            FIBER_RUNNING,
            FIBER_TERMINATING,
            FIBER_FINISHED
        } state = FIBER_CREATED;

        std::function<void()> callable;
        std::shared_ptr<uint8_t> stack = 0;
        boost::context::fcontext_t fiber_context = 0;
        size_t last_yield = 0;
        size_t sleep_time = 0;
        size_t stack_size = 0;
        bool preserve_fpu = false;

        // Creates a stackful fiber which will execute `callable`. Stack size can be specified, defaults ot 64K.
        // Custom stack may be passed to the function. Correct stack size must still be provided.
        // Callable may be c/c++ function pointer, c++ lambda with captured context or result of std::bind().
        fiber_t(std::function<void()> callable, bool preserve_fpu=false, size_t stack_size = default_stack_size,
                std::shared_ptr<uint8_t> stack=0);

        // Next time yield() is called it will return `false`, then it is up to coroutine to terminate execution.
        void terminate();
    };

    struct fiberset_t
    {
        std::list<fiber_t*> active_fibers;
        fiber_t* current = 0;
        boost::context::fcontext_t scheduler_context = 0;

        // Adds fiber to fiberset for scheduling execution.
        void add(fiber_t& fiber);

        // Returns number of active fibers.
        size_t count();

        // Checks active fibers and runs them if they are not sleeping. This is done once. Returns number of milliseconds
        // left until next earliest fiber.
        size_t schedule();

        // Executes fibers until all they all terminate.
        void run();
    };

    // Yields execution. It will resume no sooner than after `sleep` ms.
    bool yield(size_t sleep=0);

    // Yields execution. It will immediately start executing fiber `into` bypassing scheduler altogeather.
    bool yield(fiber_t& into);
}

