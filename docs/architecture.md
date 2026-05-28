# Architecture

Zephyr FW is structured as a small reusable firmware platform.

## Layers

Hardware description:

```text
boards/<vendor>/<board>/board.yml
boards/<vendor>/<board>/<board>_<qualifiers>.dts
boards/<vendor>/<board>/<board>_<qualifiers>_defconfig
boards/<vendor>/<board>/board.cmake
boards/<vendor>/<board>/support/openocd.cfg
```

Board profile policy:

```text
boards/<vendor>/<board>/metadata.yml
boards/<vendor>/<board>/board.conf
boards/<vendor>/<board>/<profile>.conf
boards/<vendor>/<board>/flash.conf
```

Feature selection:

```text
configs/features/
configs/profiles/
configs/boot/
```

Application code:

```text
app/src/
app/services/
app/shell/
app/tools/
```

Build/deploy workflow:

```text
scripts/
.vscode/
dist/
```

## Current App Model

There is one reusable application selected by Kconfig and config fragments. This
is the right model while board behavior differs mostly by peripherals and
features.

Add separate app directories later only when behavior truly diverges:

```text
apps/diagnostics/
apps/display_demo/
apps/sensor_node/
apps/gateway/
common/
```

Until then, keep common application logic board-independent and make board
differences visible through board-root devicetree, Kconfig, and board metadata.
