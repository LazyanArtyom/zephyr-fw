#ifndef PLATFORM_CORE_RESET_INFO_H_
#define PLATFORM_CORE_RESET_INFO_H_

#include <platform/core/string_view.h>

#include <cstdint>

namespace platform {

class ResetInfo final {
   public:
    enum class Reason : std::uint8_t {
        kUnknown = 0,
        kPin,
        kSoftware,
        kBrownout,
        kPowerOn,
        kWatchdog,
        kDebug,
        kSecurity,
        kLowPowerWake,
        kCpuLockup,
        kHardware,
        kUser,
        kTemperature,
        kBootloader,
        kFlash,
    };

    constexpr explicit ResetInfo(Reason reason = Reason::kUnknown, std::uint32_t flags = 0,
                                 bool available = false)
        : reason_(reason), flags_(flags), available_(available) {}

    [[nodiscard]] static ResetInfo Current();
    static void RequestColdReboot();

    [[nodiscard]] constexpr Reason reason() const {
        return reason_;
    }
    [[nodiscard]] constexpr std::uint32_t flags() const {
        return flags_;
    }
    [[nodiscard]] constexpr bool available() const {
        return available_;
    }
    [[nodiscard]] constexpr bool is_watchdog() const {
        return reason_ == Reason::kWatchdog;
    }
    [[nodiscard]] constexpr bool is_cpu_fault() const {
        return reason_ == Reason::kCpuLockup;
    }
    [[nodiscard]] StringView reason_text() const;

   private:
    Reason reason_{Reason::kUnknown};
    std::uint32_t flags_{0};
    bool available_{false};
};

}  // namespace platform

#endif  // PLATFORM_CORE_RESET_INFO_H_
