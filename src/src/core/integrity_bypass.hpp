#pragma once
// Runtime integrity-check bypass

#include <Windows.h>
#include <Psapi.h>
#include <atomic>
#include <cstdint>

#pragma comment(lib, "psapi.lib")

namespace integrity_bypass {

    // RVA offsets in majestic-client.dll (from decrypted dump analysis)
    // Patch 1: je -> jmp at the "integrity check failed" branch
    //   Original: 0F 84 BE 06 00 00  (je +0x6BE  -> OK path when eax==0)
    //   Patched : 90 E9 BE 06 00 00  (nop; jmp +0x6BE -> ALWAYS take OK path)
    static constexpr uintptr_t kPatch1Rva = 0x3F9734;
    static const BYTE kPatch1Orig[6] = { 0x0F, 0x84, 0xBE, 0x06, 0x00, 0x00 };
    static const BYTE kPatch1New[6]  = { 0x90, 0xE9, 0xBE, 0x06, 0x00, 0x00 };

    // Patch 2: force the rate-limit skip branch to always trigger
    //   Original: 0F 8F 97 11 00 00  (jg +0x1197 -> skip check)
    //   Patched : E9 97 11 00 00 90  (jmp +0x1197 -> always skip)
    static constexpr uintptr_t kPatch2Rva = 0x3F96B2;
    static const BYTE kPatch2Orig[6] = { 0x0F, 0x8F, 0x97, 0x11, 0x00, 0x00 };
    static const BYTE kPatch2New[6]  = { 0xE9, 0x97, 0x11, 0x00, 0x00, 0x90 };

    inline std::atomic<bool> g_applied{false};
    inline uintptr_t g_module_base = 0;
    inline DWORD g_module_size = 0;

    inline bool find_majestic_module(uintptr_t& base, DWORD& size) {
        HMODULE hMods[1024]; DWORD cb;
        if (!EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cb))
            return false;
        for (unsigned i = 0; i < cb / sizeof(HMODULE); i++) {
            char name[MAX_PATH];
            if (GetModuleFileNameA(hMods[i], name, sizeof(name))) {
                if (strstr(name, "majestic-client")) {
                    MODULEINFO mi;
                    if (GetModuleInformation(GetCurrentProcess(), hMods[i], &mi, sizeof(mi))) {
                        base = reinterpret_cast<uintptr_t>(mi.lpBaseOfDll);
                        size = mi.SizeOfImage;
                        return true;
                    }
                }
            }
        }
        return false;
    }

    inline bool safe_read(void* dst, uintptr_t src, size_t len) {
        return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(src), dst, len, nullptr) != 0;
    }

    inline bool apply_patch(uintptr_t rva, const BYTE* orig, const BYTE* patch, size_t len) {
        uintptr_t addr = g_module_base + rva;
        if (addr + len > g_module_base + g_module_size) return false;

        BYTE current[8] = {};
        if (!safe_read(current, addr, len)) return false;
        if (memcmp(current, orig, len) != 0) {
            if (memcmp(current, patch, len) == 0) return true;
            return false; // signature mismatch — do NOT patch unknown bytes
        }

        DWORD old_prot;
        if (!VirtualProtect(reinterpret_cast<LPVOID>(addr), len, PAGE_EXECUTE_READWRITE, &old_prot))
            return false;
        memcpy(reinterpret_cast<void*>(addr), patch, len);
        VirtualProtect(reinterpret_cast<LPVOID>(addr), len, old_prot, &old_prot);
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(addr), len);
        return true;
    }

    // applies both patches. must be called AFTER majestic dll is loaded and vmp has decrypted the code section
    inline bool apply() {
        if (g_applied.load(std::memory_order_acquire)) return true;
        if (!find_majestic_module(g_module_base, g_module_size)) return false;

        bool ok1 = apply_patch(kPatch1Rva, kPatch1Orig, kPatch1New, sizeof(kPatch1New));
        bool ok2 = apply_patch(kPatch2Rva, kPatch2Orig, kPatch2New, sizeof(kPatch2New));

        if (ok1 && ok2) {
            g_applied.store(true, std::memory_order_release);
        }
        return g_applied.load();
    }

    inline bool is_applied() { return g_applied.load(std::memory_order_acquire); }
}
