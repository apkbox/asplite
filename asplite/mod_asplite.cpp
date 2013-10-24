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

#include <stdio.h>

#include "asplite/asplite.h"
#include "mongoose/mongoose.h"


void asp_write(void *user_data, const char *text)
{
    struct AspPageContext *context = (struct AspPageContext *)user_data;
    struct mg_connection *conn = (struct mg_connection *)context->user_data;
    mg_write(conn, text, strlen(text));
}


void asp_error(void *user_data, const char *text)
{
    struct AspPageContext *context = (struct AspPageContext *)user_data;
    struct mg_connection *conn = (struct mg_connection *)context->user_data;
    cry(conn, text);
}


struct FormItem {
    struct string *name;
    struct string *content_type;
    struct string *content_disposition;
    struct string *file_name;
    struct string *value;
    int is_file;
};


static char *FindString(char *buf, size_t len,
        const char *str, size_t str_len)
{
    char *p;

    if (len < str_len)
        return NULL;

    for (p = buf; p < (buf + (len - str_len)); p++) {
        if (memcmp(p, str, str_len) == 0) {
            return p;
        }
    }

    return NULL;
}


static char *FindContent(char *buf, size_t len)
{
    return FindString(buf, len, "\r\n\r\n", 4);
}


// Assumes that leading and trailing whitspace were removed,
// including trailing CRLF
static int ParseContentTypeHeader(const char *value,
                                  struct string **content_type,
                                  struct string **boundary)
{
    int i, found = 0;
    struct strlist *v = str_split_sz(value, "; ", 0);

    *content_type = NULL;
    *boundary = NULL;

    if (strlist_size(v) == 0)
        return 0;

    *content_type = strlist_remove(v, 0);

    for (i = 0; i < strlist_size(v); i++) {
        struct strlist *name_value_pair = str_split(
            strlist_get(v, i), "=", 0);

        if (strlist_size(name_value_pair) == 2) {
            if (str_cmp(strlist_get(name_value_pair, 0), "boundary") == 0) {
                *boundary = str_unquote(
                    strlist_remove(name_value_pair, 1), '"');
                strlist_delete(name_value_pair);
                found = 1;
                break;
            }
        }

        strlist_delete(name_value_pair);
    }

    strlist_delete(v);
    return found;
}


static int ExtractContentDispositionParameters(const char *data,
                                               struct FormItem *form_item)
{
    size_t i;
    int result = 1;
    
    struct strlist *params = str_split_sz(data, "; ", 0);
    if (strlist_size(params) > 0) {
        form_item->content_disposition = strlist_remove(params, 0);
        if (str_cmp(form_item->content_disposition, "form-data") == 0) {
            for (i = 0; i < strlist_size(params); i++) {
                struct strlist *pair = str_split(strlist_get(params, i), "=", 0);
                if (strlist_size(pair) == 2) {
                    if (str_cmp(strlist_get(pair, 0), "name") == 0) {
                        form_item->name = strlist_remove(pair, 1);
                        str_unquote(form_item->name, '"');
                    }
                    else if (str_cmp(strlist_get(pair, 0), "file") == 0) {
                        form_item->file_name = strlist_remove(pair, 1);
                        str_unquote(form_item->file_name, '"');
                        form_item->is_file = 1;
                    }
                }
                strlist_delete(pair);
            }
        }
        else {
            // unexpected content disposition
            // TODO: the other content disposition is 'file'.
            result = 0;
        }
    }
    else {
        // no parameters
        result = 0;
    }

    strlist_delete(params);

    return result;
}


static int ProcessPartHeaders(struct strlist *headers,
                              struct FormItem *form_item)
{
    int i;

    // Go through the headers and find Content-Disposition
    for (i = 0; i < strlist_size(headers); i++) {
        struct string *header = strlist_get(headers, i);
        char *s = str_get(header);
        if (strncmp(s, "Content-Disposition:", 20) == 0) {
            s += 20;
            while (*s == ' ')
                s++;

            ExtractContentDispositionParameters(s, form_item);
        }
        else if (strncmp(s, "Content-Type:", 12) == 0) {
            struct string *content_type = NULL;
            struct string *boundary = NULL;
        
            s += 12;
            while (*s == ' ')
                s++;

            ParseContentTypeHeader(s, &content_type, &boundary);
            form_item->content_type = content_type;
            // TODO: Do not handle boundary yet.
            str_delete(boundary);
        }
    }

    return 1;
}


