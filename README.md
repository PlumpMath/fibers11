fibers11 - Stackful coroutines for c++11
========================================

* Stackful - each coroutine gets it's own stack.
* Ability to specify stack size or custom stack.
* Ability to suspend coroutines for specified amount of time.
* Uses setjmp() and longjmp().
* Runs c++11 std::function<> as fibers, including lambdas capturing context as references.
* Supports architectures: x86, x86_64 and ARM (android).
* Supported compilers: GNU GCC.
* License: MIT

Author: Rokas Kupstys
