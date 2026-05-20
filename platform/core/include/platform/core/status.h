#ifndef PLATFORM_CORE_STATUS_H_
#define PLATFORM_CORE_STATUS_H_

#include <cstdint>

namespace platform {

enum class StatusCode : std::uint8_t {
    kOk = 0,
    kInvalidArgument,
    kNotFound,
    kUnavailable,
    kInternalError,
};

class Status {
   public:
    constexpr Status() = default;
    constexpr Status(StatusCode code, const char* message = "") : code_(code), message_(message) {}

    [[nodiscard]] static constexpr Status Ok() {
        return {};
    }

    [[nodiscard]] constexpr bool ok() const {
        return code_ == StatusCode::kOk;
    }

    [[nodiscard]] constexpr StatusCode code() const {
        return code_;
    }

    [[nodiscard]] constexpr const char* message() const {
        return message_;
    }

   private:
    StatusCode code_{StatusCode::kOk};
    const char* message_{""};
};

}  // namespace platform

#endif  // PLATFORM_CORE_STATUS_H_
