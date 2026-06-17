#include <errno.h>
#include <platform/core/clock.h>
#include <platform/core/fixed_string.h>
#include <platform/core/reset_info.h>
#include <services/watchdog/watchdog_service.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_FW_SERVICE_WATCHDOG) && defined(CONFIG_WATCHDOG)
#include <zephyr/drivers/watchdog.h>
#endif

#if defined(CONFIG_FW_SERVICE_DIAGNOSTICS)
#include <services/diagnostics/diagnostics_service.h>
#endif

#include <cstddef>
#include <cstdint>

LOG_MODULE_REGISTER(watchdog_service, CONFIG_FW_SERVICE_WATCHDOG_LOG_LEVEL);

namespace services::watchdog {
namespace {

constexpr platform::StringView kDisabledMode("disabled");
constexpr platform::StringView kMissingAliasMode("missing-alias");
constexpr platform::StringView kHardwareMode("hardware");

struct SupervisedTaskSlot final {
    bool active{false};
    platform::FixedString<32> name{};
    std::uint32_t timeout_ms{0};
    std::uint64_t last_feed_uptime_ms{0};
};

#if defined(CONFIG_FW_SERVICE_WATCHDOG)
constexpr std::size_t kMaxSupervisedTasks = CONFIG_FW_SERVICE_WATCHDOG_MAX_SUPERVISED_TASKS;
#else
constexpr std::size_t kMaxSupervisedTasks = 0;
#endif

struct WatchdogState final {
    const struct device* device{nullptr};
    bool initialized{false};
    bool last_reset_watchdog{false};
    int channel_id{-1};
    int last_error{0};
    std::uint32_t feed_count{0};
    std::uint64_t last_feed_uptime_ms{0};
    SupervisedTaskSlot supervised_tasks[kMaxSupervisedTasks == 0 ? 1 : kMaxSupervisedTasks]{};
};

K_MUTEX_DEFINE(g_watchdog_lock);
WatchdogState g_state{};

void FeedWorkHandler(k_work* work);
K_WORK_DELAYABLE_DEFINE(g_feed_work, FeedWorkHandler);

platform::Status StatusFromErrno(int rc, platform::StringView message) {
    switch (-rc) {
        case 0:
            return platform::Status::Ok();
        case EINVAL:
            return platform::Status::InvalidArgument(message);
        case ENODEV:
            return platform::Status::Unavailable(message);
        case ENOTSUP:
            return platform::Status::NotSupported(message);
        case EBUSY:
        case EAGAIN:
            return platform::Status::Busy(message);
        case ETIMEDOUT:
            return platform::Status::Timeout(message);
        default:
            return platform::Status::InternalError(message);
    }
}

std::uint32_t ConfiguredTimeoutMs() {
#if defined(CONFIG_FW_SERVICE_WATCHDOG)
    return CONFIG_FW_SERVICE_WATCHDOG_TIMEOUT_MS;
#else
    return 0;
#endif
}

std::uint32_t ConfiguredFeedIntervalMs() {
#if defined(CONFIG_FW_SERVICE_WATCHDOG)
    return CONFIG_FW_SERVICE_WATCHDOG_FEED_INTERVAL_MS;
#else
    return 0;
#endif
}

bool TaskHealthy(const SupervisedTaskSlot& task, std::uint64_t now_ms) {
    if (!task.active) {
        return true;
    }
    return now_ms - task.last_feed_uptime_ms <= task.timeout_ms;
}

const SupervisedTaskSlot* FirstUnhealthyTask(std::uint64_t now_ms) {
    for (const SupervisedTaskSlot& task : g_state.supervised_tasks) {
        if (!TaskHealthy(task, now_ms)) {
            return &task;
        }
    }
    return nullptr;
}

std::uint32_t ActiveTaskCount() {
    std::uint32_t count = 0;
    for (const SupervisedTaskSlot& task : g_state.supervised_tasks) {
        if (task.active) {
            ++count;
        }
    }
    return count;
}

void ScheduleNextFeed() {
#if defined(CONFIG_FW_SERVICE_WATCHDOG)
    k_work_schedule(&g_feed_work, K_MSEC(CONFIG_FW_SERVICE_WATCHDOG_FEED_INTERVAL_MS));
#endif
}

void RecordWatchdogReset() {
#if defined(CONFIG_FW_SERVICE_DIAGNOSTICS)
    const platform::Status status =
        services::diagnostics::DiagnosticsService::RecordWatchdogBite("hardware-watchdog-reset");
    if (!status.ok()) {
        LOG_WRN("watchdog reset marker failed: %s", status.message().c_str());
    }
#endif
}

void RecordSupervisionTimeout(const SupervisedTaskSlot& task) {
#if defined(CONFIG_FW_SERVICE_DIAGNOSTICS)
    const platform::Status status = services::diagnostics::DiagnosticsService::RecordWatchdogBite(
        task.name.empty() ? platform::StringView("supervision-timeout") : task.name.view());
    if (!status.ok()) {
        LOG_WRN("watchdog supervision marker failed: %s", status.message().c_str());
    }
#else
    ARG_UNUSED(task);
#endif
}

#if defined(CONFIG_FW_SERVICE_WATCHDOG) && defined(CONFIG_WATCHDOG) && \
    DT_NODE_HAS_STATUS(DT_ALIAS(watchdog0), okay)
const struct device* WatchdogDevice() {
    return DEVICE_DT_GET(DT_ALIAS(watchdog0));
}
#endif

void FeedWorkHandler(k_work* work) {
    ARG_UNUSED(work);

    const platform::Status status = WatchdogService::Feed();
    if (!status.ok()) {
        LOG_ERR("watchdog feed failed: %s", status.message().c_str());
    }

    if (WatchdogService::Status().initialized) {
        ScheduleNextFeed();
    }
}

}  // namespace

platform::Status WatchdogService::Initialize() {
#if !defined(CONFIG_FW_SERVICE_WATCHDOG)
    return platform::Status::NotSupported("watchdog service disabled");
#elif !defined(CONFIG_WATCHDOG)
    return platform::Status::NotSupported("Zephyr watchdog disabled");
#elif !DT_NODE_HAS_STATUS(DT_ALIAS(watchdog0), okay)
    return platform::Status::Unavailable("watchdog0 alias not available");
#else
    k_mutex_lock(&g_watchdog_lock, K_FOREVER);
    if (g_state.initialized) {
        k_mutex_unlock(&g_watchdog_lock);
        return platform::Status::Ok();
    }

    const struct device* device = WatchdogDevice();
    g_state.device = device;
    g_state.last_reset_watchdog = platform::ResetInfo::Current().is_watchdog();

    if (!device_is_ready(device)) {
        g_state.last_error = -ENODEV;
        k_mutex_unlock(&g_watchdog_lock);
        return platform::Status::Unavailable("watchdog device not ready");
    }

    if (g_state.last_reset_watchdog) {
        RecordWatchdogReset();
    }

    wdt_timeout_cfg config{};
    config.window.min = 0;
    config.window.max = CONFIG_FW_SERVICE_WATCHDOG_TIMEOUT_MS;
    config.callback = nullptr;
    config.flags = WDT_FLAG_RESET_SOC;

    const int channel_id = wdt_install_timeout(device, &config);
    if (channel_id < 0) {
        g_state.last_error = channel_id;
        k_mutex_unlock(&g_watchdog_lock);
        return StatusFromErrno(channel_id, "watchdog timeout install failed");
    }

    const int setup_rc = wdt_setup(device, 0);
    if (setup_rc != 0) {
        g_state.last_error = setup_rc;
        k_mutex_unlock(&g_watchdog_lock);
        return StatusFromErrno(setup_rc, "watchdog setup failed");
    }

    g_state.channel_id = channel_id;
    g_state.initialized = true;
    g_state.last_error = 0;
    k_mutex_unlock(&g_watchdog_lock);

    LOG_INF("watchdog initialized: device=%s channel=%d timeout=%u ms feed=%u ms", device->name,
            channel_id, CONFIG_FW_SERVICE_WATCHDOG_TIMEOUT_MS,
            CONFIG_FW_SERVICE_WATCHDOG_FEED_INTERVAL_MS);
    ScheduleNextFeed();
    return platform::Status::Ok();
#endif
}

platform::Status WatchdogService::Feed() {
#if !defined(CONFIG_FW_SERVICE_WATCHDOG) || !defined(CONFIG_WATCHDOG) || \
    !DT_NODE_HAS_STATUS(DT_ALIAS(watchdog0), okay)
    return platform::Status::NotSupported("watchdog feed unavailable");
#else
    k_mutex_lock(&g_watchdog_lock, K_FOREVER);
    if (!g_state.initialized || g_state.device == nullptr || g_state.channel_id < 0) {
        k_mutex_unlock(&g_watchdog_lock);
        return platform::Status::FailedPrecondition("watchdog not initialized");
    }

    const std::uint64_t now_ms = platform::Clock::UptimeMilliseconds();
    const SupervisedTaskSlot* unhealthy_task = FirstUnhealthyTask(now_ms);
    if (unhealthy_task != nullptr) {
        g_state.last_error = -ETIMEDOUT;
        RecordSupervisionTimeout(*unhealthy_task);
        k_mutex_unlock(&g_watchdog_lock);
        return platform::Status::Timeout("watchdog supervised task expired");
    }

    const int rc = wdt_feed(g_state.device, g_state.channel_id);
    if (rc != 0) {
        g_state.last_error = rc;
        k_mutex_unlock(&g_watchdog_lock);
        return StatusFromErrno(rc, "watchdog feed failed");
    }

    ++g_state.feed_count;
    g_state.last_feed_uptime_ms = now_ms;
    g_state.last_error = 0;
    k_mutex_unlock(&g_watchdog_lock);
    return platform::Status::Ok();
#endif
}

platform::Result<int> WatchdogService::RegisterTask(platform::StringView name,
                                                    std::uint32_t timeout_ms) {
#if !defined(CONFIG_FW_SERVICE_WATCHDOG)
    ARG_UNUSED(name);
    ARG_UNUSED(timeout_ms);
    return platform::Result<int>::FromStatus(
        platform::Status::NotSupported("watchdog service disabled"));
#else
    if (kMaxSupervisedTasks == 0) {
        return platform::Result<int>::FromStatus(
            platform::Status::NotSupported("watchdog supervision disabled"));
    }
    if (name.empty() || timeout_ms == 0) {
        return platform::Result<int>::FromStatus(
            platform::Status::InvalidArgument("invalid watchdog task"));
    }

    k_mutex_lock(&g_watchdog_lock, K_FOREVER);
    for (std::size_t index = 0; index < kMaxSupervisedTasks; ++index) {
        SupervisedTaskSlot& task = g_state.supervised_tasks[index];
        if (task.active) {
            continue;
        }
        task.name.clear();
        if (!task.name.append(name)) {
            k_mutex_unlock(&g_watchdog_lock);
            return platform::Result<int>::FromStatus(
                platform::Status::InvalidArgument("watchdog task name too long"));
        }
        task.timeout_ms = timeout_ms;
        task.last_feed_uptime_ms = platform::Clock::UptimeMilliseconds();
        task.active = true;
        k_mutex_unlock(&g_watchdog_lock);
        return platform::Result<int>::FromValue(static_cast<int>(index));
    }

    k_mutex_unlock(&g_watchdog_lock);
    return platform::Result<int>::FromStatus(
        platform::Status::FailedPrecondition("no watchdog supervision slots available"));
#endif
}

platform::Status WatchdogService::FeedTask(int task_id) {
#if !defined(CONFIG_FW_SERVICE_WATCHDOG)
    ARG_UNUSED(task_id);
    return platform::Status::NotSupported("watchdog service disabled");
#else
    if (task_id < 0 || static_cast<std::size_t>(task_id) >= kMaxSupervisedTasks) {
        return platform::Status::InvalidArgument("invalid watchdog task id");
    }

    k_mutex_lock(&g_watchdog_lock, K_FOREVER);
    SupervisedTaskSlot& task = g_state.supervised_tasks[task_id];
    if (!task.active) {
        k_mutex_unlock(&g_watchdog_lock);
        return platform::Status::NotFound("watchdog task not registered");
    }
    task.last_feed_uptime_ms = platform::Clock::UptimeMilliseconds();
    k_mutex_unlock(&g_watchdog_lock);
    return platform::Status::Ok();
#endif
}

WatchdogStatus WatchdogService::Status() {
    WatchdogStatus status{};
#if defined(CONFIG_FW_SERVICE_WATCHDOG)
    status.configured = true;
    status.timeout_ms = ConfiguredTimeoutMs();
    status.feed_interval_ms = ConfiguredFeedIntervalMs();
    status.supervision_available = kMaxSupervisedTasks > 0;
#else
    status.configured = false;
    status.device_name = "disabled";
    status.mode = kDisabledMode;
    return status;
#endif

#if defined(CONFIG_FW_SERVICE_WATCHDOG) && defined(CONFIG_WATCHDOG) && \
    DT_NODE_HAS_STATUS(DT_ALIAS(watchdog0), okay)
    const struct device* device = WatchdogDevice();
    status.device_ready = device_is_ready(device);
    status.device_name = device->name;
    status.mode = kHardwareMode;

    k_mutex_lock(&g_watchdog_lock, K_FOREVER);
    status.initialized = g_state.initialized;
    status.last_reset_watchdog = g_state.last_reset_watchdog;
    status.channel_id = g_state.channel_id;
    status.last_error = g_state.last_error;
    status.feed_count = g_state.feed_count;
    status.last_feed_uptime_ms = g_state.last_feed_uptime_ms;
    status.supervised_task_count = ActiveTaskCount();
    const SupervisedTaskSlot* unhealthy_task =
        FirstUnhealthyTask(platform::Clock::UptimeMilliseconds());
    status.supervision_healthy = unhealthy_task == nullptr;
    status.unhealthy_task =
        unhealthy_task == nullptr ? platform::StringView{} : unhealthy_task->name.view();
    k_mutex_unlock(&g_watchdog_lock);
#elif defined(CONFIG_FW_SERVICE_WATCHDOG)
    status.device_ready = false;
    status.device_name = "watchdog0";
    status.mode = kMissingAliasMode;
#endif

    return status;
}

platform::Result<SupervisedTaskStatus> WatchdogService::SupervisedTask(std::size_t index) {
#if !defined(CONFIG_FW_SERVICE_WATCHDOG)
    ARG_UNUSED(index);
    return platform::Result<SupervisedTaskStatus>::FromStatus(
        platform::Status::NotSupported("watchdog service disabled"));
#else
    if (index >= kMaxSupervisedTasks) {
        return platform::Result<SupervisedTaskStatus>::FromStatus(
            platform::Status::NotFound("watchdog task not found"));
    }

    k_mutex_lock(&g_watchdog_lock, K_FOREVER);
    const SupervisedTaskSlot& task = g_state.supervised_tasks[index];
    SupervisedTaskStatus status{};
    status.active = task.active;
    status.name = task.name.view();
    status.timeout_ms = task.timeout_ms;
    status.last_feed_uptime_ms = task.last_feed_uptime_ms;
    status.healthy = TaskHealthy(task, platform::Clock::UptimeMilliseconds());
    k_mutex_unlock(&g_watchdog_lock);
    return platform::Result<SupervisedTaskStatus>::FromValue(status);
#endif
}

}  // namespace services::watchdog
