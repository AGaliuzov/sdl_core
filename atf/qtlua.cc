#line 13 "qtlua.nw"
#include <QObject>
#line 5 "main.nw"
extern "C" {
#include <lua5.2/lua.h>
#include <lua5.2/lualib.h>
#include <lua5.2/lauxlib.h>
}
#line 15 "qtlua.nw"
#include "qtlua.h"
#include "qtdynamic.h"
#include "marshal.h"
#include <QMetaObject>
#include <QDebug>
static int qtlua_createdynamic(lua_State *L);
static int qtlua_connect(lua_State *L);
static int qtlua_disconnect(lua_State *L);
static int dynamicTableId = 0; // Registry index of table used to mark dynamic userdata

int luaopen_qt(lua_State *L) {
  lua_newtable(L);
  dynamicTableId = luaL_ref(L, LUA_REGISTRYINDEX);
  luaL_Reg functions[] = {
    { "dynamic", &qtlua_createdynamic },
    { "connect", &qtlua_connect },
    { "disconnect", &qtlua_disconnect },
    { NULL, NULL }
  };
  luaL_newlib(L, functions);
  return 1;
}
#line 43 "qtlua.nw"
int qtlua_deletedynamic(lua_State *L) {
  DynamicObject * li = *static_cast<DynamicObject**>(lua_touserdata(L, 1));
  delete li;
  return 0;
}
int qtlua_createdynamic(lua_State *L) {
  lua_getglobal(L, "interp");  // QObject LuaInterpreter must have been saved in global 'interp'
  QObject * interp = *(static_cast<QObject**>(lua_touserdata(L, -1)));
  QObject **p =  static_cast<QObject**>(lua_newuserdata(L, sizeof(QObject*)));
  *p = new DynamicObject(interp);
  // Set metatable with "__index" and "__newindex" set to one empty table
  lua_newtable(L); // metatable
  lua_newtable(L); // __index and __newindex table
  lua_pushnil(L);
  lua_copy(L, -2, -1);
  lua_setfield(L, -3, "__index");
  lua_setfield(L, -2, "__newindex");
  lua_pushcfunction(L, &qtlua_deletedynamic);
  lua_setfield(L, -2, "__gc");
  lua_setmetatable(L, -2);
  // Mark userdatum with the specific uservalue
  lua_rawgeti(L, LUA_REGISTRYINDEX, dynamicTableId);
  lua_setuservalue(L, -2);
  return 1;
}
#line 70 "qtlua.nw"
// Helper function, pushes onto top of stack
// __index value of metatable of value on given index
// If value doesn't have metatable or there is no __index field,
// function creates them.
static void get_index_table(lua_State *L, int idx);
// Function emits the given signal of dynamic object.
// It takes two  upvalues: signal name and list of argument types to marshal them properly
static int qtlua_emit_signal(lua_State *L);
#line 86 "qtlua.nw"
int qtlua_connect(lua_State *L) {
  QObject *sender, *receiver;
  bool senderIsDynamic = false;
  bool receiverIsDynamic = false;
  const char *signal = luaL_checkstring(L, 2);
  const char *slot = luaL_checkstring(L, 4);

  sender = *static_cast<QObject**>(lua_touserdata(L, 1));
  receiver = *static_cast<QObject**>(lua_touserdata(L, 3));
  if (!sender) {
    return luaL_error(L, "connect: sender must be a qt object");
  }
  if (!receiver) {
    return luaL_error(L, "connect: receiver must be a qt object");
  }

  // We use uservalue bound to the userdata to identify dynamic qobjects
  lua_getuservalue(L, 1);
  lua_getuservalue(L, 3);
  lua_rawgeti(L, LUA_REGISTRYINDEX, dynamicTableId);
  senderIsDynamic = lua_rawequal(L, -1, -3);
  receiverIsDynamic = lua_rawequal(L, -1, -2);
  lua_pop(L, 3);

  if (senderIsDynamic && receiverIsDynamic) {
    DynamicObject * d_sender = static_cast<DynamicObject*>(sender);
    DynamicObject * d_receiver = static_cast<DynamicObject*>(receiver);

    
#line 134 "qtlua.nw"
    QByteArray theSignal = QMetaObject::normalizedSignature(signal);

    get_index_table(L, 1);
    // TODO: check if this function already is bound
    // Add signal emitter function
    //  Push signal name (upvalue 1)
    lua_pushstring(L, theSignal);
    //  Push marshallers table (upvalue 2)
    auto marshallers = get_marshalling_list(theSignal);
    lua_newtable(L);
    int i = 1;  // In Lua 1-based arrays are common
    for (auto m : marshallers) {
      lua_pushlightuserdata(L, m);
      lua_rawseti(L, -2, i++);
    }
    //  Push emitter
    lua_pushcclosure(L, &qtlua_emit_signal, 2);
    int idx = theSignal.indexOf('(');
    theSignal.truncate(idx);
    lua_setfield(L, -2, theSignal);
#line 115 "qtlua.nw"
    
#line 167 "qtlua.nw"
    // Register the receiver object in the registry and store its index
    lua_pushnil(L);
    lua_copy(L, 3, -1);
    int objref = luaL_ref(L, LUA_REGISTRYINDEX);
#line 116 "qtlua.nw"
    bool res = DynamicObject::connectDynamicSignalToDynamicSlot(d_sender,
      signal,
      d_receiver,
      slot,
      new DynamicSlot(L, objref, slot));
    lua_pushboolean(L, res);
  } else if (senderIsDynamic) {
#line 129 "qtlua.nw"
    DynamicObject * d_sender = static_cast<DynamicObject*>(sender);
    bool res = d_sender->connectDynamicSignal(signal, receiver, slot);

    
#line 134 "qtlua.nw"
    QByteArray theSignal = QMetaObject::normalizedSignature(signal);

    get_index_table(L, 1);
    // TODO: check if this function already is bound
    // Add signal emitter function
    //  Push signal name (upvalue 1)
    lua_pushstring(L, theSignal);
    //  Push marshallers table (upvalue 2)
    auto marshallers = get_marshalling_list(theSignal);
    lua_newtable(L);
    int i = 1;  // In Lua 1-based arrays are common
    for (auto m : marshallers) {
      lua_pushlightuserdata(L, m);
      lua_rawseti(L, -2, i++);
    }
    //  Push emitter
    lua_pushcclosure(L, &qtlua_emit_signal, 2);
    int idx = theSignal.indexOf('(');
    theSignal.truncate(idx);
    lua_setfield(L, -2, theSignal);
#line 155 "qtlua.nw"
    
    lua_pushboolean(L, res);
  } else if (receiverIsDynamic) {
    DynamicObject * d_receiver = static_cast<DynamicObject*>(receiver);
#line 165 "qtlua.nw"
    
#line 167 "qtlua.nw"
    // Register the receiver object in the registry and store its index
    lua_pushnil(L);
    lua_copy(L, 3, -1);
    int objref = luaL_ref(L, LUA_REGISTRYINDEX);
#line 172 "qtlua.nw"
    bool res = d_receiver->connectDynamicSlot(sender, signal, slot,
        new DynamicSlot(L, objref, slot));
    lua_pushboolean(L, res);
  } else {
    bool res = QObject::connect(sender, signal, receiver, slot);
    lua_pushboolean(L, res);
  }
  return 1;
}
#line 186 "qtlua.nw"
void get_index_table(lua_State *L, int idx)
{
  lua_getmetatable(L, idx);
  if (lua_isnil(L, -1)) {
    lua_newtable(L);
    lua_copy(L, -1, -2);
    lua_setmetatable(L, idx > 0 ? idx : idx - 2);
  }
  lua_getfield(L, -1, "__index");
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);
    lua_newtable(L);
    lua_pushnil(L);
    lua_copy(L, -2, -1);
    lua_setfield(L, -3, "__index");
  }
  lua_remove(L, -2);
}
#line 369 "qtlua.nw"
int qtlua_emit_signal(lua_State *L)
{
  DynamicObject * li = *static_cast<DynamicObject**>(lua_touserdata(L, 1));
  const char* theSignal = lua_tostring(L, lua_upvalueindex(1));
  int mid = lua_upvalueindex(2); // Marshallers table pseudoindex
  lua_len(L, mid);
  int msize = lua_tointegerx(L, -1, NULL);
  lua_pop(L, 1);
  void *stackargs[8];
  void **args = stackargs;
  if (msize > 8) {
    args = new void*[msize];
  }
  args[0] = NULL;
  for (int i = 1; i <= msize; ++i) {
    lua_rawgeti(L, mid, i);
    auto marshaller = static_cast<Marshaller*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    args[i] = marshaller->Marshal(L, i + 1);
  }
  li->emitDynamicSignal(theSignal, args);
  if (args != stackargs) {
    delete[] args;
  }
  return 0;
}
#line 397 "qtlua.nw"
int qtlua_disconnect(lua_State *L)
{
  (void)L;
  return 0;
}
