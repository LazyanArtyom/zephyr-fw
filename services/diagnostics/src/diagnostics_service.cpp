#include <platform/board/board_info.h>
#include <platform/core/reset_info.h>
#include <platform/settings/settings_store.h>
#include <services/diagnostics/diagnostics_service.h>
#include <zephyr/fatal.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>

#include <cstddef>
#include <cstdint>

LOG_MODULE_REGISTER(diagnostics_service, CONFIG_FW_SERVICE_DIAGNOSTICS_LOG_LEVEL);

namespace {

constexpr platform::StringView kCrashRecordKey("fw/diagnostics/crash-record");
constexpr std::uint32_t kCrashRecordMagic = 0x44524743U;  // DRGC
constexpr std::uint16_t kCrashRecordVersion = 1;
constexpr std::uint32_t kFatalMarkerMagic = 0x4654414cU;  // FTAL
constexpr std::uint32_t kNoFatalReason = 0xffffffffU;

struct StoredCrashRecord final {
    std::uint32_t magic{kCrashRecordMagic};
    std::uint16_t version{kCrashRecordVersion};
    std::uint16_t reserved{0};
    services::diagnostics::CrashRecord record{};
};

struct FatalMarker final {
    std::uint32_t magic{0};
    std::uint32_t reason{0};
    std::uint64_t uptime_ms{0};
    char thread[services::diagnostics::kCrashRecordTextLength]{};
    std::uint32_t checksum{0};
};

static_assert(sizeof(StoredCrashRecord) <= platform::kSettingsMaxValueLength,
              "diagnostics crash record must fit in one settings value");

FatalMarker g_fatal_marker __noinit;

void CopyText(char* destination, std::size_t destination_size, platform::StringView source) {
    if (destination == nullptr || destination_size == 0) {
        return;
    }

    std::size_t index = 0;
    for (; index + 1 < destination_size && index < source.size(); ++index) {
        destination[index] = source[index];
    }
    destination[index] = '\0';
}

std::uint32_t FatalMarkerChecksum(const FatalMarker& marker) {
    std::uint32_t checksum = marker.magic ^ marker.reason ^
                             static_cast<std::uint32_t>(marker.uptime_ms) ^
                             static_cast<std::uint32_t>(marker.uptime_ms >> 32U);
    for (char value : marker.thread) {
        checksum = (checksum << 5U) ^ (checksum >> 2U) ^ static_cast<std::uint8_t>(value);
    }
    return checksum;
}

bool IsValidFatalMarker(const FatalMarker& marker) {
    return marker.magic == kFatalMarkerMagic && marker.checksum == FatalMarkerChecksum(marker);
}

void ClearFatalMarker() {
    g_fatal_marker.magic = 0;
    g_fatal_marker.reason = 0;
    g_fatal_marker.uptime_ms = 0;
    g_fatal_marker.thread[0] = '\0';
    g_fatal_marker.checksum = 0;
}

platform::Status LoadStoredRecord(StoredCrashRecord* stored) {
    if (stored == nullptr) {
        return platform::Status::InvalidArgument("missing diagnostics record buffer");
    }

    std::size_t bytes_read = 0;
    const platform::Status read_status =
        platform::SettingsStore::ReadRaw(kCrashRecordKey, stored, sizeof(*stored), &bytes_read);
    if (read_status.ok() && bytes_read == sizeof(*stored) && stored->magic == kCrashRecordMagic &&
        stored->version == kCrashRecordVersion) {
        return platform::Status::Ok();
    }
    if (read_status.ok() || read_status.code() == platform::StatusCode::kNotFound) {
        *stored = StoredCrashRecord{};
        return platform::Status::Ok();
    }

    return read_status;
}

platform::Status SaveStoredRecord(const StoredCrashRecord& stored) {
    return platform::SettingsStore::WriteRaw(kCrashRecordKey, &stored, sizeof(stored));
}

void RefreshBootMetadata(services::diagnostics::CrashRecord* record) {
    if (record == nullptr) {
        return;
    }

    const platform::BoardInfo& board_info = platform::BoardInfo::Current();
    const platform::ResetInfo reset_info = platform::ResetInfo::Current();

    record->reset_cause_flags = reset_info.flags();
    CopyText(record->reset_cause, sizeof(record->reset_cause), reset_info.reason_text());
    CopyText(record->firmware_version, sizeof(record->firmware_version),
             board_info.firmware_version());
    CopyText(record->git_commit, sizeof(record->git_commit), board_info.git_commit());
    CopyText(record->build_profile, sizeof(record->build_profile), board_info.build_profile());
}

}  // namespace

