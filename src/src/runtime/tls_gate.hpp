#pragma once

#include <Windows.h>
#include <atomic>
#include <cstdint>

#include "platform/vmp.hpp"

namespace tls_gate {
    // magic value is randomized at build time by the patcher
    inline constexpr uint64_t magic = 0x0000000000000000ull;

#pragma pack(push, 1)
    struct init_data {
        uint64_t magic_value;
        uint8_t encrypted_init[64];
        uint32_t stage;
    };
#pragma pack(pop)

#pragma section(".rdata$zz", read, write)
    __declspec(allocate(".rdata$zz")) inline volatile init_data g_init_data {
        0,
        {},
        0,
    };

    inline std::atomic<bool> g_required{ false };
    inline std::atomic<bool> g_failed{ false };
    inline std::atomic<uintptr_t> g_module_base{ 0 };

    inline bool data_valid() {
        return g_init_data.magic_value == magic;
    }

    inline bool required() {
        return g_required.load(std::memory_order_acquire);
    }

    inline bool failed() {
        return g_failed.load(std::memory_order_acquire);
    }

    inline uint32_t stage() {
        return g_init_data.stage;
    }

    inline void mark_failed() {
        g_failed.store(true, std::memory_order_release);
    }

    inline void set_stage(uint32_t value) {
        g_init_data.stage = value;
    }

    inline uintptr_t module_base() {
        return g_module_base.load(std::memory_order_acquire);
    }

    inline void process_attach(HMODULE module) {
        HELLO_VMP_SCOPE_ULTRA("tls_gate.process_attach");
        g_module_base.store(reinterpret_cast<uintptr_t>(module), std::memory_order_release);
        g_required.store(data_valid(), std::memory_order_release);
        if (!data_valid()) {
            mark_failed();
        }
    }
}

// TLS callback removed - was a signature vector
// init now only via DllMain explicit call