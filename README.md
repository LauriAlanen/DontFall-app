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

Quick host build (no cross compiler, for compile errors and tests):

```sh
cmake -B build && cmake --build build && ./build/dontfall-app
```

## Blink mode

`dontfall-app --led <sysfs-led-dir>` runs forever: it takes over a Linux
LED-class LED (writes `none` to `trigger`, then toggles `brightness` every
1000 ms) after doing the one-shot OpenAMP hello. SIGTERM turns the LED off
and restores its original trigger. The DontFall image starts it this way as
`dontfall-app.service` on `/sys/class/leds/heartbeat` (blue LD8); without
`--led` the app is still a one-shot hello.

## Release

Commit and push, then bump `SRCREV` in
`DontFall/meta-df/meta-df-common/recipes-df/dontfall-app/dontfall-app.bb`
to the new commit and rebuild the image without `conf/dev.conf`.