static int ProcessMultipartFormData(struct mg_connection *conn,
                                    const char *boundary,
                                    const char *download_dir,
                                    struct string **directory,
                                    struct FormItem *form_items)
{
    static const kDefaultBufferSize = 0x4000;
    size_t buf_size;
    char *buf = NULL;
    size_t len = 0;
    struct FormItem *current_item = NULL;
    FILE *current_file = NULL;
    int form_item_index = 0;
    struct string *bd = str_new_sz("\r\n--", 4);
    bd = str_append(bd, boundary, -1);

    buf_size = str_length(bd) < kDefaultBufferSize ?
            kDefaultBufferSize : str_length(bd) + kDefaultBufferSize;

    buf = malloc(buf_size);

    // The first CRLF was consumed during HTTP request parsing,
    // so we stuff it here artificially.
    buf[0] = '\r';
    buf[1] = '\n';
    len = 2;

    while (1) {
        char *part;
        char *content;
        int read;

        read = mg_read(conn, buf + len, buf_size - len);
        if (read <= 0 && len == 0)
            break;

        len += read;

        part = FindString(buf, len, str_get(bd), str_length(bd));
        if (part != NULL) {
            if (current_item != NULL && (part - buf) > 0) {
                // TODO: current_item->Write(buf, part - buf);
                len -= part - buf;
                memmove(buf, part, len);
                continue;   // Fill the buffer.
            }

            part += str_length(bd);

            // TODO: Check that we have enough data in the buffer
            if (part[0] == '-' || part[1] == '-')
                break;   // Closing boundary marker

            if (part[0] != '\r' || part[1] != '\n')
                break;   // Unexpected.

            part += 2;   // skip CRLF

            content = FindContent(part, len - (part - buf));
            if (content == NULL)   // Too many headers
                break;

            current_item = &form_items[form_item_index++];

            if (content > part) {  // Are there headers ?
                struct strlist *part_headers;

                // terminate headers part
                *content = 0;
                part_headers = str_split_sz(part, "\r\n", 0);
                ProcessPartHeaders(part_headers, form_items);
                strlist_delete(part_headers);

                if (form_items->is_file) {
                    if (current_file) {
                        fclose(current_file);
                        current_file = NULL;
                    }

                    if (form_items->file_name != NULL) {
                        str_insert(form_items->file_name, 0, *directory);
                    }
                    else {
                        form_items->file_name = str_new_sz(*directory, -1);
                    }

                    current_file = fopen(form_items->file_name, "wb");
                }

            }

            content += 4;  // skip CRLF to content

            len -= content - buf;
            memmove(buf, content, len);
        }
        else {
            char *possible_boundary = FindString(buf, len, "\r\n", 2);
            if (possible_boundary == NULL) {
                // TODO: current_item->Write(buf, len);
                len = 0;
            }
            else {
                // TODO: current_item->Write(buf, possible_boundary - buf);
                len -= possible_boundary - buf;
            }
        }
    }

    str_delete(bd);
    free(buf);

    return 0;
}


static int handle_asp_request(struct mg_connection *conn)
{
    void *p = NULL;
    int handled = 0;

    if (strcmp(mg_get_request_info(conn)->request_method, "POST") == 0) {
        ProcessPostRequest(conn);
    }
    else if (strcmp(mg_get_request_info(conn)->request_method, "GET") == 0) {
        ; // default
    }
    else {
        // TODO: Should send 'Allow: GET, POST' header
        send_http_error(conn, 405, "Method Not Allowed",
                "%s method not allowed",
                mg_get_request_info(conn)->request_method);
    }

    // We need both mg_stat to get file size, and mg_fopen to get fd
    if (!mg_stat(conn, path, filep) || !mg_fopen(conn, path, "r", filep)) {
        send_http_error(conn, 500, http_500_error, "File [%s] not found", path);
    }
    else {
        lua_State *L;
        struct AspPageContext context;

        L = luaL_newstate();

        context.write_func = asp_write;
        context.error_func = asp_error;
        context.log_func = asp_write;
        context.user_data = conn;
        context.request.request_method = conn->request_info.request_method;
        context.request.query_string = conn->request_info.query_string;
        context.engine_config.root_path = conn->ctx->config[DOCUMENT_ROOT];
        context.engine_config.cache_path = conn->ctx->config[DOCUMENT_ROOT];
        context.engine_config.cache_lua = 0;
        context.engine_config.cache_luac = 1;

        ExeciteAspPage(L, path, &context);

        lua_close(L);
    }

    if (p != NULL)
        munmap(p, filep->size);

    mg_fclose(filep);

    return error;
}
