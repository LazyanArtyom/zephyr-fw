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
firmware.meta.json
firmware.sha256
flash.sh
README.txt
```

MCUboot packages also copy signed/update/bootloader artifacts when Zephyr
produces them.
