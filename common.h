#ifndef COMMON_H
#define COMMON_H

#ifdef WE_PIC
#include "WE/wepiclib.h"
#endif
#ifdef WE_DBUS
#include "WE/wedbuslib.h"
#include "CustomWidgets/switch.h"
#endif
#ifdef WE_WDT
#include "watchdog.h"
#endif




#define SW_NAME     "Pellet Controller"
#define VER_MAJ     "1"
#define VER_MIN     "0"

#ifdef QT_DEBUG
#define VER_FIX     GIT_REVISION
#else
#define VER_FIX     "0"
#endif

#define SW_VER      VER_MAJ "." VER_MIN "-" VER_FIX


#define BUFFSIZE                (256*1024)
#define BASH_PATH               "/bin/bash"

#endif // COMMON_H
