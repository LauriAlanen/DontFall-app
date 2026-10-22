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

Compile check only (it is a ROS 2 node, so it builds against the Yocto
sysroot, not on the host):

```sh
cd ../DontFall && ./build.sh bitbake dontfall-app
```

## What it does

`dontfall-app [--led <sysfs-led-dir>]` is the A7's ROS 2 node
`dontfall_app`. For now it watches the M4: the link is up while the M4's
statuses (`dontfall_m4: ...`) arrive on `/diagnostics`, sent at 1 Hz by
the M4's micro-ROS node through the micro-ROS Agent; 3 s without one and
it is lost. It logs `M4 link up` / `M4 link lost`.

With `--led` it takes over a Linux LED-class LED (writes `none` to
`trigger`) and blinks it every 1000 ms while the link is up; on exit the
LED is turned off and its original trigger restored. The image runs it as
`dontfall-app.service` with `--led /sys/class/leds/heartbeat` (blue LD8).

| File | Role |
|------|------|
| `src/main.cpp` | arguments, the node: `/diagnostics` subscription, LED timer |
| `src/m4_link.cpp` | link up/lost from status arrival times (no I/O) |
| `src/sysfs_led.cpp` | LED-class takeover and restore |
