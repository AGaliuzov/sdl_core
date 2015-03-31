#pragma once
extern "C" {
#include <lua5.2/lua.h>
#include <lua5.2/lualib.h>
#include <lua5.2/lauxlib.h>
}
#include <QObject>
#include <QMetaObject>
#include <QHash>
#include <QByteArray>

class LuaInterpreter : public QObject {
  Q_OBJECT
 private:
  lua_State* lua_state;
  int testObject;
 public:
  bool quitCalled = false;
  int retCode = 0;
  LuaInterpreter(QObject *parent);
  int load(const char *filename);
 public slots:
  void quit();
 public:
  ~LuaInterpreter();
};
