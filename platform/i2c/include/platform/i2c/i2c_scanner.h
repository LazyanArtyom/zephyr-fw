#ifndef PLATFORM_I2C_I2C_SCANNER_H_
#define PLATFORM_I2C_I2C_SCANNER_H_

#include <platform/core/device_ref.h>
#include <platform/core/result.h>
#include <platform/core/status.h>
#include <platform/core/string_view.h>

#include <cstddef>
#include <cstdint>

namespace platform {

enum class I2cProbeMethod : std::uint8_t {
    kQuickWrite = 0,
    kReadByte,
};

enum class I2cAddressState : std::uint8_t {
    kSkipped = 0,
    kNoResponse,
    kResponded,
    kClaimed,
    kError,
};

struct I2cScanOptions {
    bool include_reserved_addresses{false};
    I2cProbeMethod probe_method{I2cProbeMethod::kQuickWrite};
};

struct I2cAddressProbeResult {
    std::uint8_t address{0};
    I2cAddressState state{I2cAddressState::kSkipped};
    Status status{Status::Ok()};
    int native_error{0};
};

struct I2cScanResult {
    static constexpr std::size_t kAddressCount = 128;

    I2cAddressProbeResult addresses[kAddressCount]{};
};

class I2cBus final {
   public:
    constexpr I2cBus() = default;
    constexpr I2cBus(std::uint8_t index, DeviceRef device, StringView name, StringView device_name)
        : index_(index), device_(device), name_(name), device_name_(device_name) {}

    [[nodiscard]] static Result<I2cBus> Resolve(StringView bus_spec);

    [[nodiscard]] constexpr std::uint8_t index() const {
        return index_;
    }
    [[nodiscard]] constexpr StringView name() const {
        return name_;
    }
    [[nodiscard]] constexpr StringView device_name() const {
        return device_name_;
    }
    [[nodiscard]] constexpr DeviceRef device() const {
        return device_;
    }
    [[nodiscard]] bool is_ready() const {
        return device_.is_ready();
    }
    [[nodiscard]] bool is_address_claimed(std::uint8_t address) const;

   private:
    std::uint8_t index_{0};
    DeviceRef device_{};
    StringView name_{};
    StringView device_name_{};
};

class I2cScanner final {
   public:
    [[nodiscard]] static bool IsReservedAddress(std::uint8_t address);
    [[nodiscard]] static I2cAddressProbeResult Probe(const I2cBus& bus, std::uint8_t address,
                                                     I2cProbeMethod method);
    [[nodiscard]] static Status Scan(const I2cBus& bus, const I2cScanOptions& options,
                                     I2cScanResult* result);
};

}  // namespace platform

#endif  // PLATFORM_I2C_I2C_SCANNER_H_
