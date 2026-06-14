# Packaging

Create packages with:

```bash
./scripts/package.sh --board esp32_oled --profile debug --boot no-mcuboot
```

`scripts/package.sh` is only a compatibility wrapper. Package assembly,
manifest generation, checksums, partition summaries, and production policy
parsing live in `tools/firmware_package.py` behind `tools/fw.py package`. The
packager rejects builds whose generated `zephyr.dts` partition names, offsets,
or sizes disagree with the board `production.yml`.

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
manifest.json
firmware.sha256
zephyr.config
zephyr.dts
flash.sh
partition_summary.txt
production.yml
```

`manifest.json` is the single package metadata file. It records slot sizing,
scratch policy, settings partition, signing key policy, rollback policy,
factory reset behavior, and recovery process from the board production policy.
