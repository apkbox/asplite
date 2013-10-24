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

#include <cassert>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string>

#include "asplite/asplite.h"
#include "mongoose/mongoose.h"

#include "asplite/asphost.h"


struct FormItem {
    std::string name;
    std::string content_type;
    std::string content_disposition;
    std::string file_name;
    std::string value;
    bool is_file;
};


template<typename STR>
static size_t TokenizeT(const STR& str,
                        const STR& delimiters,
                        std::vector<STR> *tokens)
{
    tokens->clear();

    typename STR::size_type start = str.find_first_not_of(delimiters);
    while (start != STR::npos) {
        typename STR::size_type end = str.find_first_of(delimiters, start + 1);
        if (end == STR::npos) {
            tokens->push_back(str.substr(start));
            break;
        }
        else {
            tokens->push_back(str.substr(start, end - start));
            start = str.find_first_not_of(delimiters, end + 1);
        }
    }

    return tokens->size();
}


size_t Tokenize(const std::string& str,
                const std::string& delimiters,
                std::vector<std::string> *tokens)
{
    return TokenizeT(str, delimiters, tokens);
}


size_t FindSpan(const char *begin, const char *end, const char *delimiters)
{
    const char *p;

    for (p = begin; p < end; p++) {
        if (strchr(delimiters, *p))
            break;
    }

    return p - begin;
}


static size_t TokenizeBuffer(const char *buf, size_t buf_size,
                             const char *delimiters,
                             std::vector<std::string> *tokens)
{
    const char *p = buf;
    const char *end = buf + buf_size;

    while (p < end) {
        size_t span = FindSpan(p, end, delimiters);
        if (span == 0) {
            p++;
            continue;
        }

        tokens->push_back(std::string(p, span));
        p += span + 1;
    }

    return tokens->size();
}


static ptrdiff_t FindStringInBuffer(char *buf, size_t len,
                                const std::string &str)
{
    if (len < str.length())
        return -1;

    for (char *p = buf; p < (buf + (len - str.length())); p++) {
        if (memcmp(p, str.data(), str.length()) == 0) {
            return p - buf;
        }
    }

    return -1;
}


std::string UnquoteString(const std::string &str)
{
    const char kQuote = '"';

    if (str[0] != kQuote || str.length() < 2)
        return str;

    if (str[0] != str[str.length() - 1])
        return str;

    std::string s(str);
    s.erase(s.length() - 1, 1);
    s.erase(0, 1);
    return s;
}


// Parses Content-Type header value returning content type
// and boundary parameter value if present.
// Returns true if at least content type was found.
// The function assumes that leading and trailing whitspaces were removed,
// including trailing CRLF
bool ParseContentTypeHeader(const std::string value,
                            std::string *content_type,
                            std::string *boundary)
{
    content_type->clear();
    boundary->clear();

    std::vector<std::string> v;
    if (Tokenize(value, "; ", &v) == 0)
        return false;

    *content_type = v[0];
    if (content_type->empty())
        return false;

    v.erase(v.begin());

    for (auto iter = v.begin(); iter != v.end(); ++iter) {
        std::vector<std::string> name_value_pair;
        if (Tokenize(*iter, "=", &name_value_pair) == 2) {
            if (name_value_pair[0] == "boundary") {
                *boundary = name_value_pair[1];
                return true;
            }
        }
    }

    return true;
}


// Extracts Content-Disposition HTTP header parameters.
// |data| is value of Content-Disposition header.
static bool ExtractContentDispositionParameters(std::string data,
                                                struct FormItem *form_item)
{
    std::vector<std::string> params;
    if (Tokenize(data, ": ", &params) == 0) {
        // no parameters
        return false;
    }

    std::string content_disposition = params[0];
    params.erase(params.begin());

    form_item->content_disposition = content_disposition;

    if (content_disposition != "form-data") {
        // unexpected content disposition
        // TODO: the other content disposition is 'file'.
        return false;
    }

    for (auto iter = params.begin(); iter != params.end(); ++iter) {
        std::vector<std::string> pair;
        if (Tokenize(*iter, "=", &pair) != 2)
            continue;

        if (pair[0] == "name") {
            form_item->name = UnquoteString(pair[1]);
        }
        else if (pair[0] == "filename") {
            form_item->file_name = UnquoteString(pair[1]);
            form_item->is_file = 1;
        }
    }

    return true;
}


std::string GenerateUniqueFileName()
{
    // TODO: Do better.
    static int index = 0;
    std::ostringstream s;
    s << "file" << std::setw(8) << std::setfill('0') << index;
    return s.str();
}


