#include <platform/i2c/i2c_scanner.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

namespace {

struct BusDescriptor {
    std::uint8_t index;
    const device* dev;
    const char* name;
    const char* device_name;
};

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c0), okay)
#define FW_I2C0_ENTRY \
    { 0, DEVICE_DT_GET(DT_NODELABEL(i2c0)), "i2c0", DEVICE_DT_NAME(DT_NODELABEL(i2c0)) },
#else
#define FW_I2C0_ENTRY
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
#define FW_I2C1_ENTRY \
    { 1, DEVICE_DT_GET(DT_NODELABEL(i2c1)), "i2c1", DEVICE_DT_NAME(DT_NODELABEL(i2c1)) },
#else
#define FW_I2C1_ENTRY
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c2), okay)
#define FW_I2C2_ENTRY \
    { 2, DEVICE_DT_GET(DT_NODELABEL(i2c2)), "i2c2", DEVICE_DT_NAME(DT_NODELABEL(i2c2)) },
#else
#define FW_I2C2_ENTRY
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c3), okay)
#define FW_I2C3_ENTRY \
    { 3, DEVICE_DT_GET(DT_NODELABEL(i2c3)), "i2c3", DEVICE_DT_NAME(DT_NODELABEL(i2c3)) },
#else
#define FW_I2C3_ENTRY
#endif

constexpr BusDescriptor kBuses[] = {
    FW_I2C0_ENTRY
    FW_I2C1_ENTRY
    FW_I2C2_ENTRY
    FW_I2C3_ENTRY
    { 0xff, nullptr, "", "" },
};

bool IsUnsignedInteger(const char* value) {
    if (value == nullptr || value[0] == '\0') {
        return false;
    }

    for (const char* cursor = value; *cursor != '\0'; ++cursor) {
        if (*cursor < '0' || *cursor > '9') {
            return false;
        }
    }

    return true;
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
#define FW_I2C_CHECK_CHILD(child)                    \
    if (DT_NODE_HAS_STATUS(child, okay) &&           \
        address == static_cast<std::uint8_t>(DT_REG_ADDR(child))) { \
        return true;                                 \
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
#define FW_I2C_CHECK_CHILD(child)                    \
    if (DT_NODE_HAS_STATUS(child, okay) &&           \
        address == static_cast<std::uint8_t>(DT_REG_ADDR(child))) { \
        return true;                                 \
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
#define FW_I2C_CHECK_CHILD(child)                    \
    if (DT_NODE_HAS_STATUS(child, okay) &&           \
        address == static_cast<std::uint8_t>(DT_REG_ADDR(child))) { \
        return true;                                 \
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
#define FW_I2C_CHECK_CHILD(child)                    \
    if (DT_NODE_HAS_STATUS(child, okay) &&           \
        address == static_cast<std::uint8_t>(DT_REG_ADDR(child))) { \
        return true;                                 \
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

Result<I2cBus> I2cBus::Resolve(const char* bus_spec) {
    if (bus_spec == nullptr || bus_spec[0] == '\0') {
        return Result<I2cBus>::FromStatus(Status::InvalidArgument("missing i2c bus"));
    }

    if (IsUnsignedInteger(bus_spec)) {
        const unsigned long requested_index = strtoul(bus_spec, nullptr, 10);
        if (requested_index > 0xffUL) {
            return Result<I2cBus>::FromStatus(Status::InvalidArgument("i2c bus index is too large"));
        }

        for (const BusDescriptor& descriptor : kBuses) {
            if (descriptor.dev == nullptr) {
                continue;
            }
            if (descriptor.index == static_cast<std::uint8_t>(requested_index)) {
                return BusFromDescriptor(descriptor);
            }
        }

        return Result<I2cBus>::FromStatus(Status::NotFound("i2c bus index was not found"));
    }

    for (const BusDescriptor& descriptor : kBuses) {
        if (descriptor.dev == nullptr) {
            continue;
        }
        if (strcmp(bus_spec, descriptor.name) == 0 || strcmp(bus_spec, descriptor.device_name) == 0) {
            return BusFromDescriptor(descriptor);
        }
    }

    return Result<I2cBus>::FromStatus(Status::NotFound("i2c bus name was not found"));
}

bool I2cBus::is_address_claimed(std::uint8_t address) const {
    switch (index_) {
        case 0:
            return I2c0ClaimsAddress(address);
        case 1:
            return I2c1ClaimsAddress(address);
        case 2:
            return I2c2ClaimsAddress(address);
        case 3:
            return I2c3ClaimsAddress(address);
        default:
            return false;
    }
}

bool I2cScanner::IsReservedAddress(std::uint8_t address) {
    return address < 0x03 || address > 0x77;
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
