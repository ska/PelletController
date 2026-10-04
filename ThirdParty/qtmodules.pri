# Qt modules built from the git submodules by ThirdParty.pro as static
# libraries (same version as the device Qt, see the submodule tag):
#
#   QtSerialPort missing in the desktop Qt, and on the device only in the
#                SDK sysroot, not on the panel: always from
#                ThirdParty/qtserialport, static
#
# Include it in a project to link them; ThirdParty.pro must be built first
# (the top level PelletController.pro takes care of the order).

PC_THIRDPARTY_BUILD = $$PC_BUILD/ThirdParty

# Consumer side (application, tools): skipped by ThirdParty.pro itself
!equals(TEMPLATE, aux) {
    INCLUDEPATH    += $$PC_THIRDPARTY_BUILD/qtserialport/include
    LIBS           += $$PC_THIRDPARTY_BUILD/qtserialport/lib/libQt5SerialPort.a
    PRE_TARGETDEPS += $$PC_THIRDPARTY_BUILD/qtserialport/lib/libQt5SerialPort.a

    # The static QtSerialPort calls libudev directly when the Qt in use has
    # the libudev feature (serialport-lib.pri: LINK_LIBUDEV), and a static
    # library does not bring its dependencies: same condition and library
    # as the module, read from the Qt qmodule.pri (not included: only these
    # two values are taken)
    PC_QMODULE = $$[QT_HOST_DATA/get]/mkspecs/qmodule.pri
    exists($$PC_QMODULE) {
        PC_QT_FEATURES = $$fromfile($$PC_QMODULE, QT.global_private.enabled_features)
        contains(PC_QT_FEATURES, libudev): LIBS += $$fromfile($$PC_QMODULE, QMAKE_LIBS_LIBUDEV)
    }
}
