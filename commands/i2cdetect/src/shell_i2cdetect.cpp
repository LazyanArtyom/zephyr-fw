#include <errno.h>
#include <platform/core/fixed_string.h>
#include <platform/i2c/i2c_scanner.h>
#include <platform/shell/console.h>
#include <zephyr/shell/shell.h>

namespace {

int PrintUsage(const platform::shell::Console& output) {
    output.line("Usage:");
    output.line("  i2cdetect [-a] [-r] -y <bus>");
    output.line("  i2cdetect [-a] [-r] -y 0");
    output.line("  i2cdetect [-a] [-r] -y i2c0");
    return 0;
}

bool IsFlagToken(platform::StringView token) {
    return token.size() > 1 && token[0] == '-';
}

platform::StringView CellForState(const platform::I2cAddressProbeResult& result) {
    switch (result.state) {
        case platform::I2cAddressState::kSkipped:
            return "  ";
        case platform::I2cAddressState::kNoResponse:
            return "--";
        case platform::I2cAddressState::kClaimed:
            return "UU";
        case platform::I2cAddressState::kError:
            return "??";
        case platform::I2cAddressState::kResponded:
            break;
    }

    return {};
}

void AppendCell(platform::FixedString<64>& line, const platform::I2cAddressProbeResult& result) {
    const platform::StringView cell = CellForState(result);

    line.append(' ');
    if (!cell.empty()) {
        line.append(cell);
        return;
    }

    line.append_hex_byte(result.address);
}

void PrintScanTable(const platform::shell::Console& output, const platform::I2cScanResult& result) {
    output.line("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");

    for (std::uint8_t row = 0; row < 0x80; row += 0x10) {
        platform::FixedString<64> line;
        line.append_hex_byte(row);
        line.append(':');

        for (std::uint8_t column = 0; column < 0x10; ++column) {
            const std::uint8_t address = static_cast<std::uint8_t>(row + column);
            AppendCell(line, result.addresses[address]);
        }

        output.line(line.view());
    }
}

int ParseArgs(const platform::shell::Console& output, const platform::shell::Arguments& arguments,
              platform::I2cScanOptions* options, platform::StringView* bus_spec) {
    bool assume_yes = false;
    *bus_spec = {};

    for (std::size_t arg_index = 1; arg_index < arguments.size(); ++arg_index) {
        const platform::StringView arg = arguments.at(arg_index);

        if (arg.equals("--help") || arg.equals("-h")) {
            return PrintUsage(output);
        }

        if (!IsFlagToken(arg)) {
            if (!bus_spec->empty()) {
                output.error_value("unexpected extra argument", arg);
                return -EINVAL;
            }
            *bus_spec = arg;
            continue;
        }

        for (std::size_t flag_index = 1; flag_index < arg.size(); ++flag_index) {
            switch (arg[flag_index]) {
                case 'a':
                    options->include_reserved_addresses = true;
                    break;
                case 'r':
                    options->probe_method = platform::I2cProbeMethod::kReadByte;
                    break;
                case 'y':
                    assume_yes = true;
                    break;
                default:
                    output.error_option(arg[flag_index]);
                    return -EINVAL;
            }
        }
    }

    if (bus_spec->empty()) {
        output.error("missing i2c bus");
        PrintUsage(output);
        return -EINVAL;
    }

    if (!assume_yes) {
        output.error("refusing to scan without -y");
        output.line("Use -y after confirming this bus is safe to probe.");
        return -EACCES;
    }

    return 1;
}

int CmdI2cDetect(const shell* shell, size_t argc, char** argv) {
    const platform::shell::Console output(shell);
    const platform::shell::Arguments arguments(argc, argv);
    platform::I2cScanOptions options{};
    platform::StringView bus_spec{};

    const int parse_result = ParseArgs(output, arguments, &options, &bus_spec);
    if (parse_result <= 0) {
        return parse_result;
    }

    platform::Result<platform::I2cBus> bus_result = platform::I2cBus::Resolve(bus_spec);
    if (!bus_result.ok()) {
        output.error_subject("cannot open i2c bus", bus_spec, bus_result.status().message());
        return -ENODEV;
    }

    platform::I2cScanResult scan_result{};
    const platform::Status scan_status =
        platform::I2cScanner::Scan(bus_result.value(), options, &scan_result);
    if (!scan_status.ok()) {
        output.error_value("i2c scan failed", scan_status.message());
        return -EIO;
    }

    output.field_pair("I2C bus", bus_result.value().name(), " (", bus_result.value().device_name(),
                      ")");
    output.field("Probe", options.probe_method == platform::I2cProbeMethod::kReadByte
                              ? "read byte"
                              : "quick write");
    PrintScanTable(output, scan_result);
    return 0;
}

}  // namespace

SHELL_CMD_ARG_REGISTER(i2cdetect, NULL, "Linux-like I2C bus scanner.", CmdI2cDetect, 2, 8);
