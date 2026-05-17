# Coding Standard

Project headers use `.h` and include guards.

C++ style:

```text
C++20 baseline
C++23-ready where Zephyr/toolchain support it
No exceptions unless explicitly enabled
No RTTI unless explicitly enabled
Avoid hidden heap allocation in critical code
Prefer const correctness
Use RAII where it is deterministic and safe
Keep Zephyr C macro interactions simple
Use NULL where Zephyr C macros require it
Keep board pins and buses out of C++ code
```

Formatting:

```bash
./scripts/check_format.sh
```

Static analysis:

```bash
./scripts/run-clang-tidy.sh build/esp32_oled/debug/no-mcuboot
```
