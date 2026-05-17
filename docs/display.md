# Display

Display support is optional and controlled at build time:

```bash
./scripts/build.sh --board esp32_oled --display on
./scripts/build.sh --board esp32_oled --display off
```

The display service uses the `display0` devicetree alias. Boards without a
display should leave `default_display: off` in `board.yml` and disable
`CONFIG_APP_DISPLAY`.

Application code must not assume an OLED exists.
