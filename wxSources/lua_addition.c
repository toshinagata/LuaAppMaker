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

#if defined(WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif

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

/*  UTF-8 string from CP932 (Windows Shift-JIS) string  */
static int
utf8_from_sjis(lua_State *L)
{
    size_t len;
    const char *sjis = lua_tolstring(L, -1, &len);
    if (sjis == NULL)
        luaL_error(L, "Cannot get string");

#if defined(WIN32)
    size_t widelen, utf8len;
    wchar_t *wide;
    char *utf8;
    widelen = MultiByteToWideChar(CP_THREAD_ACP, 0, sjis, -1, NULL, 0);
    wide = calloc(widelen + 1, sizeof(wchar_t));
    if (wide == NULL)
        goto error_alloc;
    if (MultiByteToWideChar(CP_THREAD_ACP, 0, sjis, -1, wide, widelen + 1) == 0) {
        free(wide);
        goto error_conv;
    }
    utf8len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    utf8 = calloc(utf8len + 1, sizeof(char));
    if (sjis == NULL) {
        free(wide);
        goto error_alloc;
    }
    if (WideCharToMultiByte(CP_UTF8, 0, wide, widelen + 1, utf8, utf8len + 1, NULL, NULL) == 0) {
        free(wide);
        free(utf8);
        goto error_conv;
    }
    lua_pop(L, 1);
    lua_pushlstring(L, utf8, utf8len);
    free(wide);
    free(utf8);
    return 1;
#elif defined(__APPLE__)
    CFStringRef ref = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)sjis, len, kCFStringEncodingDOSJapanese, false);
    size_t widelen = CFStringGetLength(ref);
    char *utf8 = (char *)calloc(widelen * 6 + 1, sizeof(char));
    if (utf8 == NULL) {
        CFRelease(ref);
        goto error_alloc;
    }
    if (!CFStringGetCString(ref, utf8, widelen * 6 + 1, kCFStringEncodingUTF8)) {
        CFRelease(ref);
        free(utf8);
        goto error_conv;
    }
    lua_pop(L, 1);
    lua_pushlstring(L, utf8, strlen(utf8));
    free(utf8);
    CFRelease(ref);
    return 1;
#endif
error_conv:
    luaL_error(L, "Cannot convert");
error_alloc:
    luaL_error(L, "Cannot get string");
    return 0;
}

/*  UTF-8 string to CP932 (Windows Shift-JIS) string  */
static int
utf8_to_sjis(lua_State *L)
{
    size_t len;
    const char *utf8 = lua_tolstring(L, -1, &len);
    if (utf8 == NULL)
        luaL_error(L, "Cannot get string");
    
#if defined(WIN32)
    size_t widelen, sjislen;
    wchar_t *wide;
    char *sjis;
    widelen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    wide = calloc(widelen + 1, sizeof(wchar_t));
    if (wide == NULL)
        goto error_alloc;
    if (MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, widelen + 1) == 0) {
        free(wide);
        goto error_conv;
    }
    sjislen = WideCharToMultiByte(CP_THREAD_ACP, 0, wide, -1, NULL, 0, NULL, NULL);
    sjis = calloc(sjislen + 1, sizeof(char));
    if (sjis == NULL) {
        free(wide);
        goto error_alloc;
    }
    if (WideCharToMultiByte(CP_THREAD_ACP, 0, wide, widelen + 1, sjis, sjislen + 1, NULL, NULL) == 0) {
        free(wide);
        free(sjis);
        goto error_conv;
    }
    lua_pop(L, 1);
    lua_pushlstring(L, sjis, sjislen);
    free(wide);
    free(sjis);
    return 1;
#elif defined(__APPLE__)
    CFStringRef ref = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)utf8, len, kCFStringEncodingUTF8, false);
    size_t widelen = CFStringGetLength(ref);
    char *sjis = (char *)calloc(widelen * 2 + 1, sizeof(char));
    if (sjis == NULL) {
        CFRelease(ref);
        goto error_alloc;
    }
    if (!CFStringGetCString(ref, sjis, widelen * 2 + 1, kCFStringEncodingDOSJapanese)) {
        CFRelease(ref);
        free(sjis);
        goto error_conv;
    }
    lua_pop(L, 1);
    lua_pushlstring(L, sjis, strlen(sjis));
    free(sjis);
    CFRelease(ref);
    return 1;
#endif
error_conv:
    luaL_error(L, "Cannot convert");
error_alloc:
    luaL_error(L, "Cannot get string");
    return 0;
}

void
lua_register_string_ext(lua_State *L)
{
    lua_getglobal(L, "string");
    lua_pushcfunction(L, string_from_table);
    lua_setfield(L, -2, "fromtable");
    lua_pushcfunction(L, string_to_table);
    lua_setfield(L, -2, "totable");
    lua_pushcfunction(L, utf8_from_sjis);
    lua_setfield(L, -2, "fromsjis");
    lua_pushcfunction(L, utf8_to_sjis);
    lua_setfield(L, -2, "tosjis");
    lua_pop(L, 1);
}
