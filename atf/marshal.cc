#include "marshal.h"
extern "C" {
#include <lua5.2/lua.h>
#include <lua5.2/lualib.h>
#include <lua5.2/lauxlib.h>
}
#include <QMap>
#include <QList>
#include <QString>
QList<Marshaller*> get_marshalling_list(const char* signature)
{
  QList<Marshaller*> retval;
  char buffer[80];
  // Assumed that signature is normalized
  const char *p = signature;
  while (*p && *p++ != '(');
  while (*p && *p != ')') {
    char *b = buffer;
    while (*p && *p != ',' && *p != ')') {
      *b++ = *p++;
    }
    *b = '\0';
    if (*p == ',') ++p;
    retval.append(Marshaller::get(buffer));
  }
  return retval;
}
namespace {
  // int Marshaller
  class : public Marshaller {
   public:
    void* Marshal(lua_State *L, int index) {
      int isnum = 0;
      int val = lua_tointegerx(L, index, &isnum);
      if (!isnum)
        return NULL;
      return new int(val);
    }
    void Dispose(void *obj) {
      delete static_cast<int*>(obj);
    }
    void Unmarshal(void *obj, lua_State *L) {
      lua_pushinteger(L, *static_cast<int*>(obj));
    }
  } intMarshaller;
  // qint64 Marshaller
  class : public Marshaller {
   public:
    void* Marshal(lua_State *L, int index) {
      int isnum = 0;
      qint64 val = lua_tointegerx(L, index, &isnum);
      if (!isnum)
        return NULL;
      return new qint64(val);
    }
    void Dispose(void *obj) {
      delete static_cast<qint64*>(obj);
    }
    void Unmarshal(void *obj, lua_State *L) {
      lua_pushinteger(L, *static_cast<qint64*>(obj));
    }
  } qint64Marshaller;
  // QString Marshaller
  class : public Marshaller {
   public:
    void* Marshal(lua_State *L, int index) {
      const char * val = lua_tostring(L, index);
      if (!val)
        return NULL;
      return new QString(val);
    }
    void Dispose(void *obj) {
      delete static_cast<QString*>(obj);
    }
    void Unmarshal(void *obj, lua_State *L) {
      lua_pushstring(L, static_cast<QString*>(obj)->toUtf8().constData());
    }
  } QStringMarshaller;
  // QByteArray Marshaller
  class : public Marshaller {
   public:
    void* Marshal(lua_State *L, int index) {
      size_t size;
      const char * val = lua_tolstring(L, index, &size);
      if (!val)
        return NULL;
      return new QByteArray(val, size);
    }
    void Dispose(void *obj) {
      delete static_cast<QByteArray*>(obj);
    }
    void Unmarshal(void *obj, lua_State *L) {
      QByteArray *ba = static_cast<QByteArray*>(obj);
      lua_pushlstring(L, ba->constData(), ba->size());
    }
  } QByteArrayMarshaller;
  // bool Marshaller
  class : public Marshaller {
   public:
    void* Marshal(lua_State *L, int index) {
      bool val = lua_toboolean(L, index);
      return new bool(val);
    }
    void Dispose(void *obj) {
      delete static_cast<bool*>(obj);
    }
    void Unmarshal(void *obj, lua_State *L) {
      lua_pushboolean(L, *static_cast<bool*>(obj));
    }
  } boolMarshaller;
  QMap<QString, Marshaller*> marshallers = {
    { "int", &intMarshaller },
    { "qint64", &qint64Marshaller },
    { "bool", &boolMarshaller },
    { "QString", &QStringMarshaller },
    { "QByteArray", &QByteArrayMarshaller }
  };
}  // anonymous namespace

Marshaller* Marshaller::get(const QString& type)
{
  return marshallers[type];
}
