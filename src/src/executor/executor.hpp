#pragma once

#include <Windows.h>
#include <Psapi.h>
#include <atomic>
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <sstream>

#include "core/integrity_bypass.hpp"
#include "executor/payload_embedded.hpp"
#include "network/ws_bridge_server.hpp"

#pragma comment(lib, "psapi.lib")

namespace executor {

    inline std::atomic<bool> g_patched{false};
    inline std::atomic<bool> g_running{false};
    inline DWORD64 g_base = 0;
    inline HANDLE g_thread = nullptr;

    inline std::string get_payload() {
        char path[MAX_PATH];
        GetModuleFileNameA(nullptr, path, sizeof(path));
        std::string dir(path);
        dir = dir.substr(0, dir.find_last_of("\\/") + 1);
        dir += "payload.js";

        std::ifstream file(dir);
        if (file.is_open()) {
            std::stringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }

        return executor_payload::source();
    }

    // aobscan
    inline bool find_majestic_module(DWORD64& base, DWORD& size) {
        HMODULE hMods[1024]; DWORD cb;
        if (!EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cb)) return false;
        for (unsigned i = 0; i < cb / sizeof(HMODULE); i++) {
            char name[MAX_PATH];
            if (GetModuleFileNameA(hMods[i], name, sizeof(name))) {
                if (strstr(name, "majestic-client")) {
                    MODULEINFO mi;
                    GetModuleInformation(GetCurrentProcess(), hMods[i], &mi, sizeof(mi));
                    base = (DWORD64)mi.lpBaseOfDll;
                    size  = mi.SizeOfImage;
                    return true;
                }
            }
        }
        return false;
    }

    inline DWORD64 resolve_rip(DWORD64 addr, int off) {
        int32_t rel = *(int32_t*)(addr + off);
        return addr + off + 4 + rel;
    }

    inline bool find_script_buffer(DWORD64 base, DWORD size, DWORD64& out_data, DWORD64& out_size_off) {
        // Full 105-byte AOB from original noctua executor
        const char* sig =
            "\x48\x8B\x0D\x00\x00\x00\x00\x48\x8D\x50\x00\x48\x81\xFA\x00\x00\x00\x00\x72\x00"
            "\x4C\x8B\x41\x00\x48\x83\xC1\x00\x4C\x29\xC1\x48\x83\xF9\x00\x73\x00\x48\x83\xC0\x00"
            "\x48\x89\xC2\x4C\x89\xC1\xE8\x00\x00\x00\x00"
            "\x48\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\x48\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00"
            "\xC6\x05\x00\x00\x00\x00\x00\x48\x83\xC4\x00\xC3\xFF\x15\x00\x00\x00\x00"
            "\xCC\xCC\xCC\x56\x57\x53\x48\x83\xEC\x00\x48\x8B\x05";
        const char* mask =
            "xxx????xxx?xxx????x?xxx?xxx?xxxxxx?x?xxx?xxxxxxx????xxx????????xxx????????xx?????xxx?xxx????xxxxxxxxx?xxx";

        size_t len = 105; // strlen(mask)

        const BYTE* mem = (const BYTE*)base;
        for (DWORD i = 0; i + len < size; i++) {
            bool ok = true;
            for (size_t j = 0; j < len; j++) {
                if (mask[j] == 'x' && mem[i+j] != (BYTE)sig[j]) { ok = false; break; }
            }
            if (ok) {
                DWORD64 data_off = resolve_rip(base + i, 3) - base;
                out_data      = data_off;
                out_size_off  = data_off + 0x10;
                return true;
            }
        }
        return false;
    }

    inline void apply_integrity_bypass_safe() {
        // SEH-only helper: __try cannot be in a function with C++ object unwinding
        __try { integrity_bypass::apply(); }
        __except(EXCEPTION_EXECUTE_HANDLER) {}
    }

    inline void monitor_thread() {
        DWORD64 base; DWORD size;
        for (int w = 0; w < 600 && g_running; w++) {
            if (find_majestic_module(base, size)) break;
            Sleep(100);
        }
        if (!g_running) return;

        g_base = base;

        // The payload pushes ESP_DATA over ws://127.0.0.1:45678
        ws_bridge_server::start();

        apply_integrity_bypass_safe();

        DWORD64 data_off, size_off;
        for (int attempt = 0; attempt < 30000 && g_running && !g_patched; attempt++) {
            if (!find_script_buffer(base, size, data_off, size_off)) {
                Sleep(20); continue;
            }

            DWORD64* pData = (DWORD64*)(base + data_off);
            size_t*  pSize = (size_t*)(base + size_off);

            DWORD64 dataPtr = *pData;
            size_t  sizeVal = *pSize;

            if (dataPtr > 0x10000 && dataPtr < 0x00007FFFFFFFFFFF &&
                sizeVal > 100000 && sizeVal < 100000000) {

                char* scriptData = (char*)dataPtr;
                MEMORY_BASIC_INFORMATION mbi;
                if (VirtualQuery(scriptData, &mbi, sizeof(mbi)) &&
                    mbi.State == MEM_COMMIT && mbi.RegionSize >= sizeVal) {
                    if (sizeVal >= 18 && memcmp(scriptData, "// payload guard", 16) == 0) {
                        g_patched = true;
                        return;
                    }

                    std::string payload = get_payload();
                    if (payload.empty()) { Sleep(20); continue; }

                    size_t payloadLen = payload.size();
                    size_t newSize = payloadLen + sizeVal + 1;

                    char* newBuf = (char*)VirtualAlloc(nullptr, newSize,
                        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                    if (!newBuf) { Sleep(20); continue; }

                    memcpy(newBuf, "// payload guard\n", 17);
                    memcpy(newBuf + 17, payload.c_str(), payloadLen);
                    memcpy(newBuf + 17 + payloadLen, "\n", 1);
                    memcpy(newBuf + 18 + payloadLen, scriptData, sizeVal);
                    newBuf[newSize - 1] = '\0';

                    *pData = (DWORD64)newBuf;
                    *pSize = newSize;
                    g_patched = true;
                    return;
                }
            }
            Sleep(20);
        }
    }

    inline void start() {
        if (g_running.exchange(true)) return;
        g_thread = CreateThread(nullptr, 0,
            [](LPVOID) -> DWORD { monitor_thread(); return 0; },
            nullptr, 0, nullptr);
    }

    inline void shutdown() {
        g_running = false;
        ws_bridge_server::stop();
        if (g_thread) {
            WaitForSingleObject(g_thread, 3000);
            CloseHandle(g_thread);
            g_thread = nullptr;
        }
    }

    inline bool is_patched() { return g_patched; }
}
