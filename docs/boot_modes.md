# Boot Modes

## No-MCUboot

Development mode:

```bash
./scripts/build.sh --board esp32_oled --profile debug --boot no-mcuboot
```

Use this for fast local iteration, direct ESP32 flashing, UART shell testing, and
peripheral bring-up.

## MCUboot

Production/update mode:

```bash
./scripts/build.sh --board esp32_oled --profile production --boot mcuboot
```

This uses Zephyr sysbuild and the MCUboot config scaffold. Treat it as the
production path, but validate partition layout and signing policy per board.
