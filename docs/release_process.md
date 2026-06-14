# Release Process

Suggested release flow:

1. Update `VERSION`.
2. Build production no-MCUboot package if direct flashing is required.
3. Build production MCUboot package for signed update flow.
4. Verify `manifest.json`.
5. Verify `firmware.sha256`.
6. Smoke-test shell policy, logging level, and boot behavior.
7. Archive `dist/<package>/`.

Production rules:

```text
No debug shell unless explicitly approved
Quiet logging by default
Assertions policy documented
Signing keys kept outside git
Artifacts are versioned and checksummed
Board production policy validated against built devicetree
```
