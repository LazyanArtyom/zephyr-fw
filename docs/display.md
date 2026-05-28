# Display

Display support is optional and controlled at build time:

Display support is selected by the board metadata. Set `default_display` in
`boards/<vendor>/<board>/metadata.yml` and keep the driver/Kconfig selections in
`boards/<vendor>/<board>/board.conf`.

The display service uses the `display0` devicetree alias. Boards without a
display should leave `default_display: off` in `metadata.yml` and disable
`CONFIG_FW_DISPLAY`.

Application code must not assume an OLED exists.
