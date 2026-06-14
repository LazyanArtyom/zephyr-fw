# Partition Layouts

This directory is reserved for board-specific production partition layouts.

For no-MCUboot development builds, the ESP32 profile currently flashes the app
image directly at the board flash offset.

For MCUboot production builds, define and review the bootloader slot layout per
board before enabling release signing. Keep partition choices board-specific and
document the flash map in the package manifest.

## Board Policies

Board-specific production policies live beside each board as `production.yml`
and are summarized under this directory for release review. Each policy is
schema-validated by `tools/fw.py boards validate` and must cover slot0/slot1
sizing, scratch behavior, settings partition, factory reset behavior, signing
key policy, rollback policy, and recovery process.
