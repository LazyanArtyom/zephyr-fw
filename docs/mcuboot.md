# MCUboot

No-MCUboot builds are the default development path.

MCUboot production scaffolding is present:

```text
sysbuild.conf
sysbuild/mcuboot.conf
configs/boot/mcuboot.conf
configs/boot/no-mcuboot.conf
keys/
partitions/
```

Production build command:

```bash
MCUBOOT_SIGNING_KEY_FILE=/absolute/path/to/production-ec-p256.pem \
  ./scripts/build.sh --board esp32_oled --profile production --boot mcuboot
```

`production + mcuboot` builds fail unless `MCUBOOT_SIGNING_KEY_FILE` points to
an existing private key. The build script also rejects Zephyr/MCUboot bundled
sample keys, even if they are passed explicitly. For local non-release testing,
generate an ignored key under `keys/private/` or `/tmp` with MCUboot imgtool,
for example:

```bash
python3 /opt/zephyrproject/bootloader/mcuboot/scripts/imgtool.py keygen \
  -k keys/private/local-ec-p256.pem -t ecdsa-p256
```

Before using this for a real release, confirm:

```text
Board flash partition layout
Slot sizes
Rollback/swap policy
Signing key source
Image version policy
Factory flashing process
Recovery process
```

Private signing keys must not be committed. The application confirms a MCUboot
image only after startup health checks pass; a failed settings, diagnostics, or
watchdog startup path leaves a test image unconfirmed so MCUboot can roll back.

## ESP32 OLED Policy

The ESP32 OLED board defines its production policy in:

```text
boards/espressif/esp32_oled/production.yml
partitions/esp32_oled.md
```

The current policy uses the Espressif `partitions_0x1000_amp_4M` layout.
`production.yml` is the canonical source for slot sizes, settings storage,
scratch policy, factory reset behavior, rollback behavior, and signing key
policy. Package generation validates that policy against the built devicetree.
Private production signing keys stay outside git and are supplied by the CI
signing environment.
