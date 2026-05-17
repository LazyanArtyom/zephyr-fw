#include "heartbeat_service.hpp"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(heartbeat_service, CONFIG_LOG_DEFAULT_LEVEL);

namespace app {
namespace {

constexpr k_timeout_t kHeartbeatPeriod = K_SECONDS(5);

void HeartbeatHandler(k_work* work);

K_WORK_DELAYABLE_DEFINE(heartbeat_work, HeartbeatHandler);

void HeartbeatHandler(k_work* work)
{
    ARG_UNUSED(work);

    LOG_INF("heartbeat: uptime=%lld ms", k_uptime_get());
    k_work_schedule(&heartbeat_work, kHeartbeatPeriod);
}

}  // namespace

void StartHeartbeatService()
{
    LOG_INF("starting heartbeat service");
    k_work_schedule(&heartbeat_work, kHeartbeatPeriod);
}

}  // namespace app
