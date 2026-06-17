#!/usr/bin/env python3

from __future__ import annotations

import datetime as dt
import hashlib
import json
import pathlib
import re
import shutil
import subprocess
from dataclasses import dataclass

from board_metadata import (
    PROJECT_ROOT,
    PRODUCTION_PARTITIONS,
    parse_offset,
    parse_size_bytes,
    read_flat_yaml,
    require_valid_board_profile,
    resolved_flash_settings,
)


PACKAGE_COMPONENT_RE = re.compile(r"^[A-Za-z0-9._+-]+$")
VALID_BUILD_PROFILES = ("debug", "release", "production", "service")
VALID_BOOT_MODES = ("no-mcuboot", "mcuboot")


@dataclass(frozen=True)
class DtsPartition:
    node: str
    node_label: str
    label: str
    offset: int
    size: int


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


def require_package_component(name: str, value: str) -> str:
    if not value:
        raise PackageError(f"{name} is required")
    if not PACKAGE_COMPONENT_RE.fullmatch(value):
        raise PackageError(
            f"{name} contains characters that are not safe for package paths: {value}"
        )
    return value


def require_choice(name: str, value: str, choices: tuple[str, ...]) -> str:
    if value not in choices:
        raise PackageError(f"unknown {name}: {value} (valid: {', '.join(choices)})")
    return value


def package_name_from_components(
    slug: str, version: str, board_profile: str, build_profile: str, boot_mode: str
) -> str:
    build_profile = require_choice("build profile", build_profile, VALID_BUILD_PROFILES)
    boot_mode = require_choice("boot mode", boot_mode, VALID_BOOT_MODES)

    return "_".join(
        (
            require_package_component("FW_SLUG", slug),
            require_package_component("version", version),
            require_package_component("board profile", board_profile),
            require_package_component("build profile", build_profile),
            require_package_component("boot mode", boot_mode),
        )
    )


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


def parse_dts_reg(value: str) -> tuple[int, int] | None:
    parts = value.strip().strip("<>;").split()
    if len(parts) < 2:
        return None

    try:
        return int(parts[0], 0), int(parts[1], 0)
    except ValueError:
        return None


def parse_dts_partitions(source_dts: pathlib.Path) -> list[DtsPartition]:
    if not source_dts.is_file():
        return []

    partitions: list[DtsPartition] = []
    in_partition = False
    node = ""
    node_label = ""
    label = ""
    reg: tuple[int, int] | None = None

    for raw_line in source_dts.read_text(encoding="utf-8", errors="replace").splitlines():
        stripped = raw_line.strip()
        match = re.match(
            r"(?:(?P<label>[A-Za-z_][A-Za-z0-9_-]*)\s*:\s*)?(?P<node>partition@[0-9A-Fa-f]+)\s*\{",
            stripped,
        )
        if match:
            in_partition = True
            node = match.group("node")
            node_label = match.group("label") or ""
            label = ""
            reg = None
            continue

        if not in_partition:
            continue

        if stripped.startswith("label ="):
            label = stripped.removeprefix("label =").strip().strip('";')
        elif stripped.startswith("reg ="):
            reg = parse_dts_reg(stripped.removeprefix("reg ="))
        elif stripped == "};":
            if reg is not None:
                partitions.append(DtsPartition(node, node_label, label, reg[0], reg[1]))
            in_partition = False

    return partitions


def validate_production_policy_against_dts(
    policy: dict[str, str], source_dts: pathlib.Path
) -> list[str]:
    if not source_dts.is_file():
        return [f"build devicetree not found: {source_dts}"]

    partitions = parse_dts_partitions(source_dts)
    by_name: dict[str, DtsPartition] = {}
    for partition in partitions:
        for name in (partition.node_label, partition.label):
            if name:
                by_name[name] = partition

    issues: list[str] = []
    for partition_name in PRODUCTION_PARTITIONS:
        policy_partition = policy.get(f"{partition_name}_partition", "")
        expected_offset = parse_offset(policy.get(f"{partition_name}_offset", ""))
        expected_size = parse_size_bytes(policy.get(f"{partition_name}_size", ""))
        dts_partition = by_name.get(policy_partition)

        if dts_partition is None:
            issues.append(
                f"{source_dts}: production policy partition not found: {policy_partition}"
            )
            continue

        if expected_offset is not None and dts_partition.offset != expected_offset:
            issues.append(
                f"{source_dts}: {policy_partition} offset is 0x{dts_partition.offset:x}, "
                f"production.yml expects 0x{expected_offset:x}"
            )
        if expected_size is not None and dts_partition.size != expected_size:
            issues.append(
                f"{source_dts}: {policy_partition} size is {dts_partition.size} bytes, "
                f"production.yml expects {expected_size} bytes"
            )

    return issues


