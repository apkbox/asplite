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

#include "asplite/asplite.h"

#include <assert.h>
#include <ctype.h>
#ifdef _WIN32
#include <direct.h>
#endif // _WIN32
#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <string>

#include <Windows.h>

#include "lua/lua.hpp"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "asplite/generator.h"
#include "asplite/membuf.h"
#include "asplite/parser.h"

#define USE_EMBEDDED_DRIVER

#ifdef USE_EMBEDDED_DRIVER
#include "asplite_driver.inc"
#endif // USE_EMBEDDED_DRIVER


#ifdef _WIN32
static void *mmap(void *addr, long long len, int prot, int flags, int fd,
                  int offset) {
  HANDLE fh = (HANDLE) _get_osfhandle(fd);
  HANDLE mh = CreateFileMapping(fh, 0, PAGE_READONLY, 0, 0, 0);
  void *p = MapViewOfFile(mh, FILE_MAP_READ, 0, 0, (size_t) len);
  CloseHandle(mh);
  return p;
}
#define munmap(x, y)  UnmapViewOfFile(x)
#define MAP_FAILED NULL
#define MAP_PRIVATE 0
#define PROT_READ 0
#else
#include <sys/mman.h>
#endif


struct ParserData {
    struct membuf *buf;
    PageCodeGeneratorCallback handler;
};


struct ReaderState
{
    const char *ptr;
    const char *end;
};


struct FileWriterState
{
    FILE *fp;
};


static void WriteToBufferCallback(const char *text, int length, void *user_data)
{
    struct membuf *buf = ((struct ParserData *)user_data)->buf;

    if (length < 0)
        length = strlen(text);

    membuf_append(buf, text, length);
}


static void ParserEventHandler(const char *buffer, enum ChunkType chunk_type,
                               size_t lineno, size_t begin, size_t end,
                               void *user_data)
{
    struct ParserData *data = (struct ParserData *)user_data;
    GenerateBodyChunk(data->handler, buffer, chunk_type, lineno, begin, end,
            user_data);
}


static const char *StringStreamReader(lua_State *L, void *ud, size_t *sz)
{
    struct ReaderState *state = (struct ReaderState *)ud;
    const char *r;

    if (state->ptr >= state->end) {
        *sz = 0;
        return NULL;
    }

    *sz = state->end - state->ptr;
    r = state->ptr;
    state->ptr = state->end;
    return r;
}


static int FileWriter(lua_State *L, const void* p, size_t sz, void* ud)
{
    struct FileWriterState *state = (struct FileWriterState *)ud;
    fwrite(p, 1, sz, state->fp);
    return 0;
}


// Checks if path1 is a prefix path of path2
static int IsPrefixOf(const char *path1, const char *path2)
{
    size_t len1;
    size_t len2;

    len1 = strlen(path1);

    // an empty prefix always matches
    // should it?
    if (len1 == 0)
        return 1;

    len2 = strlen(path2);

    // prefix can't be longer
    if (len1 > len2)
        return 0;

    // should end on component boundary
    if (path2[len1] != '\\')
        return 0;

    return strnicmp(path1, path2, len1) == 0;
}


static int RetargetPath(const char *ancestor, const char *path,
                        const char *new_ancestor, char *result)
{
    int len = 0;
    const char *p;

    // path is not descendant of ancestor
    if (!IsPrefixOf(ancestor, path))
        return 1;

    result[0] = '\0';
    strcpy(result, new_ancestor);
    len = strlen(result);
    if (len > 0 && (result[len - 1] == '\\' || result[len - 1] == '/')) {
        result[len - 1] = '\0';
    }

    p = &path[strlen(ancestor)];
    if (*p == '\\' || *p == '/') {
        p++;
    }

    if (strlen(p) != 0 && strlen(result) != 0)
        strcat(result, "\\");
    strcat(result, p);

    return 0;
}


