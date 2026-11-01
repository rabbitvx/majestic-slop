#pragma once
#include <mutex>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <vector>

inline std::mutex g_nick_cache_mutex;
inline std::unordered_map<uint32_t, std::string> g_nick_cache;

inline std::unordered_map<uint32_t, std::string> g_nick_cache_by_altv_id;

struct NickEntry {
    std::string nick;
    uint32_t altv_id;
    uint32_t script_id;
};
inline std::vector<NickEntry> g_nick_entries;

inline std::string _get_altv_nick(uint32_t sid) {
    if (sid == 0) return "";
    std::lock_guard<std::mutex> lk(g_nick_cache_mutex);
    auto it = g_nick_cache.find(sid);
    if (it != g_nick_cache.end()) return it->second;
    auto it2 = g_nick_cache_by_altv_id.find(sid);
    if (it2 != g_nick_cache_by_altv_id.end()) return it2->second;
    return "";
}

inline std::string _get_altv_nick_any(uint32_t pedHandle) {
    if (pedHandle == 0) return "";
    std::lock_guard<std::mutex> lk(g_nick_cache_mutex);
    auto it = g_nick_cache.find(pedHandle);
    if (it != g_nick_cache.end()) return it->second;
    return "";
}

inline std::string _get_altv_nick_by_altv_id(uint32_t altv_id) {
    std::lock_guard<std::mutex> lk(g_nick_cache_mutex);
    auto it = g_nick_cache_by_altv_id.find(altv_id);
    if (it != g_nick_cache_by_altv_id.end()) return it->second;
    return "";
}
