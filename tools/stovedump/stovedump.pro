# Read only dump of the stove RAM and EEPROM (every location, hex grid and
# optional CSV). Built by the top level project with CONFIG+=tools, also
# for the panel:
#   qmake PelletController.pro CONFIG+=tools && make
#   tools/stovedump/stovedump --port /dev/ttyUSB0 --csv dump.csv
# Stop the application first. Only read requests are sent.

QT       += core
QT       -= gui
CONFIG   += c++17 console
CONFIG   -= app_bundle
TARGET    = stovedump

include($$PC_SRC/ThirdParty/qtmodules.pri)

SOURCES += stovedump.cpp