static struct membuf *GenerateLuaFile(const char *asp_path,
                                      const char *lua_path,
                                      std::string *error_message)
{
    FILE *fp;
    struct stat asp_file_stat;
    const char *asp_content;
    const char *asp_content_begin_ptr;
    struct membuf *lua_content;
    size_t estimated_size;
    int lua_fd = -1;
    struct ParserData parser_data;

    if (error_message)
        error_message->clear();

    fp = fopen(asp_path, "rt");
    if (fp == NULL) {
        if (error_message)
            *error_message = strerror(errno);
        return NULL;
    }

    fstat(_fileno(fp), &asp_file_stat);

    asp_content = (const char *)mmap(NULL, asp_file_stat.st_size, PROT_READ,
            MAP_PRIVATE, _fileno(fp), 0);
    if (asp_content == NULL) {
        if (error_message)
            *error_message = "mmap failed.";
        fclose(fp);
        return NULL;
    }

    // Check for BOM and skip if present
    asp_content_begin_ptr = asp_content;
    if (asp_file_stat.st_size >= 3) {
        if (strncmp(asp_content_begin_ptr, "\xEF\xBB\xBF", 3) == 0)
            asp_content_begin_ptr += 3;
    }

    if (lua_path != NULL) {
        FILE *lua_fp;
        lua_fp = fopen(lua_path, "wt");
        if (lua_fp != NULL) {
            lua_fd = _dup(_fileno(lua_fp));
            fclose(lua_fp);
        }
    }

    estimated_size = asp_file_stat.st_size * 4;
    lua_content = membuf_create(estimated_size, lua_fd, 0, 1);

    parser_data.buf = lua_content;
    parser_data.handler = WriteToBufferCallback;

    GenerateProlog(parser_data.handler, &parser_data);
    ParseBuffer(asp_content_begin_ptr, asp_file_stat.st_size,
            ParserEventHandler, &parser_data);
    GenerateEpilog(parser_data.handler, &parser_data);

    munmap(asp_content, asp_file_stat.st_size);
    fclose(fp);

    return lua_content;
}


int CompileAspPage(lua_State *L, const std::string &asp_path,
                   const struct AspliteCompilerParameters *params,
                   std::string *error_message)
{
    struct _stat asp_file_stat;
    struct _stat luac_file_stat;
    struct membuf *lua_content;
    lua_State *LL;
    int result;
    struct ReaderState reader_state;

    if (error_message != NULL)
        error_message->clear();

    if (_stat(asp_path.c_str(), &asp_file_stat) != 0) {
        if (error_message)
            *error_message = strerror(errno);
        return errno;
    }

    bool requires_recompilation = true;

    if (!params->luac_path.empty()) {
        if (_stat(params->luac_path.c_str(), &luac_file_stat) == 0) {
            if (asp_file_stat.st_mtime < luac_file_stat.st_mtime)
                requires_recompilation = false;
        }
    }

    if (requires_recompilation) {
        lua_content = GenerateLuaFile(asp_path.c_str(), params->lua_path.c_str(),
                error_message);
        if (lua_content == NULL)
            return 1;
    }
    else {
        FILE *luac_fp = fopen(params->luac_path.c_str(), "rb");
        if (luac_fp == NULL) {
            if (error_message)
                *error_message = strerror(errno);
            return 1;
        }

        lua_content = membuf_create(luac_file_stat.st_size,
                _dup(_fileno(luac_fp)), 1, 0);
        fclose(luac_fp);
    }

    reader_state.ptr = (char *)membuf_begin(lua_content);
    reader_state.end = (char *)membuf_end(lua_content);

    LL = L ? L : luaL_newstate();
    result = lua_load(LL, StringStreamReader, &reader_state,
            asp_path.c_str(), NULL);
    if (result != LUA_OK) {
        membuf_close(lua_content);
        if (error_message)
            *error_message = lua_tostring(LL, -1);
        if (L == NULL)
            lua_close(LL);
        return 1;
    }

    membuf_close(lua_content);

    if (requires_recompilation && !params->luac_path.empty()) {
        struct FileWriterState writer_state;

        writer_state.fp = fopen(params->luac_path.c_str(), "wb");
        if (writer_state.fp != NULL) {
            lua_dump(LL, FileWriter, &writer_state);
            fclose(writer_state.fp);
        }
    }

    if (L != NULL) {
        result = lua_pcall(LL, 0, 0, 0);
        if (result && error_message)
            *error_message = lua_tostring(LL, -1);
    }
    else {
        lua_close(LL);
    }

    return result;
}


