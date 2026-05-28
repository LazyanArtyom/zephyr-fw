#ifndef PLATFORM_STORAGE_STORAGE_INFO_H_
#define PLATFORM_STORAGE_STORAGE_INFO_H_

#include <platform/core/result.h>
#include <platform/core/status.h>
#include <platform/core/string_view.h>

#include <cstdint>

namespace platform {

struct StoragePartitionInfo {
    StringView label;
    std::uint32_t offset{0};
    std::uint32_t size{0};
    std::uint32_t erase_block_size{0};
    bool available{false};
};

class StorageInfo final {
   public:
    [[nodiscard]] static Result<StoragePartitionInfo> SettingsPartition();
    [[nodiscard]] static StringView BackendName();
    [[nodiscard]] static bool PersistentSettingsEnabled();
};

}  // namespace platform

#endif  // PLATFORM_STORAGE_STORAGE_INFO_H_
