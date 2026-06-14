#!/usr/bin/env python3

from __future__ import annotations

import datetime as dt
import hashlib
import json
import pathlib
import shutil
import subprocess
from dataclasses import dataclass

from board_metadata import PROJECT_ROOT, read_flat_yaml, require_valid_board_profile, resolved_flash_settings


@dataclass(frozen=True)
class PackageOptions:
    board_profile: str
    build_profile: str
    boot_mode: str
    build_dir: pathlib.Path | None
    dist_dir: pathlib.Path


class PackageError(RuntimeError):
    pass


def read_project_env() -> dict[str, str]:
    values: dict[str, str] = {}
    env_path = PROJECT_ROOT / "project.env"
    for line in env_path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#") or "=" not in stripped:
            continue
        key, value = stripped.split("=", 1)
        values[key.strip()] = value.strip().strip('"')
    return values


def read_version(version_file: pathlib.Path) -> str:
    values: dict[str, str] = {}
    for line in version_file.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#") or "=" not in stripped:
            continue
        key, value = stripped.split("=", 1)
        values[key.strip()] = value.strip()

    version = f"{values.get('VERSION_MAJOR', '0')}.{values.get('VERSION_MINOR', '0')}.{values.get('PATCHLEVEL', '0')}"
    tweak = values.get("VERSION_TWEAK", "0")
    extra = values.get("EXTRAVERSION", "")
    if tweak and tweak != "0":
        version = f"{version}.{tweak}"
    if extra:
        version = f"{version}-{extra}"
    return version


