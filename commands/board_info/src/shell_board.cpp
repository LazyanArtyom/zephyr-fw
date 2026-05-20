#include <platform/board/board_info.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

namespace {

void PrintBoardInfo(const shell* shell) {
    shell_print(shell, "Board profile: %s", platform::board::GetBoardProfile());
    shell_print(shell, "Board name: %s", platform::board::GetBoardName());
    shell_print(shell, "Zephyr board: %s", platform::board::GetZephyrBoardTarget());
    shell_print(shell, "Firmware: %s", platform::board::GetDisplayName());
    shell_print(shell, "Version: %s%s", platform::board::GetFirmwareVersion(),
                platform::board::IsGitDirty() ? "-dirty" : "");
}

int CmdBoardInfo(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    PrintBoardInfo(shell);
    return 0;
}

int CmdBoardCaps(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

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
