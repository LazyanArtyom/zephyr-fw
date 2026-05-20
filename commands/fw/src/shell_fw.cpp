#include <platform/board/board_info.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

namespace {

void PrintVersion(const shell* shell) {
    shell_print(shell, "Version: %s%s", platform::board::GetFirmwareVersion(),
                platform::board::IsGitDirty() ? "-dirty" : "");
}

void PrintBuild(const shell* shell) {
    shell_print(shell, "Build profile: %s", platform::board::GetBuildProfile());
    shell_print(shell, "Boot mode: %s", platform::board::GetBootMode());
    shell_print(shell, "Display mode: %s", platform::board::GetDisplayMode());
    shell_print(shell, "Git commit: %s%s", platform::board::GetGitCommit(),
                platform::board::IsGitDirty() ? " (dirty)" : "");
    shell_print(shell, "Built: %s", platform::board::GetBuildTimestamp());
}

int CmdInfo(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "Firmware: %s", platform::board::GetDisplayName());
    shell_print(shell, "Name: %s", platform::board::GetFirmwareName());
    shell_print(shell, "Slug: %s", platform::board::GetFirmwareSlug());
    shell_print(shell, "Vendor: %s", platform::board::GetVendorName());
    PrintVersion(shell);
    shell_print(shell, "Board profile: %s", platform::board::GetBoardProfile());
    shell_print(shell, "Zephyr board: %s", platform::board::GetZephyrBoardTarget());
    PrintBuild(shell);
    shell_print(shell, "Uptime: %lld ms", k_uptime_get());
    return 0;
}

int CmdVersion(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    PrintVersion(shell);
    return 0;
}

int CmdBuild(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

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
