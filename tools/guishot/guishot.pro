# Renders the GUI with sample stove data to PNG files, no display and no
# stove needed. Built by the top level project with CONFIG+=tools:
#   qmake PelletController.pro CONFIG+=tools && make
#   QT_QPA_PLATFORM=offscreen tools/guishot/guishot <output dir>

QT       += core gui widgets
CONFIG   += c++17
TARGET    = guishot

DEFINES += DESKTOP=1 DISPLAY32=2 DISPLAY64=3 DEVICE=DESKTOP

INCLUDEPATH += $$PC_SRC
include($$PC_SRC/ThirdParty/qtmodules.pri)
include($$PC_SRC/CustomWidgets/customwidgets.pri)
include($$PC_SRC/version.pri)

SOURCES += \
    guishot.cpp \
    $$PC_SRC/mainwindow.cpp \
    $$PC_SRC/serialproto.cpp \
    $$PC_SRC/timeeditdialog.cpp

HEADERS += \
    $$PC_SRC/mainwindow.h \
    $$PC_SRC/serialproto.h \
    $$PC_SRC/timeeditdialog.h

FORMS += \
    $$PC_SRC/mainwindow.ui \
    $$PC_SRC/timeeditdialog.ui

RESOURCES = $$PC_SRC/resources.qrc
