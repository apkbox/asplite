// The MIT License (MIT)
//
// Copyright (c) 2013 Alex Kozlov
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "asplite/asplite.h"

#include <assert.h>

#include "lua/lua.hpp"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

namespace {
inline NameValueCollection *GetCollection(lua_State *L, int index) {
  return *reinterpret_cast<NameValueCollection **>(lua_touserdata(L, index));
}

void StringVectorToLuaArray(
    lua_State *L,
    const NameValueCollection::value_list_type &values) {
  lua_newtable(L);
  int index = 0;
  for (auto iter = values.begin(); iter != values.end(); ++iter) {
    lua_pushlstring(L, iter->data(), iter->length());
    lua_rawseti(L, -2, ++index);
  }
}
}  // namespace

int name_value_collection_Add(lua_State *L) {
  NameValueCollection *collection = GetCollection(L, lua_upvalueindex(1));

  // TODO: permit nil
  const char *name = luaL_checkstring(L, 1);
  const char *value = luaL_checkstring(L, 2);

  lua_pushboolean(L, collection->Add(name, value));
  return 1;
}

int name_value_collection_Get(lua_State *L) {
  NameValueCollection *collection = GetCollection(L, lua_upvalueindex(1));

  // TODO: permit nil
  if (lua_type(L, 1) == LUA_TNUMBER) {
    lua_Unsigned index = luaL_checkunsigned(L, 1);
    std::string value;
    if (collection->Get(index, &value))
      lua_pushlstring(L, value.data(), value.length());
    else
      lua_pushnil(L);
  } else {
    const char *name = luaL_checkstring(L, 1);
    std::string value;
    if (collection->Get(name, &value))
      lua_pushlstring(L, value.data(), value.length());
    else
      lua_pushnil(L);
  }
  return 1;
}

int name_value_collection_GetKey(lua_State *L) {
  NameValueCollection *collection = GetCollection(L, lua_upvalueindex(1));
  lua_Unsigned index = luaL_checkunsigned(L, 1);
  std::string key;
  if (collection->GetKey(index, &key))
    lua_pushlstring(L, key.data(), key.length());
  else
    lua_pushnil(L);
  return 1;
}

int name_value_collection_GetValues(lua_State *L) {
  NameValueCollection *collection = GetCollection(L, lua_upvalueindex(1));
  const NameValueCollection::value_list_type *values;

  // TODO: permit nil
  if (lua_type(L, 1) == LUA_TNUMBER) {
    lua_Unsigned index = luaL_checkunsigned(L, 1);
    if (collection->GetValues(index, &values))
      StringVectorToLuaArray(L, *values);
    else
      lua_pushnil(L);
  } else {
    luaL_checktype(L, 1, LUA_TSTRING);
    const char *name = luaL_checkstring(L, 1);

    if (collection->GetValues(name, &values))
      StringVectorToLuaArray(L, *values);
    else
      lua_pushnil(L);
  }

  return 1;
}

int name_value_collection_Clear(lua_State *L) {
  NameValueCollection *collection = GetCollection(L, lua_upvalueindex(1));
  collection->Clear();
  return 0;
}

int name_value_collection_AllKeys(lua_State *L) {
  NameValueCollection *collection = GetCollection(L, lua_upvalueindex(1));
  StringVectorToLuaArray(L, collection->AllKeys());
  return 1;
}

int name_value_collection_Set(lua_State *L) {
  NameValueCollection *collection = GetCollection(L, lua_upvalueindex(1));

  // TODO: permit nil
  const char *name = luaL_checkstring(L, 1);
  const char *value = luaL_checkstring(L, 2);

  lua_pushboolean(L, collection->Set(name, value));
  return 1;
}

int name_value_collection_Remove(lua_State *L) {
  NameValueCollection *collection = GetCollection(L, lua_upvalueindex(1));
  lua_pushboolean(L, collection->Remove(luaL_checkstring(L, 1)));
  return 1;
}

int name_value_collection___index(lua_State *L) {
  NameValueCollection *collection = GetCollection(L, 1);
  const char *name = luaL_checkstring(L, 2);
  if (strcmp(name, "Count") == 0) {
    lua_pushunsigned(L, collection->GetCount());
  } else if (strcmp(name, "AllKeys") == 0) {
    StringVectorToLuaArray(L, collection->AllKeys());
  } else if (strcmp(name, "Add") == 0) {
    lua_pushlightuserdata(L, (void *)collection);
    lua_pushcclosure(L, name_value_collection_Add, 1);
  } else if (strcmp(name, "Get") == 0) {
    lua_pushlightuserdata(L, (void *)collection);
    lua_pushcclosure(L, name_value_collection_Get, 1);
  } else if (strcmp(name, "GetKey") == 0) {
    lua_pushlightuserdata(L, (void *)collection);
    lua_pushcclosure(L, name_value_collection_GetKey, 1);
  } else if (strcmp(name, "GetValues") == 0) {
    lua_pushlightuserdata(L, (void *)collection);
    lua_pushcclosure(L, name_value_collection_GetValues, 1);
  } else if (strcmp(name, "Clear") == 0) {
    lua_pushlightuserdata(L, (void *)collection);
    lua_pushcclosure(L, name_value_collection_Clear, 1);
  } else if (strcmp(name, "Set") == 0) {
    lua_pushlightuserdata(L, (void *)collection);
    lua_pushcclosure(L, name_value_collection_Set, 1);
  } else if (strcmp(name, "Remove") == 0) {
    lua_pushlightuserdata(L, (void *)collection);
    lua_pushcclosure(L, name_value_collection_Remove, 1);
  } else {
    lua_pushstring(L, "Unknown property or method.");
    lua_error(L);
  }

  return 1;
}

int CreateNameValueCollection(lua_State *L, NameValueCollection *collection) {
  int stack = lua_gettop(L);

  NameValueCollection **udata =
      (NameValueCollection **)lua_newuserdata(L, sizeof(NameValueCollection *));
  *udata = collection;
  if (luaL_newmetatable(L, "asplite_NameValueCollection")) {
    lua_pushcfunction(L, name_value_collection___index);
    lua_setfield(L, -2, "__index");
  }
  lua_setmetatable(L, -2);

  assert(stack + 1 == lua_gettop(L));
  return 1;
}

int QueryString___tostring(lua_State *L) {
  NameValueCollection *collection = GetCollection(L, lua_upvalueindex(1));
  std::string query_string = CreateQueryString(*collection);
  lua_pushlstring(L, query_string.data(), query_string.length());
  return 1;
}
