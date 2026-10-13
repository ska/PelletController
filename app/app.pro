# Pellet Controller application. Sources are in the repository root;
# built through the top level PelletController.pro (ThirdParty and
# CustomWidgets first).

QT       += core gui widgets svg
CONFIG   += c++17
CONFIG   += qtc_runnable
TARGET    = PelletController

CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

DEFINES += DESKTOP=1
DEFINES += DISPLAY32=2
DEFINES += DISPLAY64=3

message("Build Arch: " $${QT_ARCH})

####################################
## i5/i7 Arm32
####################################
equals(QT_ARCH, "arm") {
    DEFINES += DEVICE=DISPLAY32
}
####################################
## i10/i12 Arm64
####################################
equals(QT_ARCH, "arm64") {
    DEFINES += DEVICE=DISPLAY64
}
####################################
## Desktop x64
####################################
equals(QT_ARCH, "x86_64") {
    DEFINES += DEVICE=DESKTOP
}

INCLUDEPATH += $$PC_SRC
PC_MODULES += mqtt
include($$PC_SRC/ThirdParty/qtmodules.pri)
include($$PC_SRC/CustomWidgets/customwidgets.pri)
include($$PC_SRC/version.pri)

RESOURCES = $$PC_SRC/resources.qrc

SOURCES += \
    $$PC_SRC/chronopage.cpp \
    $$PC_SRC/clockdialog.cpp \
    $$PC_SRC/diagpage.cpp \
    $$PC_SRC/homeassistant.cpp \
    $$PC_SRC/main.cpp \
    $$PC_SRC/mainpage.cpp \
    $$PC_SRC/mainwindow.cpp \
    $$PC_SRC/serialproto.cpp \
    $$PC_SRC/timepicker.cpp \
    $$PC_SRC/uiicons.cpp

HEADERS += \
    $$PC_SRC/chronopage.h \
    $$PC_SRC/clockdialog.h \
    $$PC_SRC/diagpage.h \
    $$PC_SRC/homeassistant.h \
    $$PC_SRC/common.h \
    $$PC_SRC/mainpage.h \
    $$PC_SRC/mainwindow.h \
    $$PC_SRC/serialproto.h \
    $$PC_SRC/timepicker.h \
    $$PC_SRC/uiicons.h

DESTDIR = $$PC_BUILD/bin_$${QT_ARCH}

## Install to remote target (Qt Creator deploy)
target.path = /tmp
INSTALLS += target
