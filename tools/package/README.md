# Pellet Controller deployment package

`make_package.sh` builds the zip installed on the panel by its launcher
(JMLauncher): a `package.info` descriptor, the lifecycle hooks and the
payload. Same layout and hooks as the AcquaThermoNet and RoomSense
packages: application and `start.sh` in `deploy/`. No library is bundled:
Qt Serial Port and Qt MQTT are linked statically (built from the
`ThirdParty` submodules, the panel has neither), the rest of Qt comes from
the device. Run-time dependencies outside Qt: the C/C++ libraries and
`libudev.so.1` (used by Qt Serial Port, present on the panel).

The panel is a Cortex-A7 (hardware division, VFPv4): built with the SDK
of the panel as it is, no CPU option.

## Built with every build

`app/app.pro` runs `make_package.sh` after every link of an ARM build
(Arm32, Arm64), so building in Qt Creator or with `make` is enough:

```
<build dir>/dist/PelletController_Package_Arm32_<version>.zip
<build dir>/dist/PelletController_Arm32_<version>.debug
```

- desktop builds: only with `CONFIG+=package` (qmake argument);
- ARM builds without the package: `CONFIG+=nopackage`;
- it runs only when the binary is relinked, not on a no-op `make`.

Debug information: the binary in the zip is **stripped** (the SDK
compiles with `-g` also in release). The full binary is kept next to the
zip as `PelletController_<arch>_<version>.debug`: keep it with the release
to read a core dump of the device (`gdb PelletController_Arm32_<version>.debug
core`; `start.sh` enables core dumps). The strip is the one of the build
toolchain (`$(STRIP)` of the Makefile, searched next to the compiler when
the SDK environment is not loaded, as in Qt Creator); a missing or wrong
strip stops the packaging with an error. `--no-strip` packages the full
binary.

Version: the latest git tag `vX.Y.Z` reachable from the built commit
(`0.0.0` without one or outside a git checkout); built after the tag, the
commit short hash is appended; `--version` overrides it. The same as the
application (`version.pri`). By hand:

```
tools/package/make_package.sh --bin-dir <build>/bin_arm --arch Arm32 \
    --strip arm-poky-linux-gnueabi-strip      # SDK environment loaded
```

## Content

```
package.info                name, version, installationFolder, executeAsRoot=true, background=false
install.sh uninstall.sh update.sh run.sh stop.sh
deploy/
    PelletController        binary, stripped (Qt Serial Port, Qt MQTT linked statically)
    setting.default.ini     configuration template
    start.sh                environment and restart loop
```

`deploy/` is also the working directory of the application: `setting.ini`
is created there.

- **run.sh**: entry point called by the launcher. Closes the boot splash
  (`killall xsplash`, as the other packages of the panel), runs
  `deploy/start.sh` in a subshell, keeps `/var/run/pelletcontroller-run.pid`,
  sends the `appLoaded`/`appFinished` dbus notifications.
- **start.sh**: environment (DISPLAY, xcb, Qt plugins, `lib/` in
  `LD_LIBRARY_PATH`) and a restart loop. Exit status of the application:
  - `0`: stopped on purpose (SIGTERM/SIGINT: MQTT offline, serial port
    closed): loop ends;
  - `3`: another instance already runs on the device: loop ends;
  - anything else or a signal: crash, restarted after 2, 4, 8, 16, 20s
    (back to 2s after a run of at least a minute).
- **stop.sh**: SIGTERM to the application and wait for the clean
  shutdown; killed after 15s.
- **install.sh**: restore of the `setting.ini` saved by `uninstall.sh`
  into `deploy/`, `showProgress`/`installFinished` dbus notifications (and
  soname symlinks for libraries in `deploy/lib/`, if a package ever
  bundles some with `--lib-dir`).
- **uninstall.sh**: stops the application and saves `deploy/setting.ini`
  to `/tmp/pelletcontroller-backup`. The launcher's update is uninstall +
  install, so this is what keeps the device configuration (serial port,
  broker) across an update.
- **update.sh**: nothing to do, `updateFinished` notification.

The dbus notifications go to the launcher service, whose name (`*.JMLauncher`)
the hooks look up on the system bus at run time (`ListNames`).

On a fresh install there is no `setting.ini`: the application creates it
from `setting.default.ini` at the first start (serial port
`/dev/ttyUSB0`, MQTT off). Edit it for the device (broker for Home
Assistant) and restart.

## To verify on the device

- `background=false`: Pellet Controller is the foreground, full screen
  application of its panel;
- `installationFolder` is `PelletController`;
- the `appLoaded` check waits 5s for the process `PelletController`.
