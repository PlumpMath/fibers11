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

#undef NDEBUG
#include <assert.h>
#include <algorithm>
#include "fibers11.h"

std::vector<int32_t> numbers;

void function()
{
    int32_t i;
    for (i = 0; i < 5; i++) {
        numbers.push_back(i);
        printf("Fiber 1: %d\n", i);
        fibers::yield(100);
    }
    assert (i >= 0 && i <= 5);
}

int32_t main(int32_t argc, char *argv[])
{
    fibers::fiberset_t fs;

    fs.create(&function);

    int32_t captured_counter = 10;
    fs.create([&]() {
        for (; captured_counter >= 0 && captured_counter < 15; captured_counter++) {
            numbers.push_back(captured_counter);
            printf("Fiber 2: %d\n", captured_counter);
            fibers::yield(210);
        }
        assert (captured_counter >= 10 && captured_counter <= 15);
    });
    
    class BindDemo
    {
    public:
        void method(int32_t i)
        {
            for (; i < 105; i++) {
                numbers.push_back(i);
                printf("Fiber 3: %d\n", i);
                fibers::yield(320);
            }
            assert (i >= 100 && i <= 105);
        }
    } obj;
    fs.create(std::bind(&BindDemo::method, obj, 100));

    fs.run();

    std::vector<int32_t> expected = std::vector<int32_t>{0, 10, 100, 1, 2, 11, 3, 101, 4, 12, 13, 102, 14, 103, 104};
    assert (captured_counter == 15);
    assert (numbers.size() == expected.size());
    assert (std::equal(numbers.begin(), numbers.end(), expected.begin()));
    return 0;
}
