#include <platform/core/clock.h>
#include <platform/core/reset_info.h>
#include <platform/shell/console.h>
#include <zephyr/shell/shell.h>

namespace {

int CmdUptime(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    platform::shell::Console(shell).integer_field("Uptime", platform::Clock::UptimeMilliseconds(),
                                                  "ms");
    return 0;
}

int CmdResetReason(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    const platform::ResetInfo reset_info = platform::ResetInfo::Current();
    const platform::shell::Console output(shell);

    output.field("Reset reason", reset_info.reason_text());
    output.integer_field("Reset flags", reset_info.flags());
    output.feature("reset cause driver", reset_info.available());
    return 0;
}

int CmdReboot(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    platform::shell::Console(shell).line("Rebooting...");
    platform::Clock::SleepMilliseconds(100);
    platform::ResetInfo::RequestColdReboot();
    return 0;
}

}  // namespace

SHELL_STATIC_SUBCMD_SET_CREATE(
    system_subcommands, SHELL_CMD(uptime, NULL, "Show system uptime.", CmdUptime),
    SHELL_CMD(reset - reason, NULL, "Show last reset reason.", CmdResetReason),
    SHELL_CMD(reboot, NULL, "Cold reboot the device.", CmdReboot), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(system, &system_subcommands, "System commands.", NULL);
