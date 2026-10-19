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
    $$PC_SRC/singleinstance.cpp \
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
    $$PC_SRC/singleinstance.h \
    $$PC_SRC/timepicker.h \
    $$PC_SRC/uiicons.h

DESTDIR = $$PC_BUILD/bin_$${QT_ARCH}

## Install to remote target (Qt Creator deploy)
target.path = /tmp
INSTALLS += target

##############################################################
# Settings template: setting.ini is created from it at the first start
##############################################################
QMAKE_POST_LINK += $$quote($(COPY_FILE) $${PC_SRC}/setting.default.ini $$DESTDIR ;)
conf_file.path = /tmp
conf_file.files += $${PC_SRC}/setting.default.ini
INSTALLS += conf_file

##############################################################
# Deployable package (tools/package/README.md), as AcquaThermoNet and
# RoomSense. Built after every link of an ARM build; desktop builds only
# with CONFIG+=package, ARM builds can skip it with CONFIG+=nopackage.
# Output: <build dir>/dist/PelletController_Package_<arch>_<version>.zip
##############################################################
PKG_ARCH = $${QT_ARCH}
equals(QT_ARCH, "arm"):     PKG_ARCH = Arm32
equals(QT_ARCH, "arm64"):   PKG_ARCH = Arm64
if(equals(QT_ARCH, "arm")|equals(QT_ARCH, "arm64")|CONFIG(package)):!CONFIG(nopackage) {
    # $(STRIP) / $(CXX): the toolchain of this Makefile (the strip is searched
    # next to the compiler when the SDK environment is not loaded, Qt Creator)
    QMAKE_POST_LINK += $$quote($${PC_SRC}/tools/package/make_package.sh --bin-dir $$DESTDIR --arch $$PKG_ARCH --out-dir $$PC_BUILD/dist --strip \"$(STRIP)\" --cxx \"$(CXX)\" ;)
    message("Package: $$PC_BUILD/dist ($$PKG_ARCH)")
}
