# Display

Display support is optional and controlled at build time:

Display support is selected by the board profile. Set `default_display` in
`boards/<board>/board.yml` and keep the actual driver/Kconfig selections in
`boards/<board>/board.conf`.

The display service uses the `display0` devicetree alias. Boards without a
display should leave `default_display: off` in `board.yml` and disable
`CONFIG_APP_DISPLAY`.

Application code must not assume an OLED exists.
