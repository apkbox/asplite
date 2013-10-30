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
inline NameValueCollection *GetColl(lua_State *L)
{
    return reinterpret_cast<NameValueCollection *>(
            lua_touserdata(L, lua_upvalueindex(1)));
}


void StringVectorToLuaArray(lua_State *L,
                            NameValueCollection::value_list_type &values)
{
    lua_newtable(L);
    int index = 0;
    for (auto iter = values.begin(); iter != values.end(); ++iter) {
        lua_pushlstring(L, iter->data(), iter->length());
        lua_rawseti(L, -2, ++index);
    }
}
} // namespace


int nvcoll_Count(lua_State *L)
{
    NameValueCollection *coll = GetColl(L);
    lua_pushunsigned(L, coll->GetCount());
    return 1;
}


int nvcoll_Add(lua_State *L)
{
    NameValueCollection *coll = GetColl(L);

    // TODO: permit nil
    const char *name = luaL_checkstring(L, 1);
    const char *value = luaL_checkstring(L, 2);

    lua_pushboolean(L, coll->Add(name, value));
    return 1;
}


int nvcoll_Get(lua_State *L)
{
    NameValueCollection *coll = GetColl(L);

    std::string value;

    // TODO: permit nil
    if (lua_type(L, 1) == LUA_TNUMBER) {
        lua_Unsigned index = luaL_checkunsigned(L, 1);

        value = coll->Get(index);
    }
    else {
        const char *name = luaL_checkstring(L, 1);
        value = coll->Get(name);
    }

    lua_pushlstring(L, value.data(), value.length());
    return 1;
}


int nvcoll_GetKey(lua_State *L)
{
    NameValueCollection *coll = GetColl(L);
    lua_Unsigned index = luaL_checkunsigned(L, 1);
    std::string value = coll->GetKey(index);
    lua_pushlstring(L, value.data(), value.length());
    return 1;
}


int nvcoll_GetValues(lua_State *L)
{
    NameValueCollection *coll = GetColl(L);

    NameValueCollection::value_list_type values;

    // TODO: permit nil
    if (lua_type(L, 1) == LUA_TNUMBER) {
        lua_Unsigned index = luaL_checkunsigned(L, 1);

        values = coll->GetValues(index);
    }
    else {
        luaL_checktype(L, 1, LUA_TSTRING);
        const char *name = luaL_checkstring(L, 1);

        values = coll->GetValues(name);
    }

    StringVectorToLuaArray(L, values);
    return 1;
}


int nvcoll_Clear(lua_State *L)
{
    NameValueCollection *coll = GetColl(L);
    coll->Clear();
    return 0;
}


int nvcoll_AllKeys(lua_State *L)
{
    NameValueCollection *coll = GetColl(L);
    StringVectorToLuaArray(L, coll->AllKeys());
    return 1;
}


int nvcoll_Set(lua_State *L)
{
    NameValueCollection *coll = GetColl(L);

    // TODO: permit nil
    const char *name = luaL_checkstring(L, 1);
    const char *value = luaL_checkstring(L, 2);

    lua_pushboolean(L, coll->Set(name, value));
    return 1;
}


int nvcoll_Remove(lua_State *L)
{
    NameValueCollection *coll = GetColl(L);
    lua_pushboolean(L, coll->Remove(luaL_checkstring(L, 1)));
    return 1;
}


int nvcoll___Index(lua_State *L)
{
    lua_pushstring(L, "Get");
    lua_rawget(L, 1);
    lua_pushvalue(L, 2);
    lua_call(L, 1, 1);
    return 1;
}


int nvcoll___NewIndex(lua_State *L)
{
    lua_pushstring(L, "Set");
    lua_rawget(L, 1);
    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_call(L, 2, 1);
    return 1;
}


static luaL_Reg nvcoll_Lib[] = {
    { "Count", nvcoll_Count },
    { "Add", nvcoll_Add },
    { "Get", nvcoll_Get },
    { "GetKey", nvcoll_GetKey },
    { "GetValues", nvcoll_GetValues },
    { "Clear", nvcoll_Clear },
    { "AllKeys", nvcoll_AllKeys },
    { "Set", nvcoll_Set },
    { "Remove", nvcoll_Remove },
    { NULL, NULL }
};


static luaL_Reg nvcoll_LibMeta[] = {
    {"__index", nvcoll___Index},
    {"__newindex", nvcoll___NewIndex},
    {NULL, NULL}
};


int luaopen_nvcoll(lua_State *L, NameValueCollection *collection)
{
    luaL_newlibtable(L, nvcoll_Lib);
    lua_pushlightuserdata(L, (void *)collection);
    luaL_setfuncs(L, nvcoll_Lib, 1);

    luaL_newlibtable(L, nvcoll_LibMeta);
    lua_pushlightuserdata(L, (void *)collection);
    luaL_setfuncs(L, nvcoll_LibMeta, 1);
    lua_setmetatable(L, -2);

    return 1;
}
