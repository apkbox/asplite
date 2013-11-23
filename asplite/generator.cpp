// The MIT License (MIT)
//
// Copyright (c) 2013 Alex Kozlov
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "asplite/generator.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

static char *EscapeLuaString(const char *text,
                             size_t length,
                             size_t *encoded_length) {
  const char *s = text;
  const char *end = s + length;
  char *t;

  char *buffer = (char *)malloc(length * 2 + 1);
  if (buffer == NULL)
    return NULL;

  t = buffer;

  while (s < end) {
    switch (*s) {
      case '\0':
        *t++ = '\\';
        *t++ = '0';
        break;
      case '\a':
        *t++ = '\\';
        *t++ = 'a';
        break;
      case '\b':
        *t++ = '\\';
        *t++ = 'b';
        break;
      case '\f':
        *t++ = '\\';
        *t++ = 'f';
        break;
      case '\n':
        *t++ = '\\';
        // we need to produce real newline in the source code.
        // Lua treats \<real cr> as <real cr>
        *t++ = '\n';
        break;

      case '\r':
        *t++ = '\\';
        *t++ = 'r';
        break;
      case '\t':
        *t++ = '\\';
        *t++ = 't';
        break;
      case '\v':
        *t++ = '\\';
        *t++ = 'v';
        break;
      case '\\':
        *t++ = '\\';
        *t++ = '\\';
        break;
      case '\'':
        *t++ = '\\';
        *t++ = '\'';
        break;
      case '\"':
        *t++ = '\\';
        *t++ = '\"';
        break;

      default:
        *t++ = *s;
        break;
    }

    s++;
  }

  *encoded_length = t - buffer;

  return buffer;
}

void GenerateProlog(PageCodeGeneratorCallback callback, void *user_data) {
  const char kString[] = "function AspPage__()";
  callback(kString, sizeof(kString) - 1, user_data);
}

void GenerateEpilog(PageCodeGeneratorCallback callback, void *user_data) {
  const char kString[] = ";\nend\n";
  callback(kString, sizeof(kString) - 1, user_data);
}

// When modifying the function remember that you should not inject
// newline characters into generated code, otherwise the line numbering
// between .asp and .lua will be different.
void GenerateBodyChunk(PageCodeGeneratorCallback callback,
                       const char *buffer,
                       enum ChunkType chunk_type,
                       size_t lineno,
                       size_t begin,
                       size_t end,
                       void *user_data) {
  assert(end >= begin);

  switch (chunk_type) {
    case Content: {
      size_t encoded_length;
      char *encoded_content;

      // Ignore empty content.
      if ((end - begin) == 0)
        return;

      callback("Response.Write(\'", -1, user_data);
      encoded_content =
          EscapeLuaString(buffer + begin, end - begin, &encoded_length);

      // TODO: should handle this as an error.
      if (encoded_content == NULL)
        return;

      callback(encoded_content, encoded_length, user_data);
      free(encoded_content);

      callback("\');", -1, user_data);
      break;
    }

    case InlineCodeRenderBlock:
      callback(buffer + begin, end - begin, user_data);
      callback(";", -1, user_data);
      break;

    case InlineExpressionRenderBlock:
      callback("Response.Write(tostring(", -1, user_data);
      callback(buffer + begin, end - begin, user_data);
      callback("));", -1, user_data);
      break;

    case InlineExpressionRenderHtmlBlock:
      callback(
          "Response.Write(asplite.HtmlEscapeString(tostring(", -1, user_data);
      callback(buffer + begin, end - begin, user_data);
      callback(")));", -1, user_data);
      break;

    case DirectiveBlock:
    case ServerSideComment:
      // Use long comment to make collision less likely
      // with possible comments inside commented block.
      // We cannot use a single line comment, which would be preferrable,
      // because
      //      it is more expensive (need to do it for each line);
      //      disrupts line numbering, since the last line of comment
      //      must have newline character if followed by code.
      //          1: Some data<%-- MyComment --%><%= expr %>
      //      would produce:
      //          1: Write('Some data');-- MyComment
      //          2: Write(tostring(expr));
      //      Notice the line mismatch for the 'expr' expression.
      callback("--[=============================[", -1, user_data);
      callback(buffer + begin, end - begin, user_data);
      callback("]=============================]", -1, user_data);
      break;
  }
}
