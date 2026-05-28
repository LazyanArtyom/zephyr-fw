#include <errno.h>
#include <platform/board/board_info.h>
#include <platform/core/clock.h>
#include <platform/core/reset_info.h>
#include <platform/shell/console.h>
#include <services/health/health_service.h>
#include <zephyr/shell/shell.h>

namespace {

void PrintBoardInfo(const platform::shell::Console& output) {
    const platform::BoardInfo& board_info = platform::BoardInfo::Current();

    output.field("Firmware", board_info.display_name());
    output.field("Name", board_info.firmware_name());
    output.field_with_suffix("Version", board_info.firmware_version(),
                             board_info.is_git_dirty() ? "-dirty" : "");
    output.field_with_suffix("Git commit", board_info.git_commit(),
                             board_info.is_git_dirty() ? " (dirty)" : "");
    output.field("Build time", board_info.build_timestamp());
    output.field("Build profile", board_info.build_profile());
    output.field("Boot mode", board_info.boot_mode());
    output.field("Display mode", board_info.display_mode());
    output.blank_line();
    output.section("Board");
    output.field("Board profile", board_info.board_profile());
    output.field("Board name", board_info.board_name());
    output.field("Board status", board_info.board_status());
    output.field("Serial baud", board_info.board_serial_baud());
    output.field("Zephyr board", board_info.zephyr_board_target());
    output.field("Description", board_info.board_description());
    output.blank_line();
    output.section("Platform");
    output.field("Zephyr version", board_info.zephyr_version());
    output.field("Toolchain", board_info.toolchain_variant());
    output.field("SDK path", board_info.zephyr_sdk_install_dir());
    output.field_pair("Compiler", board_info.compiler_id(), " ", board_info.compiler_version());
    output.field("Architecture", board_info.architecture());
    output.field("SoC", board_info.soc_name());
    output.field("SoC family", board_info.soc_family());
    output.field("SoC series", board_info.soc_series());
    output.field("SoC part", board_info.soc_part_number());
    output.blank_line();
    output.section("Flash");
    output.subfield("runner", board_info.flash_runner());
    output.subfield("chip", board_info.flash_chip());
    output.subfield("offset", board_info.flash_offset());
    output.blank_line();
    output.section("Runtime");
    output.field("Reset reason", platform::ResetInfo::Current().reason_text());
    output.integer_field("Uptime", platform::Clock::UptimeMilliseconds(), "ms");
    output.blank_line();
    output.section("Enabled features");
    output.feature("shell", board_info.shell_enabled());
    output.feature("display", board_info.display_enabled());
    output.feature("i2c", board_info.i2c_enabled());
    output.feature("settings", board_info.settings_enabled());
    output.feature("flash", board_info.flash_enabled());
    output.feature("mcuboot", board_info.mcuboot_enabled());
    output.field("Settings backend", board_info.settings_backend());
    output.field("Settings backend status", board_info.settings_backend_status());
    output.field("Storage partition status", board_info.storage_partition_status());
}


int ToErrno(const platform::Status& status) {
    switch (status.code()) {
        case platform::StatusCode::kOk:
            return 0;
        case platform::StatusCode::kInvalidArgument:
            return -EINVAL;
        case platform::StatusCode::kNotFound:
            return -ENOENT;
        case platform::StatusCode::kPermissionDenied:
            return -EACCES;
        case platform::StatusCode::kBusy:
            return -EBUSY;
        case platform::StatusCode::kNotSupported:
            return -ENOTSUP;
        case platform::StatusCode::kUnavailable:
            return -ENODEV;
        default:
            return -EIO;
    }
}

int PrintStatusError(const platform::shell::Console& output, platform::StringView action,
                     const platform::Status& status) {
    output.error_subject(action, platform::ToString(status.code()), status.message());
    return ToErrno(status);
}

void PrintManufacturingValue(const platform::shell::Console& output,
                             const services::health::ManufacturingValue& value) {
    output.field(value.key, value.value.view());
}

int CmdBoardSerial(const shell* shell, size_t argc, char** argv) {
    const platform::shell::Console output(shell);
    const platform::shell::Arguments arguments(argc, argv);

    if (arguments.size() < 3) {
        output.line("Usage: board serial get|set <value>");
        return -EINVAL;
    }

    const platform::StringView action = arguments.at(2);
    if (action.equals("get")) {
        PrintManufacturingValue(output, services::health::ManufacturingService::BoardSerial());
        return 0;
    }
    if (action.equals("set") && arguments.size() == 4) {
        const platform::Status status =
            services::health::ManufacturingService::SetBoardSerial(arguments.at(3));
        if (!status.ok()) {
            return PrintStatusError(output, "board serial set failed", status);
        }
        output.field("board/serial", arguments.at(3));
        return 0;
    }

    output.error_value("unknown board serial action", action);
    return -EINVAL;
}

int CmdBoardHardwareRevision(const shell* shell, size_t argc, char** argv) {
    const platform::shell::Console output(shell);
    const platform::shell::Arguments arguments(argc, argv);

    if (arguments.size() < 3) {
        output.line("Usage: board hw-rev get|set <value>");
        return -EINVAL;
    }

    const platform::StringView action = arguments.at(2);
    if (action.equals("get")) {
        PrintManufacturingValue(output,
                                services::health::ManufacturingService::BoardHardwareRevision());
        return 0;
    }
    if (action.equals("set") && arguments.size() == 4) {
        const platform::Status status =
            services::health::ManufacturingService::SetBoardHardwareRevision(arguments.at(3));
        if (!status.ok()) {
            return PrintStatusError(output, "board hw-rev set failed", status);
        }
        output.field("board/hw-rev", arguments.at(3));
        return 0;
    }

    output.error_value("unknown board hw-rev action", action);
    return -EINVAL;
}

int CmdBoardInfo(const shell* shell, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    PrintBoardInfo(platform::shell::Console(shell));
    return 0;
}

int CmdBoardCaps(const shell* shell, size_t argc, char** argv) {
    (void)argc;
    (void)argv;

    const platform::BoardInfo& board_info = platform::BoardInfo::Current();
    const platform::shell::Console output(shell);

    output.section("Capabilities");
    output.feature("shell", board_info.shell_enabled());
    output.feature("display", board_info.display_enabled());
    output.feature("i2c", board_info.i2c_enabled());
    output.feature("settings", board_info.settings_enabled());
    output.feature("flash", board_info.flash_enabled());
    output.feature("mcuboot", board_info.mcuboot_enabled());
    return 0;
}

}  // namespace

SHELL_STATIC_SUBCMD_SET_CREATE(board_subcommands,
                               SHELL_CMD(info, NULL, "Show board information.", CmdBoardInfo),
                               SHELL_CMD(caps, NULL, "Show board capabilities.", CmdBoardCaps),
                               SHELL_CMD_ARG(serial, NULL, "Get or set board serial.", CmdBoardSerial, 3, 1),
                               SHELL_CMD_ARG(hw-rev, NULL, "Get or set board hardware revision.", CmdBoardHardwareRevision, 3, 1),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(board_info, NULL, "Show board information.", CmdBoardInfo);
SHELL_CMD_REGISTER(board, &board_subcommands, "Board commands.", NULL);
