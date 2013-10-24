// The MIT License (MIT)
//
// Copyright (c) 2013 Alex Kozlov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef ASPLITE_ASPHOST_H_562542B9_D0D5_4362_9B23_E9E1CABF9903
#define ASPLITE_ASPHOST_H_562542B9_D0D5_4362_9B23_E9E1CABF9903

#include <string>
#include <vector>

class IAspliteConnection {
public:
    virtual ~IAspliteConnection() {}

    virtual std::string GetHeader(const char *name) const = 0;
    virtual void SendError(int code, const char *msg,
            const std::vector<std::string> *headers = NULL,
            const char *body = NULL) = 0;

    virtual int Read(void *buffer, size_t buffer_size) = 0;
};

#endif // ASPLITE_ASPHOST_H_562542B9_D0D5_4362_9B23_E9E1CABF9903
