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

#include "asplite/mongoose_adapter.h"

#include <string>
#include <vector>

namespace {

AspliteMongooseAdapter *DefaultGetter(void *user_data)
{
    return reinterpret_cast<AspliteMongooseAdapter *>(user_data);
}

} // namespace


class HttpHeader {
public:
    HttpHeader(const char *name, const char *value)
        : name(name), value(value) {}

    std::string name;
    std::string value;
};


class IHttpServerAdapter {
public:
    virtual ~IHttpServerAdapter() {}

    virtual std::string MapPath(const std::string &uri) = 0;
};


class IHttpRequestAdapter {
public:
    virtual ~IHttpRequestAdapter() {}

    virtual std::string GetUri() = 0;
    virtual std::string GetQueryString() = 0;
    virtual std::string GetRequestMethod() = 0;
    virtual const std::vector<HttpHeader> &GetHeaders() = 0;
};


class IHttpResponseAdapter {
public:
    virtual ~IHttpResponseAdapter() {}

    virtual void Respond405(const std::string &allow,
                            const std::string &extra) = 0;
};


class MongooseHttpServerAdapter : public IHttpServerAdapter {
public:
    MongooseHttpServerAdapter(struct mg_connection *conn) : conn_(conn) {}

    std::string MapPath(const std::string &uri) override {
        // TODO: Make it more versatile
        char buffer[1024];
        mg_map_path(conn_, uri.c_str(), buffer, sizeof(buffer));
        return std::string(buffer);
    }

private:
    struct mg_connection *conn_;
};


class MongooseHttpRequestAdapter : public IHttpRequestAdapter {
public:
    MongooseHttpRequestAdapter(struct mg_connection *conn)
            : conn_(conn), request_info_(mg_get_request_info(conn)) {
        for (int i = 0; i < request_info_->num_headers; i++) {
            const char *name = request_info_->http_headers[i].name;
            const char *value = request_info_->http_headers[i].value;
            headers_.push_back(HttpHeader(name, value));
        }
    }

    std::string GetUri() override {
        return std::string(request_info_->uri);
    }

    std::string GetQueryString() override {
        return std::string(request_info_->query_string);
    }

    std::string GetRequestMethod() override {
        return std::string(request_info_->request_method);
    }

    const std::vector<HttpHeader> &GetHeaders() {
        return headers_;
    }

private:
    struct mg_connection *conn_;
    struct mg_request_info *request_info_;
    std::vector<HttpHeader> headers_;
};


class MongooseHttpResponseAdapter : public IHttpResponseAdapter {
public:
    MongooseHttpResponseAdapter(struct mg_connection *conn) : conn_(conn) {}

    void Respond405(const std::string &allow,
                    const std::string &extra) override {
        mg_printf(conn_, "HTTP/1.1 405 Method Not Allowed\r\n"
                  "Content-Length: %d\r\n"
                  "Allow: %s\r\n\r\n"
                  "%s", extra.length(), allow.c_str(), extra.c_str());
    }

private:
    struct mg_connection *conn_;
};



int AspliteMongooseAdapter::RequestHandler(struct mg_connection *conn)
{
    struct mg_request_info *request_info = mg_get_request_info(conn);
    AspliteMongooseAdapter *adapter = DefaultGetter(request_info->user_data);

    MongooseHttpRequestAdapter request_adapter(conn);
    MongooseHttpResponseAdapter response_adapter(conn);
    MongooseHttpServerAdapter server_adapter(conn);

    if (request_adapter.GetRequestMethod() == "POST") {
        ProcessPostRequest(conn);
    }
    if (request_adapter.GetRequestMethod() == "GET") {
        ; // default
    }
    else {
        response_adapter.Respond405("GET, POST", "");
    }

    std::string asp_path = server_adapter.MapPath(request_adapter.GetUri());
    if (asp_path.empty()) {
        // TODO: Respond with error
        return 0;
    }

    lua_State *L = luaL_newstate();

    struct AspPageContext context;
    //context.write_func = asp_write;
    //context.error_func = asp_error;
    //context.log_func = asp_write;
    //context.user_data = conn;
    context.request.request_method = request_adapter.GetRequestMethod();
    context.request.query_string = request_adapter.GetQueryString();
    context.engine_config.root_path = conn->ctx->config[DOCUMENT_ROOT];
    context.engine_config.cache_path = conn->ctx->config[DOCUMENT_ROOT];
    context.engine_config.cache_lua = adapter->config_.cache_lua;
    context.engine_config.cache_luac = adapter->config_.cache_luac;

    ExeciteAspPage(L, asp_path.c_str(), &context);

    lua_close(L);

    return 1;
}


bool AspliteMongooseAdapter::Init(const AspliteConfig &config)
{
    config_ = config;
    return true;
}
