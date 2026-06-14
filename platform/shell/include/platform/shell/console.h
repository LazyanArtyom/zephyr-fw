#ifndef PLATFORM_SHELL_CONSOLE_H_
#define PLATFORM_SHELL_CONSOLE_H_

#include <platform/core/status.h>
#include <platform/core/string_view.h>

#include <cstddef>
#include <cstdint>

struct shell;

namespace platform::shell {

class Arguments final {
   public:
    constexpr Arguments(std::size_t count, char* const* values) : count_(count), values_(values) {}

    [[nodiscard]] constexpr std::size_t size() const {
        return count_;
    }
    [[nodiscard]] StringView at(std::size_t index) const {
        return index < count_ ? StringView(values_[index]) : StringView{};
    }

   private:
    std::size_t count_{0};
    char* const* values_{nullptr};
};

class Console final {
   public:
    explicit constexpr Console(const ::shell* native_shell) : native_shell_(native_shell) {}

    void line(StringView text) const;
    void blank_line() const;
    void section(StringView title) const;
    void field(StringView label, StringView value) const;
    void field_with_suffix(StringView label, StringView value, StringView suffix) const;
    void field_pair(StringView label, StringView first, StringView separator, StringView second,
                    StringView suffix = {}) const;
    void integer_field(StringView label, std::int64_t value, StringView unit = {}) const;
    void subfield(StringView label, StringView value) const;
    void feature(StringView label, bool enabled) const;
    void error(StringView message) const;
    void error_value(StringView message, StringView value) const;
    void error_subject(StringView message, StringView subject, StringView detail) const;
    void error_option(char option) const;

   private:
    const ::shell* native_shell_{nullptr};
};

[[nodiscard]] int StatusToErrno(const Status& status);
int PrintStatusError(const Console& output, StringView action, const Status& status);

}  // namespace platform::shell

#endif  // PLATFORM_SHELL_CONSOLE_H_
