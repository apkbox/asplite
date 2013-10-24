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

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "lua/lua.h"
#include "lua/lauxlib.h"


static int url_decode(const char *src, int src_len, char *dst,
        int dst_len, int is_form_url_encoded)
{
    int i, j, a, b;
#define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')

    for (i = j = 0; i < src_len && j < dst_len - 1; i++, j++) {
        if (src[i] == '%' && i < src_len - 2 &&
                isxdigit(*(const unsigned char *)(src + i + 1)) &&
                isxdigit(*(const unsigned char *)(src + i + 2))) {
            a = tolower(*(const unsigned char *)(src + i + 1));
            b = tolower(*(const unsigned char *)(src + i + 2));
            dst[j] = (char)((HEXTOI(a) << 4) | HEXTOI(b));
            i += 2;
        }
        else if (is_form_url_encoded && src[i] == '+') {
            dst[j] = ' ';
        }
        else {
            dst[j] = src[i];
        }
    }

    dst[j] = '\0'; // Null-terminate the destination
#undef HEXTOI

    return i >= src_len ? j : -1;
}


int asplite_ParseQueryString(lua_State *L)
{
    int query_string_table;
    const char *p;
    const char *query_string = lua_tostring(L, -1);
    if (query_string == NULL)
        query_string = "";

    // QueryString table
    lua_newtable(L);
    query_string_table = lua_gettop(L);

    p = query_string;

    while (*p != '\0') {
        const char *name = p;
        int name_len = 0;
        const char *value = p;
        int value_len = 0;
        const char *eq;
        const char *amp;
        char *decoded_value = NULL;
        int decoded_value_len = 0;

        amp = strchr(name, '&');
        if (amp == NULL) {
            amp = name + strlen(name);
        }

        eq = strchr(name, '=');
        if (eq != NULL && eq < amp) {
            name_len = eq - name;
            value = eq + 1;
            value_len = amp - value;
        }
        else {
            // TODO: If '=' is missing, use nil instead of empty string for a value?
            name_len = amp - name;
            value = amp;
            value_len = 0;
        }

        if (value_len > 0) {
            decoded_value_len = value_len + 1;
            decoded_value = (char *)malloc(decoded_value_len);
            if (decoded_value == NULL) {
                lua_pushstring(L, "Out of memory while parsing query string.");
                lua_error(L);
                return 1;
            }

            decoded_value_len = url_decode(value, value_len,
                    decoded_value, decoded_value_len, 0);
            // should never be out of buffer space since
            // decoded value cannot be longer than encoded one.
            assert(decoded_value_len >= 0);
            if (decoded_value_len < 0) {
                lua_pushstring(L, "Internal error.");
                lua_error(L);
                return 1;
            }
        }
        else {
            decoded_value = "";
            decoded_value_len = 0;
        }

        // Push name
        lua_pushlstring(L, name, name_len);
        // Check if we already have the name
        lua_gettable(L, query_string_table);
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);   // Remove nil
            lua_newtable(L);   // Create new array
            lua_pushlstring(L, name, name_len);   // Push name
            lua_pushvalue(L, -2);   // Duplicate array
            lua_settable(L, query_string_table);  // QueryString[name] = {};
        }

        // Push value
        lua_pushlstring(L, decoded_value, decoded_value_len);
        lua_rawseti(L, -2, lua_rawlen(L, -2) + 1);
        lua_pop(L, 1);  // remove array

        if (decoded_value_len > 0)
            free(decoded_value);

        if (*amp == '&')
            amp++;

        p = amp;
    }

    return 1;
}


static const luaL_Reg asplite[] = {
    {"ParseQueryString", asplite_ParseQueryString},
    {NULL, NULL}
};


int luaopen_asplite(lua_State *L)
{
    luaL_newlib(L, asplite);
    lua_pushvalue(L, -1);
    lua_setglobal(L, "asplite");
    return 1;
}

