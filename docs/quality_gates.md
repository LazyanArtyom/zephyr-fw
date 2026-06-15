# Quality Gates

Recommended local checks:

```bash
./scripts/check_format.sh
./scripts/check_build_matrix.sh
./scripts/package.sh build/esp32_oled/debug/no-mcuboot
```

Optional heavier checks:

```bash
./scripts/check_build_matrix.sh --include-production
./scripts/check_build_matrix.sh --include-production --include-mcuboot
./scripts/run-clang-tidy.sh build/esp32_oled/debug/no-mcuboot
```

CI should run at least:

```text
format check
debug no-MCUboot build
release no-MCUboot build
package generation
metadata/checksum verification
```
