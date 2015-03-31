#line 9 "qtlua.nw"
#pragma once
#line 5 "main.nw"
extern "C" {
#include <lua5.2/lua.h>
#include <lua5.2/lualib.h>
#include <lua5.2/lauxlib.h>
}
#line 11 "qtlua.nw"
int luaopen_qt(lua_State *L);
