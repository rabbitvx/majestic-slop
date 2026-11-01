#pragma once

#include <Windows.h>
#include <cstdint>
#include <cstring>

namespace memory {
    struct page_cache_entry {
        uintptr_t begin = 0;
        uintptr_t end = 0;
    };

    inline bool readable_page(DWORD protect) {
        if (protect & (PAGE_GUARD | PAGE_NOACCESS)) {
            return false;
        }

        switch (protect & 0xff) {
        case PAGE_READONLY:
        case PAGE_READWRITE:
        case PAGE_WRITECOPY:
        case PAGE_EXECUTE_READ:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
            return true;
        default:
            return false;
        }
    }

    inline bool in_user_address_range(uintptr_t address, size_t size) {
        constexpr uintptr_t k_min_user_address = 0x10000;
        constexpr uintptr_t k_max_user_address = 0x00007FFFFFFFFFFF;

        return address >= k_min_user_address && address <= k_max_user_address && size != 0 && size - 1 <= k_max_user_address - address;
    }

    inline bool readable_region_end(uintptr_t address, uintptr_t& region_end) {
        thread_local page_cache_entry cache[64]{};
        thread_local size_t next_cache_entry = 0;

        for (const auto& entry : cache) {
            if (address >= entry.begin && address < entry.end) {
                region_end = entry.end;
                return true;
            }
        }

        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQuery(reinterpret_cast<const void*>(address), &mbi, sizeof(mbi)) == 0) {
            return false;
        }

        const uintptr_t begin = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        const uintptr_t end = begin + mbi.RegionSize;
        if (end <= address || mbi.State != MEM_COMMIT || !readable_page(mbi.Protect)) {
            return false;
        }

        cache[next_cache_entry] = { begin, end };
        next_cache_entry = (next_cache_entry + 1) % 64;

        region_end = end;
        return true;
    }

    inline bool valid_range(const void* ptr, size_t size) {
        const uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
        if (!in_user_address_range(address, size)) {
            return false;
        }

        const uintptr_t end = address + size;
        uintptr_t current = address;
        while (current < end) {
            uintptr_t region_end = 0;
            if (!readable_region_end(current, region_end)) {
                return false;
            }

            current = region_end;
        }

        return true;
    }

    inline bool valid(const void* ptr) {
        return valid_range(ptr, 1);
    }

    template <typename T>
    inline T read(uintptr_t address, T fallback = {}) {
        if (!valid_range(reinterpret_cast<const void*>(address), sizeof(T))) {
            return fallback;
        }

        __try {
            return *reinterpret_cast<T*>(address);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return fallback;
        }
    }

    template <typename T>
    inline bool write(uintptr_t address, const T& value) {
        if (!valid_range(reinterpret_cast<const void*>(address), sizeof(T))) {
            return false;
        }

        __try {
            *reinterpret_cast<T*>(address) = value;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool patch(uintptr_t address, const void* data, size_t size) {
        DWORD old_protect = 0;
        if (!VirtualProtect(reinterpret_cast<void*>(address), size, PAGE_EXECUTE_READWRITE, &old_protect)) {
            return false;
        }

        std::memcpy(reinterpret_cast<void*>(address), data, size);
        VirtualProtect(reinterpret_cast<void*>(address), size, old_protect, &old_protect);
        return true;
    }
}

#ifndef IsValidPtr
#define IsValidPtr(x) memory::valid(reinterpret_cast<const void*>(x))
#endif
