#ifndef PLATFORM_CORE_RESULT_H_
#define PLATFORM_CORE_RESULT_H_

#include <platform/core/status.h>

#include <utility>

namespace platform {

template <typename T>
class Result {
   public:
    constexpr Result(const T& value) : value_(value), status_(Status::Ok()), has_value_(true) {}
    constexpr Result(T&& value)
        : value_(std::move(value)), status_(Status::Ok()), has_value_(true) {}
    constexpr Result(Status status) : status_(status), has_value_(false) {}

    [[nodiscard]] constexpr bool ok() const {
        return has_value_ && status_.ok();
    }

    [[nodiscard]] constexpr const Status& status() const {
        return status_;
    }

    [[nodiscard]] constexpr const T& value() const {
        return value_;
    }

    [[nodiscard]] constexpr T& value() {
        return value_;
    }

   private:
    T value_{};
    Status status_{StatusCode::kInternalError, "result has no value"};
    bool has_value_{false};
};

}  // namespace platform

#endif  // PLATFORM_CORE_RESULT_H_
