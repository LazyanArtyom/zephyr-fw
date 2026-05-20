#ifndef PLATFORM_CORE_STATUS_H_
#define PLATFORM_CORE_STATUS_H_

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
    constexpr Status(StatusCode code, const char* message = "") : code_(code), message_(message) {}

    [[nodiscard]] static constexpr Status Ok() {
        return {};
    }
    [[nodiscard]] static constexpr Status InvalidArgument(const char* message = "") {
        return {StatusCode::kInvalidArgument, message};
    }
    [[nodiscard]] static constexpr Status NotFound(const char* message = "") {
        return {StatusCode::kNotFound, message};
    }
    [[nodiscard]] static constexpr Status AlreadyExists(const char* message = "") {
        return {StatusCode::kAlreadyExists, message};
    }
    [[nodiscard]] static constexpr Status PermissionDenied(const char* message = "") {
        return {StatusCode::kPermissionDenied, message};
    }
    [[nodiscard]] static constexpr Status FailedPrecondition(const char* message = "") {
        return {StatusCode::kFailedPrecondition, message};
    }
    [[nodiscard]] static constexpr Status Busy(const char* message = "") {
        return {StatusCode::kBusy, message};
    }
    [[nodiscard]] static constexpr Status Timeout(const char* message = "") {
        return {StatusCode::kTimeout, message};
    }
    [[nodiscard]] static constexpr Status NotSupported(const char* message = "") {
        return {StatusCode::kNotSupported, message};
    }
    [[nodiscard]] static constexpr Status Unavailable(const char* message = "") {
        return {StatusCode::kUnavailable, message};
    }
    [[nodiscard]] static constexpr Status InternalError(const char* message = "") {
        return {StatusCode::kInternalError, message};
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

[[nodiscard]] constexpr const char* ToString(StatusCode code) {
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
