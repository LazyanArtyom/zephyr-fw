#ifndef SERVICES_DIAGNOSTICS_DIAGNOSTICS_SERVICE_H_
#define SERVICES_DIAGNOSTICS_DIAGNOSTICS_SERVICE_H_

#include <platform/core/result.h>
#include <platform/core/status.h>
#include <platform/core/string_view.h>

#include <cstddef>
#include <cstdint>

namespace services::diagnostics {

inline constexpr std::size_t kCrashRecordTextLength = 32;

struct CrashRecord final {
    std::uint32_t boot_count{0};
    std::uint32_t panic_count{0};
    std::uint32_t reset_cause_flags{0};
    std::uint32_t last_fatal_reason{0};
    std::uint64_t last_fatal_uptime_ms{0};
    char reset_cause[kCrashRecordTextLength]{};
    char last_fatal_text[kCrashRecordTextLength]{};
    char last_fatal_thread[kCrashRecordTextLength]{};
    char firmware_version[kCrashRecordTextLength]{};
    char git_commit[kCrashRecordTextLength]{};
    char build_profile[16]{};
    char last_watchdog_reason[kCrashRecordTextLength]{};
};

class DiagnosticsService final {
   public:
    [[nodiscard]] static platform::Status Initialize();
    [[nodiscard]] static platform::Result<CrashRecord> ReadCrashRecord();
    [[nodiscard]] static platform::Status ClearCrashRecord();
    [[nodiscard]] static platform::Status RecordWatchdogBite(platform::StringView reason);

    static void CaptureFatal(unsigned int reason);
};

[[nodiscard]] platform::StringView FatalReasonText(std::uint32_t reason);

}  // namespace services::diagnostics

#endif  // SERVICES_DIAGNOSTICS_DIAGNOSTICS_SERVICE_H_
