QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += qtc_runnable
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

DEFINES += DESKTOP=1
DEFINES += DISPLAY32=2
DEFINES += DISPLAY64=3
DEFINES += BUILD_DATE='"\\\"$(shell date)\\\""'
DEFINES += GIT_REVISION='\\"$$system(git rev-parse --short HEAD)\\"'
DEPLOY_PATH=/tmp/

CONFIG(release, debug|release):message("Build Arch: " $${QT_ARCH} " Release")
CONFIG(debug,   debug|release):message("Build Arch: " $${QT_ARCH} " Debug")
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    serialproto.cpp \
    timeeditdialog.cpp

HEADERS += \
    mainwindow.h \
    common.h \
    serialproto.h \
    timeeditdialog.h

FORMS += \
    mainwindow.ui \
    timeeditdialog.ui

RESOURCES = resources.qrc

####################################
## Lib LS
####################################
LIBS        += -L$$OUT_PWD/../MyCustomWidget/lib
INCLUDEPATH += $$OUT_PWD/../MyCustomWidget/lib
DEPENDPATH  += $$OUT_PWD/../MyCustomWidget/lib

####################################
## i5/i7 Arm32
####################################
equals(QT_ARCH, "arm") {
    DEFINES += DEVICE=DISPLAY32
    LIB_PATH=ThirdParty/Arm32/

    INCLUDEPATH     += $$PWD/$$LIB_PATH/include/
    DEPENDPATH      += $$PWD/$$LIB_PATH/lib/
    DEPENDPATH      += $$PWD/$$LIB_PATH/include
    LIBS            += -L$$PWD/$$LIB_PATH/lib/

    LIBS            += -lQt5SerialPort
    LIBS            += -lwcollectionplugin


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
    LIB_PATH=ThirdParty/x86_64/
    #LIB_PATH=../build/Desktop_5_13_2-Debug/LsCustomWidget

    INCLUDEPATH     += $$PWD/$$LIB_PATH/include/
    DEPENDPATH      += $$PWD/$$LIB_PATH/lib/
    DEPENDPATH      += $$PWD/$$LIB_PATH/include
    LIBS            += -L$$PWD/$$LIB_PATH/lib/


    LIBS            += -lQt5SerialPort
    LIBS            += -lwcollectionplugin
}
# Default rules for deployment.


# Default rules for deployment.
target.path = $${DEPLOY_PATH}/
INSTALLS += target
DESTDIR += bin_$$QT_ARCH

##############################################################
# Install SO files
##############################################################
QMAKE_POST_LINK += $$quote($(COPY_DIR) $$DEPENDPATH  $$DESTDIR/ ;)
## Install SO Libs to remote target
so_libs.path =$${DEPLOY_PATH}/lib
so_libs.files += $${PWD}/$$LIB_PATH/lib/*.so*
INSTALLS += so_libs

