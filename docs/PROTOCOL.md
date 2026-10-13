# Stove serial protocol

How Pellet Controller talks to the stove board: line settings, message
format, parameter map, polling, write verification and link handling.
The code is `serialproto.h` / `serialproto.cpp` (`SerialProto`). An HTML
version of this page is in [protocol.html](protocol.html).

All values are in hexadecimal unless written otherwise. All sums are
modulo 256.

The stove board is a Micronova controller. The protocol is the same on
every Micronova board, the parameter map is not: the map below was checked
on this stove with a full memory dump (see
[Verified on the stove](#verified-on-the-stove)).

## Line

| Setting | Value |
|---|---|
| Port | `setting.ini` in the working directory, `[SERIAL] port`, default `/dev/ttyUSB0` |
| Speed | 1200 baud |
| Frame | 8 data bits, no parity, 2 stop bits (8N2), no flow control |
| Byte time | 11 bits = about 9.2 ms |
| Echo | yes: the line is shared, every byte sent comes back before the reply |

`setting.ini` is optional:

```ini
[SERIAL]
port=/dev/ttyUSB0
```

The panel is the master: the stove board only answers requests. A reply
is complete when the line stays idle for 100 ms
(`SERIAL_RX_IDLE_TIMEOUT`).

## Banks

| Bank | Code | Content |
|---|---|---|
| RAM | `00` | Live values: room temperature, state, flame power, smoke, clock |
| EEPROM | `20` | Settings: setpoint, power, chrono programs |

The same address can exist in both banks with a different meaning (for
example `5A` is the smoke temperature in RAM and a chrono time in
EEPROM).

## Messages

### Read

| Direction | Bytes |
|---|---|
| Sent | `bank`, `address` |
| Received | echo `bank`, `address`, then `checksum`, `value` |

`checksum = bank + address + value`.

Example, room temperature (RAM `01`), 20.0 °C:

```text
sent      00 01
received  00 01 29 28      echo 00 01, checksum 29 = 00 + 01 + 28, value 28 = 40 -> 40 / 2 = 20.0 °C
```

Example, stove state (RAM `21`), Working (4):

```text
sent      00 21
received  00 21 25 04
```

### Write

| Direction | Bytes |
|---|---|
| Sent | `80 + bank`, `address`, `value`, `checksum` |
| Received | echo of the 4 bytes, then `checksum`, `value` |

`checksum = 80 + bank + address + value`. The stove answers with the
same checksum and the value it has stored.

Example, setpoint (EEPROM `7D`) to 21.5 °C (raw 43 = `2B`):

```text
sent      A0 7D 2B 48      A0 = 80 + 20, 48 = A0 + 7D + 2B
received  A0 7D 2B 48 48 2B
```

Example, switch the stove on (RAM `21`, state 1 Starting):

```text
sent      80 21 01 A2
received  80 21 01 A2 A2 01
```

### Reply checks

| Reply | Accepted when | Otherwise |
|---|---|---|
| Read | 4 bytes, the first 2 equal to the request, checksum matches | counted (Err: length or echo, Chk: checksum), the parameter is read again once at the next tick |
| Write | 6 bytes, the first 4 equal to the request, byte 5 = checksum sent, byte 6 = value sent | sent once more, then reported to the user (`writeFailed`) |

A read that gets no byte at all (not even the echo) before the next
request is a timeout: counted (Tmo) and read again once, like an invalid
reply. A reply with the echo only (2 bytes) means the stove board did not
answer (Err).

The panel shows the counters in the top bar: `Tx` requests sent, `Rx`
valid replies, `Err` replies with a wrong length or echo, `Chk` replies
with a wrong checksum, `Tmo` requests without any reply. On this stove
about 7 % of the requests got no reply during Ignition (2026-10-13),
none with the stove Off.

## Stove states

RAM `21`, read and written.

| Value | State | Panel button |
|---|---|---|
| 0 | Off | On (writes 1) |
| 1 | Starting | Off, disabled |
| 2 | Pellet Loading | Off, disabled |
| 3 | Ignition | Off, disabled |
| 4 | Working | Off (writes 6) |
| 5 | Brazier Cleaning | Off, disabled |
| 6 | Final Cleaning | Off, disabled |
| 7 | Standby | Off, disabled |
| 8 | Pellet Missing | Off disabled, Reset (writes 0) |
| 9 | Ignition Failure | Off disabled, Reset (writes 0) |
| 10 | Alarm | Off disabled, Reset (writes 0) |
| other | shown as `Unknown (n)` | Off, disabled |

Commands: On = write 1 (Starting), Off = write 6 (Final Cleaning),
Reset after an alarm = write 0 (Off).

## Parameter map

`requestsMap` in `serialproto.h`, 63 entries; the index is
`RequestsIndex`.

### Live values and settings

| Index | Bank | Address | Parameter | Encoding |
|---|---|---|---|---|
| 0 | RAM | `01` | Room temperature | value / 2 °C |
| 1 | RAM | `0D` | Auger on time | value / 10 |
| 2 | RAM | `21` | Stove state | see [Stove states](#stove-states) |
| 3 | RAM | `34` | Flame power | 0-16, shown as value × 90 / 16 + 10 %; 0 % when Off or from Final Cleaning on |
| 4 | RAM | `37` | Smoke fan speed | value × 10 + 250 rpm |
| 5 | RAM | `5A` | Smoke temperature | °C |
| 6 | RAM | `65` | Seconds | 0-59 |
| 7 | RAM | `66` | Day of week | 1 Monday - 7 Sunday |
| 8 | RAM | `67` | Hours | 0-23 |
| 9 | RAM | `68` | Minutes | 0-59 |
| 10 | RAM | `69` | Day of month | 1-31 |
| 11 | RAM | `6A` | Month | 1-12 |
| 12 | RAM | `6B` | Year | year - 2000 |
| 13 | EEPROM | `7D` | Setpoint | value / 2 °C; the panel writes 10-60 (5.0-30.0 °C) |
| 14 | EEPROM | `7F` | Power | 1-5 |
| 15 | EEPROM | `4C` | Chrono on/off | 0 / 1 |

### Chrono programs

Times are in tens of minutes from 00:00: 0-143 (00:00-23:50), 144 = slot
off. Example: 42 = 07:00, 132 = 22:00. Day flags are 0 / 1.

| Index | Address | Parameter |
|---|---|---|
| 16 | `42` | Weekend program on/off |
| 17-18 | `43`, `44` | Weekend slot 1 start, stop |
| 19-20 | `45`, `46` | Weekend slot 2 start, stop |
| 21 | `47` | Daily program on/off |
| 22-23 | `48`, `49` | Daily slot 1 start, stop |
| 24-25 | `4A`, `4B` | Daily slot 2 start, stop |
| 26 | `50` | Weekly program on/off |
| 27-28 | `51`, `52` | Weekly slot 1 start, stop |
| 29-35 | `53`-`59` | Weekly slot 1 days, Monday to Sunday |
| 36-37 | `5A`, `5B` | Weekly slot 2 start, stop |
| 38-44 | `5C`-`62` | Weekly slot 2 days, Monday to Sunday |
| 45-46 | `63`, `64` | Weekly slot 3 start, stop |
| 47-53 | `65`-`6B` | Weekly slot 3 days, Monday to Sunday |
| 54-55 | `6C`, `6D` | Weekly slot 4 start, stop |
| 56-62 | `6E`-`74` | Weekly slot 4 days, Monday to Sunday |

All chrono parameters are in the EEPROM bank. The panel edits chrono
on/off, weekend on/off and weekend slot 1 (start, stop or off); the rest
is read only (issue 19).

### Written by the panel

| Parameter | Bank / address | Values written |
|---|---|---|
| Stove state | RAM `21` | 1 On, 6 Off, 0 Reset |
| Setpoint | EEPROM `7D` | read value ± 1, kept in 10-60 |
| Power | EEPROM `7F` | read value ± 1, kept in 1-5 |
| Chrono on/off | EEPROM `4C` | 0 / 1 |
| Weekend on/off | EEPROM `42` | 0 / 1 |
| Weekend slot 1 start, stop | EEPROM `43`, `44` | 0-143, 144 off |

Setpoint and power -/+ do nothing until the value has been read from the
stove (and again after the serial port was lost).

### Other known locations

Not used by the panel yet.

| Bank / address | Content |
|---|---|
| RAM `00` | Internal counter, 0-255 continuously |
| RAM `19` | Likely the power level in use: `90` Off, 1 Pellet Loading, 2 Working at power 2 |
| RAM `36` | Likely the smoke fan speed requested (`37` the actual one): 0 Off, about 200 in Pellet Loading |
| RAM `9B`-`B2`, `BD`-`C8` | Text shown on the stove display, ASCII, scrolling (`OFF`, "CARICA PELLET" read in pieces) |
| EEPROM `F8`-`FD` | Clock to set: day of week (1 Monday), hours, minutes, day, month, year - 2000, in BCD (`13` = 13) |

## Verified on the stove

Full dump of RAM and EEPROM with `tools/stovedump` on 2026-10-13, stove
Off, compared with the stove display:

- Room temperature RAM `01` = `26` (19.0 °C), state RAM `21` = 0 (Off),
  smoke temperature RAM `5A` = 21 °C, power EEPROM `7F` = 2
- Setpoint EEPROM `7D` = `2D` (45): the display shows 22.5 °C, so value / 2
- Clock RAM `65`-`6B` = `15 02 09 0C 0D 0A 1A`: 21 s, Tuesday, 09:12,
  13/10/26, the actual date and time
- Weekend program `42`-`46` = `01 12 24 66 90`: on, 03:00-06:00,
  17:00-off, as on the stove display
- Weekly program: the "slot off" times (`90`) are at `51`/`52`, `5A`/`5B`,
  `63`/`64`, `6C`/`6D`, a slot every 9 bytes as in the map above
- EEPROM `F8`-`FD` = `02 09 13 13 10 26`: the clock in BCD

RAM dump and three more reads 20 s apart with the stove in Pellet
Loading (state RAM `21` = 2):

- Auger RAM `0D` = 37 (0 when Off)
- Smoke fan speed RAM `37` = 199-201, about 2250 rpm (0 when Off)
- Smoke temperature RAM `5A` from 21 to 23 °C
- Display text: scrolling, a different piece at each read

Ignition (state 3) and Working (state 4, power 2), three reads each:

- Flame power RAM `34`: 0 in Pellet Loading, 0-1 in Ignition, 4 in
  Working (shown as 32 %), steady
- Smoke temperature RAM `5A`: 68-72 °C in Ignition, 119 °C in Working
- Smoke fan speed RAM `37`: about 2450 rpm in Ignition, 1980 rpm in Working
- RAM `19`: 1 in Pellet Loading, 2 in Working at power 2 (`90` Off)
- During Ignition about 7 % of the requests got no reply at all

### Other Micronova boards

Other maps published for Micronova stoves (for example
<https://github.com/eni23/micronova-controller>, with lists from other
sources) differ from this stove:

| Parameter | This stove | Elsewhere |
|---|---|---|
| Clock in RAM | `65`-`6B` (seconds first) | `63`-`69`, or `7B`-`80` |
| Smoke temperature | RAM `5A` | RAM `3E` on some boards |
| Setpoint | EEPROM `7D`, value / 2 | `7D` 1:1 in °C on some boards |
| Weekly chrono | `50` on/off, slots every 9 bytes from `51` | slots every 10 bytes from `4D`, or a daily timer at `50`-`59` with power and setpoint per slot |
| RAM `19` | likely the power level in use (2 Working at power 2, `90` Off) | set power on some boards |
| Display text | RAM `9B`-`B2`, `BD`-`C8`, scrolling | RAM `8D`-`9C` |
| Switch off | write 6 to RAM `21` | some boards switch off slowly this way and use the infrared command (`5A` to RAM `58`, 10 times) |

Check a new stove model with `tools/stovedump` before trusting the map.

## Polling

One request every 200 ms (`SERIAL_MESSAGE_DELAY`).

Main page: indexes 0-15, with the stove state every other request, then
a 2 s pause (`SERIAL_SESSION_DELAY`). One round, about 8 s:

```text
01 21 0D 21 34 21 37 21 5A 21 65 21 66 21 67 21 68 21 69 21 6A 21 6B 21 7D 21 7F 21 4C 21
```

The stove date and time is shown at the end of each round.

Chrono page: chrono on/off and all the chrono programs in order (indexes
15-62, `4C`, then `42`-`74`), then a 2 s pause; about 12 s per round.

An invalid reply is followed by one more read of the same parameter at
the next tick, without moving the sequence on.

## Writes

Writes are queued and sent one at a time:

1. The polling stops; the write is sent 150 ms later
   (`SERIAL_WRITE_DELAY`), after any read reply still arriving.
2. The reply is awaited until the line is idle for 100 ms, at most 600 ms
   (`SERIAL_WRITE_TIMEOUT`).
3. Not confirmed: sent once more (`SERIAL_WRITE_ATTEMPTS` = 2), then
   `writeFailed` and a message on the panel.
4. Next write in the queue, or the polling restarts from the beginning
   after 600 ms.

A write with the serial port closed is reported at once.

## Link handling

| Event | Behaviour |
|---|---|
| Port does not open | tried again every 3 s (`SERIAL_REOPEN_DELAY`), logged once; panel shows "No serial port" |
| Port lost (USB adapter unplugged: resource, read or write error) | port closed, polling stopped, setpoint and power to be read again; reopened every 3 s, polling restarts by itself |
| No valid reply for 10 s (`SERIAL_NO_REPLY_TIMEOUT`) | panel shows "No reply" |
| Valid reply | link OK, the next state read replaces the message |

While the link is not OK the On/Off button is disabled.

## Safety

Writes change the stove: setpoint, power, chrono and state are stored by
the stove board (EEPROM for the settings). Never send writes to a real
stove from a test: use the simulator `tools/stovesim` (README, "Stove
simulator"), which the tests in `tests/` use too.
