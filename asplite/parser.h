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

#ifndef ASPLITE_PARSER_H_F17D23E3_CAC5_4AB0_B47C_A457BE4E0173
#define ASPLITE_PARSER_H_F17D23E3_CAC5_4AB0_B47C_A457BE4E0173

#include <stdlib.h>


enum ChunkType
{
    Content,
    DirectiveBlock,         // parsed, but ignored and replaced with a comment
    CodeDeclarationBlock,   // not yet implemented
    InlineCodeRenderBlock,
    InlineExpressionRenderBlock,
    InlineExpressionRenderHtmlBlock,
    ServerSideComment
};


typedef void (*ParserCallback)(const char *buffer, enum ChunkType type,
        size_t lineno, size_t begin, size_t end, void *user_data);

int ParseBuffer(const char *buffer, size_t buffer_size,
        ParserCallback callback, void *user_data);

#endif // ASPLITE_PARSER_H_F17D23E3_CAC5_4AB0_B47C_A457BE4E0173
