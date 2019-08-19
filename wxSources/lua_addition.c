/*
 *  lua_addition.c
 *
 *  Created by Toshi Nagata on 2018/10/17.
 *  Copyright 2018 Toshi Nagata. All rights reserved.
 *
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation version 2 of the License.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 */

#include "lua_addition.h"
#include <stdlib.h>

#if 0
#pragma mark ====== Backported from Lua 5.2 ======
#endif

const char *
luaL_tolstring(lua_State *L, int idx, size_t *len) {
    if (!luaL_callmeta(L, idx, "__tostring")) {  /* no metafield? */
        switch (lua_type(L, idx)) {
            case LUA_TNUMBER:
            case LUA_TSTRING:
                lua_pushvalue(L, idx);
                break;
            case LUA_TBOOLEAN:
                lua_pushstring(L, (lua_toboolean(L, idx) ? "true" : "false"));
                break;
            case LUA_TNIL:
                lua_pushliteral(L, "nil");
                break;
            default:
                lua_pushfstring(L, "%s: %p", luaL_typename(L, idx),
                                lua_topointer(L, idx));
                break;
        }
    }
    return lua_tolstring(L, -1, len);
}

#if 0
#pragma mark ====== Addition for string library ======
#endif

/*  Table of numbers (0-255) to a string  */
static int
string_from_table(lua_State *L)
{
    int i;
    size_t len = lua_objlen(L, -1);
    char *p = (char *)malloc(len + 1);
    if (p == NULL)
        luaL_error(L, "Cannot allocate memory");
    for (i = 0; i < len; i++) {
        lua_pushinteger(L, i + 1);
        lua_gettable(L, -2);
        p[i] = lua_tointeger(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_pushlstring(L, p, len);
    return 1;
}

/*  String to a table of numbers (0-255)  */
static int
string_to_table(lua_State *L)
{
    int i;
    size_t len;
    const char *p = lua_tolstring(L, -1, &len);
    if (p == NULL)
        luaL_error(L, "Cannot get string");
    lua_createtable(L, len, 0);
    for (i = 0; i < len; i++) {
        lua_pushinteger(L, (unsigned char)p[i]);
        lua_rawseti(L, -2, i + 1);
    }
    lua_remove(L, -2);
    return 1;
}

void
lua_register_string_ext(lua_State *L)
{
    lua_getglobal(L, "string");
    lua_pushcfunction(L, string_from_table);
    lua_setfield(L, -2, "fromtable");
    lua_pushcfunction(L, string_to_table);
    lua_setfield(L, -2, "totable");
    lua_pop(L, 1);
}