struct AspliteCallbackUserdata
{
    const struct AspPageContext *context;
    asplite_WriteCallback write_func;
};


static int asplite_Write(lua_State *L)
{
    const AspPageContext *context = reinterpret_cast<AspPageContext *>(
            lua_touserdata(L, lua_upvalueindex(1)));
    assert(context != NULL);
    assert(context->response != NULL);
    context->response->Write(lua_tostring(L, 1));
    return 0;
}


static int asplite_Error(lua_State *L)
{
    const AspPageContext *context = reinterpret_cast<AspPageContext *>(
            lua_touserdata(L, lua_upvalueindex(1)));
    assert(context != NULL);
    assert(context->server != NULL);
    context->server->OnError(lua_tostring(L, 1));
    return 0;
}


static int asplite_MapPath(lua_State *L)
{
    const AspPageContext *context = reinterpret_cast<AspPageContext *>(
        lua_touserdata(L, lua_upvalueindex(1)));
    assert(context != NULL);
    assert(context->server != NULL);
    const char *uri = lua_tostring(L, 1);
    lua_pushstring(L, context->server->MapPath(uri ? uri : std::string()).c_str());
    return 1;
}


static int asplite_WriteLog(lua_State *L)
{
    const AspPageContext *context = reinterpret_cast<AspPageContext *>(
            lua_touserdata(L, lua_upvalueindex(1)));
    assert(context != NULL);
    assert(context->server != NULL);
    context->server->WriteLog(lua_tostring(L, 1));
    return 0;
}


static bool CreateDirectoriesRecursively(const std::string &base_dir,
                                         const std::string &rel_path)
{
    std::string path(base_dir);
    std::string rpath(rel_path);

    while (true) {
        if (mkdir(path.c_str()) && errno != EEXIST)
            return false;

        if (rpath.empty())
            break;

        std::string::size_type sep = rpath.find('\\');
        if (sep != std::string::npos) {
            path += '\\';
            path += rpath.substr(0, sep);
            rpath.erase(0, sep + 1);
        }
        else {
            path += '\\';
            path += rpath;
            rpath.clear();
        }
    }

    return true;
}


