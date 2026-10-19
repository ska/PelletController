#!/bin/sh
# Starts PelletController and restarts it if it crashes. Stays in the
# foreground, so the launcher's supervision of start.sh still applies.
#
# Exit status of the application (see common.h, as AcquaThermoNet):
#   0  stopped on purpose (SIGTERM/SIGINT: MQTT offline, serial port closed)
#   3  another instance already runs on this device
#   anything else, or killed by a signal: crash -> restart
# Restart delay grows 2, 4, 8, 16, 20s while it keeps crashing within a
# minute, and goes back to 2s after a run of at least a minute: the stove
# keeps working on its own meanwhile, the panel only reads and commands it.

cd "$(dirname "$0")"
SCRIPTDIR=$(pwd)

[ -z "$DISPLAY" ] && export DISPLAY=:0
[ -z "$QT_QPA_PLATFORM" ] && [ -e /tmp/.X0-lock ] && export QT_QPA_PLATFORM=xcb
[ -e "$SCRIPTDIR/plugin/platforminputcontexts/libdbusvirtualkeyboardplugin.so" ] && export QT_IM_MODULE=dbusvirtualkeyboard

export LD_LIBRARY_PATH="$SCRIPTDIR:$SCRIPTDIR/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$SCRIPTDIR/plugin:$SCRIPTDIR:$QT_PLUGIN_PATH"
export PATH="$SCRIPTDIR:$PATH"

ulimit -c unlimited
# reduce stack size
ulimit -s 1024

delay=2
while true; do
    started=$(date +%s)
    ./PelletController "$@"
    status=$?

    case "$status" in
        0) echo "PelletController: stopped" | logger -t PelletController; break ;;
        3) echo "PelletController: another instance is running, not restarting" | logger -t PelletController; break ;;
    esac

    if [ $(( $(date +%s) - started )) -ge 60 ]; then
        delay=2
    fi
    echo "PelletController: exited with status $status, restarting in ${delay}s" | logger -t PelletController
    sleep $delay
    delay=$(( delay * 2 ))
    [ $delay -gt 20 ] && delay=20
done
