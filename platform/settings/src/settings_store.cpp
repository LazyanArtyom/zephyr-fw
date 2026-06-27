#include <errno.h>
#include <platform/settings/settings_store.h>
#include <stdio.h>
#include <zephyr/settings/settings.h>

#include <cstddef>
#include <cstdint>

namespace {

constexpr std::size_t kRegistryCapacity = 64;
constexpr std::size_t kMaxKeyDepth = 8;
constexpr std::int64_t kInt32MaxValue = 2147483647LL;
constexpr std::int64_t kInt32MinMagnitude = 2147483648LL;
constexpr std::uint64_t kUInt32MaxValue = 0xffffffffULL;
constexpr std::uint64_t kDecimalBase = 10ULL;
const platform::SettingMetadata* g_registry[kRegistryCapacity]{};
std::size_t g_registry_size = 0;

bool IsDigit(char value) {
    return value >= '0' && value <= '9';
}

bool IsLower(char value) {
    return value >= 'a' && value <= 'z';
}

bool IsUpper(char value) {
    return value >= 'A' && value <= 'Z';
}

bool IsAllowedKeyChar(char value) {
    return IsLower(value) || IsUpper(value) || IsDigit(value) || value == '_' || value == '-' ||
           value == '.' || value == '/';
}

bool EqualsNamespace(platform::StringView value, platform::StringView name_space) {
    return value.equals(name_space);
}

bool StartsWithNamespace(platform::StringView key, platform::StringView name_space) {
    if (key.size() <= name_space.size()) {
        return false;
    }
    if (key[name_space.size()] != '/') {
        return false;
    }

    for (std::size_t index = 0; index < name_space.size(); ++index) {
        if (key[index] != name_space[index]) {
            return false;
        }
    }

    return true;
}

bool IsNamespaceRoot(platform::StringView value) {
    return EqualsNamespace(value, "fw") || EqualsNamespace(value, "board") ||
           EqualsNamespace(value, "user") || EqualsNamespace(value, "network") ||
           EqualsNamespace(value, "display");
}

platform::Status StatusFromErrno(int rc, platform::StringView message) {
    if (rc == 0) {
        return platform::Status::Ok();
    }

    switch (rc) {
        case -EINVAL:
            return platform::Status::InvalidArgument(message);
        case -ENOENT:
            return platform::Status::NotFound(message);
        case -EACCES:
        case -EPERM:
            return platform::Status::PermissionDenied(message);
        case -EBUSY:
            return platform::Status::Busy(message);
        case -ENOTSUP:
            return platform::Status::NotSupported(message);
        case -ENODEV:
            return platform::Status::Unavailable(message);
        default:
            return platform::Status::InternalError(message);
    }
}

struct ReadStringContext {
    platform::SettingValue value;
    bool found{false};
    platform::Status status{platform::Status::Ok()};
};

int ReadStringCallback(const char* key, std::size_t len, settings_read_cb read_cb, void* cb_arg,
                       void* param) {
    if (settings_name_next(key, nullptr) != 0) {
        return 0;
    }

    auto* context = static_cast<ReadStringContext*>(param);
    char buffer[platform::kSettingsMaxValueLength + 1]{};
    const std::size_t read_size =
        len < platform::kSettingsMaxValueLength ? len : platform::kSettingsMaxValueLength;
    const ssize_t rc = read_cb(cb_arg, buffer, read_size);
    if (rc < 0) {
        context->status = StatusFromErrno(static_cast<int>(rc), "failed to read setting");
        return 0;
    }

    buffer[rc] = '\0';
    context->found = true;
    context->value.clear();
    if (!context->value.append(platform::StringView(buffer))) {
        context->status = platform::Status::InvalidArgument("setting value is too large");
    }
    return 0;
}

struct ReadRawContext {
    void* value{nullptr};
    std::size_t value_size{0};
    std::size_t* bytes_read{nullptr};
    bool found{false};
    platform::Status status{platform::Status::Ok()};
};

int ReadRawCallback(const char* key, std::size_t len, settings_read_cb read_cb, void* cb_arg,
                    void* param) {
    if (settings_name_next(key, nullptr) != 0) {
        return 0;
    }

    auto* context = static_cast<ReadRawContext*>(param);
    if (len > context->value_size) {
        context->status = platform::Status::InvalidArgument("setting value is too large");
        context->found = true;
        return 0;
    }

    const ssize_t rc = read_cb(cb_arg, context->value, len);
    if (rc < 0) {
        context->status = StatusFromErrno(static_cast<int>(rc), "failed to read setting");
        return 0;
    }

    context->found = true;
    if (context->bytes_read != nullptr) {
        *context->bytes_read = static_cast<std::size_t>(rc);
    }
    return 0;
}

struct ListContext {
    platform::SettingsStore::ListVisitor visitor{nullptr};
    void* visitor_context{nullptr};
    platform::StringView subtree;
    platform::Status status{platform::Status::Ok()};
};

int ListCallback(const char* key, std::size_t, settings_read_cb, void*, void* param) {
    auto* context = static_cast<ListContext*>(param);
    platform::FixedString<platform::kSettingsMaxKeyLength + 1> full_key;

    if (!context->subtree.empty()) {
        if (!full_key.append(context->subtree) || !full_key.append('/') ||
            !full_key.append(platform::StringView(key))) {
            context->status = platform::Status::InvalidArgument("setting key is too large");
            return -ENOMEM;
        }
    } else if (!full_key.append(platform::StringView(key))) {
        context->status = platform::Status::InvalidArgument("setting key is too large");
        return -ENOMEM;
    }

    context->status = context->visitor(full_key.view(), context->visitor_context);
    return context->status.ok() ? 0 : -ECANCELED;
}

platform::Result<std::int32_t> ParseInt32(platform::StringView value) {
    if (value.empty()) {
        return platform::Result<std::int32_t>::FromStatus(
            platform::Status::InvalidArgument("empty integer value"));
    }

    std::size_t index = 0;
    bool negative = false;
    if (value[0] == '-') {
        negative = true;
        index = 1;
    }
    if (index == value.size()) {
        return platform::Result<std::int32_t>::FromStatus(
            platform::Status::InvalidArgument("invalid integer value"));
    }

    std::int64_t parsed = 0;
    for (; index < value.size(); ++index) {
        if (!IsDigit(value[index])) {
            return platform::Result<std::int32_t>::FromStatus(
                platform::Status::InvalidArgument("invalid integer value"));
        }
        parsed = (parsed * static_cast<std::int64_t>(kDecimalBase)) +
                 static_cast<std::int64_t>(value[index] - '0');
        const std::int64_t limit = negative ? kInt32MinMagnitude : kInt32MaxValue;
        if (parsed > limit) {
            return platform::Result<std::int32_t>::FromStatus(
                platform::Status::InvalidArgument("integer value is out of range"));
        }
    }

    return platform::Result<std::int32_t>::FromValue(
        static_cast<std::int32_t>(negative ? -parsed : parsed));
}

platform::Result<std::uint32_t> ParseUInt32(platform::StringView value) {
    if (value.empty()) {
        return platform::Result<std::uint32_t>::FromStatus(
            platform::Status::InvalidArgument("empty integer value"));
    }

    std::uint64_t parsed = 0;
    for (std::size_t index = 0; index < value.size(); ++index) {
        if (!IsDigit(value[index])) {
            return platform::Result<std::uint32_t>::FromStatus(
                platform::Status::InvalidArgument("invalid integer value"));
        }
        parsed = (parsed * kDecimalBase) + static_cast<std::uint64_t>(value[index] - '0');
        if (parsed > kUInt32MaxValue) {
            return platform::Result<std::uint32_t>::FromStatus(
                platform::Status::InvalidArgument("integer value is out of range"));
        }
    }

    return platform::Result<std::uint32_t>::FromValue(static_cast<std::uint32_t>(parsed));
}

platform::Result<bool> ParseBool(platform::StringView value) {
    if (value.equals("true") || value.equals("1") || value.equals("on") || value.equals("yes")) {
        return platform::Result<bool>::FromValue(true);
    }
    if (value.equals("false") || value.equals("0") || value.equals("off") || value.equals("no")) {
        return platform::Result<bool>::FromValue(false);
    }
    return platform::Result<bool>::FromStatus(
        platform::Status::InvalidArgument("invalid boolean value"));
}

}  // namespace

