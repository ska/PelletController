# Builds the Qt modules from the git submodules as static libraries, with
# the qmake of the Qt in use (desktop or cross SDK), into
# <build>/ThirdParty/<module>. Only when the library is missing: after
# changing a submodule, delete <build>/ThirdParty (or run a clean).
#
# First checkout: git submodule update --init

TEMPLATE = aux
include(qtmodules.pri)

# $$1 = submodule dir (qtserialport), $$2 = library (Qt5SerialPort)
defineTest(pcBuildModule) {
    src = $$PWD/$$1
    out = $$PC_THIRDPARTY_BUILD/$$1
    lib = $$out/lib/lib$${2}.a
    !exists($$src/$${1}.pro): \
        error("ThirdParty/$$1 is empty: run 'git submodule update --init'")

    name = build_$$1
    $${name}.target   = $$lib
    $${name}.commands = mkdir -p $$out && cd $$out && \
        $$QMAKE_QMAKE $$src/$${1}.pro CONFIG+=static CONFIG+=release CONFIG-=debug_and_release && \
        $(MAKE) sub-src
    export($${name}.target)
    export($${name}.commands)
    QMAKE_EXTRA_TARGETS += $$name
    PRE_TARGETDEPS += $$lib
    export(QMAKE_EXTRA_TARGETS)
    export(PRE_TARGETDEPS)
    return(true)
}

pcBuildModule(qtserialport, Qt5SerialPort)
