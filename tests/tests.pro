# Unit tests (QtTest), no stove needed: SerialProto against the stove
# simulator (tools/stovesim) on a pseudo terminal, Linux only; the Home
# Assistant payloads (no broker needed). Built by the
# top level project with CONFIG+=tests:
#   qmake PelletController.pro CONFIG+=tests && make && ./tests/tests
# Real time (200 ms per request, as on the panel): about 1.5 minutes.

QT       += core gui testlib
CONFIG   += c++17 console testcase
CONFIG   -= app_bundle
TARGET    = tests

INCLUDEPATH += $$PC_SRC
PC_MODULES += mqtt
include($$PC_SRC/ThirdParty/qtmodules.pri)
include($$PC_SRC/version.pri)
include($$PC_SRC/tools/stovesim/stovesim.pri)

SOURCES += \
    main.cpp \
    tst_homeassistant.cpp \
    tst_serialproto.cpp \
    $$PC_SRC/homeassistant.cpp \
    $$PC_SRC/serialproto.cpp

HEADERS += \
    testutil.h \
    $$PC_SRC/homeassistant.h \
    $$PC_SRC/serialproto.h
