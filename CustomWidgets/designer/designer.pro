# Qt Designer plugin with the custom widgets, to see them in the forms.
# Built by the top level project with CONFIG+=designer; `make install`
# copies it to the plugins/designer folder of the Qt in use.

TEMPLATE = lib
CONFIG  += plugin c++17
QT      += widgets designer
TARGET   = $$qtLibraryTarget(customwidgetsplugin)

include(../widgets.pri)

HEADERS += \
    wClickableLabelPlugin.h \
    wClickableLCDNumberPlugin.h \
    wClockWidgetPlugin.h \
    wTimePickerPlugin.h \
    wcollection.h

SOURCES += \
    wClickableLabelPlugin.cpp \
    wClickableLCDNumberPlugin.cpp \
    wClockWidgetPlugin.cpp \
    wTimePickerPlugin.cpp \
    wcollection.cpp

RESOURCES = icons.qrc

target.path = $$[QT_INSTALL_PLUGINS]/designer
INSTALLS   += target
