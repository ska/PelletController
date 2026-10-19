#!/bin/sh
# The launcher's update is an uninstall of the old version followed by an
# install of the new one (as found for CanGateway on the same devices), so
# everything not in the package is lost: save the device configuration
# (setting.ini: serial port, broker). install.sh restores it.

cd "$(dirname "$0")"
BACKUP=/tmp/pelletcontroller-backup

./stop.sh

rm -rf "$BACKUP"
mkdir -p "$BACKUP"
for f in setting.ini; do
    [ -f "deploy/$f" ] && cp -p "deploy/$f" "$BACKUP/"
done
sync

echo "PelletController uninstall: configuration saved to $BACKUP" | logger -t PelletController
exit 0
