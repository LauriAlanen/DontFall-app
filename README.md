# DontFall-app

DontFall application for the Cortex-A7 (Linux) side of the STM32MP157C-DK2.
Built into the image by the `dontfall-app` recipe in the
[DontFall](https://github.com/LauriAlanen/DontFall) integration repo and
installed as `/usr/local/bin/dontfall-app`.

Full workflow across all DontFall repos (setup, dev loops, release):
[DontFall/docs/development.md](https://github.com/LauriAlanen/DontFall/blob/main/docs/development.md).

## Develop

Keep this checkout next to the integration workspace
(`~/Projects/DontFall-app` beside `~/Projects/DontFall`). With
`conf/dev.conf` in the Yocto build dir, bitbake builds this working tree
directly (externalsrc):

```sh
cd ../DontFall
./deploy.sh <board-ip> --run /usr/local/bin/dontfall-app   # build + push + run
```

Quick host build (no cross compiler, for compile errors):

```sh
cmake -B build && cmake --build build
```

## What it does

`dontfall-app [--led <sysfs-led-dir>] [device]` runs the A7 side of the
A7 <-> M4 heartbeat until SIGTERM/SIGINT. Every 500 ms it sends
`PING <seq>` on `/dev/ttyRPMSG0`; the M4 answers `PONG <seq>` (protocol:
`DontFall-M4/app/src/protocol.h`). The link is up while PONGs come back
within 1.5 s, and the app logs `M4 link up` / `M4 link lost`. If the M4
restarts, the app reopens the tty when it reappears.

With `--led` it takes over a Linux LED-class LED (writes `none` to
`trigger`) and blinks it every 1000 ms while the link is up; on exit the
LED is turned off and its original trigger restored. The image runs it as
`dontfall-app.service` with `--led /sys/class/leds/heartbeat` (blue LD8).

| File | Role |
|------|------|
| `src/main.cpp` | arguments, signals, the `poll()` event loop |
| `src/heartbeat.cpp` | PING schedule and link up/lost state machine (no I/O) |
| `src/protocol.cpp` | PING/PONG wire format, line reassembly |
| `src/rpmsg_tty.cpp` | raw, non-blocking `/dev/ttyRPMSG0` |
| `src/sysfs_led.cpp` | LED-class takeover and restore |

## Release

Commit and push, then bump `SRCREV` in
`DontFall/meta-df/meta-df-common/recipes-df/dontfall-app/dontfall-app.bb`
to the new commit and rebuild the image without `conf/dev.conf`.
