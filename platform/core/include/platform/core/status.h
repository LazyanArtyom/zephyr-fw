#ifndef PLATFORM_CORE_STATUS_H_
#define PLATFORM_CORE_STATUS_H_

#include <platform/core/string_view.h>

#include <cstdint>

namespace platform {

enum class StatusCode : std::uint8_t {
    kOk = 0,
    kInvalidArgument,
    kNotFound,
    kAlreadyExists,
    kPermissionDenied,
    kFailedPrecondition,
    kBusy,
    kTimeout,
    kNotSupported,
    kUnavailable,
    kInternalError,
};

class Status {
   public:
    constexpr Status() = default;
    constexpr Status(StatusCode code, StringView message = {}) : code_(code), message_(message) {}

    [[nodiscard]] static constexpr Status Ok() {
        return {};
    }
    [[nodiscard]] static constexpr Status InvalidArgument(StringView message = {}) {
        return {StatusCode::kInvalidArgument, message};
    }
    [[nodiscard]] static constexpr Status NotFound(StringView message = {}) {
        return {StatusCode::kNotFound, message};
    }
    [[nodiscard]] static constexpr Status AlreadyExists(StringView message = {}) {
        return {StatusCode::kAlreadyExists, message};
    }
    [[nodiscard]] static constexpr Status PermissionDenied(StringView message = {}) {
        return {StatusCode::kPermissionDenied, message};
    }
    [[nodiscard]] static constexpr Status FailedPrecondition(StringView message = {}) {
        return {StatusCode::kFailedPrecondition, message};
    }
    [[nodiscard]] static constexpr Status Busy(StringView message = {}) {
        return {StatusCode::kBusy, message};
    }
    [[nodiscard]] static constexpr Status Timeout(StringView message = {}) {
        return {StatusCode::kTimeout, message};
    }
    [[nodiscard]] static constexpr Status NotSupported(StringView message = {}) {
        return {StatusCode::kNotSupported, message};
    }
    [[nodiscard]] static constexpr Status Unavailable(StringView message = {}) {
        return {StatusCode::kUnavailable, message};
    }
    [[nodiscard]] static constexpr Status InternalError(StringView message = {}) {
        return {StatusCode::kInternalError, message};
    }

    [[nodiscard]] constexpr bool ok() const {
        return code_ == StatusCode::kOk;
    }
    [[nodiscard]] constexpr StatusCode code() const {
        return code_;
    }
    [[nodiscard]] constexpr StringView message() const {
        return message_;
    }

   private:
    StatusCode code_{StatusCode::kOk};
    StringView message_{};
};

[[nodiscard]] constexpr StringView ToString(StatusCode code) {
    switch (code) {
        case StatusCode::kOk:
            return "ok";
        case StatusCode::kInvalidArgument:
            return "invalid_argument";
        case StatusCode::kNotFound:
            return "not_found";
        case StatusCode::kAlreadyExists:
            return "already_exists";
        case StatusCode::kPermissionDenied:
            return "permission_denied";
        case StatusCode::kFailedPrecondition:
            return "failed_precondition";
        case StatusCode::kBusy:
            return "busy";
        case StatusCode::kTimeout:
            return "timeout";
        case StatusCode::kNotSupported:
            return "not_supported";
        case StatusCode::kUnavailable:
            return "unavailable";
        case StatusCode::kInternalError:
            return "internal_error";
    }

    return "unknown";
}

}  // namespace platform

#endif  // PLATFORM_CORE_STATUS_H_
