#pragma once

#include <Windows.h>
#include <eh.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdarg>
#include <exception>
#include <cstdlib>
#include <string>

#include "platform/debug.hpp"
#include "platform/build_profile.hpp"

namespace crash_logger {
    inline constexpr bool file_log_enabled = false;
    inline constexpr bool debug_output_enabled = false;
    inline constexpr bool report_enabled = false;

    inline uintptr_t g_module_base = 0;
    inline DWORD g_module_size = 0;
    inline char g_module_name[MAX_PATH] = "hello";
    inline char g_module_path[MAX_PATH] = {};
    inline volatile LONG g_installed = 0;
    inline volatile LONG g_logging = 0;
    inline volatile LONG g_first_exception_logged = 0;
    inline PVOID g_vectored_handler = nullptr;
    using report_callback_t = void(*)(const char* code, const char* stage, const char* report);
    inline report_callback_t g_report_callback = nullptr;

    inline char g_crash_buffer[8192] = {};
    inline char g_first_exception_buffer[4096] = {};
    inline char g_message_buffer[2048] = {};
    struct fixed_buffer {
        char* data;
        size_t capacity;
        size_t length;
    };

    inline void reset_buffer(fixed_buffer& out) {
        out.length = 0;
        if (out.data && out.capacity != 0) {
            out.data[0] = '\0';
        }
    }

    inline void append_raw(fixed_buffer& out, const char* text) {
        if (!out.data || out.capacity == 0 || !text) return;
        if (out.length >= out.capacity - 1) return;

        const size_t remaining = out.capacity - out.length - 1;
        size_t count = 0;
        while (count < remaining && text[count] != '\0') {
            out.data[out.length + count] = text[count];
            ++count;
        }
        out.length += count;
        out.data[out.length] = '\0';
    }

    inline void appendf(fixed_buffer& out, const char* fmt, ...) {
        if (!out.data || out.capacity == 0 || !fmt) return;
        if (out.length >= out.capacity - 1) return;

        va_list args;
        va_start(args, fmt);
        const int written = vsnprintf(out.data + out.length, out.capacity - out.length, fmt, args);
        va_end(args);

        if (written <= 0) return;

        const size_t available = out.capacity - out.length - 1;
        out.length += (static_cast<size_t>(written) > available) ? available : static_cast<size_t>(written);
        out.data[out.length] = '\0';
    }

    inline const char* base_name(const char* path) {
        if (!path || !path[0]) return "unknown";

        const char* slash = strrchr(path, '\\');
        const char* slash2 = strrchr(path, '/');
        const char* last = slash;
        if (slash2 && (!last || slash2 > last)) {
            last = slash2;
        }

        return last ? last + 1 : path;
    }

    inline void ensure_log_dir() {
        if constexpr (!file_log_enabled) {
            return;
        }
        hello_paths::ensure_local_root();
    }

    inline void write_text_file(const char* path, const char* text, size_t text_size) {
        if constexpr (!file_log_enabled) {
            return;
        }
        if (!path || !path[0] || !text) return;

        HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return;

        DWORD written = 0;
        WriteFile(file, text, static_cast<DWORD>(text_size), &written, nullptr);
        CloseHandle(file);
    }

    inline void build_log_path(char* out, size_t out_size) {
        if (!out || out_size == 0) return;

        SYSTEMTIME st{};
        GetLocalTime(&st);
        const std::string root = hello_paths::local_root_string();
        snprintf(
            out,
            out_size,
            "%s\\crash_%04u-%02u-%02u_%02u-%02u-%02u_pid%lu_tid%lu.log",
            root.c_str(),
            st.wYear,
            st.wMonth,
            st.wDay,
            st.wHour,
            st.wMinute,
            st.wSecond,
            GetCurrentProcessId(),
            GetCurrentThreadId());
    }

