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

#ifndef ASPLITE_MEMBUF_H_DE76E377_F1BF_4939_AE8E_7BD93F040F44
#define ASPLITE_MEMBUF_H_DE76E377_F1BF_4939_AE8E_7BD93F040F44

#include <stdlib.h>

struct membuf {
    int fd;
    int read;
    int write;
    void *ptr;
    size_t capacity;
    size_t size;
};


#define membuf_begin(b)  ((b)->ptr)
#define membuf_end(b)  ((void *)(((char *)(b)->ptr) + (b)->size))
#define membuf_size(b)  ((b)->size)
#define membuf_capacity(b)  ((b)->capacity)
#define membuf_ptr(b)  membuf_end((b))

// dangerous without assert(s < capacity).
//#define membuf_set_size(b, s)  ((b)->size = (s))

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct membuf *membuf_create(size_t capacity, int fd, int read, int write);
int membuf_commit(struct membuf *buf);
int membuf_ensure(struct membuf *buf, size_t new_size);
int membuf_ensure_extra(struct membuf *buf, size_t size);
int membuf_append(struct membuf *buf, const void *data, size_t length);
void membuf_close(struct membuf *buf);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // ASPLITE_MEMBUF_H_DE76E377_F1BF_4939_AE8E_7BD93F040F44
