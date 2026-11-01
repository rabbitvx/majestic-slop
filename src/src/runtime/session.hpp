#pragma once

#include <Windows.h>
#include <bcrypt.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>

#pragma comment(lib, "bcrypt.lib")

#include "platform/vmp.hpp"

namespace runtime_session {
    struct state {
        bool ready = false;
        char product_session[33]{};
        char product_code[17]{};
        char tg_id[33]{};
        char hwid[129]{};
        std::array<unsigned char, 32> runtime_seed{};
        std::array<unsigned char, 32> server_nonce{};
        uint32_t rolling_counter = 0;
        char stage0_token[33]{};
        std::array<unsigned char, 32> pointer_xor_seed{};
        std::array<unsigned char, 32> preinit_client_nonce{};
        std::array<unsigned char, 32> preinit_server_nonce{};
        uint64_t preinit_expires_at_ms = 0;
        char bridge_proof[65]{};
        char user_name[65]{};
        char expires_at[33]{};
        bool security_latch = false;
        uint32_t security_flags = 0;
        uint32_t security_counter = 0;
        uint32_t encrypted_feature_mask = 0;
    };

    struct integrity_report_snapshot {
        bool ready = false;
        char product_session[33]{};
        char product_code[17]{};
        char hwid[129]{};
        std::array<unsigned char, 32> runtime_seed{};
        uint32_t flags = 0;
        uint32_t source = 0;
        uint32_t counter = 0;
    };

    inline SRWLOCK g_lock = SRWLOCK_INIT;
    inline state g_state{};
    inline std::array<unsigned char, 32> g_state_key{};
    inline bool g_state_key_ready = false;
    inline bool g_state_present = false;
    inline std::atomic<bool> g_unload_requested{ false };

    inline uint64_t mix64(uint64_t value) {
        value ^= value >> 30;
        value *= 0xbf58476d1ce4e5b9ull;
        value ^= value >> 27;
        value *= 0x94d049bb133111ebull;
        value ^= value >> 31;
        return value;
    }

