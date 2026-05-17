# Versioning

Firmware version numbers live in:

```text
VERSION
```

Project identity lives in:

```text
project.env
```

CMake generates runtime metadata into:

```text
<build-dir>/generated/include/app/app_metadata.h
```

Runtime metadata includes:

```text
display name
slug
firmware name
version
board profile
build profile
boot mode
app profile
git commit
dirty marker
build timestamp
```

Do not reintroduce independent version strings in C++ or scripts.
