#pragma once

#include <Windows.h>
#include <mutex>
#include <set>
#include <vector>

namespace object_hash_registry {
    inline std::mutex g_mutex;
    inline std::set<DWORD> g_hashes;

    inline void clear_visible() {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_hashes.clear();
    }

    inline void observe(DWORD hash) {
        if (hash == 0) {
            return;
        }

        std::lock_guard<std::mutex> lock(g_mutex);
        g_hashes.insert(hash);
    }

    inline std::vector<DWORD> snapshot() {
        std::lock_guard<std::mutex> lock(g_mutex);
        return { g_hashes.begin(), g_hashes.end() };
    }
}
