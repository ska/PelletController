# Known issues

Code review of Pellet Controller (2026-10-04). Line numbers refer to
version 0.1.0. The numbers are stable ids: fixed issues keep their number.

Severity:
- **High**: can write wrong values to the stove or crash the application
- **Medium**: wrong behaviour or values shown, no damage
- **Low**: code quality, maintenance

| # | Issue | Severity | Status |
|---|---|---|---|
| 1 | [Setpoint -/+ without limits](#1-setpoint---without-limits) | High | Fixed in 0.2.0 |
| 2 | [Crash on an unknown stove state](#2-crash-on-an-unknown-stove-state) | High | Fixed in 0.2.0 |
| 3 | [Values read from the stove are written back](#3-values-read-from-the-stove-are-written-back) | High | Fixed in 0.2.0 |
| 4 | [Editing Stop 1 writes Start 1](#4-editing-stop-1-writes-start-1) | High | Fixed in 0.2.0 |
| 5 | [Weekend Start 1 shown 100 minutes late](#5-weekend-start-1-shown-100-minutes-late) | Medium | Fixed in 0.2.0 |
| 6 | [Last parameter never read](#6-last-parameter-never-read) | Low | Fixed in 0.2.0 |
| 7 | [Blocking serial I/O in the GUI thread](#7-blocking-serial-io-in-the-gui-thread) | Medium | Fixed in 0.1.0 |
| 8 | [Reply recognised by its length only](#8-reply-recognised-by-its-length-only) | Medium | Fixed in 0.2.0 |
| 9 | [Writes not verified](#9-writes-not-verified) | Medium | Fixed in 0.2.0 |
| 10 | [Fixed serial port, no reconnection](#10-fixed-serial-port-no-reconnection) | Medium | Fixed in 0.2.0 |
| 11 | [Stale flame power when the stove is off](#11-stale-flame-power-when-the-stove-is-off) | Low | Fixed in 0.2.0 |
| 12 | [200-line switch and dead code](#12-200-line-switch-and-dead-code) | Low | Open |
| 13 | [String based connections, `on_` slot names](#13-string-based-connections-on_-slot-names) | Low | Open |
| 14 | [`QSignalMapper` deprecated](#14-qsignalmapper-deprecated) | Low | Open |
| 15 | [Button logic on the button text](#15-button-logic-on-the-button-text) | Low | Open |
| 16 | [Leaks](#16-leaks) | Low | Open |
| 17 | [Swapped ON/OFF descriptions](#17-swapped-onoff-descriptions) | Low | Open |
| 18 | [Archives in the working tree](#18-archives-in-the-working-tree) | Low | Fixed: `*.tar` ignored |
| 19 | [Daily and weekly chrono read only](#19-daily-and-weekly-chrono-read-only) | Feature | Open |
| 20 | [Chrono read resets Start 1 to 00:00](#20-chrono-read-resets-start-1-to-0000) | High | Fixed in 0.2.0 |
| 21 | [Stop 1 not connected](#21-stop-1-not-connected) | Medium | Fixed in 0.2.0 (slot 2 still missing) |
| 22 | [Off button active during an alarm](#22-off-button-active-during-an-alarm) | Medium | Fixed in 0.2.0 |
| 23 | [GUI texts](#23-gui-texts) | Low | Open |
| 24 | [Invalid reply not read again](#24-invalid-reply-not-read-again) | Low | Fixed in 0.2.0 |
| 25 | [Stove state read twice in a row](#25-stove-state-read-twice-in-a-row) | Low | Fixed in 0.2.0 |
| 26 | [Lost replies not counted](#26-lost-replies-not-counted) | Low | Fixed in 0.2.0 |

Suggested order: 20, 3, 1, 2, 4, 22, then the protocol (8, 9, 10).

---

## 1. Setpoint -/+ without limits

Fixed in 0.2.0: setpoint and power -/+ are ignored until the value
has been read from the stove, and kept inside 5.0-30.0 °C and 1-5
(`SETPOINT_MIN_RAW`, `SETPOINT_MAX_RAW`, `POWER_MIN`, `POWER_MAX` in
`serialproto.h`); a value read outside the range moves to the nearest
limit.

`serialproto.cpp:618`, `serialproto.cpp:623`

`writeStoveDecSetPoint` writes `m_setTemp*2 - 1` without a lower or upper
limit. Before the first read `m_setTemp` is 0: a tap on "-" writes 255
(127.5 °C) to the EEPROM setpoint. The float to `quint8` conversion is
implicit.

Fix: clamp to the stove range, and ignore -/+ until the setpoint has been
read once (the same for the power, which starts at 0).

## 2. Crash on an unknown stove state

Fixed in 0.2.0: a state outside the table is shown as "Unknown (n)";
the On/Off button stays disabled (default branch).

`serialproto.cpp:238`

`m_stoveStateStr.at(m_stoveState)` with a state above 10 is out of range:
`QList::at` asserts in debug and is undefined behaviour in release. A noisy
reply that passes the checksum is enough.

Fix: check the range and show "Unknown (n)".

## 3. Values read from the stove are written back

Fixed in 0.2.0: the slots that show the values read from the stove
set the check boxes under a `QSignalBlocker`, so only a touch on the
panel writes to the stove.

`mainwindow.cpp:193`, `196`, `202`, `214`, `218`

The slots that show the chrono values call `setChecked()` on the check
boxes. `setChecked` emits `stateChanged`, connected to the slots that
write to the stove (`on_chronoEnableCB_stateChanged`, ...). Every value
that changes on the stove (or from the panel at startup) is written back
to the EEPROM: extra writes, EEPROM wear, and writes in the middle of the
polling. See also 20, where the value written back is wrong.

Fix: `QSignalBlocker` around `setChecked`, or connect the writes to
`clicked` / `toggled` from the user only.

## 4. Editing Stop 1 writes Start 1

Fixed in 0.2.0: the LCD that opened the time picker is remembered
(`m_editLcd`), Save writes Start 1 or Stop 1 (`writeChronoWke1Off`).

`mainwindow.cpp:299-304`

The time picker opens for both LCDs (`slot_chronoWkE_LCD_clicked`), but
`chronoWkEStart1_update` always calls `writeChronoWke1On`: a new Stop 1
time is saved as Start 1.

Fix: remember which LCD opened the dialog and write the matching
parameter; add `writeChronoWke1Off`.

## 5. Weekend Start 1 shown 100 minutes late

Fixed in 0.2.0: `+10` removed, the value read is shown as it is.

`mainwindow.cpp:227`

`on_updateChronoWkE1On` adds 10 (100 minutes) to the value read before
showing it. It looks like a debug leftover. With 144 (slot off) read from
the stove, 154 is shown as a time instead of "off".

Fix: remove `+10`.

## 6. Last parameter never read

Fixed in 0.2.0: `requestsMapSize` is the array size, and the array
has `LastIndex` entries.

`serialproto.h:24`

`requestsMapSize` removes one entry for a "fake" position that the array
does not have: entry 62 (`chronoSet_4_DomEnab`) is never read.

Fix: use the array size, or `LastIndex`.

## 7. Blocking serial I/O in the GUI thread

Fixed in 0.1.0.

`waitForReadyRead` in the `readyRead` slot, `waitForBytesWritten` and
`QThread::msleep(150)` blocked the GUI thread. Now replies are buffered
and parsed when the line stays idle for 100 ms, writes are queued and
sent one at a time, and a write without a reply ends after 600 ms so the
polling always restarts (before, a lost write reply stopped the polling
forever).

## 8. Reply recognised by its length only

Fixed in 0.2.0: a read reply is accepted only with 4 bytes starting
with the request sent (`m_readSent`), a write reply only when the write is
confirmed (issue 9); anything else counts as an error (Err) and the
parameter is read again once (issue 24). A reply with the echo only (stove
board not answering) is no longer taken for a value.

`serialproto.cpp:187`

The reply is told apart by its length (6 bytes for a write with its echo,
4 for a read with its echo, 2 without echo) and matched to the request
through `m_previousState`. The echo is not compared with the bytes sent.

Fix: keep the request in flight, check the echo against it, then the
checksum.

## 9. Writes not verified

Fixed in 0.2.0: a write is confirmed only by its full echo, then
`0x80 + bank + address + value` and the value written. Otherwise (no
reply, wrong echo or value) it is sent once more, then `writeFailed` is
emitted and the panel shows a message; the polling shows the value the
stove really has. The echo of the reads is checked too (issue 8).

The reply to a write is parsed like a read, but nothing checks that the
value returned is the one written, and a failed write is not retried or
reported to the user.

Fix: compare the value, retry once, then show an error.

## 10. Fixed serial port, no reconnection

Fixed in 0.2.0: the port name comes from `setting.ini` in the working
directory (`[SERIAL] port`, default `/dev/ttyUSB0`). A port that does not
open, or is lost (`errorOccurred`: resource, read, write error), is opened
again every 3 s (`SERIAL_REOPEN_DELAY`); the polling runs only while the
port is open and restarts by itself. The panel shows "No serial port", or
"No reply" after 10 s without a valid reply (`SERIAL_NO_REPLY_TIMEOUT`),
in place of the state, with On/Off disabled; a write with the port closed
is reported at once (`writeFailed`).

`serialproto.cpp:89`, `mainwindow.cpp:75`

`openSerPort` always uses `/dev/ttyUSB0` (`setSerPort` is overwritten),
`QSerialPort::errorOccurred` is not handled, nothing reopens the port if
the USB adapter is unplugged, and `startSerLoop` runs even when the port
did not open.

Fix: port name from a setting or the command line, reopen on a timer after
an error, start the polling only when the port is open, and show the
serial state on the panel.

## 11. Stale flame power when the stove is off

Fixed in 0.2.0: the flame power is 0 in the Off state too.

`serialproto.cpp:240-250`

With state 0 (Off) the flame power is not updated and keeps the last
value; it is set to 0 only for states 6 and above.

Fix: 0 for state 0 as well.

## 12. 200-line switch and dead code

`serialproto.cpp:223` and the `#if 0` blocks (`serialproto.cpp:17`, `23`,
`57`, `82`, `318`, `456`; `serialproto.h:103`)

Each parameter has its own `case`, and most chrono cases are disabled code.

Fix: keep the scale and a handler in `requestsMap`, one generic parse; drop
the dead code.

## 13. String based connections, `on_` slot names

`mainwindow.cpp:49-68`

`SIGNAL()` / `SLOT()` connections are checked only at runtime. The slots
`on_updateAmbTemp`, `on_updateStats`, ... follow the auto-connection
pattern `on_<object>_<signal>`, so `connectSlotsByName` prints a warning for
each of them at startup.

Fix: pointer to member connections; rename the slots (e.g. `showAmbTemp`).

## 14. `QSignalMapper` deprecated

`mainwindow.cpp:63`

Fix: one lambda per LCD.

## 15. Button logic on the button text

`mainwindow.cpp:312`

`on_stateBtn_released` decides on / off from the button text. Changing or
translating the text breaks the button.

Fix: decide on the last stove state.

## 16. Leaks

`mainwindow.cpp:48`: `m_chronoShowTimer` has no parent. The `SerialProto`
singleton is never deleted (by design: it lives as long as the
application), so the serial port is closed only by `closeSerPort` in
`main`.

Fix: `new QTimer(this)`.

## 17. Swapped ON/OFF descriptions

`serialproto.h:258-261`, `264-267`

In `requestsMap` the weekend and daily descriptions are "1 ON, 1 ON, 2 OFF,
2 OFF" instead of "1 ON, 1 OFF, 2 ON, 2 OFF". Only used for debugging
today.

## 18. Archives in the working tree

Fixed: `*.tar` is in `.gitignore` (`PelletController.tar`,
`PelletControllerFull.tar`, 14 MB); `*.pro.user*` was already ignored.

## 19. Daily and weekly chrono read only

The daily program (2 slots) and the weekly program (4 slots with day
flags) are read from the stove, but their tabs are empty: only chrono
on/off, weekend on/off and weekend Start 1 can be changed.

## 20. Chrono read resets Start 1 to 00:00

Fixed in 0.2.0: `updateUiData_ChronoWdg` sets the LCD with
`setTensOfMins()` before the check box, and the check box no longer
writes back (issue 3). The time picker opens on the time shown.

`mainwindow.cpp:209-222`, `mainwindow.cpp:249-263`

`updateUiData_ChronoWdg` shows the time with `lcd->display()`, which does
not update `wClickableLCDNumber::tensOfMins()` (still 0). Then it checks
the check box, and through issue 3 `on_chronoWkEStart1CB_stateChanged`
writes `tensOfMins()` to the stove. Result: the first time the weekend
Start 1 is read as active, the panel writes 00:00 over it. The screenshot
tool shows it in its log:

```
void MainWindow::on_chronoWkEStart1CB_stateChanged(int)  -  Qt::Checked
Write LCD:  0
```

The time picker opens on 00:00 for the same reason, whatever the LCD shows
(see the [time picker screenshot](images/chrono-time-edit.png): Start 1
shows 07:00).

Fix: `lcd->setTensOfMins(u)` in place of `display()`, set the LCD before
the check box, and fix issue 3.

## 21. Stop 1 not connected

Fixed in 0.2.0: Stop 1 shows the value read (`updateChronoWkE1Off`),
its check box writes the time or 144 (`writeChronoWke1Off`). Weekend
slot 2 is still not on the page.

`mainwindow.cpp:44`

The Stop 1 check box has no slot, `updateChronoWkE1Off` is emitted but not
connected, and the Stop 1 LCD always shows 00:30 (set in the constructor).
Weekend slot 2 is not on the page.

## 22. Off button active during an alarm

Fixed in 0.2.0: in the alarm states the On/Off button is disabled
(shows "Off"), only Reset is active. The button is also disabled at
startup until the stove state has been read.

`mainwindow.cpp:149-157`

For Pellet Missing, Ignition Failure and Alarm only the Reset button is
changed: the On/Off button keeps the text and state it had before the
alarm. After an alarm while working, "Off" is still active and writes
Final Cleaning (see the [alarm screenshot](images/main-alarm.png)).

Fix: disable the On/Off button in the alarm states, or define what it
should do there.

## 23. GUI texts

- The chrono tabs show the object names: `chronoWkETab`, `chronoDayTab`,
  `chronoSetTab` (`mainwindow.ui`)
- "Chrono WeekEnd Enable" is cut ("Chrono  WeekEnd Enal") and has a double
  space
- "Power setted" should be "Set power"
- Texts are not wrapped in `tr()`

## 24. Invalid reply not read again

Fixed in 0.2.0: after a reply with a wrong checksum or length the same
parameter is read again at the next tick, once (`m_readRetryPending`),
without moving the polling on.

`serialproto.cpp` (`processStoveReply`, `getStoveInfos`)

Found on the panel on 2026-10-12 with a noisy serial line (about 1.6 % of
the replies with one bit flipped): a reply that fails the checksum is
dropped, and the value waits for the next round (about 27 s; the stove
state, read every other request, a few hundred ms).

## 25. Stove state read twice in a row

Fixed in 0.2.0: the next parameter after the stove state skips the
state index.

`serialproto.cpp` (`getStoveInfos`)

The state is read every other request, but the parameter sequence did not
skip its index: after the parameter just before it (cochlea, 0x0D) the
state was read twice (`01 21 0d 21 21 34`), one request lost per round.

## 26. Lost replies not counted

Fixed in 0.2.0: a read without any byte back before the next request
is counted as a timeout (`m_rxTimeouts`, `Tmo` in the top bar) and the
parameter is read again once (as issue 24).

`serialproto.cpp` (`checkReadTimeout`, `getStoveInfos`, `sendPendingWrite`)

Found on the panel on 2026-10-13 during Ignition: `Tx 1091 Rx 1016 Err 0
Chk 0`, 75 requests (about 7 %) without any reply, not even the echo,
counted nowhere and not read again.
