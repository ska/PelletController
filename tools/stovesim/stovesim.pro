# Stove board simulator on a pseudo terminal (Linux): run the application
# without a stove. Built by the top level project with CONFIG+=tools:
#   qmake PelletController.pro CONFIG+=tools && make
#   tools/stovesim/stovesim --link /tmp/pellet-stove
# The tests (CONFIG+=tests) use the same simulator (stovesim.pri).

QT       += core
QT       -= gui
CONFIG   += c++17 console
CONFIG   -= app_bundle
TARGET    = stovesim

include(stovesim.pri)

SOURCES += main.cpp
