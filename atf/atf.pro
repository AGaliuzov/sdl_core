HEADERS = network.h \
          timers.h \
          qtdynamic.h \
          qtlua.h \
          marshal.h \
          lua_interpreter.h
SOURCES = network.cc \
          timers.cc \
          qtdynamic.cc \
          qtlua.cc \
          marshal.cc \
          main.cc \
          lua_interpreter.cc
TARGET  = interp
QT += network websockets
CONFIG += c++11 qt debug
QMAKE_RPATHDIR = ./libs
LIBS += -llua5.2
