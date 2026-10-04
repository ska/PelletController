# Pellet Controller: open this file in Qt Creator or run qmake on it.
#
#   ThirdParty     QtSerialPort, static, from the git submodule (missing in
#                  the desktop Qt and on the panel)
#                  (first checkout: git submodule update --init)
#   CustomWidgets  the custom widgets used by the GUI, static library
#   app            the application -> <build>/bin_<arch>/PelletController
#
# Optional, qmake arguments:
#   CONFIG+=tools     GUI screenshot tool (tools/guishot), desktop only
#   CONFIG+=designer  Qt Designer plugin with the custom widgets
#                     (CustomWidgets/designer, make install)

TEMPLATE = subdirs

SUBDIRS += ThirdParty CustomWidgets app
app.depends = ThirdParty CustomWidgets

CONFIG(tools) {
    SUBDIRS += guishot
    guishot.subdir = tools/guishot
    guishot.depends = ThirdParty CustomWidgets
}

CONFIG(designer) {
    SUBDIRS += designer
    designer.subdir = CustomWidgets/designer
}

OTHER_FILES += \
    .qmake.conf \
    version.pri \
    ThirdParty/qtmodules.pri \
    CustomWidgets/customwidgets.pri \
    CustomWidgets/widgets.pri
