#include <errno.h>
#include <platform/shell/console.h>
#include <zephyr/shell/shell.h>

namespace platform::shell {

void Console::line(StringView text) const {
    shell_print(native_shell_, "%s", text.c_str());
}

void Console::blank_line() const {
    shell_print(native_shell_, "");
}

void Console::section(StringView title) const {
    shell_print(native_shell_, "%s:", title.c_str());
}

void Console::field(StringView label, StringView value) const {
    shell_print(native_shell_, "%s: %s", label.c_str(), value.c_str());
}

void Console::field_with_suffix(StringView label, StringView value, StringView suffix) const {
    shell_print(native_shell_, "%s: %s%s", label.c_str(), value.c_str(), suffix.c_str());
}

void Console::field_pair(StringView label, StringView first, StringView separator,
                         StringView second, StringView suffix) const {
    shell_print(native_shell_, "%s: %s%s%s%s", label.c_str(), first.c_str(), separator.c_str(),
                second.c_str(), suffix.c_str());
}

void Console::integer_field(StringView label, std::int64_t value, StringView unit) const {
    if (unit.empty()) {
        shell_print(native_shell_, "%s: %lld", label.c_str(), static_cast<long long>(value));
    } else {
        shell_print(native_shell_, "%s: %lld %s", label.c_str(), static_cast<long long>(value),
                    unit.c_str());
    }
}

void Console::subfield(StringView label, StringView value) const {
    shell_print(native_shell_, "  %s: %s", label.c_str(), value.c_str());
}

void Console::feature(StringView label, bool enabled) const {
    shell_print(native_shell_, "  %s: %s", label.c_str(), enabled ? "enabled" : "disabled");
}

void Console::error(StringView message) const {
    shell_error(native_shell_, "%s", message.c_str());
}

void Console::error_value(StringView message, StringView value) const {
    shell_error(native_shell_, "%s: %s", message.c_str(), value.c_str());
}

void Console::error_subject(StringView message, StringView subject, StringView detail) const {
    shell_error(native_shell_, "%s '%s': %s", message.c_str(), subject.c_str(), detail.c_str());
}

void Console::error_option(char option) const {
    shell_error(native_shell_, "unknown option: -%c", option);
}

int StatusToErrno(const Status& status) {
    switch (status.code()) {
        case StatusCode::kOk:
            return 0;
        case StatusCode::kInvalidArgument:
            return -EINVAL;
        case StatusCode::kNotFound:
            return -ENOENT;
        case StatusCode::kAlreadyExists:
            return -EEXIST;
        case StatusCode::kPermissionDenied:
            return -EACCES;
        case StatusCode::kFailedPrecondition:
            return -EINVAL;
        case StatusCode::kBusy:
            return -EBUSY;
        case StatusCode::kTimeout:
            return -ETIMEDOUT;
        case StatusCode::kNotSupported:
            return -ENOTSUP;
        case StatusCode::kUnavailable:
            return -ENODEV;
        case StatusCode::kInternalError:
            return -EIO;
    }

    return -EIO;
}

int PrintStatusError(const Console& output, StringView action, const Status& status) {
    output.error_subject(action, ToString(status.code()), status.message());
    return StatusToErrno(status);
}

}  // namespace platform::shell
