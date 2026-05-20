#include <platform/core/clock.h>
#include <platform/core/reset_info.h>
#include <zephyr/shell/shell.h>

namespace {

int CmdUptime(const shell* shell, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    shell_print(shell, "Uptime: %lld ms",
                static_cast<long long>(platform::Clock::UptimeMilliseconds()));
    return 0;
}

int CmdResetReason(const shell* shell, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    const platform::ResetInfo reset_info = platform::ResetInfo::Current();

    shell_print(shell, "Reset reason: %s", reset_info.reason_string());
    return 0;
}

int CmdReboot(const shell* shell, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    shell_print(shell, "Rebooting...");
    platform::Clock::SleepMilliseconds(100);
    platform::ResetInfo::RequestColdReboot();
    return 0;
}

}  // namespace

SHELL_STATIC_SUBCMD_SET_CREATE(system_subcommands,
                               SHELL_CMD(uptime, NULL, "Show system uptime.", CmdUptime),
                               SHELL_CMD(reset_reason, NULL, "Show reset reason.", CmdResetReason),
                               SHELL_CMD(reboot, NULL, "Reboot the board.", CmdReboot),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(system, &system_subcommands, "System commands.", NULL);
