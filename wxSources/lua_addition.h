/*
 *  lua_addition.h
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

#ifndef LUA_ADDITION_H
#define LUA_ADDITION_H

#include "lualib.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *luaL_tolstring(lua_State *L, int idx, size_t *len);
void lua_register_string_ext(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif /* LUA_ADDITION_H */