namespace services::diagnostics {

platform::StringView FatalReasonText(std::uint32_t reason) {
    switch (reason) {
        case K_ERR_CPU_EXCEPTION:
            return "cpu-exception";
        case K_ERR_SPURIOUS_IRQ:
            return "spurious-irq";
        case K_ERR_STACK_CHK_FAIL:
            return "stack-check-fail";
        case K_ERR_KERNEL_OOPS:
            return "kernel-oops";
        case K_ERR_KERNEL_PANIC:
            return "kernel-panic";
        case kNoFatalReason:
            return "none";
        default:
            return "unknown";
    }
}

platform::Status DiagnosticsService::Initialize() {
    StoredCrashRecord stored{};
    const platform::Status load_status = LoadStoredRecord(&stored);
    if (!load_status.ok()) {
        return load_status;
    }

    ++stored.record.boot_count;
    RefreshBootMetadata(&stored.record);

    if (IsValidFatalMarker(g_fatal_marker)) {
        stored.record.last_fatal_reason = g_fatal_marker.reason;
        stored.record.last_fatal_uptime_ms = g_fatal_marker.uptime_ms;
        CopyText(stored.record.last_fatal_text, sizeof(stored.record.last_fatal_text),
                 FatalReasonText(g_fatal_marker.reason));
        CopyText(stored.record.last_fatal_thread, sizeof(stored.record.last_fatal_thread),
                 platform::StringView(g_fatal_marker.thread));
        ++stored.record.panic_count;
        ClearFatalMarker();
    } else if (stored.record.last_fatal_text[0] == '\0') {
        stored.record.last_fatal_reason = kNoFatalReason;
        CopyText(stored.record.last_fatal_text, sizeof(stored.record.last_fatal_text), "none");
    }

    if (stored.record.last_watchdog_reason[0] == '\0') {
        CopyText(stored.record.last_watchdog_reason, sizeof(stored.record.last_watchdog_reason),
                 "none");
    }

    const platform::Status save_status = SaveStoredRecord(stored);
    if (!save_status.ok()) {
        return save_status;
    }

    LOG_INF("Diagnostics record updated: boot=%lu reset=%s panic=%lu",
            static_cast<unsigned long>(stored.record.boot_count), stored.record.reset_cause,
            static_cast<unsigned long>(stored.record.panic_count));
    return platform::Status::Ok();
}

platform::Result<CrashRecord> DiagnosticsService::ReadCrashRecord() {
    StoredCrashRecord stored{};
    const platform::Status status = LoadStoredRecord(&stored);
    if (!status.ok()) {
        return platform::Result<CrashRecord>::FromStatus(status);
    }
    return platform::Result<CrashRecord>::FromValue(stored.record);
}

platform::Status DiagnosticsService::ClearCrashRecord() {
    StoredCrashRecord stored{};
    const platform::Status load_status = LoadStoredRecord(&stored);
    if (!load_status.ok()) {
        return load_status;
    }

    const std::uint32_t boot_count = stored.record.boot_count;
    stored = StoredCrashRecord{};
    stored.record.boot_count = boot_count;
    stored.record.last_fatal_reason = kNoFatalReason;
    RefreshBootMetadata(&stored.record);
    CopyText(stored.record.last_fatal_text, sizeof(stored.record.last_fatal_text), "none");
    CopyText(stored.record.last_watchdog_reason, sizeof(stored.record.last_watchdog_reason),
             "none");
    ClearFatalMarker();

    return SaveStoredRecord(stored);
}

platform::Status DiagnosticsService::RecordWatchdogBite(platform::StringView reason) {
    StoredCrashRecord stored{};
    const platform::Status load_status = LoadStoredRecord(&stored);
    if (!load_status.ok()) {
        return load_status;
    }

    CopyText(stored.record.last_watchdog_reason, sizeof(stored.record.last_watchdog_reason),
             reason.empty() ? platform::StringView("watchdog") : reason);
    return SaveStoredRecord(stored);
}

void DiagnosticsService::CaptureFatal(unsigned int reason) {
    g_fatal_marker.magic = kFatalMarkerMagic;
    g_fatal_marker.reason = reason;
    g_fatal_marker.uptime_ms = static_cast<std::uint64_t>(k_uptime_get());

#if defined(CONFIG_THREAD_NAME) && defined(CONFIG_MULTITHREADING)
    const char* thread_name = k_thread_name_get(k_current_get());
    CopyText(g_fatal_marker.thread, sizeof(g_fatal_marker.thread),
             platform::StringView(thread_name == nullptr || thread_name[0] == '\0' ? "unknown"
                                                                                   : thread_name));
#else
    CopyText(g_fatal_marker.thread, sizeof(g_fatal_marker.thread), "unknown");
#endif

    g_fatal_marker.checksum = FatalMarkerChecksum(g_fatal_marker);
}

}  // namespace services::diagnostics

extern "C" void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf* esf) {
    ARG_UNUSED(esf);

    services::diagnostics::DiagnosticsService::CaptureFatal(reason);
    LOG_PANIC();

#if defined(CONFIG_FW_SERVICE_DIAGNOSTICS_REBOOT_ON_FATAL)
    sys_reboot(SYS_REBOOT_COLD);
#else
    k_fatal_halt(reason);
#endif

    CODE_UNREACHABLE;
}
