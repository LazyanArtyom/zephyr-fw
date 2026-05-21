#include <platform/i2c/i2c_scanner.h>
#include <zephyr/shell/shell.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

namespace {

int PrintUsage(const shell* shell) {
    shell_print(shell, "Usage:");
    shell_print(shell, "  i2cdetect [-a] [-r] -y <bus>");
    shell_print(shell, "  i2cdetect [-a] [-r] -y 0");
    shell_print(shell, "  i2cdetect [-a] [-r] -y i2c0");
    return 0;
}

bool IsFlagToken(const char* token) {
    return token != nullptr && token[0] == '-' && token[1] != '\0';
}

const char* CellForState(const platform::I2cAddressProbeResult& result) {
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

    return nullptr;
}

void AppendCell(char* line, size_t line_size, size_t* offset,
                const platform::I2cAddressProbeResult& result) {
    const char* cell = CellForState(result);

    if (cell != nullptr) {
        *offset += static_cast<size_t>(
            snprintf(&line[*offset], line_size - *offset, " %s", cell));
        return;
    }

    *offset += static_cast<size_t>(
        snprintf(&line[*offset], line_size - *offset, " %02x", result.address));
}

void PrintScanTable(const shell* shell, const platform::I2cScanResult& result) {
    shell_print(shell, "     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");

    for (std::uint8_t row = 0; row < 0x80; row += 0x10) {
        char line[64]{};
        size_t offset = static_cast<size_t>(snprintf(line, sizeof(line), "%02x:", row));

        for (std::uint8_t column = 0; column < 0x10; ++column) {
            const std::uint8_t address = static_cast<std::uint8_t>(row + column);
            AppendCell(line, sizeof(line), &offset, result.addresses[address]);
        }

        shell_print(shell, "%s", line);
    }
}

int ParseArgs(const shell* shell, size_t argc, char** argv, platform::I2cScanOptions* options,
              const char** bus_spec) {
    bool assume_yes = false;
    *bus_spec = nullptr;

    for (size_t arg_index = 1; arg_index < argc; ++arg_index) {
        const char* arg = argv[arg_index];

        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            return PrintUsage(shell);
        }

        if (!IsFlagToken(arg)) {
            if (*bus_spec != nullptr) {
                shell_error(shell, "unexpected extra argument: %s", arg);
                return -EINVAL;
            }
            *bus_spec = arg;
            continue;
        }

        for (const char* flag = &arg[1]; *flag != '\0'; ++flag) {
            switch (*flag) {
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
                    shell_error(shell, "unknown option: -%c", *flag);
                    return -EINVAL;
            }
        }
    }

    if (*bus_spec == nullptr) {
        shell_error(shell, "missing i2c bus");
        PrintUsage(shell);
        return -EINVAL;
    }

    if (!assume_yes) {
        shell_error(shell, "refusing to scan without -y");
        shell_print(shell, "Use -y after confirming this bus is safe to probe.");
        return -EACCES;
    }

    return 1;
}

int CmdI2cDetect(const shell* shell, size_t argc, char** argv) {
    platform::I2cScanOptions options{};
    const char* bus_spec = nullptr;

    const int parse_result = ParseArgs(shell, argc, argv, &options, &bus_spec);
    if (parse_result <= 0) {
        return parse_result;
    }

    platform::Result<platform::I2cBus> bus_result = platform::I2cBus::Resolve(bus_spec);
    if (!bus_result.ok()) {
        shell_error(shell, "cannot open i2c bus '%s': %s", bus_spec,
                    bus_result.status().message().c_str());
        return -ENODEV;
    }

    platform::I2cScanResult scan_result{};
    const platform::Status scan_status =
        platform::I2cScanner::Scan(bus_result.value(), options, &scan_result);
    if (!scan_status.ok()) {
        shell_error(shell, "i2c scan failed: %s", scan_status.message().c_str());
        return -EIO;
    }

    shell_print(shell, "I2C bus: %s (%s)", bus_result.value().name().c_str(),
                bus_result.value().device_name().c_str());
    shell_print(shell, "Probe: %s",
                options.probe_method == platform::I2cProbeMethod::kReadByte ? "read byte"
                                                                             : "quick write");
    PrintScanTable(shell, scan_result);
    return 0;
}

}  // namespace

SHELL_CMD_ARG_REGISTER(i2cdetect, NULL, "Linux-like I2C bus scanner.", CmdI2cDetect, 2, 8);
