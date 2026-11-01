#pragma once
// manual inline hook on NtWriteFile: minhook's MH_EnableHook returns
// MH_ERROR_NOT_EXECUTABLE on this function (hotpatch prologue in ntdll), so we
// patch the 16-byte prologue ourselves with a mov rax,imm64; jmp rax trampoline
// and call the saved prologue from our hook

#include <Windows.h>
#include <string>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <cstdio>
#include <cstdint>
#include "network/ws_bridge.hpp"

namespace log_hook {

    using NtWriteFile_t = NTSTATUS(NTAPI*)(HANDLE, HANDLE, PVOID, PVOID, PVOID, PVOID, ULONG, PLARGE_INTEGER, PULONG);
    inline NtWriteFile_t original_NtWriteFile = nullptr;

    inline std::atomic<bool> g_hooked{false};
    inline std::atomic<int> g_install_status{0};
    inline std::mutex g_path_cache_mutex;
    inline HANDLE g_log_handle = INVALID_HANDLE_VALUE;

    inline uint8_t g_saved_prologue[32] = {};
    inline void* g_target = nullptr;          // NtWriteFile address
    inline void* g_trampoline = nullptr;      // allocated copy: saved prologue + jmp back

    inline bool is_log_file_handle(HANDLE h) {
        if (h == INVALID_HANDLE_VALUE || !h) return false;
        if (h == g_log_handle) return true;
        char buf[MAX_PATH * 2] = {};
        DWORD len = GetFinalPathNameByHandleA(h, buf, sizeof(buf), 0);
        if (len == 0 || len >= sizeof(buf)) return false;
        std::string path(buf);
        const std::string needle = "\\Multiplayer\\logs\\client_";
        if (path.find(needle) != std::string::npos && path.find(".log") != std::string::npos) {
            std::lock_guard<std::mutex> lk(g_path_cache_mutex);
            g_log_handle = h;
            return true;
        }
        return false;
    }

    inline void swallow_esp_data(const char* data, size_t len) {
        static const char marker[] = "\"type\":\"ESP_DATA\"";
        const size_t mlen = sizeof(marker) - 1;
        size_t pos = 0;
        while (pos + mlen <= len) {
            const char* hit = std::search(data + pos, data + len, marker, marker + mlen);
            if (hit == data + len) break;
            const char* line_start = hit;
            while (line_start > data && line_start[-1] != '\n' && line_start[-1] != '\r') --line_start;
            const char* line_end = hit;
            while (line_end < data + len && *line_end != '\n' && *line_end != '\r') ++line_end;
            if (line_end > line_start) {
                std::string line(line_start, line_end);
                ws_server::parse_log_line(line);
            }
            pos = (line_end - data) + 1;
        }
    }

    inline NTSTATUS NTAPI hooked_NtWriteFile(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine,
                                             PVOID ApcContext, PVOID IoStatusBlock, PVOID Buffer,
                                             ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key) {
        if (Length > 0 && Buffer && is_log_file_handle(FileHandle)) {
            const char* data = static_cast<const char*>(Buffer);
            const char* marker = "\"type\":\"ESP_DATA\"";
            const size_t mlen = 16;
            if (std::search(data, data + Length, marker, marker + mlen) != data + Length) {
                swallow_esp_data(data, Length);
                if (IoStatusBlock) {
                    *reinterpret_cast<NTSTATUS*>(IoStatusBlock) = 0;
                    *reinterpret_cast<ULONG_PTR*>(reinterpret_cast<char*>(IoStatusBlock) + 8) = Length;
                }
                return 0;
            }
        }
        using Fn = NTSTATUS(NTAPI*)(HANDLE, HANDLE, PVOID, PVOID, PVOID, PVOID, ULONG, PLARGE_INTEGER, PULONG);
        return reinterpret_cast<Fn>(g_trampoline)(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock,
                                                  Buffer, Length, ByteOffset, Key);
    }

    inline bool install() {
        if (g_hooked.exchange(true)) return true;

        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (!ntdll) { g_install_status.store(1); g_hooked.store(false); return false; }
        g_target = reinterpret_cast<void*>(GetProcAddress(ntdll, "NtWriteFile"));
        if (!g_target) { g_install_status.store(2); g_hooked.store(false); return false; }

        // save prologue (16 bytes: 4c 8b d1 b8 08 00 00 00 f6 04 25 08 03 fe 7f 01)
        const size_t prologue_len = 16;
        memcpy(g_saved_prologue, g_target, prologue_len);

        // saved prologue + mov rax, target+16; jmp rax
        g_trampoline = VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!g_trampoline) { g_install_status.store(3); g_hooked.store(false); return false; }
        uint8_t* tp = static_cast<uint8_t*>(g_trampoline);
        memcpy(tp, g_saved_prologue, prologue_len);
        tp += prologue_len;
        uintptr_t resume = reinterpret_cast<uintptr_t>(g_target) + prologue_len;
        // mov rax, imm64 (48 B8) ; jmp rax (FF E0)
        *tp++ = 0x48; *tp++ = 0xB8;
        memcpy(tp, &resume, 8); tp += 8;
        *tp++ = 0xFF; *tp++ = 0xE0;

        // patch target: mov rax, hooked ; jmp rax ; nop*n
        uint8_t patch[16] = {};
        uint8_t* pp = patch;
        uintptr_t hook_addr = reinterpret_cast<uintptr_t>(&hooked_NtWriteFile);
        *pp++ = 0x48; *pp++ = 0xB8;
        memcpy(pp, &hook_addr, 8); pp += 8;
        *pp++ = 0xFF; *pp++ = 0xE0;
        while (pp < patch + prologue_len) *pp++ = 0x90; // nop pad

        DWORD old = 0;
        if (!VirtualProtect(g_target, prologue_len, PAGE_EXECUTE_READWRITE, &old)) {
            g_install_status.store(4); g_hooked.store(false); return false;
        }
        memcpy(g_target, patch, prologue_len);
        VirtualProtect(g_target, prologue_len, old, &old);
        FlushInstructionCache(GetCurrentProcess(), g_target, prologue_len);

        g_install_status.store(5);
        return true;
    }

    inline void uninstall() {
        if (!g_hooked.exchange(false)) return;
        if (g_target) {
            DWORD old = 0;
            if (VirtualProtect(g_target, 16, PAGE_EXECUTE_READWRITE, &old)) {
                memcpy(g_target, g_saved_prologue, 16);
                VirtualProtect(g_target, 16, old, &old);
                FlushInstructionCache(GetCurrentProcess(), g_target, 16);
            }
        }
        if (g_trampoline) { VirtualFree(g_trampoline, 0, MEM_RELEASE); g_trampoline = nullptr; }
        g_log_handle = INVALID_HANDLE_VALUE;
    }
}
