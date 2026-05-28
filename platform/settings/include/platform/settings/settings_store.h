#ifndef PLATFORM_SETTINGS_SETTINGS_STORE_H_
#define PLATFORM_SETTINGS_SETTINGS_STORE_H_

#include <platform/core/fixed_string.h>
#include <platform/core/result.h>
#include <platform/core/status.h>
#include <platform/core/string_view.h>

#include <cstddef>
#include <cstdint>

namespace platform {

inline constexpr std::size_t kSettingsMaxKeyLength = 72;
inline constexpr std::size_t kSettingsMaxValueLength = 256;

using SettingValue = FixedString<kSettingsMaxValueLength + 1>;

enum class SettingValueType : std::uint8_t {
    kString = 0,
    kBool,
    kInt32,
    kUInt32,
    kBytes,
};

template <typename T>
class SettingKey final {
   public:
    constexpr SettingKey(StringView name, T default_value)
        : name_(name), default_value_(default_value) {}

    [[nodiscard]] constexpr StringView name() const {
        return name_;
    }
    [[nodiscard]] constexpr const T& default_value() const {
        return default_value_;
    }

   private:
    StringView name_{};
    T default_value_{};
};

struct SettingMetadata {
    StringView key;
    SettingValueType type{SettingValueType::kString};
    StringView default_value;
    StringView description;
};

class SettingRegistry final {
   public:
    using Visitor = Status (*)(const SettingMetadata& metadata, void* context);

    [[nodiscard]] static bool IsAllowedNamespace(StringView key);
    [[nodiscard]] static bool IsValidKey(StringView key);
    [[nodiscard]] static Status Register(const SettingMetadata& metadata);
    [[nodiscard]] static const SettingMetadata* Find(StringView key);
    [[nodiscard]] static Status ForEach(Visitor visitor, void* context);
};

class SettingsStore final {
   public:
    using ListVisitor = Status (*)(StringView key, void* context);

    [[nodiscard]] static Status Initialize();
    [[nodiscard]] static Status Load();
    [[nodiscard]] static Status Save();

    [[nodiscard]] static Result<SettingValue> GetString(StringView key);
    [[nodiscard]] static Status SetString(StringView key, StringView value);
    [[nodiscard]] static Status ReadRaw(StringView key, void* value, std::size_t value_size,
                                        std::size_t* bytes_read);
    [[nodiscard]] static Status WriteRaw(StringView key, const void* value, std::size_t value_size);
    [[nodiscard]] static Status Reset(StringView key);
    [[nodiscard]] static Status List(ListVisitor visitor, void* context, StringView subtree = {});

    [[nodiscard]] static Result<bool> Get(const SettingKey<bool>& key);
    [[nodiscard]] static Status Set(const SettingKey<bool>& key, bool value);
    [[nodiscard]] static Result<std::int32_t> Get(const SettingKey<std::int32_t>& key);
    [[nodiscard]] static Status Set(const SettingKey<std::int32_t>& key, std::int32_t value);
    [[nodiscard]] static Result<std::uint32_t> Get(const SettingKey<std::uint32_t>& key);
    [[nodiscard]] static Status Set(const SettingKey<std::uint32_t>& key, std::uint32_t value);
};

}  // namespace platform

#endif  // PLATFORM_SETTINGS_SETTINGS_STORE_H_
