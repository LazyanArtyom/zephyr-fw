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
