#line 6 "timers.nw"
#pragma once
#line 5 "main.nw"
extern "C" {
#include <lua5.2/lua.h>
#include <lua5.2/lualib.h>
#include <lua5.2/lauxlib.h>
}
#line 8 "timers.nw"
#include <QObject>

int luaopen_timers(lua_State *L);
