#!/usr/bin/env python3

from __future__ import annotations

import pathlib
import re
import shlex
from dataclasses import dataclass


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
BOARDS_DIR = PROJECT_ROOT / "boards"


REQUIRED_BOARD_FIELDS = (
    "profile",
    "display_name",
    "status",
    "default_display",
    "default_settings",
    "default_boot",
    "serial_baud",
)

REQUIRED_BOARD_FILES = (
    "board.yml",
    "board.conf",
    "board.overlay",
    "debug.conf",
    "release.conf",
    "production.conf",
    "flash.conf",
    "README.md",
)

VALID_DISPLAY_MODES = ("on", "off")
VALID_SETTINGS_MODES = ("on", "off")
VALID_BOOT_MODES = ("no-mcuboot", "mcuboot")
LEGACY_APP_CONF = "app.conf"

PRODUCTION_POLICY_FIELDS = (
    "mcuboot_partition_layout",
    "slot0_partition",
    "slot0_offset",
    "slot0_size",
    "slot1_partition",
    "slot1_offset",
    "slot1_size",
    "scratch_partition",
    "scratch_offset",
    "scratch_size",
    "scratch_policy",
    "settings_partition",
    "settings_offset",
    "settings_size",
    "settings_backend",
    "factory_reset_behavior",
    "signing_key_policy",
    "rollback_policy",
    "recovery_process",
)

PRODUCTION_PARTITIONS = ("slot0", "slot1", "scratch", "settings")
PRODUCTION_POLICY_CHOICES = {
    "scratch_policy": ("swap-scratch", "overwrite-only", "none"),
    "settings_backend": ("nvs", "none"),
    "factory_reset_behavior": ("settings-only", "full-flash", "disabled"),
    "signing_key_policy": ("ci-secret-store", "external", "development-local"),
    "rollback_policy": ("confirm-before-permanent", "manual-confirm", "disabled"),
    "recovery_process": ("serial-rom-esptool", "board-specific", "factory-programmer"),
}

IDENTIFIER_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_+-]*$")
HEX_OFFSET_RE = re.compile(r"^0x[0-9A-Fa-f]+$")
SIZE_RE = re.compile(r"^([1-9][0-9]*)([KkMm]?)$")


class BoardMetadataError(RuntimeError):
    pass


@dataclass(frozen=True)
class BoardProfile:
    metadata_path: pathlib.Path
    metadata: dict[str, str]

    @property
    def board_dir(self) -> pathlib.Path:
        return self.metadata_path.parent

    @property
    def board_yml_path(self) -> pathlib.Path:
        return self.board_dir / "board.yml"

    @property
    def board_conf_path(self) -> pathlib.Path:
        return self.board_dir / "board.conf"

    @property
    def board_overlay_path(self) -> pathlib.Path:
        return self.board_dir / "board.overlay"

    @property
    def flash_conf_path(self) -> pathlib.Path:
        return self.board_dir / "flash.conf"

    @property
    def production_policy_path(self) -> pathlib.Path:
        return self.board_dir / "production.yml"

    @property
    def legacy_app_conf_path(self) -> pathlib.Path:
        return self.board_dir / LEGACY_APP_CONF

    @property
    def profile(self) -> str:
        return self.metadata.get("profile") or self.board_dir.name

    def get(self, key: str, default: str = "") -> str:
        return self.metadata.get(key, default)

    def require(self, key: str) -> str:
        value = self.get(key)
        if not value:
            raise BoardMetadataError(f"{self.metadata_path}: missing {key}")
        return value


