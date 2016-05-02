#*
#* The MIT License (MIT)
#*
#* Copyright (c) 2016 Rokas Kupstys
#* 
#* Permission is hereby granted, free of charge, to any person obtaining a copy
#* of this software and associated documentation files (the "Software"), to 
#* deal in the Software without restriction, including without limitation the 
#* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
#* sell copies of the Software, and to permit persons to whom the Software is 
#* furnished to do so, subject to the following conditions:
#* 
#* The above copyright notice and this permission notice shall be included in 
#* all copies or substantial portions of the Software.
#* 
#* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
#* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
#* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
#* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
#* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
#* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
#* IN THE SOFTWARE.
#*

project(fibers11)
cmake_minimum_required(VERSION 2.8)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set (SOURCE_FILES fibers11.h fibers11.cpp)

if (NOT X86 AND NOT X64 AND NOT ARM)
    execute_process (COMMAND uname -m OUTPUT_VARIABLE ARCH)
    STRING (STRIP "${ARCH}" ARCH)
    if ("${ARCH}" STREQUAL "x86_64")
        set (X64 1)
    elseif ("${ARCH}" STREQUAL "x86")
        set (X86 1)
    elseif ("${ARCH}" MATCHES "^arm")
        set (ARM 1)
    endif ()
endif ()

if (X86)
    enable_language(ASM-ATT)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
    set(CMAKE_ASM-ATT_FLAGS "--32")
    set(PLATFORM_NAME "i386")
elseif (X64)
    enable_language(ASM-ATT)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
    set(CMAKE_ASM-ATT_FLAGS "--64")
    set(PLATFORM_NAME "x86_64")
elseif (ARM)
    enable_language(ASM)
    set(PLATFORM_NAME "arm")
else ()
    message (ERROR "Unsupported architecture")
endif ()

list(APPEND SOURCE_FILES
    platform/${PLATFORM_NAME}/setsp.s
    platform/${PLATFORM_NAME}/setjmp.s
    platform/${PLATFORM_NAME}/longjmp.s
)

add_library(fibers11 STATIC ${SOURCE_FILES})
if (NOT FIBERS11_NO_EXAMPLE)
    add_executable(fibers11_example example.cpp)
    target_link_libraries (fibers11_example fibers11)
endif ()
