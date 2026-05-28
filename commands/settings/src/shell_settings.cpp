#include <errno.h>
#include <platform/settings/settings_store.h>
#include <platform/shell/console.h>
#include <zephyr/shell/shell.h>

namespace {

int PrintUsage(const platform::shell::Console& output) {
    output.line("Usage:");
    output.line("  settings list [subtree]");
    output.line("  settings get <key>");
    output.line("  settings set <key> <value>");
    output.line("  settings reset <key>");
    output.line("  settings save");
    output.line("  settings load");
    return 0;
}

int ToErrno(const platform::Status& status) {
    switch (status.code()) {
        case platform::StatusCode::kOk:
            return 0;
        case platform::StatusCode::kInvalidArgument:
            return -EINVAL;
        case platform::StatusCode::kNotFound:
            return -ENOENT;
        case platform::StatusCode::kPermissionDenied:
            return -EACCES;
        case platform::StatusCode::kBusy:
            return -EBUSY;
        case platform::StatusCode::kNotSupported:
            return -ENOTSUP;
        case platform::StatusCode::kUnavailable:
            return -ENODEV;
        default:
            return -EIO;
    }
}

int PrintStatusError(const platform::shell::Console& output, platform::StringView action,
                     const platform::Status& status) {
    output.error_subject(action, platform::ToString(status.code()), status.message());
    return ToErrno(status);
}

platform::Status PrintSettingKey(platform::StringView key, void* context) {
    static_cast<const platform::shell::Console*>(context)->line(key);
    return platform::Status::Ok();
}

int CmdList(const platform::shell::Console& output, const platform::shell::Arguments& arguments) {
    const platform::StringView subtree =
        arguments.size() >= 3 ? arguments.at(2) : platform::StringView{};
    const platform::Status status = platform::SettingsStore::List(
        PrintSettingKey, const_cast<platform::shell::Console*>(&output), subtree);
    if (!status.ok()) {
        return PrintStatusError(output, "settings list failed", status);
    }
    return 0;
}

int CmdGet(const platform::shell::Console& output, const platform::shell::Arguments& arguments) {
    if (arguments.size() != 3) {
        return PrintUsage(output);
    }

    const platform::Result<platform::SettingValue> value =
        platform::SettingsStore::GetString(arguments.at(2));
    if (!value.ok()) {
        return PrintStatusError(output, "settings get failed", value.status());
    }

    output.line(value.value().view());
    return 0;
}

int CmdSet(const platform::shell::Console& output, const platform::shell::Arguments& arguments) {
    if (arguments.size() != 4) {
        return PrintUsage(output);
    }

    const platform::Status status =
        platform::SettingsStore::SetString(arguments.at(2), arguments.at(3));
    if (!status.ok()) {
        return PrintStatusError(output, "settings set failed", status);
    }

    output.field("Saved", arguments.at(2));
    return 0;
}

int CmdReset(const platform::shell::Console& output, const platform::shell::Arguments& arguments) {
    if (arguments.size() != 3) {
        return PrintUsage(output);
    }

    const platform::Status status = platform::SettingsStore::Reset(arguments.at(2));
    if (!status.ok()) {
        return PrintStatusError(output, "settings reset failed", status);
    }

    output.field("Reset", arguments.at(2));
    return 0;
}

int CmdSave(const platform::shell::Console& output) {
    const platform::Status status = platform::SettingsStore::Save();
    if (!status.ok()) {
        return PrintStatusError(output, "settings save failed", status);
    }
    output.line("Settings saved.");
    return 0;
}

int CmdLoad(const platform::shell::Console& output) {
    const platform::Status status = platform::SettingsStore::Load();
    if (!status.ok()) {
        return PrintStatusError(output, "settings load failed", status);
    }
    output.line("Settings loaded.");
    return 0;
}

int CmdSettings(const shell* shell, size_t argc, char** argv) {
    const platform::shell::Console output(shell);
    const platform::shell::Arguments arguments(argc, argv);

    if (arguments.size() < 2 || arguments.at(1).equals("--help") || arguments.at(1).equals("-h")) {
        return PrintUsage(output);
    }

    const platform::StringView command = arguments.at(1);
    if (command.equals("list")) {
        return CmdList(output, arguments);
    }
    if (command.equals("get")) {
        return CmdGet(output, arguments);
    }
    if (command.equals("set")) {
        return CmdSet(output, arguments);
    }
    if (command.equals("reset")) {
        return CmdReset(output, arguments);
    }
    if (command.equals("save")) {
        return CmdSave(output);
    }
    if (command.equals("load")) {
        return CmdLoad(output);
    }

    output.error_value("unknown settings command", command);
    return -EINVAL;
}

}  // namespace

SHELL_CMD_ARG_REGISTER(settings, NULL, "Persistent settings commands.", CmdSettings, 1, 4);
