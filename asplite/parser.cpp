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

#include "asplite/parser.h"

#include <assert.h>
#include <stdlib.h>


// Searches char c in range between |begin| and |end| exclusive.
// The function updates new_lines with number of new lines encountered.
//    begin Points to the beginning of the range.
//    end   Point just past the last character in the range.
//    c     Character to search for.
//    new_lines Number of encountered new lines will be added here if specified.
// Returns pointer pointing to teh found character, otherwise NULL.
static const char *SearchCharacter(const char *begin, const char *end, char c,
        size_t *new_lines)
{
    size_t newline_count = 0;
    const char *p;

    assert(begin != NULL);
    assert(end != NULL);
    assert(begin <= end);

    for (p = begin; p < end; p++) {
        if (*p == '\n')
            newline_count++;

        if (*p == c) {
            if (new_lines != NULL)
                *new_lines += newline_count;
            return p;
        }
    }

    return NULL;
}


static const char *TryParseChunk(const char *buffer, const char *current,
        const char *end, ParserCallback callback, void *user_data,
        size_t *lineno)
{
    enum ChunkType type = InlineCodeRenderBlock;
    size_t available = end - current;
    size_t lines = 0;
    const char *iter;

    // There should be at least 4 characters that make
    // an empty block <%%>.
    // The longest prefix is <%--
    if (available < 4)
        return NULL;

    assert(*current == '<');
    if (*++current != '%')
        return NULL;

    switch(*++current) {
        case '@':
            type = DirectiveBlock;
            current++;
            break;

        case '=':
            type = InlineExpressionRenderBlock;
            current++;
            break;

        case ':':
            type = InlineExpressionRenderHtmlBlock;
            current++;
            break;

        case '-':
            if (*(current + 1) == '-') {
                type = ServerSideComment;
                current += 2;
            }
            break;
    }

    for (iter = current; iter < end; iter++) {
        if (*iter == '\n')
            lines++;

        if (type == ServerSideComment) {
            if (*iter == '-' && (iter + 3) < end &&
                    iter[1] == '-' && iter[2] == '%' && iter[3] == '>') {
                callback(buffer, type, *lineno,
                    current - buffer, (iter - 1) - buffer, user_data);
                *lineno += lines;
                return iter + 4;
            }
        }
        else if (*iter == '%') {
            if ((iter + 1) < end && iter[1] == '>') {
                callback(buffer, type, *lineno,
                        current - buffer, (iter - 1) - buffer, user_data);
                *lineno += lines;
                return iter + 2;
            }
        }
    }

    // TODO: If there is an unterminated server side block, raise an error.

    return NULL;
}


int ParseBuffer(const char *buffer, size_t buffer_size,
        ParserCallback callback, void *user_data)
{
    const char *current = buffer;
    const char *last = current;
    const char *end = buffer + buffer_size;
    const char *new_position;

    size_t lineno = 1;

    while (current < end) {
        size_t lines = 0;
        const char *at = SearchCharacter(current, end, '<', &lines);
        if (at != NULL) {
            callback(buffer, Content, lineno, last - buffer, at - buffer,
                    user_data);
            lineno += lines;
            current = at;
            last = at;

            new_position = TryParseChunk(buffer, current, end, callback,
                    user_data, &lineno);
            if (new_position != NULL) {
                // Point just past the directive
                last = current = new_position;
            }
            else {
                // Skip <. last is pointing to <
                current += 1;
            }
        }
        else {
            callback(buffer, Content, lineno, last - buffer, end - buffer,
                    user_data);
            break;
        }
    }

    return 0;
}