def read_flat_yaml(path: pathlib.Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#") or ":" not in stripped:
            continue
        key, value = stripped.split(":", 1)
        values[key.strip()] = value.strip().strip('"')
    return values


def read_shell_assignments(path: pathlib.Path) -> dict[str, str]:
    values: dict[str, str] = {}
    if not path.is_file():
        return values

    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#") or "=" not in stripped:
            continue
        key, value = stripped.split("=", 1)
        key = key.strip()
        if not key.replace("_", "").isalnum():
            continue
        parts = shlex.split(value, comments=True, posix=True)
        values[key] = parts[0] if parts else ""
    return values


def parse_size_bytes(value: str) -> int | None:
    match = SIZE_RE.fullmatch(value)
    if not match:
        return None

    amount = int(match.group(1))
    suffix = match.group(2).upper()
    if suffix == "K":
        return amount * 1024
    if suffix == "M":
        return amount * 1024 * 1024
    return amount


def parse_offset(value: str) -> int | None:
    if not HEX_OFFSET_RE.fullmatch(value):
        return None
    return int(value, 16)


def read_production_policy(board: BoardProfile) -> dict[str, str]:
    if not board.production_policy_path.is_file():
        return {}
    return read_flat_yaml(board.production_policy_path)


def validate_production_policy(board: BoardProfile) -> list[str]:
    path = board.production_policy_path
    if not path.is_file():
        return [f"{board.board_dir}: missing production.yml"]

    policy = read_production_policy(board)
    issues: list[str] = []
    allowed_fields = set(PRODUCTION_POLICY_FIELDS)

    missing = [field for field in PRODUCTION_POLICY_FIELDS if not policy.get(field)]
    if missing:
        issues.append(f"{path}: missing {', '.join(missing)}")

    unknown = sorted(set(policy) - allowed_fields)
    if unknown:
        issues.append(f"{path}: unknown fields: {', '.join(unknown)}")

    layout = policy.get("mcuboot_partition_layout", "")
    if layout and not IDENTIFIER_RE.fullmatch(layout):
        issues.append(f"{path}: mcuboot_partition_layout must be an identifier")

    ranges: list[tuple[int, int, str]] = []
    seen_partitions: dict[str, str] = {}
    for partition in PRODUCTION_PARTITIONS:
        partition_field = f"{partition}_partition"
        offset_field = f"{partition}_offset"
        size_field = f"{partition}_size"
        partition_name = policy.get(partition_field, "")
        offset = policy.get(offset_field, "")
        size = policy.get(size_field, "")

        if partition_name and not IDENTIFIER_RE.fullmatch(partition_name):
            issues.append(f"{path}: {partition_field} must be an identifier")
        elif partition_name:
            previous = seen_partitions.get(partition_name)
            if previous:
                issues.append(f"{path}: {partition_field} duplicates {previous}")
            seen_partitions[partition_name] = partition_field

        offset_bytes = parse_offset(offset) if offset else None
        if offset and offset_bytes is None:
            issues.append(f"{path}: {offset_field} must be a hex offset like 0x20000")

        size_bytes = parse_size_bytes(size) if size else None
        if size and size_bytes is None:
            issues.append(f"{path}: {size_field} must be bytes, K, or M, for example 1344K")

        if offset_bytes is not None and size_bytes is not None:
            ranges.append((offset_bytes, offset_bytes + size_bytes, partition))

    slot0_size = parse_size_bytes(policy.get("slot0_size", ""))
    slot1_size = parse_size_bytes(policy.get("slot1_size", ""))
    if slot0_size is not None and slot1_size is not None and slot0_size != slot1_size:
        issues.append(f"{path}: slot0_size and slot1_size must match for rollback/swap")

    for field, choices in PRODUCTION_POLICY_CHOICES.items():
        value = policy.get(field, "")
        if value and value not in choices:
            issues.append(f"{path}: {field} must be one of {', '.join(choices)}")

    sorted_ranges = sorted(ranges)
    for index, (start, end, name) in enumerate(sorted_ranges):
        if start >= end:
            issues.append(f"{path}: {name} partition size must be greater than zero")
        if index == 0:
            continue
        previous_start, previous_end, previous_name = sorted_ranges[index - 1]
        if start < previous_end:
            issues.append(
                f"{path}: {name} partition overlaps {previous_name} "
                f"(0x{start:x}-0x{end:x} vs 0x{previous_start:x}-0x{previous_end:x})"
            )

    return issues


def iter_board_profiles() -> list[BoardProfile]:
    boards: list[BoardProfile] = []
    for metadata_yml in sorted(BOARDS_DIR.glob("*/*/metadata.yml")):
        boards.append(BoardProfile(metadata_yml, read_flat_yaml(metadata_yml)))
    return boards


def find_board_profile(profile: str) -> BoardProfile | None:
    for board in iter_board_profiles():
        if board.profile == profile:
            return board
    return None


def require_board_profile(profile: str) -> BoardProfile:
    board = find_board_profile(profile)
    if board is None:
        raise BoardMetadataError(
            f"board metadata not found for '{profile}' under boards/<vendor>/<board>/metadata.yml"
        )
    return board


def validate_board_profile(board: BoardProfile) -> list[str]:
    issues: list[str] = []
    metadata_yml = board.metadata_path
    board_dir = board.board_dir

    missing_fields = [field for field in REQUIRED_BOARD_FIELDS if not board.get(field)]
    if missing_fields:
        issues.append(f"{metadata_yml}: missing {', '.join(missing_fields)}")

    for required_file in REQUIRED_BOARD_FILES:
        if not (board_dir / required_file).is_file():
            issues.append(f"{board_dir}: missing {required_file}")

    issues.extend(validate_production_policy(board))

    if board.legacy_app_conf_path.exists():
        issues.append(
            f"{board.legacy_app_conf_path}: legacy app.conf is not supported; "
            "move board application defaults to board.conf"
        )

    if not any(board_dir.glob("*.dts")):
        issues.append(f"{board_dir}: missing Zephyr devicetree (*.dts)")
    if not any(board_dir.glob("*_defconfig")):
        issues.append(f"{board_dir}: missing Zephyr board defconfig (*_defconfig)")

    status = board.get("status")
    if status == "enabled" and not board.get("zephyr_board"):
        issues.append(f"{metadata_yml}: enabled boards must set zephyr_board")
    if board.get("default_display") and board.get("default_display") not in VALID_DISPLAY_MODES:
        issues.append(f"{metadata_yml}: default_display must be on or off")
    if board.get("default_settings") and board.get("default_settings") not in VALID_SETTINGS_MODES:
        issues.append(f"{metadata_yml}: default_settings must be on or off")
    if board.get("default_boot") and board.get("default_boot") not in VALID_BOOT_MODES:
        issues.append(f"{metadata_yml}: default_boot must be no-mcuboot or mcuboot")

    return issues


def require_valid_board_profile(profile: str) -> BoardProfile:
    board = require_board_profile(profile)
    issues = validate_board_profile(board)
    if issues:
        raise BoardMetadataError("\n".join(issues))
    return board


def resolved_flash_settings(board: BoardProfile) -> dict[str, str]:
    flash_conf = read_shell_assignments(board.flash_conf_path)
    return {
        "runner": board.get("flash_runner"),
        "chip": flash_conf.get("FLASH_CHIP") or board.get("flash_chip") or "esp32",
        "offset": flash_conf.get("FLASH_OFFSET") or board.get("flash_offset") or "0x1000",
        "baud": flash_conf.get("FLASH_BAUD") or "460800",
        "mode": flash_conf.get("FLASH_MODE") or "dio",
        "freq": flash_conf.get("FLASH_FREQ") or "40m",
        "size": flash_conf.get("FLASH_SIZE") or "detect",
    }


def shell_quote(value: str | pathlib.Path) -> str:
    return shlex.quote(str(value))


def board_shell_environment(board: BoardProfile) -> dict[str, str]:
    flash = resolved_flash_settings(board)
    return {
        "BOARD_METADATA": str(board.metadata_path),
        "BOARD_DIR": str(board.board_dir),
        "BOARD_YML": str(board.board_yml_path),
        "BOARD_CONF": str(board.board_conf_path),
        "BOARD_OVERLAY": str(board.board_overlay_path),
        "BOARD_FLASH_CONF": str(board.flash_conf_path),
        "BOARD_PRODUCTION_POLICY": str(board.production_policy_path),
        "BOARD_ROOT": str(PROJECT_ROOT),
        "BOARD_PROFILE": board.profile,
        "BOARD_STATUS": board.get("status"),
        "ZEPHYR_BOARD": board.get("zephyr_board"),
        "BOARD_DISPLAY_NAME": board.get("display_name"),
        "BOARD_SERIAL_BAUD": board.get("serial_baud"),
        "BOARD_FLASH_RUNNER": flash["runner"],
        "BOARD_FLASH_CHIP": flash["chip"],
        "BOARD_FLASH_OFFSET": flash["offset"],
        "BOARD_FLASH_BAUD": flash["baud"],
        "BOARD_FLASH_MODE": flash["mode"],
        "BOARD_FLASH_FREQ": flash["freq"],
        "BOARD_FLASH_SIZE": flash["size"],
        "BOARD_DESCRIPTION": board.get("description"),
        "DEFAULT_DISPLAY": board.get("default_display"),
        "DEFAULT_SETTINGS": board.get("default_settings"),
        "DEFAULT_BOOT": board.get("default_boot"),
    }


def print_shell_environment(board: BoardProfile) -> None:
    for key, value in board_shell_environment(board).items():
        print(f"{key}={shell_quote(value)}")
