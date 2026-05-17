#ifndef APP_STATUS_H_
#define APP_STATUS_H_

#include <cstdint>

namespace app {

enum class Status : std::uint8_t {
    kOk,
    kError,
};

}  // namespace app

#endif  // APP_STATUS_H_
