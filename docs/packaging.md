# Packaging

Create packages with:

```bash
./scripts/package.sh --board esp32_oled --profile debug --boot no-mcuboot
```

Package naming:

```text
zephyr-fw_<version>_<board>_<profile>_<boot>
```

Package contents include available build artifacts:

```text
zephyr.bin
zephyr.elf
zephyr.map
zephyr.dts
zephyr.config
build_info.yml
manifest.json
firmware.meta.json
firmware.sha256
flash.sh
README.txt
```

MCUboot packages also copy signed/update/bootloader artifacts when Zephyr
produces them.

## Production Package Contract

Packages include the production release surface expected by factory and field
workflows:

```text
zephyr.bin
zephyr.signed.bin          # when MCUboot signing produced it
app_update.bin             # when MCUboot/update output exists
mcuboot.bin                # when sysbuild produced it
firmware.meta.json
manifest.json
firmware.sha256
zephyr.config
zephyr.dts
flash.sh
partition_summary.txt
partition_policy.md
production.yml
```

`manifest.json` is canonical. `firmware.meta.json` is retained as a compatibility
copy for older tools. The manifest records slot sizing, scratch policy,
settings partition, signing key policy, rollback policy, factory reset behavior,
and recovery process from the board production policy.
