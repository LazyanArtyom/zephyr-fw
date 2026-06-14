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

Build command:

```bash
./scripts/build.sh --board esp32_oled --profile production --boot mcuboot
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

Private signing keys must not be committed.

## ESP32 OLED Policy

The ESP32 OLED board defines its production policy in:

```text
boards/espressif/esp32_oled/production.yml
partitions/esp32_oled.md
```

The current policy uses the Espressif `partitions_0x1000_amp_4M` layout:
slot0 and slot1 are both 1344K, settings use the 192K `storage_partition`, and
MCUboot scratch uses the 124K `scratch_partition` with `swap-scratch` policy.
Factory reset is `settings-only`, rollback is `confirm-before-permanent`, and
production signing keys use the `ci-secret-store` policy. Private production
signing keys stay outside git and are supplied by the CI signing environment.
