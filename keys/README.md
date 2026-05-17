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
