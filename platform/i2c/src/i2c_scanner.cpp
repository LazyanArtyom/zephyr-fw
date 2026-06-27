#include <errno.h>
#include <platform/i2c/i2c_scanner.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>

#include <cstdint>

namespace {

constexpr std::uint8_t kI2c0Index = 0;
constexpr std::uint8_t kI2c1Index = 1;
constexpr std::uint8_t kI2c2Index = 2;
constexpr std::uint8_t kI2c3Index = 3;
constexpr std::uint8_t kInvalidBusIndex = 0xffU;
constexpr std::uint8_t kFirstUsableAddress = 0x03U;
constexpr std::uint8_t kLastUsableAddress = 0x77U;
constexpr std::uint16_t kMaxBusIndexValue = 0xffU;
constexpr std::uint8_t kDecimalBase = 10U;

struct BusDescriptor {
    std::uint8_t index;
    const device* dev;
    platform::StringView name;
    platform::StringView device_name;
};

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c0), okay)
#define FW_I2C0_ENTRY \
    {kI2c0Index, DEVICE_DT_GET(DT_NODELABEL(i2c0)), "i2c0", DEVICE_DT_NAME(DT_NODELABEL(i2c0))},
#else
#define FW_I2C0_ENTRY
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
#define FW_I2C1_ENTRY \
    {kI2c1Index, DEVICE_DT_GET(DT_NODELABEL(i2c1)), "i2c1", DEVICE_DT_NAME(DT_NODELABEL(i2c1))},
#else
#define FW_I2C1_ENTRY
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c2), okay)
#define FW_I2C2_ENTRY \
    {kI2c2Index, DEVICE_DT_GET(DT_NODELABEL(i2c2)), "i2c2", DEVICE_DT_NAME(DT_NODELABEL(i2c2))},
#else
#define FW_I2C2_ENTRY
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c3), okay)
#define FW_I2C3_ENTRY \
    {kI2c3Index, DEVICE_DT_GET(DT_NODELABEL(i2c3)), "i2c3", DEVICE_DT_NAME(DT_NODELABEL(i2c3))},
#else
#define FW_I2C3_ENTRY
#endif

constexpr BusDescriptor kBuses[] = {
    FW_I2C0_ENTRY FW_I2C1_ENTRY FW_I2C2_ENTRY FW_I2C3_ENTRY{kInvalidBusIndex, nullptr, "", ""},
};

bool IsUnsignedInteger(platform::StringView value) {
    if (value.empty()) {
        return false;
    }

    for (std::size_t index = 0; index < value.size(); ++index) {
        if (value[index] < '0' || value[index] > '9') {
            return false;
        }
    }

    return true;
}

platform::Result<std::uint8_t> ParseIndex(platform::StringView value) {
    std::uint16_t index_value = 0;

    for (std::size_t index = 0; index < value.size(); ++index) {
        index_value = static_cast<std::uint16_t>(index_value * kDecimalBase +
                                                 static_cast<std::uint16_t>(value[index] - '0'));
        if (index_value > kMaxBusIndexValue) {
            return platform::Result<std::uint8_t>::FromStatus(
                platform::Status::InvalidArgument("i2c bus index is too large"));
        }
    }

    return platform::Result<std::uint8_t>::FromValue(static_cast<std::uint8_t>(index_value));
}

platform::Result<platform::I2cBus> BusFromDescriptor(const BusDescriptor& descriptor) {
    platform::I2cBus bus(descriptor.index, platform::DeviceRef(descriptor.dev), descriptor.name,
                         descriptor.device_name);

    if (!bus.is_ready()) {
        return platform::Result<platform::I2cBus>::FromStatus(
            platform::Status::Unavailable("i2c bus is not ready"));
    }

    return platform::Result<platform::I2cBus>::FromValue(bus);
}

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c0), okay)
bool I2c0ClaimsAddress(std::uint8_t address) {
#define FW_I2C_CHECK_CHILD(child)                                   \
    if (DT_NODE_HAS_STATUS(child, okay) &&                          \
        address == static_cast<std::uint8_t>(DT_REG_ADDR(child))) { \
        return true;                                                \
    }
    DT_FOREACH_CHILD(DT_NODELABEL(i2c0), FW_I2C_CHECK_CHILD)
#undef FW_I2C_CHECK_CHILD
    return false;
}
#else
bool I2c0ClaimsAddress(std::uint8_t) {
    return false;
}
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
bool I2c1ClaimsAddress(std::uint8_t address) {
#define FW_I2C_CHECK_CHILD(child)                                   \
    if (DT_NODE_HAS_STATUS(child, okay) &&                          \
        address == static_cast<std::uint8_t>(DT_REG_ADDR(child))) { \
        return true;                                                \
    }
    DT_FOREACH_CHILD(DT_NODELABEL(i2c1), FW_I2C_CHECK_CHILD)
#undef FW_I2C_CHECK_CHILD
    return false;
}
#else
bool I2c1ClaimsAddress(std::uint8_t) {
    return false;
}
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c2), okay)
bool I2c2ClaimsAddress(std::uint8_t address) {
#define FW_I2C_CHECK_CHILD(child)                                   \
    if (DT_NODE_HAS_STATUS(child, okay) &&                          \
        address == static_cast<std::uint8_t>(DT_REG_ADDR(child))) { \
        return true;                                                \
    }
    DT_FOREACH_CHILD(DT_NODELABEL(i2c2), FW_I2C_CHECK_CHILD)
#undef FW_I2C_CHECK_CHILD
    return false;
}
#else
bool I2c2ClaimsAddress(std::uint8_t) {
    return false;
}
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c3), okay)
bool I2c3ClaimsAddress(std::uint8_t address) {
#define FW_I2C_CHECK_CHILD(child)                                   \
    if (DT_NODE_HAS_STATUS(child, okay) &&                          \
        address == static_cast<std::uint8_t>(DT_REG_ADDR(child))) { \
        return true;                                                \
    }
    DT_FOREACH_CHILD(DT_NODELABEL(i2c3), FW_I2C_CHECK_CHILD)
#undef FW_I2C_CHECK_CHILD
    return false;
}
#else
bool I2c3ClaimsAddress(std::uint8_t) {
    return false;
}
#endif

}  // namespace

