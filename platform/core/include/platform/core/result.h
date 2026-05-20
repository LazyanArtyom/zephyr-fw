#ifndef PLATFORM_CORE_RESULT_H_
#define PLATFORM_CORE_RESULT_H_

#include <platform/core/status.h>

#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>

namespace platform {

template <typename T>
class Result {
   public:
    Result(const T& value) : status_(Status::Ok()), has_value_(true) {
        new (&storage_.value) T(value);
    }

    Result(T&& value) : status_(Status::Ok()), has_value_(true) {
        new (&storage_.value) T(std::move(value));
    }

    explicit Result(Status status) : status_(status), has_value_(false) {}

    Result(const Result& other) : status_(other.status_), has_value_(other.has_value_) {
        if (has_value_) {
            new (&storage_.value) T(other.storage_.value);
        }
    }

    Result(Result&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        : status_(other.status_), has_value_(other.has_value_) {
        if (has_value_) {
            new (&storage_.value) T(std::move(other.storage_.value));
        }
    }

    Result& operator=(const Result& other) {
        if (this == &other) {
            return *this;
        }

        DestroyValue();
        status_ = other.status_;
        has_value_ = other.has_value_;
        if (has_value_) {
            new (&storage_.value) T(other.storage_.value);
        }
        return *this;
    }

    Result& operator=(Result&& other) noexcept(std::is_nothrow_move_assignable_v<T> &&
                                               std::is_nothrow_move_constructible_v<T>) {
        if (this == &other) {
            return *this;
        }

        DestroyValue();
        status_ = other.status_;
        has_value_ = other.has_value_;
        if (has_value_) {
            new (&storage_.value) T(std::move(other.storage_.value));
        }
        return *this;
    }

    ~Result() {
        DestroyValue();
    }

    [[nodiscard]] static Result<T> FromValue(const T& value) {
        return Result<T>(value);
    }
    [[nodiscard]] static Result<T> FromValue(T&& value) {
        return Result<T>(std::move(value));
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
        return storage_.value;
    }
    [[nodiscard]] T& value() {
        return storage_.value;
    }
    [[nodiscard]] const T* value_if_ok() const {
        return ok() ? &storage_.value : nullptr;
    }
    [[nodiscard]] T* value_if_ok() {
        return ok() ? &storage_.value : nullptr;
    }

   private:
    union Storage {
        constexpr Storage() : empty(0) {}
        ~Storage() {}

        std::uint8_t empty;
        T value;
    };

    void DestroyValue() {
        if (has_value_) {
            storage_.value.~T();
            has_value_ = false;
        }
    }

    Storage storage_{};
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
