#ifndef PLATFORM_CORE_RESET_INFO_H_
#define PLATFORM_CORE_RESET_INFO_H_

#include <platform/core/string_view.h>

#include <cstdint>

namespace platform {

class ResetInfo final {
   public:
    enum class Reason : std::uint8_t {
        kUnknown = 0,
    };

    constexpr explicit ResetInfo(Reason reason = Reason::kUnknown) : reason_(reason) {}

    [[nodiscard]] static ResetInfo Current();
    static void RequestColdReboot();

    [[nodiscard]] constexpr Reason reason() const {
        return reason_;
    }
    [[nodiscard]] StringView reason_text() const;

   private:
    Reason reason_{Reason::kUnknown};
};

}  // namespace platform

#endif  // PLATFORM_CORE_RESET_INFO_H_