namespace platform {

Result<I2cBus> I2cBus::Resolve(StringView bus_spec) {
    if (bus_spec.empty()) {
        return Result<I2cBus>::FromStatus(Status::InvalidArgument("missing i2c bus"));
    }

    if (IsUnsignedInteger(bus_spec)) {
        const Result<std::uint8_t> index_result = ParseIndex(bus_spec);
        if (!index_result.ok()) {
            return Result<I2cBus>::FromStatus(index_result.status());
        }

        for (const BusDescriptor& descriptor : kBuses) {
            if (descriptor.dev == nullptr) {
                continue;
            }
            if (descriptor.index == index_result.value()) {
                return BusFromDescriptor(descriptor);
            }
        }

        return Result<I2cBus>::FromStatus(Status::NotFound("i2c bus index was not found"));
    }

    for (const BusDescriptor& descriptor : kBuses) {
        if (descriptor.dev == nullptr) {
            continue;
        }
        if (bus_spec.equals(descriptor.name) || bus_spec.equals(descriptor.device_name)) {
            return BusFromDescriptor(descriptor);
        }
    }

    return Result<I2cBus>::FromStatus(Status::NotFound("i2c bus name was not found"));
}

bool I2cBus::is_address_claimed(std::uint8_t address) const {
    switch (index_) {
        case kI2c0Index:
            return I2c0ClaimsAddress(address);
        case kI2c1Index:
            return I2c1ClaimsAddress(address);
        case kI2c2Index:
            return I2c2ClaimsAddress(address);
        case kI2c3Index:
            return I2c3ClaimsAddress(address);
        default:
            return false;
    }
}

bool I2cScanner::IsReservedAddress(std::uint8_t address) {
    return address < kFirstUsableAddress || address > kLastUsableAddress;
}

I2cAddressProbeResult I2cScanner::Probe(const I2cBus& bus, std::uint8_t address,
                                        I2cProbeMethod method) {
    I2cAddressProbeResult result{};
    result.address = address;

    if (!bus.is_ready()) {
        result.state = I2cAddressState::kError;
        result.status = Status::Unavailable("i2c bus is not ready");
        result.native_error = -ENODEV;
        return result;
    }

    if (bus.is_address_claimed(address)) {
        result.state = I2cAddressState::kClaimed;
        return result;
    }

    std::uint8_t byte = 0;
    int rc = 0;

    if (method == I2cProbeMethod::kReadByte) {
        rc = i2c_read(bus.device().native_handle(), &byte, sizeof(byte), address);
    } else {
        rc = i2c_write(bus.device().native_handle(), nullptr, 0, address);
    }

    result.native_error = rc;
    if (rc == 0) {
        result.state = I2cAddressState::kResponded;
        return result;
    }

    if (rc == -EIO || rc == -ENXIO || rc == -ETIMEDOUT || rc == -EAGAIN) {
        result.state = I2cAddressState::kNoResponse;
        result.status = Status::NotFound("no i2c response");
        return result;
    }

    result.state = I2cAddressState::kError;
    result.status = Status::InternalError("i2c probe failed");
    return result;
}

Status I2cScanner::Scan(const I2cBus& bus, const I2cScanOptions& options, I2cScanResult* result) {
    if (result == nullptr) {
        return Status::InvalidArgument("missing i2c scan result");
    }

    if (!bus.is_ready()) {
        return Status::Unavailable("i2c bus is not ready");
    }

    for (std::uint8_t address = 0; address < I2cScanResult::kAddressCount; ++address) {
        result->addresses[address].address = address;

        if (!options.include_reserved_addresses && IsReservedAddress(address)) {
            result->addresses[address].state = I2cAddressState::kSkipped;
            continue;
        }

        result->addresses[address] = Probe(bus, address, options.probe_method);
    }

    return Status::Ok();
}

}  // namespace platform
