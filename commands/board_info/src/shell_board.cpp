#include <platform/board/board_info.h>
#include <platform/core/clock.h>
#include <platform/core/reset_info.h>
#include <zephyr/shell/shell.h>

namespace {

void PrintBoardInfo(const shell* shell) {
    const platform::BoardInfo& board_info = platform::BoardInfo::Current();

    shell_print(shell, "Board profile: %s", board_info.board_profile());
    shell_print(shell, "Board name: %s", board_info.board_name());
    shell_print(shell, "Zephyr board: %s", board_info.zephyr_board_target());
    shell_print(shell, "Firmware: %s", board_info.display_name());
    shell_print(shell, "Version: %s%s", board_info.firmware_version(),
                board_info.is_git_dirty() ? "-dirty" : "");
    shell_print(shell, "Build profile: %s", board_info.build_profile());
    shell_print(shell, "Boot mode: %s", board_info.boot_mode());
    shell_print(shell, "Display mode: %s", board_info.display_mode());
    shell_print(shell, "Git commit: %s%s", board_info.git_commit(),
                board_info.is_git_dirty() ? " (dirty)" : "");
    shell_print(shell, "Built: %s", board_info.build_timestamp());
    shell_print(shell, "Reset reason: %s", platform::ResetInfo::Current().reason_string());
    shell_print(shell, "Uptime: %lld ms",
                static_cast<long long>(platform::Clock::UptimeMilliseconds()));
}

int CmdBoardInfo(const shell* shell, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    PrintBoardInfo(shell);
    return 0;
}

int CmdBoardCaps(const shell* shell, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    shell_print(shell, "Capabilities:");
    shell_print(shell, "  shell: enabled");
#if defined(CONFIG_I2C)
    shell_print(shell, "  i2c: enabled");
#else
    shell_print(shell, "  i2c: disabled");
#endif
#if defined(CONFIG_FW_DISPLAY)
    shell_print(shell, "  display: enabled");
#else
    shell_print(shell, "  display: disabled");
#endif
    return 0;
}

}  // namespace

SHELL_STATIC_SUBCMD_SET_CREATE(board_subcommands,
                               SHELL_CMD(info, NULL, "Show board information.", CmdBoardInfo),
                               SHELL_CMD(caps, NULL, "Show board capabilities.", CmdBoardCaps),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(board_info, NULL, "Show board information.", CmdBoardInfo);
SHELL_CMD_REGISTER(board, &board_subcommands, "Board commands.", NULL);
