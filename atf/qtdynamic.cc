#line 66 "dynamic_object.nw"
#include "qtdynamic.h"
#include <QObject>
#include <QDebug>
#line 5 "main.nw"
extern "C" {
#include <lua5.2/lua.h>
#include <lua5.2/lualib.h>
#include <lua5.2/lauxlib.h>
}
#line 70 "dynamic_object.nw"
DynamicObject::DynamicObject(QObject *parent)
  : QObject(parent) { }

static QList<QByteArray> typesFromString(const char *str, const char *end)
{
  QList<QByteArray> result;
  const char *p = str;
  while (p != end)
  {
    if (*p == ',') ++p;
    const char *b = p;
    while (p != end && *p++ != ',');
    if (p > b)
      result.append(QByteArray(b, p - b));
  }
  return result;
}

static int *queuedConnectionTypes(const QList<QByteArray> &typeNames)
{
    int *types = new int [typeNames.count() + 1];
    Q_CHECK_PTR(types);
    for (int i = 0; i < typeNames.count(); ++i) {
        const QByteArray typeName = typeNames.at(i);
        if (typeName.endsWith('*'))
            types[i] = QMetaType::VoidStar;
        else
            types[i] = QMetaType::type(typeName);

        if (!types[i]) {
            qWarning("QObject::connect: Cannot queue arguments of type '%s'\n"
                     "(Make sure '%s' is registered using qRegisterMetaType().)",
                     typeName.constData(), typeName.constData());
            delete [] types;
            return 0;
        }
    }
    types[typeNames.count()] = 0;

    return types;
}

bool DynamicObject::connectDynamicSlot(QObject *obj, const char *signal, const char *slot,
    DynamicSlot *s)
{
  
#line 189 "dynamic_object.nw"
QByteArray theSignal = QMetaObject::normalizedSignature(signal);
QByteArray theSlot   = QMetaObject::normalizedSignature(slot);
if (!QMetaObject::checkConnectArgs(theSignal, theSlot)) {
  qWarning() << "Cannot connect signal" << theSignal << "to slot" << theSlot;
  return false;
}

#line 117 "dynamic_object.nw"
  int signalId = obj->metaObject()->indexOfSignal(theSignal);
  if (signalId < 0) {
    qWarning() << "No such signal " << theSignal;
    return false;
  }

  int slotId = slotIndices.value(theSlot, -1);
  if (slotId < 0) {
    slotId = slotList.size();
    slotIndices[theSlot] = slotId;
    slotList.append(s);
  }

  return QMetaObject::connect(obj, signalId,
          this, slotId + metaObject()->methodCount(),
          Qt::QueuedConnection,
          queuedConnectionTypes(typesFromString(theSlot.constData() + theSlot.indexOf('(') + 1,
                                                theSlot.constData() + theSlot.indexOf(')'))));
}

bool DynamicObject::connectDynamicSignal(const char *signal, QObject *obj, const char *slot)
{
  
#line 189 "dynamic_object.nw"
QByteArray theSignal = QMetaObject::normalizedSignature(signal);
QByteArray theSlot   = QMetaObject::normalizedSignature(slot);
if (!QMetaObject::checkConnectArgs(theSignal, theSlot)) {
  qWarning() << "Cannot connect signal" << theSignal << "to slot" << theSlot;
  return false;
}

#line 141 "dynamic_object.nw"
  int slotId = obj->metaObject()->indexOfSlot(theSlot);
  if (slotId < 0) {
    qWarning() << "Cannot find slot " << theSlot;
    return false;
  }

  int signalId = signalIndices.value(theSignal, -1);
  if (signalId < 0) {
      signalId = signalIndices.size();
      signalIndices[theSignal] = signalId;
  }
  return QMetaObject::connect(this, signalId + metaObject()->methodCount(), obj, slotId,
    Qt::QueuedConnection,
    queuedConnectionTypes(typesFromString(theSignal.constData() + theSignal.indexOf('(') + 1,
                                          theSignal.constData() + theSignal.indexOf(')'))));
}

bool DynamicObject::connectDynamicSignalToDynamicSlot(
  DynamicObject* sender,
  const char *signal,
  DynamicObject* receiver,
  const char *slot,
  DynamicSlot *s)
{
  
#line 189 "dynamic_object.nw"
QByteArray theSignal = QMetaObject::normalizedSignature(signal);
QByteArray theSlot   = QMetaObject::normalizedSignature(slot);
if (!QMetaObject::checkConnectArgs(theSignal, theSlot)) {
  qWarning() << "Cannot connect signal" << theSignal << "to slot" << theSlot;
  return false;
}

#line 167 "dynamic_object.nw"
  int signalId = sender->signalIndices.value(theSignal, -1);
  if (signalId < 0) {
      signalId = sender->signalIndices.size();
      sender->signalIndices[theSignal] = signalId;
  }

  int slotId = receiver->slotIndices.value(theSlot, -1);
  if (slotId < 0) {
    slotId = receiver->slotList.size();
    receiver->slotIndices[theSlot] = slotId;
    receiver->slotList.append(s);
  }

  return QMetaObject::connect(sender,
    signalId + sender->metaObject()->methodCount(),
    receiver,
    slotId + receiver->metaObject()->methodCount(),
    Qt::QueuedConnection,
    queuedConnectionTypes(typesFromString(theSignal.constData() + theSignal.indexOf('(') + 1,
                                          theSignal.constData() + theSignal.indexOf(')'))));
}
#line 196 "dynamic_object.nw"
int DynamicObject::qt_metacall(QMetaObject::Call c, int id, void **arguments)
{
  id = QObject::qt_metacall(c, id, arguments);
  if (id < 0 || c != QMetaObject::InvokeMetaMethod)
      return id;
  Q_ASSERT(id < slotList.size());

  slotList[id]->call(sender(), arguments);
  return -1;
}

bool DynamicObject::emitDynamicSignal(const char *signal, void **arguments)
{
  QByteArray theSignal = QMetaObject::normalizedSignature(signal);
  int signalId = signalIndices.value(theSignal, -1);
  if (signalId >= 0) {
      QMetaObject::activate(this, metaObject(), signalId + metaObject()->methodCount(),
          arguments);
      return true;
  } else {
      return false;
  }
}

DynamicSlot::DynamicSlot(lua_State *L, int objidx, const char *signature)
  : lua_state(L),
    objidx_(objidx)
{
  QByteArray theSignal = QMetaObject::normalizedSignature(signature);
  marshallers_ = get_marshalling_list(theSignal);
  QByteArray slotName = signature;
  slot_ = QString(QByteArray(slotName, slotName.indexOf('(')));
}

void DynamicSlot::call(QObject *sender, void **arguments)
{
  (void)sender;
  lua_rawgeti(lua_state, LUA_REGISTRYINDEX, objidx_);
  lua_getfield(lua_state, -1, slot_.toUtf8().constData());
  if (lua_isfunction(lua_state, -1)) {
    lua_pushnil(lua_state);
    lua_copy(lua_state, -3, -1);
    ++arguments;  // skip return value
    int argc = 1;
    for (auto m : marshallers_) {
      m->Unmarshal(*arguments++, lua_state);
      ++argc;
    }
    lua_call(lua_state, argc, 0);
  } else {
    lua_pop(lua_state, 1); // Remove slot from stack
  }
  lua_pop(lua_state, 1);   // Remove object from stack
}
