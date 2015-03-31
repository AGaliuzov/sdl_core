#line 213 "qtlua.nw"
#pragma once
#line 5 "main.nw"
extern "C" {
#include <lua5.2/lua.h>
#include <lua5.2/lualib.h>
#include <lua5.2/lauxlib.h>
}
#line 215 "qtlua.nw"
#include <QList>
#include <QMap>
#include <QString>

class Marshaller
{
 public:
  static Marshaller *get(const QString& type);
  virtual void* Marshal(lua_State *L, int index) = 0;
  virtual void Dispose(void *obj) = 0;
  virtual void Unmarshal(void *obj, lua_State *L) = 0;
};
// These functions create lists of marshallers
// to bind them to signal emitters and slot callers
QList<Marshaller*> get_marshalling_list(const char* signature);
