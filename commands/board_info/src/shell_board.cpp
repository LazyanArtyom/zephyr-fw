#include <platform/board/board_info.h>
#include <platform/core/clock.h>
#include <platform/core/reset_info.h>
#include <zephyr/shell/shell.h>

namespace {

const char* EnabledDisabled(bool enabled) {
    return enabled ? "enabled" : "disabled";
}

void PrintBoardInfo(const shell* shell) {
    const platform::BoardInfo& board_info = platform::BoardInfo::Current();

    shell_print(shell, "Firmware: %s", board_info.display_name());
    shell_print(shell, "Name: %s", board_info.firmware_name());
    shell_print(shell, "Version: %s%s", board_info.firmware_version(),
                board_info.is_git_dirty() ? "-dirty" : "");
    shell_print(shell, "Git commit: %s%s", board_info.git_commit(),
                board_info.is_git_dirty() ? " (dirty)" : "");
    shell_print(shell, "Build time: %s", board_info.build_timestamp());
    shell_print(shell, "Build profile: %s", board_info.build_profile());
    shell_print(shell, "Boot mode: %s", board_info.boot_mode());
    shell_print(shell, "Display mode: %s", board_info.display_mode());
    shell_print(shell, "");
    shell_print(shell, "Board:");
    shell_print(shell, "Board profile: %s", board_info.board_profile());
    shell_print(shell, "Board name: %s", board_info.board_name());
    shell_print(shell, "Board status: %s", board_info.board_status());
    shell_print(shell, "Serial baud: %s", board_info.board_serial_baud());
    shell_print(shell, "Zephyr board: %s", board_info.zephyr_board_target());
    shell_print(shell, "Description: %s", board_info.board_description());
    shell_print(shell, "");
    shell_print(shell, "Platform:");
    shell_print(shell, "Zephyr version: %s", board_info.zephyr_version());
    shell_print(shell, "Toolchain: %s", board_info.toolchain_variant());
    shell_print(shell, "SDK path: %s", board_info.zephyr_sdk_install_dir());
    shell_print(shell, "Compiler: %s %s", board_info.compiler_id(), board_info.compiler_version());
    shell_print(shell, "Architecture: %s", board_info.architecture());
    shell_print(shell, "SoC: %s", board_info.soc_name());
    shell_print(shell, "SoC family: %s", board_info.soc_family());
    shell_print(shell, "SoC series: %s", board_info.soc_series());
    shell_print(shell, "SoC part: %s", board_info.soc_part_number());
    shell_print(shell, "");
    shell_print(shell, "Flash:");
    shell_print(shell, "  runner: %s", board_info.flash_runner());
    shell_print(shell, "  chip: %s", board_info.flash_chip());
    shell_print(shell, "  offset: %s", board_info.flash_offset());
    shell_print(shell, "");
    shell_print(shell, "Runtime:");
    shell_print(shell, "Reset reason: %s", platform::ResetInfo::Current().reason_string());
    shell_print(shell, "Uptime: %lld ms",
                static_cast<long long>(platform::Clock::UptimeMilliseconds()));
    shell_print(shell, "");
    shell_print(shell, "Enabled features:");
    shell_print(shell, "  shell: %s", EnabledDisabled(board_info.shell_enabled()));
    shell_print(shell, "  display: %s", EnabledDisabled(board_info.display_enabled()));
    shell_print(shell, "  i2c: %s", EnabledDisabled(board_info.i2c_enabled()));
    shell_print(shell, "  settings: %s", EnabledDisabled(board_info.settings_enabled()));
    shell_print(shell, "  flash: %s", EnabledDisabled(board_info.flash_enabled()));
    shell_print(shell, "  mcuboot: %s", EnabledDisabled(board_info.mcuboot_enabled()));
    shell_print(shell, "Settings backend: %s", board_info.settings_backend());
    shell_print(shell, "Settings backend status: %s", board_info.settings_backend_status());
    shell_print(shell, "Storage partition status: %s", board_info.storage_partition_status());
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

    const platform::BoardInfo& board_info = platform::BoardInfo::Current();

    shell_print(shell, "Capabilities:");
    shell_print(shell, "  shell: %s", EnabledDisabled(board_info.shell_enabled()));
    shell_print(shell, "  display: %s", EnabledDisabled(board_info.display_enabled()));
    shell_print(shell, "  i2c: %s", EnabledDisabled(board_info.i2c_enabled()));
    shell_print(shell, "  settings: %s", EnabledDisabled(board_info.settings_enabled()));
    shell_print(shell, "  flash: %s", EnabledDisabled(board_info.flash_enabled()));
    shell_print(shell, "  mcuboot: %s", EnabledDisabled(board_info.mcuboot_enabled()));
    return 0;
}

}  // namespace

SHELL_STATIC_SUBCMD_SET_CREATE(board_subcommands,
                               SHELL_CMD(info, NULL, "Show board information.", CmdBoardInfo),
                               SHELL_CMD(caps, NULL, "Show board capabilities.", CmdBoardCaps),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(board_info, NULL, "Show board information.", CmdBoardInfo);
SHELL_CMD_REGISTER(board, &board_subcommands, "Board commands.", NULL);
