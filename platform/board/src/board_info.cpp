#include <platform/board/board_info.h>
#include <platform/board/firmware_metadata.h>
#include <zephyr/logging/log.h>
#include <zephyr/version.h>

LOG_MODULE_REGISTER(platform_board_info, CONFIG_LOG_DEFAULT_LEVEL);

namespace {

constexpr platform::BoardInfo kCurrentBoardInfo{};
constexpr const char* kUnknown = "unknown";

const char* EnabledDisabled(bool enabled) {
    return enabled ? "enabled" : "disabled";
}

}  // namespace

namespace platform {

const BoardInfo& BoardInfo::Current() {
    return kCurrentBoardInfo;
}

StringView BoardInfo::display_name() const {
    return FW_META_DISPLAY_NAME;
}

StringView BoardInfo::firmware_name() const {
    return FW_META_FIRMWARE_NAME;
}

StringView BoardInfo::firmware_slug() const {
    return FW_META_SLUG;
}

StringView BoardInfo::vendor_name() const {
    return FW_META_VENDOR;
}

StringView BoardInfo::board_name() const {
    return FW_META_BOARD_DISPLAY_NAME;
}

StringView BoardInfo::board_profile() const {
    return FW_META_BOARD_PROFILE;
}

StringView BoardInfo::board_status() const {
    return FW_META_BOARD_STATUS;
}

StringView BoardInfo::board_serial_baud() const {
    return FW_META_BOARD_SERIAL_BAUD;
}

StringView BoardInfo::board_description() const {
    return FW_META_BOARD_DESCRIPTION;
}

StringView BoardInfo::zephyr_board_target() const {
    return CONFIG_BOARD_TARGET;
}

StringView BoardInfo::flash_runner() const {
    return FW_META_BOARD_FLASH_RUNNER;
}

StringView BoardInfo::flash_chip() const {
    return FW_META_BOARD_FLASH_CHIP;
}

StringView BoardInfo::flash_offset() const {
    return FW_META_BOARD_FLASH_OFFSET;
}

StringView BoardInfo::firmware_version() const {
    return FW_META_VERSION_STRING;
}

StringView BoardInfo::build_profile() const {
    return FW_META_BUILD_PROFILE;
}

StringView BoardInfo::boot_mode() const {
    return FW_META_BOOT_MODE;
}

StringView BoardInfo::display_mode() const {
    return FW_META_DISPLAY_MODE;
}

StringView BoardInfo::git_commit() const {
    return FW_META_GIT_COMMIT;
}

StringView BoardInfo::build_timestamp() const {
    return FW_META_BUILD_TIMESTAMP;
}

bool BoardInfo::is_git_dirty() const {
    return FW_META_GIT_DIRTY != 0;
}

StringView BoardInfo::zephyr_version() const {
    return KERNEL_VERSION_STRING;
}

StringView BoardInfo::toolchain_variant() const {
    return FW_META_TOOLCHAIN_VARIANT;
}

StringView BoardInfo::zephyr_sdk_install_dir() const {
    return FW_META_ZEPHYR_SDK_INSTALL_DIR;
}

StringView BoardInfo::compiler_id() const {
    return FW_META_CXX_COMPILER_ID;
}

StringView BoardInfo::compiler_version() const {
    return FW_META_CXX_COMPILER_VERSION;
}

StringView BoardInfo::architecture() const {
#if defined(CONFIG_ARCH)
    return CONFIG_ARCH;
#else
    return kUnknown;
#endif
}

StringView BoardInfo::soc_name() const {
#if defined(CONFIG_SOC)
    return CONFIG_SOC;
#else
    return kUnknown;
#endif
}

StringView BoardInfo::soc_series() const {
#if defined(CONFIG_SOC_SERIES)
    return CONFIG_SOC_SERIES;
#else
    return kUnknown;
#endif
}

StringView BoardInfo::soc_family() const {
#if defined(CONFIG_SOC_FAMILY)
    return CONFIG_SOC_FAMILY;
#else
    return kUnknown;
#endif
}

StringView BoardInfo::soc_part_number() const {
#if defined(CONFIG_SOC_PART_NUMBER)
    return CONFIG_SOC_PART_NUMBER;
#else
    return kUnknown;
#endif
}

bool BoardInfo::shell_enabled() const {
#if defined(CONFIG_FW_SHELL) && defined(CONFIG_SHELL)
    return true;
#else
    return false;
#endif
}

bool BoardInfo::display_enabled() const {
#if defined(CONFIG_FW_DISPLAY) && defined(CONFIG_DISPLAY)
    return true;
#else
    return false;
#endif
}

bool BoardInfo::i2c_enabled() const {
#if defined(CONFIG_I2C)
    return true;
#else
    return false;
#endif
}

bool BoardInfo::settings_enabled() const {
#if defined(CONFIG_SETTINGS)
    return true;
#else
    return false;
#endif
}

bool BoardInfo::flash_enabled() const {
#if defined(CONFIG_FLASH)
    return true;
#else
    return false;
#endif
}

bool BoardInfo::mcuboot_enabled() const {
#if defined(CONFIG_BOOTLOADER_MCUBOOT)
    return true;
#else
    return false;
#endif
}

StringView BoardInfo::settings_backend() const {
#if defined(CONFIG_SETTINGS_NVS)
    return "nvs";
#elif defined(CONFIG_SETTINGS_ZMS)
    return "zms";
#elif defined(CONFIG_SETTINGS_FCB)
    return "fcb";
#elif defined(CONFIG_SETTINGS_FILE)
    return "file";
#elif defined(CONFIG_SETTINGS_NONE)
    return "none";
#elif defined(CONFIG_SETTINGS)
    return "custom";
#else
    return "disabled";
#endif
}

StringView BoardInfo::settings_backend_status() const {
    return settings_enabled() ? "enabled" : "disabled";
}

StringView BoardInfo::storage_partition_status() const {
#if !defined(CONFIG_SETTINGS)
    return "not required (settings disabled)";
#elif defined(CONFIG_FLASH_MAP)
    return "flash map enabled";
#elif defined(CONFIG_FLASH)
    return "flash enabled, flash map disabled";
#else
    return "not configured";
#endif
}

void BoardInfo::LogBootSummary() const {
    LOG_INF("%s booted", display_name().c_str());
    LOG_INF("Board profile: %s", board_profile().c_str());
    LOG_INF("Zephyr board: %s", zephyr_board_target().c_str());
    LOG_INF("Version: %s%s", firmware_version().c_str(), is_git_dirty() ? "-dirty" : "");
    LOG_INF("Build profile: %s, boot: %s", build_profile().c_str(), boot_mode().c_str());
    LOG_INF("Features: shell=%s display=%s i2c=%s settings=%s", EnabledDisabled(shell_enabled()),
            EnabledDisabled(display_enabled()), EnabledDisabled(i2c_enabled()),
            EnabledDisabled(settings_enabled()));
}

}  // namespace platform