def copy_if_exists(source: pathlib.Path, package_dir: pathlib.Path, dest_name: str | None = None) -> None:
    if source.is_file():
        shutil.copy2(source, package_dir / (dest_name or source.name))


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git_value(args: list[str], default: str) -> str:
    try:
        return subprocess.check_output(
            ["git", "-C", str(PROJECT_ROOT), *args],
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip()
    except (OSError, subprocess.CalledProcessError):
        return default


def git_dirty() -> bool:
    return bool(git_value(["status", "--porcelain"], ""))


def partition_summary(source_dts: pathlib.Path, board_profile: str, boot_mode: str, production_policy_file: pathlib.Path) -> str:
    lines = [
        "Partition summary",
        "",
        f"Board profile: {board_profile}",
        f"Boot mode: {boot_mode}",
        f"Production policy: {production_policy_file}",
        "",
    ]

    if not source_dts.is_file():
        lines.append("zephyr.dts was not present in the build output.")
        return "\n".join(lines) + "\n"

    in_partition = False
    node = ""
    label = ""
    reg = ""
    for raw_line in source_dts.read_text(encoding="utf-8", errors="replace").splitlines():
        stripped = raw_line.strip()
        if stripped.startswith("partition@") and stripped.endswith("{"):
            in_partition = True
            node = stripped[:-1].strip()
            label = ""
            reg = ""
            continue
        if not in_partition:
            continue
        if stripped.startswith("label ="):
            label = stripped.removeprefix("label =").strip().strip('";')
        elif stripped.startswith("reg ="):
            reg = stripped.removeprefix("reg =").strip().strip("<>;")
        elif stripped == "};":
            lines.append(f"- {node} label={label} reg=<{reg}>")
            in_partition = False

    return "\n".join(lines) + "\n"


def write_flash_helper(package_dir: pathlib.Path, board_profile: str, flash: dict[str, str]) -> None:
    flash_sh = package_dir / "flash.sh"
    if flash["runner"] != "esp32_esptool":
        flash_sh.write_text(
            "#!/usr/bin/env bash\n"
            "set -euo pipefail\n"
            f"echo \"No self-contained flash helper is available for board profile: {board_profile}\" >&2\n"
            "exit 1\n",
            encoding="utf-8",
        )
        flash_sh.chmod(0o755)
        return

    flash_sh.write_text(
        "#!/usr/bin/env bash\n"
        "set -euo pipefail\n\n"
        "SCRIPT_DIR=\"$(cd \"$(dirname \"${BASH_SOURCE[0]}\")\" && pwd)\"\n"
        "PORT=\"${1:-}\"\n\n"
        "if [[ -z \"${PORT}\" ]]; then\n"
        "    echo \"Usage: $0 <serial_port>\"\n"
        "    exit 1\n"
        "fi\n\n"
        "PYTHON=\"${ESPTOOL_PYTHON:-}\"\n"
        "if [[ -z \"${PYTHON}\" ]]; then\n"
        "    if [[ -x \"${HOME}/.venvs/esptool/bin/python\" ]]; then\n"
        "        PYTHON=\"${HOME}/.venvs/esptool/bin/python\"\n"
        "    else\n"
        "        PYTHON=\"python3\"\n"
        "    fi\n"
        "fi\n\n"
        "IMAGE=\"${SCRIPT_DIR}/zephyr.bin\"\n"
        "if [[ -f \"${SCRIPT_DIR}/zephyr.signed.bin\" ]]; then\n"
        "    IMAGE=\"${SCRIPT_DIR}/zephyr.signed.bin\"\n"
        "fi\n\n"
        "\"${PYTHON}\" -m esptool \\\n"
        f"    --chip \"{flash['chip']}\" \\\n"
        "    --port \"${PORT}\" \\\n"
        f"    --baud \"{flash['baud']}\" \\\n"
        "    write-flash \\\n"
        f"    --flash-mode \"{flash['mode']}\" \\\n"
        f"    --flash-freq \"{flash['freq']}\" \\\n"
        f"    --flash-size \"{flash['size']}\" \\\n"
        f"    \"{flash['offset']}\" \"${{IMAGE}}\"\n",
        encoding="utf-8",
    )
    flash_sh.chmod(0o755)


def create_package(options: PackageOptions) -> pathlib.Path:
    project_env = read_project_env()
    app_version_file = project_env.get("APP_VERSION_FILE", "VERSION")
    app_version = read_version(PROJECT_ROOT / app_version_file)
    app_slug = project_env.get("APP_SLUG", "firmware")

    board = require_valid_board_profile(options.board_profile)
    production_policy_file = board.production_policy_path
    production_policy = read_flat_yaml(production_policy_file) if production_policy_file.is_file() else {}
    flash = resolved_flash_settings(board)

    build_dir = options.build_dir or PROJECT_ROOT / "build" / options.board_profile / options.build_profile / options.boot_mode
    zephyr_dir = build_dir / "zephyr"
    firmware_image = zephyr_dir / "zephyr.bin"
    if not build_dir.is_dir():
        raise PackageError(f"build directory not found: {build_dir}")
    if not firmware_image.is_file():
        raise PackageError(f"firmware image not found: {firmware_image}")

    package_name = f"{app_slug}_{app_version}_{options.board_profile}_{options.build_profile}_{options.boot_mode}"
    package_dir = options.dist_dir / package_name
    if package_dir.exists():
        shutil.rmtree(package_dir)
    package_dir.mkdir(parents=True)

    copy_if_exists(zephyr_dir / "zephyr.bin", package_dir)
    copy_if_exists(zephyr_dir / "zephyr.elf", package_dir)
    copy_if_exists(zephyr_dir / "zephyr.map", package_dir)
    copy_if_exists(zephyr_dir / "zephyr.hex", package_dir)
    copy_if_exists(zephyr_dir / "zephyr.signed.bin", package_dir)
    copy_if_exists(zephyr_dir / "app_update.bin", package_dir)
    copy_if_exists(zephyr_dir / "zephyr.dts", package_dir)
    copy_if_exists(zephyr_dir / ".config", package_dir, "zephyr.config")
    copy_if_exists(build_dir / "build_info.yml", package_dir)
    copy_if_exists(build_dir / "compile_commands.json", package_dir)
    copy_if_exists(production_policy_file, package_dir, "production.yml")
    copy_if_exists(PROJECT_ROOT / "partitions" / f"{options.board_profile}.md", package_dir, "partition_policy.md")
    copy_if_exists(build_dir / "domains.yaml", package_dir)
    copy_if_exists(build_dir / "mcuboot" / "zephyr" / "zephyr.bin", package_dir, "mcuboot.bin")

    (package_dir / "partition_summary.txt").write_text(
        partition_summary(zephyr_dir / "zephyr.dts", options.board_profile, options.boot_mode, production_policy_file),
        encoding="utf-8",
    )

    manifest = {
        "display_name": project_env.get("APP_DISPLAY_NAME", ""),
        "slug": app_slug,
        "firmware_name": project_env.get("APP_FIRMWARE_NAME", app_slug),
        "version": app_version,
        "board_profile": board.profile,
        "board_display_name": board.get("display_name", "unknown"),
        "zephyr_board": board.get("zephyr_board", "unknown"),
        "build_profile": options.build_profile,
        "boot_mode": options.boot_mode,
        "git_commit": git_value(["rev-parse", "--short=12", "HEAD"], "unknown"),
        "git_dirty": git_dirty(),
        "created_utc": dt.datetime.now(dt.UTC).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "flash": {
            "runner": flash["runner"],
            "chip": flash["chip"],
            "offset": flash["offset"],
            "baud": flash["baud"],
            "mode": flash["mode"],
            "freq": flash["freq"],
            "size": flash["size"],
        },
        "production": {
            "mcuboot_partition_layout": production_policy.get("mcuboot_partition_layout", "unknown"),
            "slot0_size": production_policy.get("slot0_size", "unknown"),
            "slot1_size": production_policy.get("slot1_size", "unknown"),
            "scratch_policy": production_policy.get("scratch_policy", "unknown"),
            "settings_partition": production_policy.get("settings_partition", "unknown"),
            "factory_reset_behavior": production_policy.get("factory_reset_behavior", "settings-only"),
            "signing_key_policy": production_policy.get("signing_key_policy", "external"),
            "rollback_policy": production_policy.get("rollback_policy", "manual-confirm"),
            "recovery_process": production_policy.get("recovery_process", "board-specific"),
        },
        "artifacts": {
            "firmware_image": "zephyr.bin",
            "signed_image": "zephyr.signed.bin when present",
            "manifest": "manifest.json",
            "sha256": "firmware.sha256",
            "zephyr_config": "zephyr.config",
            "devicetree": "zephyr.dts",
            "flash_helper": "flash.sh",
            "partition_summary": "partition_summary.txt",
        },
    }
    manifest_json = json.dumps(manifest, indent=2, sort_keys=False) + "\n"
    (package_dir / "manifest.json").write_text(manifest_json, encoding="utf-8")
    (package_dir / "firmware.meta.json").write_text(manifest_json, encoding="utf-8")

    readme = (
        f"{project_env.get('APP_DISPLAY_NAME', app_slug)} firmware package\n\n"
        f"Package: {package_name}\n"
        f"Version: {app_version}\n"
        f"Board profile: {board.profile}\n"
        f"Zephyr board: {board.get('zephyr_board', 'unknown')}\n"
        f"Build profile: {options.build_profile}\n"
        f"Boot mode: {options.boot_mode}\n\n"
        "Primary image:\n"
        "  zephyr.bin\n\n"
        "Metadata:\n"
        "  manifest.json\n"
        "  firmware.meta.json\n"
        "  firmware.sha256\n"
        "  partition_summary.txt\n"
        "  production.yml\n\n"
        "For ESP32 development flashing with esptool:\n"
        "  ./flash.sh /dev/ttyUSB0\n"
    )
    (package_dir / "README.txt").write_text(readme, encoding="utf-8")
    write_flash_helper(package_dir, board.profile, flash)

    sha_lines = []
    for artifact in sorted(package_dir.iterdir()):
        if artifact.is_file() and artifact.name != "firmware.sha256":
            sha_lines.append(f"{sha256_file(artifact)}  {artifact.name}")
    (package_dir / "firmware.sha256").write_text("\n".join(sha_lines) + "\n", encoding="utf-8")

    return package_dir
