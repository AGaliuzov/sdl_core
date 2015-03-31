#pragma once
extern "C" {
#include <lua5.2/lua.h>
#include <lua5.2/lualib.h>
#include <lua5.2/lauxlib.h>
}
#include <QObject>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QTcpServer>
int luaopen_network(lua_State *L);
