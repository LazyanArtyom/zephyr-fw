#include <services/health/heartbeat_service.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(heartbeat_service, CONFIG_LOG_DEFAULT_LEVEL);

namespace services::health {
namespace {

constexpr k_timeout_t kHeartbeatPeriod = K_SECONDS(5);

void HeartbeatHandler(k_work* work);

K_WORK_DELAYABLE_DEFINE(heartbeat_work, HeartbeatHandler);

void HeartbeatHandler(k_work* work) {
    ARG_UNUSED(work);

    LOG_INF("heartbeat: uptime=%lld ms", k_uptime_get());
    k_work_schedule(&heartbeat_work, kHeartbeatPeriod);
}

}  // namespace

void StartHeartbeatService() {
    LOG_INF("starting heartbeat service");
    k_work_schedule(&heartbeat_work, kHeartbeatPeriod);
}

}  // namespace services::health
