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

#include "lua/lua.hpp"
#include "lua/lualib.h"
#include "lua/lauxlib.h"


namespace {
inline HttpFileCollection *GetColl(lua_State *L)
{
    return reinterpret_cast<HttpFileCollection *>(
            lua_touserdata(L, lua_upvalueindex(1)));
}


void KeyListToLuaArray(lua_State *L,
                       HttpFileCollection::key_list_type &values)
{
    lua_newtable(L);
    int index = 0;
    for (auto iter = values.begin(); iter != values.end(); ++iter) {
        lua_pushlstring(L, iter->data(), iter->length());
        lua_rawseti(L, -2, ++index);
    }
}


void HttpPostedFileToObject(lua_State *L, const HttpPostedFile &posted_file)
{
    CreateHttpPostedFileObject(L, &posted_file);
}


void ValueListToLuaArray(lua_State *L,
                         const HttpFileCollection::value_list_type &values)
{
    lua_newtable(L);
    int index = 0;
    for (auto iter = values.begin(); iter != values.end(); ++iter) {
        CreateHttpPostedFileObject(L, &*iter);
        lua_rawseti(L, -2, ++index);
    }
}

} // namespace


int fcoll_Count(lua_State *L)
{
    HttpFileCollection *coll = GetColl(L);
    lua_pushunsigned(L, coll->GetCount());
    return 1;
}


int fcoll_Get(lua_State *L)
{
    HttpFileCollection *coll = GetColl(L);

    if (lua_type(L, 1) == LUA_TNUMBER) {
        lua_Unsigned index = luaL_checkunsigned(L, 1);
        HttpPostedFile value;
        if (coll->Get(index, &value))
            CreateHttpPostedFileObject(L, &value);
        else
            lua_pushnil(L);
    }
    else {
        const char *name = luaL_checkstring(L, 1);
        HttpPostedFile value;
        if (coll->Get(name, &value))
            CreateHttpPostedFileObject(L, &value);
        else
            lua_pushnil(L);
    }

    return 1;
}


int fcoll_GetKey(lua_State *L)
{
    HttpFileCollection *coll = GetColl(L);
    lua_Unsigned index = luaL_checkunsigned(L, 1);
    std::string value;
    if (coll->GetKey(index, &value))
        lua_pushlstring(L, value.data(), value.length());
    else
        lua_pushnil(L);
    return 1;
}


int fcoll_GetMultiple(lua_State *L)
{
    HttpFileCollection *coll = GetColl(L);

    const HttpFileCollection::value_list_type *values;

    const char *name = luaL_checkstring(L, 1);
    if (coll->GetMultiple(name, &values))
        ValueListToLuaArray(L, *values);
    else
        lua_pushnil(L);
    return 1;
}


int fcoll_AllKeys(lua_State *L)
{
    HttpFileCollection *coll = GetColl(L);
    KeyListToLuaArray(L, coll->AllKeys());
    return 1;
}


int fcoll___Index(lua_State *L)
{
    lua_pushstring(L, "Get");
    lua_rawget(L, 1);
    lua_pushvalue(L, 2);
    lua_call(L, 1, 1);
    return 1;
}

static luaL_Reg fcoll_Lib[] = {
    { "Count", fcoll_Count },
    { "Get", fcoll_Get },
    { "GetKey", fcoll_GetKey },
    { "GetMultiple", fcoll_GetMultiple },
    { "AllKeys", fcoll_AllKeys },
    { NULL, NULL }
};


int luaopen_fcoll(lua_State *L, HttpFileCollection *collection)
{
    luaL_newlibtable(L, fcoll_Lib);
    lua_pushlightuserdata(L, (void *)collection);
    luaL_setfuncs(L, fcoll_Lib, 1);

    return 1;
}

