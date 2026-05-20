#ifndef PLATFORM_CORE_CLOCK_H_
#define PLATFORM_CORE_CLOCK_H_

#include <cstdint>

namespace platform {

class Clock final {
   public:
    using Milliseconds = std::int64_t;

    Clock() = delete;

    [[nodiscard]] static Milliseconds UptimeMilliseconds();
    static void SleepMilliseconds(std::int32_t duration_ms);
};

}  // namespace platform

#endif  // PLATFORM_CORE_CLOCK_H_