static bool ProcessPartHeaders(const std::vector<std::string> &headers,
                               struct FormItem *form_item)
{
    // Go through the headers and find Content-Disposition
    for (auto iter = headers.begin(); iter != headers.end(); ++iter) {
        if (strncmp(iter->c_str(), "Content-Disposition:", 20) == 0) {
            // TODO: Strip leading WS
            ExtractContentDispositionParameters(iter->substr(20), form_item);
        }
        else if (strncmp(iter->c_str(), "Content-Type:", 12) == 0) {
            std::string content_type;
            std::string boundary;

            // TODO: Strip leading WS
            ParseContentTypeHeader(iter->substr(12), &content_type, &boundary);
            form_item->content_type = content_type;
            // TODO: Do not handle boundary yet.
        }
    }

    return true;
}


int ProcessMultipartFormData(IAspliteConnection *conn,
                             const std::string &boundary,
                             const std::string &request_upload_directory,
                             struct FormItem *form_items)
{
    static const size_t kDefaultBufferSize = 0x4000;
    struct FormItem *current_item = NULL;
    int form_item_index = 0;
    FILE *current_file = NULL;

    std::string bd("\r\n--");
    bd += boundary;

    std::vector<char> buf(
            std::max(kDefaultBufferSize, bd.length() + kDefaultBufferSize));

    // The first CRLF was consumed during HTTP request parsing,
    // so we stuff it here artificially.
    buf[0] = '\r';
    buf[1] = '\n';
    size_t len = 2;

    while (true) {
        int read = conn->Read(&buf[len], buf.size() - len);
        if (read <= 0 && len == 0)
            break;

        len += read;

        ptrdiff_t part_offset = FindStringInBuffer(&buf[0], len, bd);
        if (part_offset >= 0) {
            if (current_item != NULL && part_offset > 0) {
                // TODO: current_item->Write(buf, part - buf);

                // Remove written part
                len -= part_offset;
                memmove(&buf[0], &buf[part_offset], len);
                continue;   // Fill the buffer.
            }

            assert(part_offset == 0);

            part_offset += bd.length();

            if (len < 2) {
                // Malformed request. The boundary must end with either
                //      --   (the last boundary)
                // or
                //      CRLF (next part)
                break;
            }

            if (buf[part_offset] == '-' && buf[part_offset + 1] == '-')
                break;   // Closing boundary marker

            if (buf[part_offset] != '\r' || buf[part_offset + 1] != '\n')
                break;   // Malformed request.

            part_offset += 2;   // skip CRLF

            ptrdiff_t content_offset = FindStringInBuffer(
                    &buf[part_offset], len - part_offset,
                    "\r\n\r\n");
            if (content_offset < 0)   // Too many headers
                break;

            current_item = &form_items[form_item_index++];

            // Check if there are headers
            if (content_offset > part_offset) {
                assert(part_offset <= content_offset);

                std::vector<std::string> part_headers;
                TokenizeBuffer(&buf[part_offset], content_offset - part_offset,
                        "\r\n", &part_headers);

                ProcessPartHeaders(part_headers, form_items);

                if (form_items->is_file) {
                    if (current_file != NULL) {
                        fclose(current_file);
                        current_file = NULL;
                    }

                    if (form_items->file_name.empty()) {
                        // No file name was provided by the browser
                        form_items->file_name = request_upload_directory +
                                "\\" + GenerateUniqueFileName();
                    }
                    else {
                        // File name provided in the request
                        form_items->file_name = request_upload_directory +
                                "\\" + form_items->file_name;
                    }

                    current_file = fopen(form_items->file_name.c_str(), "wb");
                }
            }

            content_offset += 4;  // skip CRLF to content

            len -= content_offset;
            memmove(&buf[0], &buf[content_offset], len);
        }
        else {
            // Check for possible boundary
            ptrdiff_t crlf_offset = FindStringInBuffer(&buf[0], len, "\r\n");
            if (crlf_offset < 0) {
                // TODO: current_item->Write(buf, len);
                len = 0;
            }
            else {
                // TODO: current_item->Write(buf, crlf_offset);
                len -= crlf_offset;
            }
        }
    }

    if (current_file != NULL) {
        fclose(current_file);
        current_file = NULL;
    }

    return 0;
}


void ProcessPostRequest(IAspliteConnection *conn)
{
    std::string content_type_header = conn->GetHeader("Content-Type");
    if (!content_type_header.empty()) {
        std::string content_type;
        std::string boundary;

        if (ParseContentTypeHeader(content_type_header, &content_type, &boundary)) {
            if (content_type == "multipart/form-data") {
                std::string request_directory;
                ProcessMultipartFormData(conn, boundary, ".", NULL);
            }
            else if (content_type == "application/x-www-form-urlencoded") {
                // TODO: Process form
            }
            else {
                conn->SendError(415, "Unsupported Media Type", NULL, content_type_header.c_str());
            }
        }
    }
}
