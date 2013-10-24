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

#include "asplite/membuf.h"

#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct membuf *membuf_create(size_t capacity, int fd, int read, int write)
{
    struct membuf *buf = (struct membuf *)malloc(sizeof(struct membuf));
    buf->fd = fd;
    buf->read = read;
    buf->write = write;
    buf->ptr = malloc(capacity);
    memset(buf->ptr, 0, capacity);

    buf->size = 0;
    buf->capacity = capacity;

    if (fd && read) {
        _read(fd, buf->ptr, buf->capacity);
        buf->size = capacity;
    }

    return buf;
}


int membuf_commit(struct membuf *buf)
{
    int bytes;

    if (buf == NULL)
        return EINVAL;

    if (buf->fd >= 0 && buf->write) {
        buf->capacity = buf->size;

        // Setting file size of text files may truncate them
        // so we just start from scratch.
        if (_chsize(buf->fd, 0) == -1)
            return errno;

        _lseek(buf->fd, 0, SEEK_SET);

        bytes = _write(buf->fd, buf->ptr, buf->capacity);
        if (bytes == -1)
            return errno;
    }

    return 0;
}


int membuf_ensure(struct membuf *buf, size_t new_size)
{
    void *p = NULL;

    if (buf == NULL)
        return EINVAL;

    p = realloc(buf->ptr, new_size);
    if (p == NULL)
        return ENOMEM;

    buf->ptr = p;
    buf->capacity = new_size;
    return 0;
}


int membuf_ensure_extra(struct membuf *buf, size_t size)
{
    size_t available;

    if (buf == NULL)
        return EINVAL;

    available = buf->capacity - buf->size;
    if (available < size)
         return membuf_ensure(buf, buf->size + size);

    return 0;
}


int membuf_append(struct membuf *buf, const void *data, size_t length)
{
    int result;

    if (buf == NULL)
        return EINVAL;

    result = membuf_ensure_extra(buf, length);
    if (result)
        return result;

    memmove(((char *)buf->ptr) + buf->size, data, length);
    buf->size += length;

    return 0;
}


void membuf_close(struct membuf *buf)
{
    int result;

    if (buf == NULL)
        return;

    result = membuf_commit(buf);

    if (buf->fd >= 0)
        _close(buf->fd);

    free(buf->ptr);
    free(buf);
}
