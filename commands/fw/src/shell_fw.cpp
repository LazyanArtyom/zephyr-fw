#include <platform/board/board_info.h>
#include <platform/core/clock.h>
#include <zephyr/shell/shell.h>

namespace {

void PrintVersion(const shell* shell) {
    const platform::BoardInfo& board_info = platform::BoardInfo::Current();

    shell_print(shell, "Version: %s%s", board_info.firmware_version().c_str(),
                board_info.is_git_dirty() ? "-dirty" : "");
}

void PrintBuild(const shell* shell) {
    const platform::BoardInfo& board_info = platform::BoardInfo::Current();

    shell_print(shell, "Build profile: %s", board_info.build_profile().c_str());
    shell_print(shell, "Boot mode: %s", board_info.boot_mode().c_str());
    shell_print(shell, "Display mode: %s", board_info.display_mode().c_str());
    shell_print(shell, "Git commit: %s%s", board_info.git_commit().c_str(),
                board_info.is_git_dirty() ? " (dirty)" : "");
    shell_print(shell, "Built: %s", board_info.build_timestamp().c_str());
    shell_print(shell, "Zephyr version: %s", board_info.zephyr_version().c_str());
    shell_print(shell, "Toolchain: %s", board_info.toolchain_variant().c_str());
    shell_print(shell, "SDK path: %s", board_info.zephyr_sdk_install_dir().c_str());
    shell_print(shell, "Compiler: %s %s", board_info.compiler_id().c_str(), board_info.compiler_version().c_str());
}

int CmdInfo(const shell* shell, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    const platform::BoardInfo& board_info = platform::BoardInfo::Current();

    shell_print(shell, "Firmware: %s", board_info.display_name().c_str());
    shell_print(shell, "Name: %s", board_info.firmware_name().c_str());
    shell_print(shell, "Slug: %s", board_info.firmware_slug().c_str());
    shell_print(shell, "Vendor: %s", board_info.vendor_name().c_str());
    PrintVersion(shell);
    shell_print(shell, "Board profile: %s", board_info.board_profile().c_str());
    shell_print(shell, "Zephyr board: %s", board_info.zephyr_board_target().c_str());
    shell_print(shell, "SoC: %s", board_info.soc_name().c_str());
    PrintBuild(shell);
    shell_print(shell, "Uptime: %lld ms",
                static_cast<long long>(platform::Clock::UptimeMilliseconds()));
    return 0;
}

int CmdVersion(const shell* shell, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    PrintVersion(shell);
    return 0;
}

int CmdBuild(const shell* shell, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    PrintBuild(shell);
    return 0;
}

}  // namespace

SHELL_STATIC_SUBCMD_SET_CREATE(fw_subcommands,
                               SHELL_CMD(info, NULL, "Show firmware information.", CmdInfo),
                               SHELL_CMD(version, NULL, "Show firmware version.", CmdVersion),
                               SHELL_CMD(build, NULL, "Show build metadata.", CmdBuild),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(fw, &fw_subcommands, "Firmware commands.", NULL);
