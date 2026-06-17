# Release Process

Suggested release flow:

1. Update `VERSION`.
2. Build production no-MCUboot package if direct flashing is required.
3. Provide the release MCUboot key through `MCUBOOT_SIGNING_KEY_FILE`.
4. Build production MCUboot package for signed update flow.
5. Verify `manifest.json`.
6. Verify `firmware.sha256`.
7. Smoke-test shell policy, logging level, boot behavior, and MCUboot image confirmation.
8. Archive `dist/<package>/`.

Production rules:

```text
No debug shell unless explicitly approved
Quiet logging by default
Assertions policy documented
Signing keys kept outside git
Production MCUboot builds reject bundled sample keys
App confirms MCUboot images only after startup health checks pass
Artifacts are versioned and checksummed
Board production policy validated against built devicetree
```
