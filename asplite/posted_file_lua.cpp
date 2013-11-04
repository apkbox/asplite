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
inline const HttpPostedFile *GetObj(lua_State *L)
{
    return reinterpret_cast<const HttpPostedFile *>(
            lua_touserdata(L, lua_upvalueindex(1)));
}
} // namespace


static int http_file_GetContentLength(lua_State *L)
{
    const HttpPostedFile *posted_file = GetObj(L);
    lua_pushunsigned(L, posted_file->GetContentLength());
    return 1;
}


static int http_file_GetContentType(lua_State *L)
{
    const HttpPostedFile *posted_file = GetObj(L);
    lua_pushstring(L, posted_file->GetContentType().c_str());
    return 1;
}


static int http_file_GetFileName(lua_State *L)
{
    const HttpPostedFile *posted_file = GetObj(L);
    lua_pushstring(L, posted_file->GetFileName().c_str());
    return 1;
}


static int http_file_SaveAs(lua_State *L)
{
    const HttpPostedFile *posted_file = GetObj(L);
    const char *name = luaL_checkstring(L, -1);
    posted_file->SaveAs(name);
    return 1;
}


int http_file___Index(lua_State *L)
{
    // TODO: Implement normal properties.
    lua_pushnil(L);
    return 1;
}


static luaL_Reg http_file_Lib[] = {
    { "GetContentLength", http_file_GetContentLength },
    { "GetContentType", http_file_GetContentType },
    { "GetFileName", http_file_GetFileName },
    { "SaveAs", http_file_SaveAs },
    { NULL, NULL }
};


static luaL_Reg http_file_LibMeta[] = {
    { "__index", http_file___Index },
    { NULL, NULL }
};


int CreateHttpPostedFileObject(lua_State *L, const HttpPostedFile *posted_file)
{
    luaL_newlibtable(L, http_file_Lib);
    lua_pushlightuserdata(L, (void *)posted_file);
    luaL_setfuncs(L, http_file_Lib, 1);

    luaL_newlibtable(L, http_file_LibMeta);
    lua_pushlightuserdata(L, (void *)posted_file);
    luaL_setfuncs(L, http_file_LibMeta, 1);
    lua_setmetatable(L, -2);

    return 1;
}
