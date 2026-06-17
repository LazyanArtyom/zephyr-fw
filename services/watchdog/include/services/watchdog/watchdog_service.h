#ifndef SERVICES_WATCHDOG_WATCHDOG_SERVICE_H_
#define SERVICES_WATCHDOG_WATCHDOG_SERVICE_H_

#include <platform/core/result.h>
#include <platform/core/status.h>
#include <platform/core/string_view.h>

#include <cstddef>
#include <cstdint>

namespace services::watchdog {

struct WatchdogStatus {
    bool configured{false};
    bool device_ready{false};
    bool initialized{false};
    bool last_reset_watchdog{false};
    bool supervision_available{false};
    bool supervision_healthy{true};
    platform::StringView device_name;
    platform::StringView mode;
    platform::StringView unhealthy_task;
    int channel_id{-1};
    int last_error{0};
    std::uint32_t timeout_ms{0};
    std::uint32_t feed_interval_ms{0};
    std::uint32_t feed_count{0};
    std::uint32_t supervised_task_count{0};
    std::uint64_t last_feed_uptime_ms{0};
};

struct SupervisedTaskStatus {
    bool active{false};
    bool healthy{true};
    platform::StringView name;
    std::uint32_t timeout_ms{0};
    std::uint64_t last_feed_uptime_ms{0};
};

class WatchdogService final {
   public:
    [[nodiscard]] static platform::Status Initialize();
    [[nodiscard]] static platform::Status Feed();
    [[nodiscard]] static platform::Result<int> RegisterTask(platform::StringView name,
                                                            std::uint32_t timeout_ms);
    [[nodiscard]] static platform::Status FeedTask(int task_id);
    [[nodiscard]] static WatchdogStatus Status();
    [[nodiscard]] static platform::Result<SupervisedTaskStatus> SupervisedTask(std::size_t index);
};

}  // namespace services::watchdog

#endif  // SERVICES_WATCHDOG_WATCHDOG_SERVICE_H_
