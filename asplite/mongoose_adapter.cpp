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

#include <direct.h>
#include <ctime>
#include <string>
#include <vector>

#include "asplite/post.h"


namespace {

AspliteMongooseAdapter *DefaultGetter(void *user_data)
{
    return reinterpret_cast<AspliteMongooseAdapter *>(user_data);
}

} // namespace


class MongooseHttpServerAdapter : public IHttpServerAdapter {
public:
    MongooseHttpServerAdapter(struct mg_connection *conn) : conn_(conn) {}

    std::string MapPath(const std::string &uri) override {
        // TODO: Make it more versatile
        char buffer[1024];
        mg_map_path(conn_, uri.c_str(), buffer, sizeof(buffer));
        return std::string(buffer);
    }

    void OnError(const char *text) override {
        // no log.
    }

    void WriteLog(const char *text) override {
        // no log.
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
        return std::string(request_info_->query_string ? 
                request_info_->query_string : "");
    }

    std::string GetRequestMethod() override {
        return std::string(request_info_->request_method);
    }

    const std::vector<HttpHeader> &GetHeaders() const override {
        return headers_;
    }

    std::string GetHeader(const char *name) const override {
        for (auto v = headers_.rbegin(); v != headers_.rend(); ++v) {
            if (v->name == name)
                return v->value;
        }

        return std::string();
    }

    int Read(void *buffer, size_t buffer_size) override {
        return mg_read(conn_, buffer, buffer_size);
    }

    void SetFormData(const std::vector<FormItem> &form_items) {
        // TODO: Move form data
    }

private:
    struct mg_connection *conn_;
    struct mg_request_info *request_info_;
    std::vector<HttpHeader> headers_;
};


class MongooseHttpResponseAdapter : public IHttpResponseAdapter {
public:
    MongooseHttpResponseAdapter(struct mg_connection *conn) : conn_(conn) {}

    void Write(const char *data, size_t len) override {
        mg_write(conn_, data, len);
    }

    void Write(const char *text) override {
        mg_write(conn_, text, strlen(text));
    }

    void Respond405(const std::string &allow,
                    const std::string &extra) override {
        mg_printf(conn_, "HTTP/1.1 405 Method Not Allowed\r\n"
                  "Content-Length: %d\r\n"
                  "Allow: %s\r\n\r\n"
                  "%s", extra.length(), allow.c_str(), extra.c_str());
    }

    void Respond415(const std::string &content_type) override {
        mg_printf(conn_, "HTTP/1.1 415 Unsupported Media Type\r\n"
                  "Content-Length: %d\r\n"
                  "Content type: %s not allowed",
                  content_type.length(), content_type.c_str());
    }


private:
    struct mg_connection *conn_;
};


static bool CreateRequestUploadDirectory(const std::string &upload_dir,
                                         std::string *request_dir)
{
    std::string new_path(upload_dir);

    if (new_path.length() > 0) {
        if (*(new_path.end() - 1) == '\\')
            new_path.resize(new_path.length() - 1);
    }

    new_path += "\\";

    srand(static_cast<int>(time(NULL)));

    for (int count = 0; count < 50; ++count) {
        // Try create a new temporary directory with random generated name.
        // If the one exists, keep trying another path name until we reach
        // some limit.

        char unique_name[64];
        sprintf(unique_name, "%012d", rand() % INT_MAX);

        std::string new_dir_name(new_path);
        new_dir_name.append(unique_name);

        if (mkdir(new_dir_name.c_str()) == 0) {
            *request_dir = new_dir_name;
            return true;
        }
    }

    return false;
}


int AspliteMongooseAdapter::RequestHandler(struct mg_connection *conn)
{
    struct mg_request_info *request_info = mg_get_request_info(conn);
    AspliteMongooseAdapter *adapter = DefaultGetter(request_info->user_data);

    MongooseHttpRequestAdapter request_adapter(conn);
    MongooseHttpResponseAdapter response_adapter(conn);
    MongooseHttpServerAdapter server_adapter(conn);

    std::vector<FormItem> form_items;
    std::string upload_dir;

    if (request_adapter.GetRequestMethod() == "POST") {
        CreateRequestUploadDirectory(adapter->config_.upload_directory,
                &upload_dir);

        ProcessPostRequest(&request_adapter, &response_adapter,
                upload_dir, &form_items);

        request_adapter.SetFormData(form_items);
    }
    else if (request_adapter.GetRequestMethod() == "GET") {
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
    context.config = &adapter->config_;
    context.server = &server_adapter;
    context.request = &request_adapter;
    context.response = &response_adapter;

    ExeciteAspPage(L, asp_path, context);

    lua_close(L);

    // TODO: Delete directories recursively.
    if (!upload_dir.empty())
        rmdir(upload_dir.c_str());

    return 1;
}


bool AspliteMongooseAdapter::Init(const AspliteConfig &config)
{
    config_ = config;
    return true;
}
