#!/bin/sh
# Stops PelletController with SIGTERM: the application publishes MQTT
# "offline", disconnects (max 1s), closes the serial port and exits 0,
# which also ends the start.sh restart loop.
# Waits for the exit so that an update never replaces a running binary;
# after 15s the process is killed.

pkill -TERM -x PelletController 2>/dev/null || exit 0

i=0
while pidof PelletController >/dev/null 2>&1; do
    i=$((i + 1))
    if [ $i -gt 15 ]; then
        echo "PelletController: not stopped after 15s, killing it" | logger -t PelletController
        pkill -KILL -x PelletController 2>/dev/null
        break
    fi
    sleep 1
done
exit 0
