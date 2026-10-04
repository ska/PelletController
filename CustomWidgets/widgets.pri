# Sources of the custom widgets, shared by the static library
# (CustomWidgets.pro) and the Qt Designer plugin (designer/designer.pro)

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/wClickableLabel.h \
    $$PWD/wClickableLCDNumber.h \
    $$PWD/wClockWidget.h \
    $$PWD/wTimePicker.h

SOURCES += \
    $$PWD/wClickableLabel.cpp \
    $$PWD/wClickableLCDNumber.cpp \
    $$PWD/wClockWidget.cpp \
    $$PWD/wTimePicker.cpp

FORMS += \
    $$PWD/wClickableLabel.ui \
    $$PWD/wTimePicker.ui
