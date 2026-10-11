# DontFall-app

DontFall application for the Cortex-A7 (Linux) side of the STM32MP157C-DK2.
Built into the image by the `dontfall-app` recipe in the
[DontFall](https://github.com/LauriAlanen/DontFall) integration repo and
installed as `/usr/local/bin/dontfall-app`.

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

## Release

Commit and push, then bump `SRCREV` in
`DontFall/meta-df/meta-df-common/recipes-df/dontfall-app/dontfall-app.bb`
to the new commit and rebuild the image without `conf/dev.conf`.