namespace platform {

bool SettingRegistry::IsAllowedNamespace(StringView key) {
    return StartsWithNamespace(key, "fw") || StartsWithNamespace(key, "board") ||
           StartsWithNamespace(key, "user") || StartsWithNamespace(key, "network") ||
           StartsWithNamespace(key, "display");
}

bool SettingRegistry::IsValidKey(StringView key) {
    if (key.empty() || key.size() > kSettingsMaxKeyLength || !IsAllowedNamespace(key)) {
        return false;
    }
    if (key[0] == '/' || key[key.size() - 1] == '/') {
        return false;
    }

    char previous = '\0';
    std::size_t depth = 1;
    for (std::size_t index = 0; index < key.size(); ++index) {
        const char current = key[index];
        if (!IsAllowedKeyChar(current)) {
            return false;
        }
        if (current == '/') {
            if (previous == '/') {
                return false;
            }
            ++depth;
            if (depth > kMaxKeyDepth) {
                return false;
            }
        }
        previous = current;
    }

    return true;
}

Status SettingRegistry::Register(const SettingMetadata& metadata) {
    if (!IsValidKey(metadata.key)) {
        return Status::InvalidArgument("invalid setting key");
    }
    if (Find(metadata.key) != nullptr) {
        return Status::AlreadyExists("setting key already registered");
    }
    if (g_registry_size >= kRegistryCapacity) {
        return Status::Unavailable("setting registry is full");
    }

    g_registry[g_registry_size++] = &metadata;
    return Status::Ok();
}

const SettingMetadata* SettingRegistry::Find(StringView key) {
    for (std::size_t index = 0; index < g_registry_size; ++index) {
        if (g_registry[index] != nullptr && g_registry[index]->key.equals(key)) {
            return g_registry[index];
        }
    }

    return nullptr;
}

Status SettingRegistry::ForEach(Visitor visitor, void* context) {
    if (visitor == nullptr) {
        return Status::InvalidArgument("missing settings registry visitor");
    }

    for (std::size_t index = 0; index < g_registry_size; ++index) {
        const Status status = visitor(*g_registry[index], context);
        if (!status.ok()) {
            return status;
        }
    }

    return Status::Ok();
}

Status SettingsStore::Initialize() {
    return StatusFromErrno(settings_subsys_init(), "settings initialization failed");
}

Status SettingsStore::Load() {
    const Status init_status = Initialize();
    if (!init_status.ok()) {
        return init_status;
    }
    return StatusFromErrno(settings_load(), "settings load failed");
}

Status SettingsStore::Save() {
    const Status init_status = Initialize();
    if (!init_status.ok()) {
        return init_status;
    }
    return StatusFromErrno(settings_save(), "settings save failed");
}

Result<SettingValue> SettingsStore::GetString(StringView key) {
    if (!SettingRegistry::IsValidKey(key)) {
        return Result<SettingValue>::FromStatus(Status::InvalidArgument("invalid setting key"));
    }

    const Status init_status = Initialize();
    if (!init_status.ok()) {
        return Result<SettingValue>::FromStatus(init_status);
    }

    ReadStringContext context{};
    const int rc = settings_load_subtree_direct(key.c_str(), ReadStringCallback, &context);
    if (rc != 0) {
        return Result<SettingValue>::FromStatus(StatusFromErrno(rc, "setting read failed"));
    }
    if (!context.status.ok()) {
        return Result<SettingValue>::FromStatus(context.status);
    }
    if (!context.found) {
        return Result<SettingValue>::FromStatus(Status::NotFound("setting key was not found"));
    }

    return Result<SettingValue>::FromValue(context.value);
}

Status SettingsStore::SetString(StringView key, StringView value) {
    if (!SettingRegistry::IsValidKey(key)) {
        return Status::InvalidArgument("invalid setting key");
    }
    if (value.size() > kSettingsMaxValueLength) {
        return Status::InvalidArgument("setting value is too large");
    }

    return WriteRaw(key, value.data(), value.size());
}

Status SettingsStore::ReadRaw(StringView key, void* value, std::size_t value_size,
                              std::size_t* bytes_read) {
    if (value == nullptr || value_size == 0) {
        return Status::InvalidArgument("missing setting value buffer");
    }
    if (!SettingRegistry::IsValidKey(key)) {
        return Status::InvalidArgument("invalid setting key");
    }

    const Status init_status = Initialize();
    if (!init_status.ok()) {
        return init_status;
    }

    if (bytes_read != nullptr) {
        *bytes_read = 0;
    }

    ReadRawContext context{value, value_size, bytes_read};
    const int rc = settings_load_subtree_direct(key.c_str(), ReadRawCallback, &context);
    if (rc != 0) {
        return StatusFromErrno(rc, "setting read failed");
    }
    if (!context.status.ok()) {
        return context.status;
    }
    if (!context.found) {
        return Status::NotFound("setting key was not found");
    }

    return Status::Ok();
}

Status SettingsStore::WriteRaw(StringView key, const void* value, std::size_t value_size) {
    if (value == nullptr && value_size != 0) {
        return Status::InvalidArgument("missing setting value");
    }
    if (!SettingRegistry::IsValidKey(key)) {
        return Status::InvalidArgument("invalid setting key");
    }
    if (value_size > kSettingsMaxValueLength) {
        return Status::InvalidArgument("setting value is too large");
    }

    const Status init_status = Initialize();
    if (!init_status.ok()) {
        return init_status;
    }

    return StatusFromErrno(settings_save_one(key.c_str(), value, value_size),
                           "setting write failed");
}

Status SettingsStore::Reset(StringView key) {
    if (!SettingRegistry::IsValidKey(key)) {
        return Status::InvalidArgument("invalid setting key");
    }

    const Status init_status = Initialize();
    if (!init_status.ok()) {
        return init_status;
    }

    return StatusFromErrno(settings_delete(key.c_str()), "setting reset failed");
}

Status SettingsStore::List(ListVisitor visitor, void* context, StringView subtree) {
    if (visitor == nullptr) {
        return Status::InvalidArgument("missing settings list visitor");
    }
    if (!subtree.empty() && !SettingRegistry::IsValidKey(subtree) && !IsNamespaceRoot(subtree)) {
        return Status::InvalidArgument("invalid setting subtree");
    }

    const Status init_status = Initialize();
    if (!init_status.ok()) {
        return init_status;
    }

    ListContext list_context{visitor, context, subtree};
    const int rc = settings_load_subtree_direct(subtree.empty() ? nullptr : subtree.c_str(),
                                                ListCallback, &list_context);
    if (rc != 0 && list_context.status.ok()) {
        return StatusFromErrno(rc, "settings list failed");
    }
    return list_context.status;
}

Result<bool> SettingsStore::Get(const SettingKey<bool>& key) {
    const Result<SettingValue> value = GetString(key.name());
    if (!value.ok()) {
        if (value.status().code() == StatusCode::kNotFound) {
            return Result<bool>::FromValue(key.default_value());
        }
        return Result<bool>::FromStatus(value.status());
    }
    return ParseBool(value.value().view());
}

Status SettingsStore::Set(const SettingKey<bool>& key, bool value) {
    return SetString(key.name(), value ? "true" : "false");
}

Result<std::int32_t> SettingsStore::Get(const SettingKey<std::int32_t>& key) {
    const Result<SettingValue> value = GetString(key.name());
    if (!value.ok()) {
        if (value.status().code() == StatusCode::kNotFound) {
            return Result<std::int32_t>::FromValue(key.default_value());
        }
        return Result<std::int32_t>::FromStatus(value.status());
    }
    return ParseInt32(value.value().view());
}

Status SettingsStore::Set(const SettingKey<std::int32_t>& key, std::int32_t value) {
    char buffer[16]{};
    snprintf(buffer, sizeof(buffer), "%ld", static_cast<long>(value));
    return SetString(key.name(), buffer);
}

Result<std::uint32_t> SettingsStore::Get(const SettingKey<std::uint32_t>& key) {
    const Result<SettingValue> value = GetString(key.name());
    if (!value.ok()) {
        if (value.status().code() == StatusCode::kNotFound) {
            return Result<std::uint32_t>::FromValue(key.default_value());
        }
        return Result<std::uint32_t>::FromStatus(value.status());
    }
    return ParseUInt32(value.value().view());
}

Status SettingsStore::Set(const SettingKey<std::uint32_t>& key, std::uint32_t value) {
    char buffer[16]{};
    snprintf(buffer, sizeof(buffer), "%lu", static_cast<unsigned long>(value));
    return SetString(key.name(), buffer);
}

}  // namespace platform
