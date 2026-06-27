from __future__ import annotations

import pathlib
import unittest

PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]


def read_config(path: str) -> str:
    return (PROJECT_ROOT / path).read_text(encoding="utf-8")


class ConfigPolicyTests(unittest.TestCase):
    def test_diagnostics_profiles_have_service_stack_headroom(self) -> None:
        for profile in ("debug", "service"):
            with self.subTest(profile=profile):
                config = read_config(f"configs/profiles/{profile}.conf")
                self.assertIn("CONFIG_SHELL_STACK_SIZE=6144", config)
                self.assertIn("CONFIG_LOG_PROCESS_THREAD_STACK_SIZE=4096", config)

    def test_shell_and_no_shell_log_backends_are_not_both_uart_and_shell(self) -> None:
        shell = read_config("configs/features/shell.conf")
        no_shell = read_config("configs/features/no_shell.conf")

        self.assertIn("CONFIG_SHELL_LOG_BACKEND=y", shell)
        self.assertIn("CONFIG_LOG_BACKEND_UART=n", shell)
        self.assertIn("CONFIG_SHELL_LOG_BACKEND=n", no_shell)
        self.assertIn("CONFIG_LOG_BACKEND_UART=y", no_shell)

    def test_esp32_oled_declares_four_megabyte_flash(self) -> None:
        board = read_config("boards/espressif/esp32_oled/board.conf")
        self.assertIn("CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y", board)


if __name__ == "__main__":
    unittest.main()
