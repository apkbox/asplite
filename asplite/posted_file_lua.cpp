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

#include "lua/lua.hpp"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "asplite/posted_file.h"


namespace {
inline const HttpPostedFile *GetHttpPostedFile(lua_State *L, int index)
{
    return reinterpret_cast<const HttpPostedFile *>(lua_touserdata(L, index));
}
} // namespace


static int http_file_SaveAs(lua_State *L)
{
    const HttpPostedFile *posted_file = GetHttpPostedFile(L, lua_upvalueindex(1));
    const char *name = luaL_checkstring(L, -1);
    posted_file->SaveAs(name);
    lua_pushboolean(L, 1);
    return 1;
}


int http_file___index(lua_State *L)
{
    const HttpPostedFile *posted_file = GetHttpPostedFile(L, 1);
    const char *name = luaL_checkstring(L, 2);
    if (strcmp(name, "ContentLength") == 0) {
        lua_pushunsigned(L, posted_file->GetContentLength());
    }
    else if (strcmp(name, "ContentType") == 0) {
        lua_pushstring(L, posted_file->GetContentType().c_str());
    }
    else if (strcmp(name, "FileName") == 0) {
        lua_pushstring(L, posted_file->GetFileName().c_str());
    }
    else if (strcmp(name, "SaveAs") == 0) {
        lua_pushlightuserdata(L, (void *)posted_file);
        lua_pushcclosure(L, http_file_SaveAs, 1);
    }
    else {
        lua_pushstring(L, "Unknown property or method.");
        lua_error(L);
    }
    return 1;
}


int CreateHttpPostedFileObject(lua_State *L, const HttpPostedFile *posted_file)
{
    lua_pushlightuserdata(L, (void *)posted_file);
    if (luaL_newmetatable(L, "asplite_HttpPostedFile")) {
        lua_pushcfunction(L, http_file___index);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);
    return 1;
}