    inline const char* exception_name(DWORD code) {
        switch (code) {
        case EXCEPTION_ACCESS_VIOLATION: return "access_violation";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "array_bounds_exceeded";
        case EXCEPTION_BREAKPOINT: return "breakpoint";
        case EXCEPTION_DATATYPE_MISALIGNMENT: return "datatype_misalignment";
        case EXCEPTION_FLT_DENORMAL_OPERAND: return "flt_denormal_operand";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "flt_divide_by_zero";
        case EXCEPTION_FLT_INEXACT_RESULT: return "flt_inexact_result";
        case EXCEPTION_FLT_INVALID_OPERATION: return "flt_invalid_operation";
        case EXCEPTION_FLT_OVERFLOW: return "flt_overflow";
        case EXCEPTION_FLT_STACK_CHECK: return "flt_stack_check";
        case EXCEPTION_FLT_UNDERFLOW: return "flt_underflow";
        case EXCEPTION_ILLEGAL_INSTRUCTION: return "illegal_instruction";
        case EXCEPTION_IN_PAGE_ERROR: return "in_page_error";
        case EXCEPTION_INT_DIVIDE_BY_ZERO: return "int_divide_by_zero";
        case EXCEPTION_INT_OVERFLOW: return "int_overflow";
        case EXCEPTION_INVALID_DISPOSITION: return "invalid_disposition";
        case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "noncontinuable_exception";
        case EXCEPTION_PRIV_INSTRUCTION: return "privileged_instruction";
        case EXCEPTION_SINGLE_STEP: return "single_step";
        case EXCEPTION_STACK_OVERFLOW: return "stack_overflow";
        default: return "unknown_exception";
        }
    }

    inline const char* access_kind(ULONG_PTR kind) {
        switch (kind) {
        case 0: return "read";
        case 1: return "write";
        case 8: return "execute";
        default: return "unknown";
        }
    }