def partition_summary(
    source_dts: pathlib.Path,
    board_profile: str,
    boot_mode: str,
    production_policy_file: pathlib.Path,
) -> str:
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

    partitions = parse_dts_partitions(source_dts)
    if not partitions:
        lines.append("No flash partitions were found in zephyr.dts.")
        return "\n".join(lines) + "\n"

    for partition in partitions:
        names = []
        if partition.node_label:
            names.append(f"node_label={partition.node_label}")
        if partition.label:
            names.append(f"label={partition.label}")
        names_text = " ".join(names) if names else "unnamed"
        lines.append(
            f"- {partition.node} {names_text} "
            f"reg=<0x{partition.offset:x} 0x{partition.size:x}>"
        )

    return "\n".join(lines) + "\n"


def read_zephyr_config(config_file: pathlib.Path) -> dict[str, str]:
    values: dict[str, str] = {}
    if not config_file.is_file():
        return values

    for line in config_file.read_text(encoding="utf-8", errors="replace").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#") or "=" not in stripped:
            continue
        key, value = stripped.split("=", 1)
        values[key] = value.strip().strip('"')
    return values


def config_bool(config: dict[str, str], key: str) -> bool:
    return config.get(key) == "y"


def write_flash_helper(package_dir: pathlib.Path, board_profile: str, flash: dict[str, str]) -> None:
    del board_profile
    del flash
    source = PROJECT_ROOT / "scripts" / "flash.sh"
    if not source.is_file():
        raise PackageError(f"flash helper source not found: {source}")

    destination = package_dir / "flash.sh"
    shutil.copy2(source, destination)
    destination.chmod(0o755)


