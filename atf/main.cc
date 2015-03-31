#line 5 "main.nw"
extern "C" {
#include <lua5.2/lua.h>
#include <lua5.2/lualib.h>
#include <lua5.2/lauxlib.h>
}
#line 16 "main.nw"
#include <QObject>
#include <QCoreApplication>
#include <assert.h>
#include "lua_interpreter.h"
#include <iostream>

int main(int argc, char** argv)
{
  if (argc < 2) {
    std::cerr << "Path to Lua script needed" << std::endl;
    return 1;
  }

  QCoreApplication app(argc, argv);

  LuaInterpreter lua_interpreter(&app);
  int res = lua_interpreter.load(argv[1]);
  if (res) {
    return res;
  }
  if (lua_interpreter.quitCalled) {
    return lua_interpreter.retCode;
  }
  return app.exec();
}