    inline void ensure_state_key() {
        if (g_state_key_ready) return;
        if (BCryptGenRandom(nullptr, g_state_key.data(), static_cast<ULONG>(g_state_key.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) >= 0) {
            g_state_key_ready = true;
            return;
        }
        LARGE_INTEGER counter{};
        QueryPerformanceCounter(&counter);
        uint64_t stack_marker = reinterpret_cast<uint64_t>(&counter);
        uint64_t seed = mix64(static_cast<uint64_t>(counter.QuadPart) ^ stack_marker ^ reinterpret_cast<uint64_t>(GetModuleHandleA("ntdll.dll")) ^ reinterpret_cast<uint64_t>(GetModuleHandleA("kernel32.dll")) ^ GetCurrentProcessId());
        for (size_t i = 0; i < g_state_key.size(); i += 8) {
            seed = mix64(seed + i + 0x9e3779b97f4a7c15ull);
            memcpy(g_state_key.data() + i, &seed, sizeof(seed));
        }
        g_state_key_ready = true;
    }

    inline void crypt_state(state& value) {
        ensure_state_key();
        auto* bytes = reinterpret_cast<unsigned char*>(&value);
        for (size_t i = 0; i < sizeof(value); ++i) {
            bytes[i] ^= static_cast<unsigned char>(g_state_key[i % g_state_key.size()] + (i * 37u));
        }
    }

    inline state decrypt_locked() {
        state copy = g_state;
        if (g_state_present) crypt_state(copy);
        return g_state_present ? copy : state{};
    }

    inline void store_locked(const state& value) {
        g_state = value;
        crypt_state(g_state);
        g_state_present = true;
    }

    inline void request_unload() {
        g_unload_requested.store(true, std::memory_order_release);
    }

    inline bool unload_requested() {
        return g_unload_requested.load(std::memory_order_acquire);
    }

    inline void poison_state(state& value, uint32_t flags, uint32_t source) {
        HELLO_VMP_SCOPE_MUTATION("runtime_session.poison_state");
        value.security_latch = true;
        value.security_flags |= flags;
        value.security_counter += 1;
        uint64_t lane = mix64((static_cast<uint64_t>(flags) << 32) ^ source ^ value.security_counter ^ 0x9e3779b97f4a7c15ull);
        for (size_t index = 0; index < value.pointer_xor_seed.size(); ++index) {
            if ((index & 7u) == 0u) lane = mix64(lane + index + flags);
            value.pointer_xor_seed[index] ^= static_cast<unsigned char>(lane >> ((index & 7u) * 8));
        }
    }

    inline integrity_report_snapshot terminal_integrity_event(uint32_t flags, uint32_t source) {
        HELLO_VMP_SCOPE_ULTRA("runtime_session.terminal_integrity_event");
        integrity_report_snapshot report{};
        AcquireSRWLockExclusive(&g_lock);
        state copy = decrypt_locked();
        if (copy.ready && copy.product_session[0] && copy.product_code[0] && copy.hwid[0]) {
            report.ready = true;
            strncpy_s(report.product_session, copy.product_session, _TRUNCATE);
            strncpy_s(report.product_code, copy.product_code, _TRUNCATE);
            strncpy_s(report.hwid, copy.hwid, _TRUNCATE);
            report.runtime_seed = copy.runtime_seed;
        }
        poison_state(copy, flags, source);
        report.flags = flags;
        report.source = source;
        report.counter = copy.security_counter;
        store_locked(copy);
        g_unload_requested.store(true, std::memory_order_release);
        ReleaseSRWLockExclusive(&g_lock);
        return report;
    }

    inline void set(const state& value) {
        AcquireSRWLockExclusive(&g_lock);
        store_locked(value);
        ReleaseSRWLockExclusive(&g_lock);
    }

    inline bool is_ready() {
        AcquireSRWLockShared(&g_lock);
        const bool ready = decrypt_locked().ready;
        ReleaseSRWLockShared(&g_lock);
        return ready;
    }

    inline state snapshot() {
        AcquireSRWLockShared(&g_lock);
        const state copy = decrypt_locked();
        ReleaseSRWLockShared(&g_lock);
        return copy;
    }

    inline void set_bridge_proof(const char* proof) {
        HELLO_VMP_SCOPE_VIRTUALIZATION("runtime_session.set_bridge_proof");
        AcquireSRWLockExclusive(&g_lock);
        state copy = decrypt_locked();
        strncpy_s(copy.bridge_proof, proof ? proof : "", _TRUNCATE);
        store_locked(copy);
        ReleaseSRWLockExclusive(&g_lock);
    }

    inline void set_profile(const char* user_name_value, const char* expires_at_value) {
        HELLO_VMP_SCOPE_VIRTUALIZATION("runtime_session.set_profile");
        AcquireSRWLockExclusive(&g_lock);
        state copy = decrypt_locked();
        strncpy_s(copy.user_name, user_name_value ? user_name_value : "", _TRUNCATE);
        strncpy_s(copy.expires_at, expires_at_value ? expires_at_value : "", _TRUNCATE);
        store_locked(copy);
        ReleaseSRWLockExclusive(&g_lock);
    }

    inline std::string bridge_proof() {
        return snapshot().bridge_proof;
    }

    inline std::string user_name() {
        return snapshot().user_name;
    }

    inline std::string expires_at() {
        return snapshot().expires_at;
    }

    inline int expires_in_days() {
        const std::string value = expires_at();
        if (value.size() < 10) return 0;
        SYSTEMTIME st{};
        st.wYear = static_cast<WORD>(std::atoi(value.substr(0, 4).c_str()));
        st.wMonth = static_cast<WORD>(std::atoi(value.substr(5, 2).c_str()));
        st.wDay = static_cast<WORD>(std::atoi(value.substr(8, 2).c_str()));
        FILETIME ft{};
        if (!SystemTimeToFileTime(&st, &ft)) return 0;
        ULARGE_INTEGER end{};
        end.LowPart = ft.dwLowDateTime;
        end.HighPart = ft.dwHighDateTime;
        FILETIME now_ft{};
        GetSystemTimeAsFileTime(&now_ft);
        ULARGE_INTEGER now{};
        now.LowPart = now_ft.dwLowDateTime;
        now.HighPart = now_ft.dwHighDateTime;
        if (end.QuadPart <= now.QuadPart) return 0;
        const unsigned long long day_ticks = 24ull * 60ull * 60ull * 10000000ull;
        return static_cast<int>((end.QuadPart - now.QuadPart + day_ticks - 1) / day_ticks);
    }

    inline uint32_t state_key_dword() {
        ensure_state_key();
        uint32_t dword = 0;
        memcpy(&dword, g_state_key.data(), sizeof(dword));
        return dword;
    }

    inline uint32_t encrypt_feature_mask(uint32_t mask) {
        return mask ^ state_key_dword();
    }

    inline uint32_t decrypt_feature_mask(uint32_t encrypted) {
        return encrypted ^ state_key_dword();
    }

    inline uint32_t feature_mask() {
        return decrypt_feature_mask(snapshot().encrypted_feature_mask);
    }

    inline void set_feature_mask(uint32_t mask) {
        HELLO_VMP_SCOPE_VIRTUALIZATION("runtime_session.set_feature_mask");
        AcquireSRWLockExclusive(&g_lock);
        state copy = decrypt_locked();
        copy.encrypted_feature_mask = encrypt_feature_mask(mask);
        store_locked(copy);
        ReleaseSRWLockExclusive(&g_lock);
    }

    inline void clear() {
        HELLO_VMP_SCOPE_VIRTUALIZATION("runtime_session.clear");
        AcquireSRWLockExclusive(&g_lock);
        SecureZeroMemory(&g_state, sizeof(g_state));
        g_state_present = false;
        g_unload_requested.store(false, std::memory_order_release);
        ReleaseSRWLockExclusive(&g_lock);
    }
}
