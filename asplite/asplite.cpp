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

#include <Windows.h>

#include "lua/lua.h"
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
        size_t lineno, size_t begin, size_t end, void *user_data)
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


static int CreateDirectories(const char *path)
{

    return 0;
}


static struct membuf *GenerateLuaFile(const char *asp_path,
                                      const char *lua_path,
                                      char **error_message)
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
        *error_message = NULL;

    fp = fopen(asp_path, "rt");
    if (fp == NULL) {
        if (error_message)
            *error_message = strdup(strerror(errno));
        return NULL;
    }

    fstat(_fileno(fp), &asp_file_stat);

    asp_content = (const char *)mmap(NULL, asp_file_stat.st_size, PROT_READ,
            MAP_PRIVATE, _fileno(fp), 0);
    if (asp_content == NULL) {
        if (error_message)
            *error_message = strdup("mmap failed.");
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


int CompileAspPage(lua_State *L, const char *asp_path,
        const struct AspEngineConfig *config, char **error_message)
{
    int requires_recompilation = 0;
    struct _stat asp_file_stat;
    struct _stat luac_file_stat;
    char cache_path[MAX_PATH];
    char lua_file[MAX_PATH];
    char luac_file[MAX_PATH];
    struct membuf *lua_content;
    lua_State *LL;
    int result;
    struct ReaderState reader_state;
    int use_cache;

    assert(((!config->cache_lua && !config->cache_luac) ||
            ((config->cache_lua || config->cache_luac)) &&
            config->root_path && config->cache_path));

    if (error_message != NULL)
        *error_message = NULL;

    if (config->cache_path && !RetargetPath(config->root_path, asp_path,
            config->cache_path, cache_path)) {
        use_cache = config->cache_lua || config->cache_luac;
    }
    else {
        use_cache = 0;
    }

    if (_stat(asp_path, &asp_file_stat) != 0) {
        if (error_message)
            *error_message = strdup(strerror(errno));
        return errno;
    }

    requires_recompilation = 1;

    if (use_cache) {
        CreateDirectories(cache_path);
        strcat(strcpy(lua_file, cache_path), ".lua");
        strcat(strcpy(luac_file, cache_path), ".luac");

        if (_stat(luac_file, &luac_file_stat) == 0) {
            if (asp_file_stat.st_mtime < luac_file_stat.st_mtime)
                requires_recompilation = 0;
        }
    }

    if (requires_recompilation) {
        lua_content = GenerateLuaFile(asp_path,
                (use_cache && config->cache_lua) ? lua_file : NULL,
                error_message);
        if (lua_content == NULL)
            return 1;
    }
    else {
        FILE *luac_fp = fopen(luac_file, "rb");
        if (luac_fp == NULL) {
            if (error_message)
                *error_message = strdup(strerror(errno));
            return 1;
        }

        lua_content = membuf_create(luac_file_stat.st_size,
                _dup(_fileno(luac_fp)), 1, 0);
        fclose(luac_fp);
    }

    reader_state.ptr = (char *)membuf_begin(lua_content);
    reader_state.end = (char *)membuf_end(lua_content);

    LL = L ? L : luaL_newstate();
    result = lua_load(LL, StringStreamReader, &reader_state, asp_path, NULL);
    if (result != LUA_OK) {
        membuf_close(lua_content);
        if (error_message)
            *error_message = strdup(lua_tostring(LL, -1));
        if (L == NULL)
            lua_close(LL);
        return 1;
    }

    membuf_close(lua_content);

    if (use_cache && requires_recompilation && config->cache_luac) {
        struct FileWriterState writer_state;

        writer_state.fp = fopen(luac_file, "wb");
        if (writer_state.fp != NULL) {
            lua_dump(LL, FileWriter, &writer_state);
            fclose(writer_state.fp);
        }
    }

    if (L != NULL) {
        result = lua_pcall(LL, 0, 0, 0);
        if (result && error_message)
            *error_message = strdup(lua_tostring(LL, -1));
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
    struct AspPageContext *context =
            (struct AspPageContext *)lua_touserdata(L, lua_upvalueindex(1));
    assert(context != NULL);
    assert(context->response != NULL);
    context->response->Write(lua_tostring(L, 1));
    return 0;
}


static int asplite_Error(lua_State *L)
{
    struct AspPageContext *context =
        (struct AspPageContext *)lua_touserdata(L, lua_upvalueindex(1));
    assert(context != NULL);
    assert(context->server != NULL);
    context->server->OnError(lua_tostring(L, 1));
    return 0;
}


static int asplite_WriteLog(lua_State *L)
{
    struct AspPageContext *context =
        (struct AspPageContext *)lua_touserdata(L, lua_upvalueindex(1));
    assert(context != NULL);
    assert(context->server != NULL);
    context->server->WriteLog(lua_tostring(L, 1));
    return 0;
}


void ExeciteAspPage(lua_State *L, const char *asp_page,
                    const struct AspPageContext *context)
{
    void *compiled_page = NULL;
    int result;
    char *error_message = NULL;

    int stack = lua_gettop(L);

    luaL_openlibs(L);

    luaopen_asplite(L);

    // create and populate 'context' table
    lua_pushstring(L, "context");
    lua_newtable(L);

    lua_pushstring(L, "write_func");
    lua_pushlightuserdata(L, (void *)context);
    lua_pushcclosure(L, asplite_Write, 1);
    lua_settable(L, -3);

    lua_pushstring(L, "error_func");
    lua_pushlightuserdata(L, (void *)context);
    lua_pushcclosure(L, asplite_Error, 1);
    lua_settable(L, -3);

    lua_pushstring(L, "log_func");
    lua_pushlightuserdata(L, (void *)context);
    lua_pushcclosure(L, asplite_WriteLog, 1);
    lua_settable(L, -3);

    // create request table
    lua_pushstring(L, "request");
    lua_newtable(L);

    lua_pushstring(L, "QUERY_STRING");
    lua_pushstring(L, context->request->GetQueryString().c_str());
    lua_settable(L, -3);

    lua_pushstring(L, "HTTP_METHOD");
    lua_pushstring(L, context->request->GetRequestMethod().c_str());
    lua_settable(L, -3);
    lua_pushstring(L, "REQUEST_METHOD");
    lua_pushstring(L, context->request->GetRequestMethod().c_str());
    lua_settable(L, -3);

    // set context.request field
    lua_settable(L, -3);

    // set asplite.context field
    lua_settable(L, -3);

    lua_pop(L, 1); // pop asplite table

    AspEngineConfig engine_config;
    engine_config.cache_lua = context->config->cache_lua;
    engine_config.cache_luac = context->config->cache_luac;
    // TODO: let the function know ahead of time where each patch will be
    //      asp file path
    //      lua file path
    //      luac file path
    //engine_config.cache_path = context->config->cache_directory.c_str();
    // TODO: document path.
    //engine_config.upload_path = context->config->upload_directory.c_str();

    result = CompileAspPage(L, asp_page, &engine_config, &error_message);
    if (result) {
        lua_pushlightuserdata(L, (void *)context);
        lua_pushcclosure(L, asplite_Error, 1);
        lua_pushstring(L, error_message);
        lua_call(L, 1, 0);
        free(error_message);
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
        lua_pushlightuserdata(L, (void *)context);
        lua_pushcclosure(L, asplite_Error, 1);
        lua_pushvalue(L, -2);
        lua_call(L, 1, 0);
    }

    assert(stack == lua_gettop(L));

    return;
}
