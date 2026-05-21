#ifndef PLATFORM_CORE_RESULT_H_
#define PLATFORM_CORE_RESULT_H_

#include <platform/core/status.h>

#include <cstdint>

namespace platform {

template <typename T>
class Result {
   public:
    Result(const T& value) : value_(value), status_(Status::Ok()), has_value_(true) {}
    explicit Result(Status status) : status_(status), has_value_(false) {}

    [[nodiscard]] static Result<T> FromValue(const T& value) {
        return Result<T>(value);
    }
    [[nodiscard]] static Result<T> FromStatus(Status status) {
        return Result<T>(status);
    }

    [[nodiscard]] constexpr bool ok() const {
        return has_value_ && status_.ok();
    }
    [[nodiscard]] constexpr bool has_value() const {
        return has_value_;
    }
    [[nodiscard]] constexpr const Status& status() const {
        return status_;
    }

    [[nodiscard]] const T& value() const {
        return value_;
    }
    [[nodiscard]] T& value() {
        return value_;
    }
    [[nodiscard]] const T* value_if_ok() const {
        return ok() ? &value_ : nullptr;
    }
    [[nodiscard]] T* value_if_ok() {
        return ok() ? &value_ : nullptr;
    }

   private:
    T value_{};
    Status status_{Status::InternalError("result has no value")};
    bool has_value_{false};
};

template <>
class Result<void> {
   public:
    constexpr Result() = default;
    explicit constexpr Result(Status status) : status_(status) {}

    [[nodiscard]] static constexpr Result<void> Ok() {
        return {};
    }
    [[nodiscard]] static constexpr Result<void> FromStatus(Status status) {
        return Result<void>(status);
    }

    [[nodiscard]] constexpr bool ok() const {
        return status_.ok();
    }
    [[nodiscard]] constexpr const Status& status() const {
        return status_;
    }

   private:
    Status status_{Status::Ok()};
};

}  // namespace platform

#endif  // PLATFORM_CORE_RESULT_H_
