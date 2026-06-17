#include <platform/settings/settings_store.h>
#include <platform/shell/console.h>
#include <zephyr/shell/shell.h>

namespace {

platform::Status PrintSettingKey(platform::StringView key, void* context) {
    static_cast<const platform::shell::Console*>(context)->line(key);
    return platform::Status::Ok();
}

int CmdList(const shell* shell, size_t argc, char** argv) {
    const platform::shell::Console output(shell);
    const platform::shell::Arguments arguments(argc, argv);
    const platform::StringView subtree =
        arguments.size() >= 3 ? arguments.at(2) : platform::StringView{};
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
    const platform::Result<platform::SettingValue> value =
        platform::SettingsStore::GetString(arguments.at(2));
    if (!value.ok()) {
        return platform::shell::PrintStatusError(output, "settings get failed", value.status());
    }

    output.line(value.value().view());
    return 0;
}

int CmdSet(const shell* shell, size_t argc, char** argv) {
    const platform::shell::Console output(shell);
    const platform::shell::Arguments arguments(argc, argv);
    const platform::Status status =
        platform::SettingsStore::SetString(arguments.at(2), arguments.at(3));
    if (!status.ok()) {
        return platform::shell::PrintStatusError(output, "settings set failed", status);
    }

    output.field("Saved", arguments.at(2));
    return 0;
}

int CmdReset(const shell* shell, size_t argc, char** argv) {
    const platform::shell::Console output(shell);
    const platform::shell::Arguments arguments(argc, argv);
    const platform::Status status = platform::SettingsStore::Reset(arguments.at(2));
    if (!status.ok()) {
        return platform::shell::PrintStatusError(output, "settings reset failed", status);
    }

    output.field("Reset", arguments.at(2));
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
                               SHELL_CMD_ARG(list, NULL, "List settings keys.", CmdList, 2, 1),
                               SHELL_CMD_ARG(get, NULL, "Read a setting.", CmdGet, 3, 0),
                               SHELL_CMD_ARG(set, NULL, "Write a setting.", CmdSet, 4, 0),
                               SHELL_CMD_ARG(reset, NULL, "Reset a setting.", CmdReset, 3, 0),
                               SHELL_CMD(save, NULL, "Save settings.", CmdSave),
                               SHELL_CMD(load, NULL, "Load settings.", CmdLoad),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(settings, &settings_subcommands, "Persistent settings commands.", NULL);
