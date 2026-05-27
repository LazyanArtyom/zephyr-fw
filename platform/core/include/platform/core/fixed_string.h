#ifndef PLATFORM_CORE_FIXED_STRING_H_
#define PLATFORM_CORE_FIXED_STRING_H_

#include <platform/core/string_view.h>

#include <cstddef>
#include <cstdint>

namespace platform {

template <std::size_t Capacity>
class FixedString final {
    static_assert(Capacity > 0, "FixedString requires storage for a terminator");

   public:
    constexpr FixedString() = default;

    [[nodiscard]] constexpr bool empty() const {
        return size_ == 0;
    }
    [[nodiscard]] constexpr std::size_t size() const {
        return size_;
    }
    [[nodiscard]] constexpr StringView view() const {
        return StringView(data_);
    }

    constexpr void clear() {
        size_ = 0;
        data_[0] = '\0';
    }

    constexpr bool append(StringView text) {
        if (text.size() > available()) {
            return false;
        }

        for (std::size_t index = 0; index < text.size(); ++index) {
            data_[size_++] = text[index];
        }
        data_[size_] = '\0';
        return true;
    }

    constexpr bool append(char value) {
        if (available() == 0) {
            return false;
        }

        data_[size_++] = value;
        data_[size_] = '\0';
        return true;
    }

    constexpr bool append_hex_byte(std::uint8_t value) {
        constexpr char kHexDigits[] = "0123456789abcdef";
        if (available() < 2) {
            return false;
        }

        data_[size_++] = kHexDigits[(value >> 4U) & 0x0fU];
        data_[size_++] = kHexDigits[value & 0x0fU];
        data_[size_] = '\0';
        return true;
    }

   private:
    [[nodiscard]] constexpr std::size_t available() const {
        return Capacity - size_ - 1;
    }

    char data_[Capacity]{};
    std::size_t size_{0};
};

}  // namespace platform

#endif  // PLATFORM_CORE_FIXED_STRING_H_
