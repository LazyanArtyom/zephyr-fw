#include <platform/core/reset_info.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/reboot.h>

namespace platform {

namespace {

ResetInfo::Reason ReasonFromFlags(std::uint32_t flags) {
    if ((flags & RESET_WATCHDOG) != 0U) {
        return ResetInfo::Reason::kWatchdog;
    }
    if ((flags & RESET_CPU_LOCKUP) != 0U) {
        return ResetInfo::Reason::kCpuLockup;
    }
    if ((flags & RESET_SOFTWARE) != 0U) {
        return ResetInfo::Reason::kSoftware;
    }
    if ((flags & RESET_BROWNOUT) != 0U) {
        return ResetInfo::Reason::kBrownout;
    }
    if ((flags & RESET_POR) != 0U) {
        return ResetInfo::Reason::kPowerOn;
    }
    if ((flags & RESET_PIN) != 0U) {
        return ResetInfo::Reason::kPin;
    }
    if ((flags & RESET_HARDWARE) != 0U) {
        return ResetInfo::Reason::kHardware;
    }
    if ((flags & RESET_USER) != 0U) {
        return ResetInfo::Reason::kUser;
    }
    if ((flags & RESET_DEBUG) != 0U) {
        return ResetInfo::Reason::kDebug;
    }
    if ((flags & RESET_SECURITY) != 0U) {
        return ResetInfo::Reason::kSecurity;
    }
    if ((flags & RESET_LOW_POWER_WAKE) != 0U) {
        return ResetInfo::Reason::kLowPowerWake;
    }
    if ((flags & RESET_TEMPERATURE) != 0U) {
        return ResetInfo::Reason::kTemperature;
    }
    if ((flags & RESET_BOOTLOADER) != 0U) {
        return ResetInfo::Reason::kBootloader;
    }
    if ((flags & RESET_FLASH) != 0U) {
        return ResetInfo::Reason::kFlash;
    }
    return ResetInfo::Reason::kUnknown;
}

ResetInfo ReadCurrentResetInfo() {
#if defined(CONFIG_HWINFO)
    std::uint32_t flags = 0;
    const int rc = hwinfo_get_reset_cause(&flags);
    if (rc == 0) {
        return ResetInfo(ReasonFromFlags(flags), flags, true);
    }
#endif

    return ResetInfo{};
}

ResetInfo g_cached_reset_info{};
bool g_cached_reset_info_loaded = false;

}  // namespace

ResetInfo ResetInfo::Current() {
    if (!g_cached_reset_info_loaded) {
        g_cached_reset_info = ReadCurrentResetInfo();
        g_cached_reset_info_loaded = true;
    }
    return g_cached_reset_info;
}

void ResetInfo::RequestColdReboot() {
    sys_reboot(SYS_REBOOT_COLD);
}

StringView ResetInfo::reason_text() const {
    switch (reason_) {
        case Reason::kPin:
            return "pin";
        case Reason::kSoftware:
            return "software";
        case Reason::kBrownout:
            return "brownout";
        case Reason::kPowerOn:
            return "power-on";
        case Reason::kWatchdog:
            return "watchdog";
        case Reason::kDebug:
            return "debug";
        case Reason::kSecurity:
            return "security";
        case Reason::kLowPowerWake:
            return "low-power-wake";
        case Reason::kCpuLockup:
            return "cpu-lockup";
        case Reason::kHardware:
            return "hardware";
        case Reason::kUser:
            return "user";
        case Reason::kTemperature:
            return "temperature";
        case Reason::kBootloader:
            return "bootloader";
        case Reason::kFlash:
            return "flash";
        case Reason::kUnknown:
            return "unknown";
    }

    return "unknown";
}

}  // namespace platform
