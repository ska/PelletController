# Pellet Controller: open this file in Qt Creator or run qmake on it.
#
#   ThirdParty     QtSerialPort, static, from the git submodule (missing in
#                  the desktop Qt and on the panel)
#                  (first checkout: git submodule update --init)
#   CustomWidgets  the custom widgets used by the GUI, static library
#   app            the application -> <build>/bin_<arch>/PelletController
#
# Optional, qmake arguments:
#   CONFIG+=tests     unit tests -> <build>/tests/tests (stove simulator,
#                     Linux, no stove needed)
#   CONFIG+=tools     stove simulator (tools/stovesim), Linux, and read
#                     only dump of the stove memory (tools/stovedump),
#                     also ARM

TEMPLATE = subdirs

SUBDIRS += ThirdParty CustomWidgets app
app.depends = ThirdParty CustomWidgets

CONFIG(tests) {
    SUBDIRS += tests
    tests.depends = ThirdParty
}

CONFIG(tools) {
    SUBDIRS += stovesim stovedump
    stovesim.subdir = tools/stovesim
    stovedump.subdir = tools/stovedump
    stovedump.depends = ThirdParty
}

OTHER_FILES += \
    .qmake.conf \
    version.pri \
    ThirdParty/qtmodules.pri \
    CustomWidgets/customwidgets.pri \
    CustomWidgets/widgets.pri
