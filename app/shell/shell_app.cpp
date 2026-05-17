#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include <app/app_context.hpp>
#include <app/board_info.hpp>

#include "hello_tool.hpp"

namespace {

int CmdStatus(const shell* shell, size_t argc, char** argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    const auto& context = app::GetAppContext();
    shell_print(shell, "Application status: %s", context.IsInitialized() ? "running" : "not initialized");
    return 0;
}

int CmdVersion(const shell* shell, size_t argc, char** argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "Version: %s", app::GetAppVersion());
    shell_print(shell, "Build profile: %s", app::GetBuildProfile());
    return 0;
}

int CmdHelloWorld(const shell* shell, size_t argc, char** argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    return app::RunHelloTool(shell);
}

}  // namespace

SHELL_STATIC_SUBCMD_SET_CREATE(
    app_subcommands,
    SHELL_CMD(status, NULL, "Show application status.", CmdStatus),
    SHELL_CMD(version, NULL, "Show application version.", CmdVersion),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(app, &app_subcommands, "Application commands.", NULL);
SHELL_CMD_REGISTER(hello_world, NULL, "Run the hello world command.", CmdHelloWorld);
