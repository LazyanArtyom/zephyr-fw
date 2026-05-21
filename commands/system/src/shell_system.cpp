#include <platform/core/clock.h>
#include <platform/core/reset_info.h>
#include <zephyr/shell/shell.h>

#include <errno.h>
#include <string.h>

namespace {

int PrintSystemHelp(const shell* shell) {
    shell_print(shell, "Usage:");
    shell_print(shell, "  system uptime");
    shell_print(shell, "  system reset-reason");
    shell_print(shell, "  system reboot");
    return 0;
}

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

int CmdSystem(const shell* shell, size_t argc, char** argv) {
    if (argc < 2) {
        return PrintSystemHelp(shell);
    }

    if (strcmp(argv[1], "uptime") == 0) {
        return CmdUptime(shell, argc - 1, &argv[1]);
    }

    if (strcmp(argv[1], "reset-reason") == 0) {
        return CmdResetReason(shell, argc - 1, &argv[1]);
    }

    if (strcmp(argv[1], "reboot") == 0) {
        return CmdReboot(shell, argc - 1, &argv[1]);
    }

    shell_error(shell, "unknown system command: %s", argv[1]);
    return -EINVAL;
}

}  // namespace

SHELL_CMD_ARG_REGISTER(system, NULL, "System commands.", CmdSystem, 1, 1);
