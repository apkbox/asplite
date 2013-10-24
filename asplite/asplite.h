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

#ifndef ASPLITE_ASPLITE_H_562542B9_D0D5_4362_9B23_E9E1CABF9903
#define ASPLITE_ASPLITE_H_562542B9_D0D5_4362_9B23_E9E1CABF9903

#include <string>

#include "lua/lua.hpp"


class AspliteConfig {
public:
    bool cache_lua;
    bool cache_luac;
    std::string cache_directory;
    std::string upload_directory;
};


struct AspEngineConfig {
    int cache_lua;
    int cache_luac;

    // If cache_lua or cache_luac is specified,
    // then both root_path and cache_path are required.

    // Document root path.
    const char *root_path;
    // Cache directory path.
    const char *cache_path;
    // Upload path
    const char *upload_path;
};


typedef void (*asplite_WriteCallback)(void *user_data, const char *text);


struct AspPageContext
{
    asplite_WriteCallback write_func;
    asplite_WriteCallback error_func;
    asplite_WriteCallback log_func;
    void *user_data;
    struct AspEngineConfig engine_config;
    struct {
        const char *request_method;
        const char *query_string;
    } request;
};


// Compiles |asp_file| according to options specified in |config|.
// If |L| specified, then compiled page is left in the state.
// This parameter can be null.
// Returns 0 if compilation suceeded and NULL in |error_message| if specified.
// Returns a non-zero error code and an error message in |error_message|.
// If |error_message| is not NULL, then user must free the the returned
// string with free function.
int CompileAspPage(lua_State *L, const char *asp_path,
        const struct AspEngineConfig *config, char **error_message);

void ExeciteAspPage(lua_State *L, const char *asp_page,
        const struct AspPageContext *context);

bool IsAspliteOption(const std::string &option);

bool SetAspliteOption(AspliteConfig *asplite,
                      const std::string &option,
                      const std::string &value);

int luaopen_asplite(lua_State *L);


#endif // ASPLITE_ASPLITE_H_562542B9_D0D5_4362_9B23_E9E1CABF9903
