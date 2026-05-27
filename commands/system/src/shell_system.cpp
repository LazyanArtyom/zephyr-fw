#include <errno.h>
#include <platform/core/clock.h>
#include <platform/core/reset_info.h>
#include <platform/core/string_view.h>
#include <platform/shell/console.h>
#include <zephyr/shell/shell.h>

namespace {

int PrintSystemHelp(const platform::shell::Console& output) {
    output.line("Usage:");
    output.line("  system uptime");
    output.line("  system reset-reason");
    output.line("  system reboot");
    return 0;
}

int PrintUptime(const platform::shell::Console& output) {
    output.integer_field("Uptime", platform::Clock::UptimeMilliseconds(), "ms");
    return 0;
}

int PrintResetReason(const platform::shell::Console& output) {
    const platform::ResetInfo reset_info = platform::ResetInfo::Current();

    output.field("Reset reason", reset_info.reason_text());
    return 0;
}

int Reboot(const platform::shell::Console& output) {
    output.line("Rebooting...");
    platform::Clock::SleepMilliseconds(100);
    platform::ResetInfo::RequestColdReboot();
    return 0;
}

int CmdSystem(const shell* shell, size_t argc, char** argv) {
    const platform::shell::Console output(shell);
    const platform::shell::Arguments arguments(argc, argv);

    if (arguments.size() < 2) {
        return PrintSystemHelp(output);
    }

    const platform::StringView command = arguments.at(1);
    if (command.equals("uptime")) {
        return PrintUptime(output);
    }

    if (command.equals("reset-reason")) {
        return PrintResetReason(output);
    }

    if (command.equals("reboot")) {
        return Reboot(output);
    }

    output.error_value("unknown system command", command);
    return -EINVAL;
}

}  // namespace

SHELL_CMD_ARG_REGISTER(system, NULL, "System commands.", CmdSystem, 1, 1);
