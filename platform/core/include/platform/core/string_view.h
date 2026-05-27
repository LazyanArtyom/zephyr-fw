#ifndef PLATFORM_CORE_STRING_VIEW_H_
#define PLATFORM_CORE_STRING_VIEW_H_

#include <cstddef>

namespace platform {

class StringView final {
   public:
    constexpr StringView() = default;
    constexpr StringView(const char* text)
        : data_(text == nullptr ? "" : text), size_(Length(data_)) {}

    [[nodiscard]] constexpr const char* data() const {
        return data_;
    }
    [[nodiscard]] constexpr const char* c_str() const {
        return data_;
    }
    [[nodiscard]] constexpr std::size_t size() const {
        return size_;
    }
    [[nodiscard]] constexpr bool empty() const {
        return size_ == 0;
    }
    [[nodiscard]] constexpr char operator[](std::size_t index) const {
        return index < size_ ? data_[index] : '\0';
    }

    [[nodiscard]] constexpr bool equals(StringView other) const {
        if (size_ != other.size_) {
            return false;
        }

        for (std::size_t index = 0; index < size_; ++index) {
            if (data_[index] != other.data_[index]) {
                return false;
            }
        }

        return true;
    }

   private:
    [[nodiscard]] static constexpr std::size_t Length(const char* text) {
        std::size_t length = 0;
        while (text[length] != '\0') {
            ++length;
        }
        return length;
    }

    const char* data_{""};
    std::size_t size_{0};
};

}  // namespace platform

#endif  // PLATFORM_CORE_STRING_VIEW_H_
