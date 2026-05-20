#include <platform/core/clock.h>
#include <zephyr/kernel.h>

namespace platform {

Clock::Milliseconds Clock::UptimeMilliseconds() {
    return k_uptime_get();
}

void Clock::SleepMilliseconds(std::int32_t duration_ms) {
    if (duration_ms <= 0) {
        return;
    }

    k_msleep(duration_ms);
}

}  // namespace platform
