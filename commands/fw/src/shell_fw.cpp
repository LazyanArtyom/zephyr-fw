#include <platform/board/board_info.h>
#include <platform/core/clock.h>
#include <platform/shell/console.h>
#include <zephyr/shell/shell.h>

namespace {

void PrintVersion(const platform::shell::Console& output) {
    const platform::BoardInfo& board_info = platform::BoardInfo::Current();

    output.field_with_suffix("Version", board_info.firmware_version(),
                             board_info.is_git_dirty() ? "-dirty" : "");
}

void PrintBuild(const platform::shell::Console& output) {
    const platform::BoardInfo& board_info = platform::BoardInfo::Current();

    output.field("Build profile", board_info.build_profile());
    output.field("Boot mode", board_info.boot_mode());
    output.field("Display mode", board_info.display_mode());
    output.field_with_suffix("Git commit", board_info.git_commit(),
                             board_info.is_git_dirty() ? " (dirty)" : "");
    output.field("Built", board_info.build_timestamp());
    output.field("Zephyr version", board_info.zephyr_version());
    output.field("Toolchain", board_info.toolchain_variant());
    output.field("SDK path", board_info.zephyr_sdk_install_dir());
    output.field_pair("Compiler", board_info.compiler_id(), " ", board_info.compiler_version());
}

int CmdInfo(const shell* shell, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    const platform::BoardInfo& board_info = platform::BoardInfo::Current();
    const platform::shell::Console output(shell);

    output.field("Firmware", board_info.display_name());
    output.field("Name", board_info.firmware_name());
    output.field("Slug", board_info.firmware_slug());
    output.field("Vendor", board_info.vendor_name());
    PrintVersion(output);
    output.field("Board profile", board_info.board_profile());
    output.field("Zephyr board", board_info.zephyr_board_target());
    output.field("SoC", board_info.soc_name());
    PrintBuild(output);
    output.integer_field("Uptime", platform::Clock::UptimeMilliseconds(), "ms");
    return 0;
}

int CmdVersion(const shell* shell, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    PrintVersion(platform::shell::Console(shell));
    return 0;
}

int CmdBuild(const shell* shell, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    PrintBuild(platform::shell::Console(shell));
    return 0;
}

}  // namespace

SHELL_STATIC_SUBCMD_SET_CREATE(fw_subcommands,
                               SHELL_CMD(info, NULL, "Show firmware information.", CmdInfo),
                               SHELL_CMD(version, NULL, "Show firmware version.", CmdVersion),
                               SHELL_CMD(build, NULL, "Show build metadata.", CmdBuild),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(fw, &fw_subcommands, "Firmware commands.", NULL);
