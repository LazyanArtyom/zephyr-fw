#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/reboot.h>

namespace {

int CmdUptime(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "Uptime: %lld ms", k_uptime_get());
    return 0;
}

int CmdReboot(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "Rebooting...");
    k_sleep(K_MSEC(100));
    sys_reboot(SYS_REBOOT_COLD);
    return 0;
}

}  // namespace

SHELL_STATIC_SUBCMD_SET_CREATE(system_subcommands,
                               SHELL_CMD(uptime, NULL, "Show system uptime.", CmdUptime),
                               SHELL_CMD(reboot, NULL, "Reboot the board.", CmdReboot),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(system, &system_subcommands, "System commands.", NULL);
