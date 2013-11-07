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
inline HttpFileCollection *GetCollection(lua_State *L, int index)
{
    return reinterpret_cast<HttpFileCollection *>(
            lua_touserdata(L, index));
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


int file_collection_Get(lua_State *L)
{
    HttpFileCollection *collection = GetCollection(L, lua_upvalueindex(1));

    if (lua_type(L, 1) == LUA_TNUMBER) {
        lua_Unsigned index = luaL_checkunsigned(L, 1);
        HttpPostedFile value;
        if (collection->Get(index, &value))
            CreateHttpPostedFileObject(L, &value);
        else
            lua_pushnil(L);
    }
    else {
        const char *name = luaL_checkstring(L, 1);
        HttpPostedFile value;
        if (collection->Get(name, &value))
            CreateHttpPostedFileObject(L, &value);
        else
            lua_pushnil(L);
    }

    return 1;
}


int file_collection_GetKey(lua_State *L)
{
    HttpFileCollection *collection = GetCollection(L, lua_upvalueindex(1));
    lua_Unsigned index = luaL_checkunsigned(L, 1);

    std::string value;
    if (collection->GetKey(index, &value))
        lua_pushlstring(L, value.data(), value.length());
    else
        lua_pushnil(L);
    return 1;
}


int file_collection_GetMultiple(lua_State *L)
{
    HttpFileCollection *coll = GetCollection(L, lua_upvalueindex(1));
    const char *name = luaL_checkstring(L, 1);

    const HttpFileCollection::value_list_type *values;
    if (coll->GetMultiple(name, &values))
        ValueListToLuaArray(L, *values);
    else
        lua_pushnil(L);
    return 1;
}


int file_collection___index(lua_State *L)
{
    HttpFileCollection *collection = GetCollection(L, 1);
    const char *name = luaL_checkstring(L, 2);
    if (strcmp(name, "Count") == 0) {
        lua_pushunsigned(L, collection->GetCount());
    }
    else if (strcmp(name, "AllKeys") == 0) {
        KeyListToLuaArray(L, collection->AllKeys());
    }
    else if (strcmp(name, "Get") == 0) {
        lua_pushlightuserdata(L, (void *)collection);
        lua_pushcclosure(L, file_collection_Get, 1);
    }
    else if (strcmp(name, "GetKey") == 0) {
        lua_pushlightuserdata(L, (void *)collection);
        lua_pushcclosure(L, file_collection_GetKey, 1);
    }
    else if (strcmp(name, "GetMultiple") == 0) {
        lua_pushlightuserdata(L, (void *)collection);
        lua_pushcclosure(L, file_collection_GetMultiple, 1);
    }
    else {
        lua_pushstring(L, "Unknown property or method.");
        lua_error(L);
    }

    lua_pushnil(L);
    return 1;
}


int CreateHttpFileCollection(lua_State *L, HttpFileCollection *collection)
{
    lua_pushlightuserdata(L, (void *)collection);
    if (luaL_newmetatable(L, "asplite_HttpFileCollection")) {
        lua_pushcfunction(L, file_collection___index);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);
    return 1;
}
