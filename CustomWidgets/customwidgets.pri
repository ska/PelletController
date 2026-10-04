# Consumer side of the custom widgets library: include it in a project to
# use and link them; CustomWidgets.pro must be built first (the top level
# PelletController.pro takes care of the order).

INCLUDEPATH    += $$PC_SRC/CustomWidgets
LIBS           += $$PC_BUILD/CustomWidgets/libcustomwidgets.a
PRE_TARGETDEPS += $$PC_BUILD/CustomWidgets/libcustomwidgets.a
