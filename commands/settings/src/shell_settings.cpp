#include <errno.h>
#include <platform/settings/settings_store.h>
#include <platform/shell/console.h>
#include <zephyr/shell/shell.h>

#include <cstddef>

namespace {

constexpr platform::StringView kRootCommand("settings");
constexpr std::size_t kRootCommandArgIndex = 0;
constexpr std::size_t kSubcommandArgIndex = 1;
constexpr std::size_t kLocalUserArgBase = 1;
constexpr std::size_t kFullPathUserArgBase = 2;
constexpr std::size_t kSingleValueArgCount = 1;
constexpr std::size_t kKeyValueArgCount = 2;
constexpr std::size_t kSettingsListRequiredArgs = 1;
constexpr std::size_t kSettingsListOptionalArgs = 2;
constexpr std::size_t kSettingsGetRequiredArgs = 2;
constexpr std::size_t kSettingsGetOptionalArgs = 1;
constexpr std::size_t kSettingsSetRequiredArgs = 3;
constexpr std::size_t kSettingsSetOptionalArgs = 1;
constexpr std::size_t kSettingsResetRequiredArgs = 2;
constexpr std::size_t kSettingsResetOptionalArgs = 1;

std::size_t CommandArgBase(const platform::shell::Arguments& arguments,
                           platform::StringView subcommand) {
    if (arguments.size() > kSubcommandArgIndex &&
        arguments.at(kRootCommandArgIndex).equals(kRootCommand) &&
        arguments.at(kSubcommandArgIndex).equals(subcommand)) {
        return kFullPathUserArgBase;
    }
    return kLocalUserArgBase;
}

int PrintUsage(const platform::shell::Console& output, platform::StringView usage) {
    output.line(usage);
    return -EINVAL;
}

platform::Status PrintSettingKey(platform::StringView key, void* context) {
    static_cast<const platform::shell::Console*>(context)->line(key);
    return platform::Status::Ok();
}

int CmdList(const shell* shell, size_t argc, char** argv) {
    const platform::shell::Console output(shell);
    const platform::shell::Arguments arguments(argc, argv);
    const std::size_t subtree_index = CommandArgBase(arguments, "list");
    if (arguments.size() > subtree_index + kSingleValueArgCount) {
        return PrintUsage(output, "Usage: settings list [subtree]");
    }
    const platform::StringView subtree = arguments.size() == subtree_index + kSingleValueArgCount
                                             ? arguments.at(subtree_index)
                                             : platform::StringView{};
    const platform::Status status = platform::SettingsStore::List(
        PrintSettingKey, const_cast<platform::shell::Console*>(&output), subtree);
    if (!status.ok()) {
        return platform::shell::PrintStatusError(output, "settings list failed", status);
    }
    return 0;
}

int CmdGet(const shell* shell, size_t argc, char** argv) {
    const platform::shell::Console output(shell);
    const platform::shell::Arguments arguments(argc, argv);
    const std::size_t key_index = CommandArgBase(arguments, "get");
    if (arguments.size() != key_index + kSingleValueArgCount) {
        return PrintUsage(output, "Usage: settings get <key>");
    }
    const platform::Result<platform::SettingValue> value =
        platform::SettingsStore::GetString(arguments.at(key_index));
    if (!value.ok()) {
        return platform::shell::PrintStatusError(output, "settings get failed", value.status());
    }

    output.line(value.value().view());
    return 0;
}

int CmdSet(const shell* shell, size_t argc, char** argv) {
    const platform::shell::Console output(shell);
    const platform::shell::Arguments arguments(argc, argv);
    const std::size_t key_index = CommandArgBase(arguments, "set");
    if (arguments.size() != key_index + kKeyValueArgCount) {
        return PrintUsage(output, "Usage: settings set <key> <value>");
    }
    const platform::Status status = platform::SettingsStore::SetString(
        arguments.at(key_index), arguments.at(key_index + kSingleValueArgCount));
    if (!status.ok()) {
        return platform::shell::PrintStatusError(output, "settings set failed", status);
    }

    output.field("Saved", arguments.at(key_index));
    return 0;
}

int CmdReset(const shell* shell, size_t argc, char** argv) {
    const platform::shell::Console output(shell);
    const platform::shell::Arguments arguments(argc, argv);
    const std::size_t key_index = CommandArgBase(arguments, "reset");
    if (arguments.size() != key_index + kSingleValueArgCount) {
        return PrintUsage(output, "Usage: settings reset <key>");
    }
    const platform::Status status = platform::SettingsStore::Reset(arguments.at(key_index));
    if (!status.ok()) {
        return platform::shell::PrintStatusError(output, "settings reset failed", status);
    }

    output.field("Reset", arguments.at(key_index));
    return 0;
}

int CmdSave(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    const platform::shell::Console output(shell);
    const platform::Status status = platform::SettingsStore::Save();
    if (!status.ok()) {
        return platform::shell::PrintStatusError(output, "settings save failed", status);
    }
    output.line("Settings saved.");
    return 0;
}

int CmdLoad(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    const platform::shell::Console output(shell);
    const platform::Status status = platform::SettingsStore::Load();
    if (!status.ok()) {
        return platform::shell::PrintStatusError(output, "settings load failed", status);
    }
    output.line("Settings loaded.");
    return 0;
}

}  // namespace

SHELL_STATIC_SUBCMD_SET_CREATE(settings_subcommands,
                               SHELL_CMD_ARG(list, NULL, "List settings keys.", CmdList,
                                             kSettingsListRequiredArgs, kSettingsListOptionalArgs),
                               SHELL_CMD_ARG(get, NULL, "Read a setting.", CmdGet,
                                             kSettingsGetRequiredArgs, kSettingsGetOptionalArgs),
                               SHELL_CMD_ARG(set, NULL, "Write a setting.", CmdSet,
                                             kSettingsSetRequiredArgs, kSettingsSetOptionalArgs),
                               SHELL_CMD_ARG(reset, NULL, "Reset a setting.", CmdReset,
                                             kSettingsResetRequiredArgs,
                                             kSettingsResetOptionalArgs),
                               SHELL_CMD(save, NULL, "Save settings.", CmdSave),
                               SHELL_CMD(load, NULL, "Load settings.", CmdLoad),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(settings, &settings_subcommands, "Persistent settings commands.", NULL);
