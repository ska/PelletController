# Custom widgets used by the GUI, static library linked by the application
# (customwidgets.pri). Built through the top level PelletController.pro.

TEMPLATE = lib
CONFIG  += staticlib c++17
QT      += widgets
TARGET   = customwidgets

include(widgets.pri)
