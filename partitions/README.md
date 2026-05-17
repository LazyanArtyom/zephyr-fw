# Partition Layouts

This directory is reserved for board-specific production partition layouts.

For no-MCUboot development builds, the ESP32 profile currently flashes the app
image directly at the board flash offset.

For MCUboot production builds, define and review the bootloader slot layout per
board before enabling release signing. Keep partition choices board-specific and
document the flash map in the package manifest.
