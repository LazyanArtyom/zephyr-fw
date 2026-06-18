#include <errno.h>
#include <platform/core/reset_info.h>
#include <platform/shell/console.h>
#include <services/diagnostics/diagnostics_service.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include <cstddef>

namespace {

using platform::shell::Console;

int PrintRecordStatus(const Console& output) {
    const platform::Result<services::diagnostics::CrashRecord> record =
        services::diagnostics::DiagnosticsService::ReadCrashRecord();
    if (!record.ok()) {
        return platform::shell::PrintStatusError(output, "diagnostics read failed",
                                                 record.status());
    }

    output.section("Diagnostics");
    output.integer_field("Boot count", record.value().boot_count);
    output.field("Reset cause", record.value().reset_cause);
    output.integer_field("Reset flags", record.value().reset_cause_flags);
    output.integer_field("Panic count", record.value().panic_count);
    output.field("Last fatal", record.value().last_fatal_text);
    output.field("Last fatal thread", record.value().last_fatal_thread[0] == '\0'
                                          ? "unknown"
                                          : record.value().last_fatal_thread);
    output.field("Last watchdog", record.value().last_watchdog_reason);
    output.field("Firmware", record.value().firmware_version);
    output.field("Git commit", record.value().git_commit);
    output.field("Build profile", record.value().build_profile);
    return 0;
}

int CmdStatus(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    return PrintRecordStatus(Console(shell));
}

int CmdCrash(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    const Console output(shell);
    const platform::Result<services::diagnostics::CrashRecord> record =
        services::diagnostics::DiagnosticsService::ReadCrashRecord();
    if (!record.ok()) {
        return platform::shell::PrintStatusError(output, "diagnostics read failed",
                                                 record.status());
    }

    output.section("Crash");
    output.integer_field("Panic count", record.value().panic_count);
    output.field("Last fatal", record.value().last_fatal_text);
    output.integer_field("Last fatal code", record.value().last_fatal_reason);
    output.field("Last fatal thread", record.value().last_fatal_thread[0] == '\0'
                                          ? "unknown"
                                          : record.value().last_fatal_thread);
    output.integer_field("Last fatal uptime",
                         static_cast<std::int64_t>(record.value().last_fatal_uptime_ms), "ms");
    output.field("Last watchdog", record.value().last_watchdog_reason);
    return 0;
}

int CmdResetCause(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    const platform::ResetInfo reset_info = platform::ResetInfo::Current();
    const Console output(shell);
    output.field("Reset reason", reset_info.reason_text());
    output.integer_field("Reset flags", reset_info.flags());
    output.feature("reset cause driver", reset_info.available());
    return 0;
}

#if defined(CONFIG_THREAD_MONITOR)
void PrintThread(const k_thread* thread, void* context) {
    const auto* output = static_cast<const Console*>(context);
    const char* name = k_thread_name_get(const_cast<k_thread*>(thread));
    output->field(thread == k_current_get() ? "* thread" : "  thread",
                  name == nullptr || name[0] == '\0' ? "unknown" : name);
}
#endif

int CmdThreads(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    const Console output(shell);
#if defined(CONFIG_THREAD_MONITOR)
    output.section("Threads");
    k_thread_foreach_unlocked(PrintThread, const_cast<Console*>(&output));
    return 0;
#else
    output.error("thread monitoring is not enabled in this build");
    return -ENOTSUP;
#endif
}

#if defined(CONFIG_THREAD_MONITOR) && defined(CONFIG_THREAD_STACK_INFO)
void PrintStack(const k_thread* thread, void* context) {
    const auto* output = static_cast<const Console*>(context);
    const char* name = k_thread_name_get(const_cast<k_thread*>(thread));
    std::size_t unused = 0;
    const int rc = k_thread_stack_space_get(thread, &unused);
    if (rc != 0) {
        output->field(name == nullptr || name[0] == '\0' ? "unknown" : name, "stack unavailable");
        return;
    }

    const std::size_t size = thread->stack_info.size;
    const std::size_t used = size >= unused ? size - unused : 0;
    output->field(name == nullptr || name[0] == '\0' ? "unknown" : name, "");
    output->integer_field("  stack used", static_cast<std::int64_t>(used), "bytes");
    output->integer_field("  stack free", static_cast<std::int64_t>(unused), "bytes");
    output->integer_field("  stack size", static_cast<std::int64_t>(size), "bytes");
}
#endif

int CmdStacks(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    const Console output(shell);
#if defined(CONFIG_THREAD_MONITOR) && defined(CONFIG_THREAD_STACK_INFO)
    output.section("Stacks");
    k_thread_foreach_unlocked(PrintStack, const_cast<Console*>(&output));
    return 0;
#else
    output.error("thread stack diagnostics are not enabled in this build");
    return -ENOTSUP;
#endif
}

int CmdClear(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    const Console output(shell);
    const platform::Status status = services::diagnostics::DiagnosticsService::ClearCrashRecord();
    if (!status.ok()) {
        return platform::shell::PrintStatusError(output, "diagnostics clear failed", status);
    }

    output.line("Diagnostics crash record cleared.");
    return 0;
}

}  // namespace

SHELL_STATIC_SUBCMD_SET_CREATE(diag_subcommands,
                               SHELL_CMD(status, NULL, "Show diagnostics status.", CmdStatus),
                               SHELL_CMD(crash, NULL, "Show crash diagnostics.", CmdCrash),
                               SHELL_EXPR_CMD(1, reset-cause, NULL, "Show reset cause.",
                                              CmdResetCause),
                               SHELL_CMD(threads, NULL, "Show thread diagnostics.", CmdThreads),
                               SHELL_CMD(stacks, NULL, "Show stack diagnostics.", CmdStacks),
                               SHELL_CMD(clear, NULL, "Clear diagnostics crash record.", CmdClear),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(diag, &diag_subcommands, "Diagnostics commands.", NULL);
