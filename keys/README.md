# Firmware Signing Keys

Do not commit private production signing keys.

Recommended layout:

```text
keys/
├── README.md
├── public/
│   └── README.md
└── private/
    └── <not committed>
```

Development keys may be generated locally for MCUboot experiments, but release
keys should live in a controlled secret store or CI signing environment.

Production MCUboot builds require an external private key:

```bash
MCUBOOT_SIGNING_KEY_FILE=/absolute/path/to/production-ec-p256.pem \
  ./scripts/build.sh --board esp32_oled --profile production --boot mcuboot
```

The build script rejects Zephyr/MCUboot sample keys for production builds.
