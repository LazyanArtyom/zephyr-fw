from __future__ import annotations

import pathlib
import re
import unittest

PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]


def constexpr_values(source: str) -> dict[str, int]:
    pattern = re.compile(
        r"constexpr\s+(?:std::)?(?:size_t|uint8_t|uint16_t|uint32_t|int)\s+"
        r"(?P<name>k[A-Za-z0-9_]+)\s*=\s*(?P<value>\d+)\s*;"
    )
    return {match.group("name"): int(match.group("value")) for match in pattern.finditer(source)}


def resolve_arg_token(token: str, constants: dict[str, int]) -> int:
    stripped = token.strip()
    if stripped.isdigit():
        return int(stripped)
    if stripped not in constants:
        raise AssertionError(f"unresolved command contract token: {stripped}")
    return constants[stripped]


def command_arg_contract(source: str, command: str) -> tuple[int, int]:
    pattern = re.compile(
        rf"SHELL_CMD_ARG\(\s*{re.escape(command)}\s*,[^,]*,[^,]*,[^,]*,\s*([^,]+)\s*,\s*([^,)]+)\s*\)",
        re.MULTILINE,
    )
    match = pattern.search(source)
    if match is None:
        raise AssertionError(f"missing SHELL_CMD_ARG contract for {command}")
    constants = constexpr_values(source)
    return (
        resolve_arg_token(match.group(1), constants),
        resolve_arg_token(match.group(2), constants),
    )


def assert_accepts_argc(test: unittest.TestCase, contract: tuple[int, int], argc: int) -> None:
    required, optional = contract
    test.assertGreaterEqual(argc, required)
    test.assertLessEqual(argc, required + optional)


class ShellCommandContractTests(unittest.TestCase):
    def test_settings_commands_accept_local_and_full_path_argv_shapes(self) -> None:
        source = (PROJECT_ROOT / "commands/settings/src/shell_settings.cpp").read_text(encoding="utf-8")

        contracts = {
            "list": command_arg_contract(source, "list"),
            "get": command_arg_contract(source, "get"),
            "set": command_arg_contract(source, "set"),
            "reset": command_arg_contract(source, "reset"),
        }

        self.assertEqual(contracts["list"], (1, 2))
        self.assertEqual(contracts["get"], (2, 1))
        self.assertEqual(contracts["set"], (3, 1))
        self.assertEqual(contracts["reset"], (2, 1))

        # Zephyr shell integrations have been observed passing either the
        # subcommand-local argv or the full command path. Keep both valid.
        for argc in (1, 2, 3):
            assert_accepts_argc(self, contracts["list"], argc)
        for argc in (2, 3):
            assert_accepts_argc(self, contracts["get"], argc)
            assert_accepts_argc(self, contracts["reset"], argc)
        for argc in (3, 4):
            assert_accepts_argc(self, contracts["set"], argc)

        for subcommand in ("list", "get", "set", "reset"):
            self.assertIn(f'CommandArgBase(arguments, "{subcommand}")', source)

    def test_board_manufacturing_commands_accept_local_and_full_path_argv_shapes(self) -> None:
        source = (PROJECT_ROOT / "commands/board/src/shell_board.cpp").read_text(encoding="utf-8")

        serial = command_arg_contract(source, "serial")
        hw_rev = command_arg_contract(source, "hw-rev")
        self.assertEqual(serial, (2, 2))
        self.assertEqual(hw_rev, (2, 2))

        for contract in (serial, hw_rev):
            for argc in (2, 3, 4):
                assert_accepts_argc(self, contract, argc)

        self.assertIn('CommandArgBase(arguments, "serial")', source)
        self.assertIn('CommandArgBase(arguments, "hw-rev")', source)


if __name__ == "__main__":
    unittest.main()