def create_package(options: PackageOptions) -> pathlib.Path:
    project_env = read_project_env()
    app_version_file = project_env.get("FW_VERSION_FILE", "VERSION")
    app_version = read_version(PROJECT_ROOT / app_version_file)
    app_slug = project_env.get("FW_SLUG", "firmware")

    package_name = package_name_from_components(
        app_slug,
        app_version,
        options.board_profile,
        options.build_profile,
        options.boot_mode,
    )

    board = require_valid_board_profile(options.board_profile)
    production_policy_file = board.production_policy_path
    production_policy = read_flat_yaml(production_policy_file)
    flash = resolved_flash_settings(board)

    build_dir = options.build_dir or PROJECT_ROOT / "build" / options.board_profile / options.build_profile / options.boot_mode
    zephyr_dir = build_dir / "zephyr"
    zephyr_config = read_zephyr_config(zephyr_dir / ".config")
    firmware_image = zephyr_dir / "zephyr.bin"
    if not build_dir.is_dir():
        raise PackageError(f"build directory not found: {build_dir}")
    if not firmware_image.is_file():
        raise PackageError(f"firmware image not found: {firmware_image}")

    production_dts_issues = validate_production_policy_against_dts(
        production_policy, zephyr_dir / "zephyr.dts"
    )
    if production_dts_issues:
        raise PackageError(
            "production policy does not match build devicetree:\n"
            + "\n".join(production_dts_issues)
        )

    dist_dir = options.dist_dir.resolve()
    package_dir = dist_dir / package_name
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
    copy_if_exists(build_dir / "domains.yaml", package_dir)
    copy_if_exists(build_dir / "mcuboot" / "zephyr" / "zephyr.bin", package_dir, "mcuboot.bin")

    (package_dir / "partition_summary.txt").write_text(
        partition_summary(zephyr_dir / "zephyr.dts", options.board_profile, options.boot_mode, production_policy_file),
        encoding="utf-8",
    )

    manifest = {
        "display_name": project_env.get("FW_DISPLAY_NAME", ""),
        "slug": app_slug,
        "firmware_name": project_env.get("FW_FIRMWARE_NAME", app_slug),
        "version": app_version,
        "board_profile": board.profile,
        "board_display_name": board.get("display_name", "unknown"),
        "zephyr_board": board.get("zephyr_board", "unknown"),
        "build_profile": options.build_profile,
        "boot_mode": options.boot_mode,
        "git_commit": git_value(["rev-parse", "--short=12", "HEAD"], "unknown"),
        "git_dirty": git_dirty(),
        "created_utc": dt.datetime.now(dt.UTC).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "runtime": {
            "shell_enabled": config_bool(zephyr_config, "CONFIG_SHELL") and config_bool(zephyr_config, "CONFIG_FW_SHELL"),
            "zephyr_shell_enabled": config_bool(zephyr_config, "CONFIG_SHELL"),
            "firmware_shell_enabled": config_bool(zephyr_config, "CONFIG_FW_SHELL"),
            "log_default_level": zephyr_config.get("CONFIG_LOG_DEFAULT_LEVEL", "unknown"),
            "log_runtime_filtering": config_bool(zephyr_config, "CONFIG_LOG_RUNTIME_FILTERING"),
            "log_printk": config_bool(zephyr_config, "CONFIG_LOG_PRINTK"),
            "log_shell_commands": config_bool(zephyr_config, "CONFIG_LOG_CMDS"),
            "diagnostics_enabled": config_bool(zephyr_config, "CONFIG_THREAD_ANALYZER")
            or config_bool(zephyr_config, "CONFIG_DEBUG_COREDUMP"),
            "watchdog_enabled": config_bool(zephyr_config, "CONFIG_FW_SERVICE_WATCHDOG"),
            "zephyr_watchdog_enabled": config_bool(zephyr_config, "CONFIG_WATCHDOG"),
            "watchdog_timeout_ms": zephyr_config.get("CONFIG_FW_SERVICE_WATCHDOG_TIMEOUT_MS", "0"),
            "watchdog_feed_interval_ms": zephyr_config.get(
                "CONFIG_FW_SERVICE_WATCHDOG_FEED_INTERVAL_MS", "0"
            ),
            "thread_analyzer": config_bool(zephyr_config, "CONFIG_THREAD_ANALYZER"),
            "coredump": config_bool(zephyr_config, "CONFIG_DEBUG_COREDUMP"),
            "coredump_shell": config_bool(zephyr_config, "CONFIG_DEBUG_COREDUMP_SHELL"),
        },
        "flash": {
            "runner": flash["runner"],
            "chip": flash["chip"],
            "offset": flash["offset"],
            "baud": flash["baud"],
            "serial_baud": flash["serial_baud"],
            "mode": flash["mode"],
            "freq": flash["freq"],
            "size": flash["size"],
        },
        "production": {
            "mcuboot_partition_layout": production_policy["mcuboot_partition_layout"],
            "slot0_partition": production_policy["slot0_partition"],
            "slot0_offset": production_policy["slot0_offset"],
            "slot0_size": production_policy["slot0_size"],
            "slot1_partition": production_policy["slot1_partition"],
            "slot1_offset": production_policy["slot1_offset"],
            "slot1_size": production_policy["slot1_size"],
            "scratch_partition": production_policy["scratch_partition"],
            "scratch_offset": production_policy["scratch_offset"],
            "scratch_size": production_policy["scratch_size"],
            "scratch_policy": production_policy["scratch_policy"],
            "settings_partition": production_policy["settings_partition"],
            "settings_offset": production_policy["settings_offset"],
            "settings_size": production_policy["settings_size"],
            "settings_backend": production_policy["settings_backend"],
            "factory_reset_behavior": production_policy["factory_reset_behavior"],
            "signing_key_policy": production_policy["signing_key_policy"],
            "rollback_policy": production_policy["rollback_policy"],
            "recovery_process": production_policy["recovery_process"],
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

    readme = (
        f"{project_env.get('FW_DISPLAY_NAME', app_slug)} firmware package\n\n"
        f"Package: {package_name}\n"
        f"Version: {app_version}\n"
        f"Board profile: {board.profile}\n"
        f"Zephyr board: {board.get('zephyr_board', 'unknown')}\n"
        f"Build profile: {options.build_profile}\n"
        f"Boot mode: {options.boot_mode}\n"
        f"Shell: {'enabled' if manifest['runtime']['shell_enabled'] else 'disabled'}\n\n"
        "Primary image:\n"
        "  zephyr.bin\n\n"
        "Metadata:\n"
        "  manifest.json\n"
        "  firmware.sha256\n"
        "  partition_summary.txt\n"
        "  production.yml\n\n"
        "For ESP32 development flashing from the host OS:\n"
        "  ./flash.sh --port /dev/cu.usbserial-11120\n"
        "  ./flash.sh --port /dev/ttyUSB0\n"
    )
    (package_dir / "README.txt").write_text(readme, encoding="utf-8")
    write_flash_helper(package_dir, board.profile, flash)

    sha_lines = []
    for artifact in sorted(package_dir.iterdir()):
        if artifact.is_file() and artifact.name != "firmware.sha256":
            sha_lines.append(f"{sha256_file(artifact)}  {artifact.name}")
    (package_dir / "firmware.sha256").write_text("\n".join(sha_lines) + "\n", encoding="utf-8")

    return package_dir