void ExeciteAspPage(lua_State *L, const std::string &asp_path,
                    const AspPageContext &context)
{
    int result;
    std::string error_message;

    int stack = lua_gettop(L);
    luaL_openlibs(L);
    luaopen_asplite(L);

    // create and populate 'context' table
    lua_pushstring(L, "context");
    lua_newtable(L);

    lua_pushstring(L, "write_func");
    lua_pushlightuserdata(L, (void *)&context);
    lua_pushcclosure(L, asplite_Write, 1);
    lua_settable(L, -3);

    lua_pushstring(L, "error_func");
    lua_pushlightuserdata(L, (void *)&context);
    lua_pushcclosure(L, asplite_Error, 1);
    lua_settable(L, -3);

    lua_pushstring(L, "log_func");
    lua_pushlightuserdata(L, (void *)&context);
    lua_pushcclosure(L, asplite_WriteLog, 1);
    lua_settable(L, -3);

    lua_pushstring(L, "map_path");
    lua_pushlightuserdata(L, (void *)&context);
    lua_pushcclosure(L, asplite_MapPath, 1);
    lua_settable(L, -3);

    // create request table
    lua_pushstring(L, "request");
    lua_newtable(L);

    // context.request.ServerVariables table
    lua_pushstring(L, "ServerVariables");
    lua_newtable(L);

    lua_pushstring(L, "QUERY_STRING");
    lua_pushstring(L, context.request->GetQueryString().c_str());
    lua_settable(L, -3);

    lua_pushstring(L, "HTTP_METHOD");
    lua_pushstring(L, context.request->GetRequestMethod().c_str());
    lua_settable(L, -3);

    lua_pushstring(L, "REQUEST_METHOD");
    lua_pushstring(L, context.request->GetRequestMethod().c_str());
    lua_settable(L, -3);

    // set request.ServerVariables field
    lua_settable(L, -3);

    // context.request.Files table
    lua_pushstring(L, "Files");
    lua_newtable(L);

    const HttpFilesCollection &files = context.request->GetFiles();
    int index = 1;
    for (auto iter = files.begin(); iter != files.end(); ++iter) {
        lua_pushinteger(L, index++);
        lua_pushstring(L, iter->c_str());
        lua_settable(L, -3);
    }

    // set context.request.Files field
    lua_settable(L, -3);

    // set context.request field
    lua_settable(L, -3);

    // set asplite.context field
    lua_settable(L, -3);

    lua_pop(L, 1); // pop asplite table

    AspliteCompilerParameters params;

    if (!context.config->cache_directory.empty()) {

        // Check if doc_root is a prefix of ASP path, then
        // if so, extract asp relative path.
        std::string document_root = context.server->MapPath(std::string());
        if (!document_root.empty() && asp_path.find(document_root) == 0) {
            // Assign cache paths
            std::string cache_directory(context.config->cache_directory);
            std::string document_path(asp_path.substr(document_root.length()));

#ifdef _WIN32
            // Normalize separators
            std::replace(cache_directory.begin(), cache_directory.end(),
                    '/', '\\');
            std::replace(document_path.begin(), document_path.end(), '/', '\\');
#endif
            // Remove trailing separator
            if (*(cache_directory.end() - 1) == '\\')
                cache_directory.resize(cache_directory.length() - 1);

            // Remove leading separator
            if (document_path.length() > 0 && document_path[0] == '\\')
                document_path.erase(document_path.begin());

            // Create relative path to document's directory
            std::string document_dir(document_path);
            std::string::size_type last_sep = document_dir.find_last_of('\\');
            if (last_sep != std::string::npos)
                document_dir.erase(last_sep);
            else
                document_dir.clear();

            CreateDirectoriesRecursively(cache_directory, document_dir);

            if (context.config->cache_lua) {
                params.lua_path = cache_directory + "\\" + document_path + ".lua";
            }

            if (context.config->cache_luac) {
                params.luac_path = cache_directory + "\\" + document_path + ".luac";
            }
        }
    }

    result = CompileAspPage(L, asp_path, &params, &error_message);
    if (result) {
        lua_pushlightuserdata(L, (void *)&context);
        lua_pushcclosure(L, asplite_Error, 1);
        lua_pushstring(L, error_message.c_str());
        lua_call(L, 1, 0);
        assert(stack == lua_gettop(L));
        return;
    }

#ifdef USE_EMBEDDED_DRIVER
    result = luaL_loadbufferx(L, asplite_Driver, sizeof(asplite_Driver),
            "asplite_Driver", NULL);
#else
    result = luaL_loadfile(L, "asplite.lua");
#endif // USE_EMBEDDED_DRIVER
    if (result == LUA_OK) {
        result = lua_pcall(L, 0, 0, 0);
    }

    if (result != LUA_OK) {
        lua_pushlightuserdata(L, (void *)&context);
        lua_pushcclosure(L, asplite_Error, 1);
        lua_pushvalue(L, -2);
        lua_call(L, 1, 0);
    }

    assert(stack == lua_gettop(L));

    return;
}


static bool StringToBoolean(const std::string &str)
{
    if (str == "true" || str == "1" || str == "yes")
        return true;

    return false;
}


bool IsAspliteOption(const std::string &option)
{
    return option == "cache_lua" || option == "cache_luac" ||
            option == "cache_directory" || option == "upload_directory";
}


bool SetAspliteOption(AspliteConfig *config,
                      const std::string &option,
                      const std::string &value)
{
    if (option == "cache_lua")
        config->cache_lua = StringToBoolean(value);
    else if (option == "cache_luac")
        config->cache_luac = StringToBoolean(value);
    else if (option == "cache_directory")
        config->cache_directory = value;
    else if (option == "upload_directory")
        config->upload_directory = value;
    else
        return false;

    return true;
}
