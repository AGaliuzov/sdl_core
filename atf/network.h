#line 4 "network.nw"
#pragma once
#line 5 "main.nw"
extern "C" {
#include <lua5.2/lua.h>
#include <lua5.2/lualib.h>
#include <lua5.2/lauxlib.h>
}
#line 6 "network.nw"
#include <QObject>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QTcpServer>
int luaopen_network(lua_State *L);
