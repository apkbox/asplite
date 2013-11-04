// The MIT License (MIT)
//
//Copyright (c) 2013 Alex Kozlov
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
#include <vector>
#include <map>

#include "lua/lua.hpp"

#include "asplite/posted_file.h"
#include "asplite/file_collection.h"
#include "asplite/name_value_collection.h"


struct HttpHeader {
    HttpHeader(const char *name, const char *value)
        : name(name), value(value) {}

    std::string name;
    std::string value;
};


class IHttpServerAdapter {
public:
    virtual ~IHttpServerAdapter() {}

    virtual std::string MapPath(const std::string &uri) = 0;
    virtual std::string UriToFile(const std::string &uri) = 0;

    virtual void OnError(const char *text) = 0;
    virtual void WriteLog(const char *text) = 0;
};


class IHttpRequestAdapter {
public:
    virtual ~IHttpRequestAdapter() {}

    virtual std::string GetUri() = 0;
    virtual std::string GetQueryStringOld() = 0;
    virtual std::string GetRequestMethod() = 0;

    virtual const std::vector<HttpHeader> &GetHeaders() const = 0;
    virtual std::string GetHeader(const char *name) const = 0;

    virtual NameValueCollection &GetQueryString() = 0;
    virtual NameValueCollection &GetForm() = 0;
    virtual HttpFileCollection &GetFiles() = 0;

    virtual int Read(void *buffer, size_t buffer_size) = 0;
};


class IHttpResponseAdapter {
public:
    virtual ~IHttpResponseAdapter() {}

    virtual void Write(const char *data, size_t len) = 0;
    virtual void Write(const char *text) = 0;

    virtual void Respond405(const std::string &allow,
                            const std::string &extra) = 0;

    virtual void Respond415(const std::string &content_type) = 0;

//    virtual void SendError(int code, const char *msg,
//                           const std::vector<std::string> *headers = NULL,
//                           const char *body = NULL) = 0;
};


class AspliteConfig {
public:
    bool cache_lua;
    bool cache_luac;
    std::string cache_directory;
    std::string upload_directory;
};


struct AspliteCompilerParameters {
    std::string lua_path;
    std::string luac_path;
};


typedef void (*asplite_WriteCallback)(void *user_data, const char *text);


struct AspPageContext
{
    AspliteConfig *config;
    IHttpServerAdapter *server;
    IHttpRequestAdapter *request;
    IHttpResponseAdapter *response;
};


// Compiles |asp_file| according to options specified in |config|.
// If |L| specified, then compiled page is left in the state.
// This parameter can be null.
// Returns 0 if compilation suceeded and NULL in |error_message| if specified.
// Returns a non-zero error code and an error message in |error_message|.
// If |error_message| is not NULL, then user must free the the returned
// string with free function.
int CompileAspPage(lua_State *L, const std::string &asp_path,
                   const struct AspEngineConfig *config,
                   std::string *error_message);

void ExeciteAspPage(lua_State *L, const std::string &asp_path,
                    const AspPageContext &context);

bool IsAspliteOption(const std::string &option);

bool SetAspliteOption(AspliteConfig *asplite,
                      const std::string &option,
                      const std::string &value);

int luaopen_asplite(lua_State *L);
int luaopen_nvcoll(lua_State *L, NameValueCollection *collection);
int luaopen_fcoll(lua_State *L, HttpFileCollection *collection);
int CreateHttpPostedFileObject(lua_State *L, const HttpPostedFile *posted_file);

int QueryString___tostring(lua_State *L);


#endif // ASPLITE_ASPLITE_H_562542B9_D0D5_4362_9B23_E9E1CABF9903
