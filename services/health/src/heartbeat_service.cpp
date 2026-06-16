#include <platform/core/clock.h>
#include <services/health/heartbeat_service.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(heartbeat_service, CONFIG_FW_SERVICE_HEALTH_HEARTBEAT_LOG_LEVEL);

namespace services::health {
namespace {

constexpr k_timeout_t kHeartbeatPeriod = K_SECONDS(5);

void HeartbeatHandler(k_work* work);

K_WORK_DELAYABLE_DEFINE(heartbeat_work, HeartbeatHandler);

void HeartbeatHandler(k_work* work) {
    ARG_UNUSED(work);

    LOG_INF("heartbeat: uptime=%lld ms",
            static_cast<long long>(platform::Clock::UptimeMilliseconds()));
    k_work_schedule(&heartbeat_work, kHeartbeatPeriod);
}

}  // namespace

void StartHeartbeatService() {
    LOG_INF("starting heartbeat service");
    k_work_schedule(&heartbeat_work, kHeartbeatPeriod);
}

}  // namespace services::health