    inline bool read_byte(uintptr_t address, unsigned char& value) {
        __try {
            value = *reinterpret_cast<unsigned char*>(address);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool read_u64(uintptr_t address, uint64_t& value) {
        __try {
            value = *reinterpret_cast<uint64_t*>(address);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline void append_code_bytes(fixed_buffer& out, uintptr_t address, size_t count) {
        if (address == 0 || count == 0) return;

        for (size_t i = 0; i < count; ++i) {
            unsigned char value = 0;
            if (i != 0) append_raw(out, " ");
            if (!read_byte(address + i, value)) {
                append_raw(out, "??");
                continue;
            }
            appendf(out, "%02X", value);
        }
    }

    inline bool is_self_address(uintptr_t address) {
        if (!address || !g_module_base) return false;
        if (g_module_size != 0) {
            return address >= g_module_base && address < g_module_base + g_module_size;
        }
        return address >= g_module_base && address < g_module_base + 0x08000000ull;
    }

    inline DWORD image_size_from_module(HMODULE module) {
        if (!module) return 0;

        __try {
            auto dos = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
            if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;

            auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<unsigned char*>(module) + dos->e_lfanew);
            if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;

            return nt->OptionalHeader.SizeOfImage;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    inline bool module_from_address(uintptr_t address, HMODULE& module, uintptr_t& base, DWORD& size, char* name, size_t name_size) {
        module = nullptr;
        base = 0;
        size = 0;
        if (name && name_size) name[0] = '\0';
        if (!address) return false;

        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQuery(reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) == 0 || !mbi.AllocationBase) {
            return false;
        }

        module = reinterpret_cast<HMODULE>(mbi.AllocationBase);
        char path[MAX_PATH] = {};
        if (GetModuleFileNameA(module, path, MAX_PATH) == 0) {
            return false;
        }

        base = reinterpret_cast<uintptr_t>(module);
        size = image_size_from_module(module);
        if (name && name_size) {
            strncpy_s(name, name_size, base_name(path), _TRUNCATE);
        }
        return true;
    }

    inline void append_module_address(fixed_buffer& out, uintptr_t address) {
        if (is_self_address(address)) {
            appendf(out, "%s+0x%llX", g_module_name, static_cast<unsigned long long>(address - g_module_base));
            return;
        }

        HMODULE module = nullptr;
        uintptr_t base = 0;
        DWORD size = 0;
        char name[MAX_PATH] = {};
        if (module_from_address(address, module, base, size, name, sizeof(name)) && base != 0 && name[0] != '\0') {
            appendf(out, "%s+0x%llX", name, static_cast<unsigned long long>(address - base));
            return;
        }

        appendf(out, "0x%llX", static_cast<unsigned long long>(address));
    }

    inline void remember_module(HMODULE module) {
        g_module_base = reinterpret_cast<uintptr_t>(module);
        g_module_size = image_size_from_module(module);

        if (module) {
            char path[MAX_PATH] = {};
            if (GetModuleFileNameA(module, path, MAX_PATH) > 0) {
                strncpy_s(g_module_path, path, _TRUNCATE);
                strncpy_s(g_module_name, base_name(path), _TRUNCATE);
            }
        }
    }

    inline void append_stack_dump(fixed_buffer& out, uintptr_t stack_pointer) {
        append_raw(out, "stack_qwords:\n");
        if (stack_pointer == 0) {
            append_raw(out, "  unavailable\n");
            return;
        }

        for (int i = 0; i < 32; ++i) {
            const uintptr_t address = stack_pointer + static_cast<uintptr_t>(i) * sizeof(uint64_t);
            uint64_t value = 0;
            appendf(out, "  +0x%03X 0x%016llX ", i * 8, static_cast<unsigned long long>(address));
            if (read_u64(address, value)) {
                appendf(out, "0x%016llX", static_cast<unsigned long long>(value));
                if (is_self_address(static_cast<uintptr_t>(value))) {
                    append_raw(out, " ");
                    append_module_address(out, static_cast<uintptr_t>(value));
                }
                append_raw(out, "\n");
            }
            else {
                append_raw(out, "????????????????\n");
            }
        }
    }

    inline void append_exception_report(fixed_buffer& out, const char* tag, EXCEPTION_POINTERS* exception, bool full) {
        EXCEPTION_RECORD* record = exception ? exception->ExceptionRecord : nullptr;
        CONTEXT context{};
        if (exception && exception->ContextRecord) {
            context = *exception->ContextRecord;
        }
        else {
            RtlCaptureContext(&context);
        }

        runtime_debug::last_exception_code = record ? record->ExceptionCode : 0;
        runtime_debug::last_exception_addr = record ? record->ExceptionAddress : nullptr;

        const uintptr_t exception_address = record ? reinterpret_cast<uintptr_t>(record->ExceptionAddress) : 0;

        append_raw(out, full ? "================ CRASH ================\n" : "================ FIRST EXCEPTION ================\n");
        appendf(out, "tag=%s\n", tag ? tag : "unknown");
        appendf(out, "section=%s\n", runtime_debug::last_section ? runtime_debug::last_section : "unknown");
        appendf(out, "pid=%lu tid=%lu\n", GetCurrentProcessId(), GetCurrentThreadId());
        if (full) {
            appendf(out, "uptime_ms=%llu\n", static_cast<unsigned long long>(GetTickCount64()));
        }
        appendf(out, "code=0x%08lX\n", record ? record->ExceptionCode : 0ul);
        appendf(out, "code_name=%s\n", record ? exception_name(record->ExceptionCode) : "unknown");
        appendf(out, "native_source=%s\n", runtime_debug::last_native_source ? runtime_debug::last_native_source : "none");
        appendf(out, "native_hash=0x%016llX\n", static_cast<unsigned long long>(runtime_debug::last_native_hash));
        appendf(out, "native_handler=0x%016llX\n", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(runtime_debug::last_native_handler)));
        appendf(out, "native_args=%u\n", runtime_debug::last_native_arg_count);
        appendf(out, "address=0x%llX\n", static_cast<unsigned long long>(exception_address));
        append_raw(out, "module=");
        append_module_address(out, exception_address);
        append_raw(out, "\n");

        if (record && (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || record->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) && record->NumberParameters >= 2) {
            appendf(out, "access=%s\n", access_kind(record->ExceptionInformation[0]));
            appendf(out, "access_address=0x%llX\n", static_cast<unsigned long long>(record->ExceptionInformation[1]));
        }

        if (g_module_base != 0) {
            appendf(out, "self_base=0x%llX\n", static_cast<unsigned long long>(g_module_base));
            appendf(out, "self_size=0x%lX\n", g_module_size);
            appendf(out, "self_name=%s\n", g_module_name);
            if (full && g_module_path[0]) {
                appendf(out, "self_path=%s\n", g_module_path);
            }
        }

#if defined(_M_X64)
        appendf(out, "RIP=0x%016llX RSP=0x%016llX RBP=0x%016llX\n", static_cast<unsigned long long>(context.Rip), static_cast<unsigned long long>(context.Rsp), static_cast<unsigned long long>(context.Rbp));
        appendf(out, "RAX=0x%016llX RBX=0x%016llX RCX=0x%016llX RDX=0x%016llX\n", static_cast<unsigned long long>(context.Rax), static_cast<unsigned long long>(context.Rbx), static_cast<unsigned long long>(context.Rcx), static_cast<unsigned long long>(context.Rdx));
        if (full) {
            appendf(out, "RSI=0x%016llX RDI=0x%016llX\n", static_cast<unsigned long long>(context.Rsi), static_cast<unsigned long long>(context.Rdi));
            appendf(out, "R8 =0x%016llX R9 =0x%016llX R10=0x%016llX R11=0x%016llX\n", static_cast<unsigned long long>(context.R8), static_cast<unsigned long long>(context.R9), static_cast<unsigned long long>(context.R10), static_cast<unsigned long long>(context.R11));
            appendf(out, "R12=0x%016llX R13=0x%016llX R14=0x%016llX R15=0x%016llX\n", static_cast<unsigned long long>(context.R12), static_cast<unsigned long long>(context.R13), static_cast<unsigned long long>(context.R14), static_cast<unsigned long long>(context.R15));
        }
        append_raw(out, "code_bytes=");
        append_code_bytes(out, static_cast<uintptr_t>(context.Rip), 16);
        append_raw(out, "\n");
        if (full) {
            append_stack_dump(out, static_cast<uintptr_t>(context.Rsp));
        }
#elif defined(_M_IX86)
        appendf(out, "EIP=0x%08lX ESP=0x%08lX EBP=0x%08lX\n", context.Eip, context.Esp, context.Ebp);
        appendf(out, "EAX=0x%08lX EBX=0x%08lX ECX=0x%08lX EDX=0x%08lX\n", context.Eax, context.Ebx, context.Ecx, context.Edx);
        append_raw(out, "code_bytes=");
        append_code_bytes(out, static_cast<uintptr_t>(context.Eip), 16);
        append_raw(out, "\n");
#endif

        append_raw(out, full ? "=======================================\n" : "=================================================\n");
    }

    inline void flush_report(const fixed_buffer& report) {
        if constexpr (!report_enabled) {
            return;
        }

        if constexpr (debug_output_enabled) {
            OutputDebugStringA(report.data);
        }
        if constexpr (file_log_enabled) {
            ensure_log_dir();

            char log_path[MAX_PATH] = {};
            build_log_path(log_path, sizeof(log_path));
            write_text_file(log_path, report.data, report.length);
            const std::string latest_path = (hello_paths::local_root() / "crash_latest.log").string();
            write_text_file(latest_path.c_str(), report.data, report.length);
        }
    }

    inline void append_user_crash_message(fixed_buffer& message, EXCEPTION_POINTERS* exception) {
        EXCEPTION_RECORD* record = exception ? exception->ExceptionRecord : nullptr;
        CONTEXT context{};
        if (exception && exception->ContextRecord) {
            context = *exception->ContextRecord;
        }
        else {
            RtlCaptureContext(&context);
        }

        const uintptr_t exception_address = record ? reinterpret_cast<uintptr_t>(record->ExceptionAddress) : 0;

        append_raw(message, "Hello has been crashed due to fatal error.\n\n");
        append_raw(message, "If this is the first time you've seen this message, restart Hello.\n");
        append_raw(message, "If this message appears again, contact support.\n\n");
        append_raw(message, "Debug logs:\n");
        appendf(message, "Build: %s\n", build_profile::debug ? "Debug" : "Release");
        appendf(message, "Section: %s\n\n", runtime_debug::last_section ? runtime_debug::last_section : "unknown");
        appendf(message, "Error code: 0x%08lX", record ? record->ExceptionCode : 0ul);
        if (record) {
            appendf(message, " (%s)", exception_name(record->ExceptionCode));
        }
        append_raw(message, "\n\n");
        if (record && (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || record->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) && record->NumberParameters >= 2) {
            appendf(message, "Access: %s 0x%llX\n\n", access_kind(record->ExceptionInformation[0]), static_cast<unsigned long long>(record->ExceptionInformation[1]));
        }
        appendf(message, "Address: 0x%llX (", static_cast<unsigned long long>(exception_address));
        append_module_address(message, exception_address);
        append_raw(message, ")\n\n");

#if defined(_M_X64)
        appendf(message, "RIP = 0x%016llX\n", static_cast<unsigned long long>(context.Rip));
        appendf(message, "RSP = 0x%016llX\n", static_cast<unsigned long long>(context.Rsp));
        appendf(message, "RBP = 0x%016llX\n\n", static_cast<unsigned long long>(context.Rbp));
        appendf(message, "RAX = 0x%016llX\n", static_cast<unsigned long long>(context.Rax));
        appendf(message, "RBX = 0x%016llX\n", static_cast<unsigned long long>(context.Rbx));
        appendf(message, "RCX = 0x%016llX\n", static_cast<unsigned long long>(context.Rcx));
        appendf(message, "RDX = 0x%016llX\n", static_cast<unsigned long long>(context.Rdx));
#elif defined(_M_IX86)
        appendf(message, "EIP = 0x%08lX\n", context.Eip);
        appendf(message, "ESP = 0x%08lX\n", context.Esp);
        appendf(message, "EBP = 0x%08lX\n\n", context.Ebp);
        appendf(message, "EAX = 0x%08lX\n", context.Eax);
        appendf(message, "EBX = 0x%08lX\n", context.Ebx);
        appendf(message, "ECX = 0x%08lX\n", context.Ecx);
        appendf(message, "EDX = 0x%08lX\n", context.Edx);
#endif

        if constexpr (file_log_enabled) {
            append_raw(message, "\nFull report: ");
            append_raw(message, (hello_paths::local_root() / "crash_latest.log").string().c_str());
        }
    }

    inline void show_crash_message(EXCEPTION_POINTERS* exception) {
        fixed_buffer message{ g_message_buffer, sizeof(g_message_buffer), 0 };
        reset_buffer(message);
        append_user_crash_message(message, exception);
        MessageBoxA(nullptr, message.data, "Oops...", MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
    }

    inline void emit_server_report(const char* code, const char* stage, const fixed_buffer& report);

    inline void write_first_exception_report(const char* tag, EXCEPTION_POINTERS* exception) {
        if constexpr (!report_enabled) {
            return;
        }

        fixed_buffer report{ g_first_exception_buffer, sizeof(g_first_exception_buffer), 0 };
        reset_buffer(report);
        append_exception_report(report, tag, exception, true);
        if constexpr (debug_output_enabled) {
            OutputDebugStringA(report.data);
        }
        if constexpr (file_log_enabled) {
            ensure_log_dir();
            const std::string latest_path = (hello_paths::local_root() / "first_exception_latest.log").string();
            write_text_file(latest_path.c_str(), report.data, report.length);
        }
    }

    inline void set_report_callback(report_callback_t callback) {
        g_report_callback = callback;
    }

    inline void emit_server_report(const char* code, const char* stage, const fixed_buffer& report) {
        report_callback_t callback = g_report_callback;
        if (!callback || !report.data || report.length == 0) return;
        callback(code ? code : "RCRASH", stage ? stage : "crash", report.data);
    }
    inline void log_exception(const char* tag, EXCEPTION_POINTERS* exception) {
        if (!exception) return;
        if constexpr (report_enabled) {
            fixed_buffer report{ g_crash_buffer, sizeof(g_crash_buffer), 0 };
            reset_buffer(report);
            append_exception_report(report, tag, exception, true);
            flush_report(report);
            emit_server_report("RCRASH", tag ? tag : "crash", report);
        }
        else {
            fixed_buffer report{ g_crash_buffer, sizeof(g_crash_buffer), 0 };
            reset_buffer(report);
            append_exception_report(report, tag, exception, true);
            emit_server_report("RCRASH", tag ? tag : "crash", report);
        }
    }

    inline LONG WINAPI vectored_exception_handler(EXCEPTION_POINTERS* exception) {
        EXCEPTION_RECORD* record = exception ? exception->ExceptionRecord : nullptr;
        if (!record) return EXCEPTION_CONTINUE_SEARCH;

        const DWORD code = record->ExceptionCode;
        const bool interesting =
            code == EXCEPTION_ACCESS_VIOLATION ||
            code == EXCEPTION_IN_PAGE_ERROR ||
            code == EXCEPTION_ILLEGAL_INSTRUCTION ||
            code == EXCEPTION_STACK_OVERFLOW ||
            code == EXCEPTION_PRIV_INSTRUCTION ||
            code == EXCEPTION_INT_DIVIDE_BY_ZERO ||
            code == EXCEPTION_FLT_INVALID_OPERATION ||
            code == EXCEPTION_NONCONTINUABLE_EXCEPTION ||
            code == EXCEPTION_INVALID_DISPOSITION ||
            code == 0xC0000409 ||
            code == 0xC0000374;

        if (interesting && InterlockedCompareExchange(&g_first_exception_logged, 1, 0) == 0) {
            write_first_exception_report("first_chance", exception);
            if constexpr (build_profile::debug) {
                log_exception("first_chance", exception);
            }
        }

        return EXCEPTION_CONTINUE_SEARCH;
    }

    inline bool should_show_crash_message(EXCEPTION_POINTERS* exception) {
        if constexpr (build_profile::debug) {
            return true;
        }

        EXCEPTION_RECORD* record = exception ? exception->ExceptionRecord : nullptr;
        if (!record) return true;
        if (record->ExceptionCode == 0xE0000001) return true;

        const uintptr_t address = reinterpret_cast<uintptr_t>(record->ExceptionAddress);
        return is_self_address(address);
    }

    inline LONG WINAPI unhandled_exception_filter(EXCEPTION_POINTERS* exception) {
        if (InterlockedCompareExchange(&g_logging, 1, 0) == 0) {
            log_exception("unhandled_exception", exception);
            if (should_show_crash_message(exception)) {
                show_crash_message(exception);
            }
        }
        return EXCEPTION_EXECUTE_HANDLER;
    }

    inline void log_c_runtime_failure(const char* tag) {
        if (InterlockedCompareExchange(&g_logging, 1, 0) != 0) {
            return;
        }

        CONTEXT context{};
        RtlCaptureContext(&context);

        EXCEPTION_RECORD record{};
        record.ExceptionCode = 0xE0000001;
#if defined(_M_X64)
        record.ExceptionAddress = reinterpret_cast<void*>(context.Rip);
#elif defined(_M_IX86)
        record.ExceptionAddress = reinterpret_cast<void*>(context.Eip);
#endif

        EXCEPTION_POINTERS pointers{};
        pointers.ExceptionRecord = &record;
        pointers.ContextRecord = &context;

        log_exception(tag, &pointers);
        if (should_show_crash_message(&pointers)) {
            show_crash_message(&pointers);
        }
    }

    inline void __cdecl terminate_handler() {
        log_c_runtime_failure("std_terminate");
        TerminateProcess(GetCurrentProcess(), 0xE0000001);
    }

    inline void __cdecl purecall_handler() {
        log_c_runtime_failure("purecall");
        TerminateProcess(GetCurrentProcess(), 0xE0000001);
    }

    inline void __cdecl invalid_parameter_handler(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t) {
        log_c_runtime_failure("invalid_parameter");
        TerminateProcess(GetCurrentProcess(), 0xE0000001);
    }

    inline void install() {
        if (InterlockedCompareExchange(&g_installed, 1, 0) != 0) {
            return;
        }

        // No vectored handler, no unhandled filter, no crash UI in release:
        // the cheat must not leave any trace (files, message boxes, debug out).
        (void)g_vectored_handler;
    }
}
