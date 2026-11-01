#pragma once

#include <vector>
#include <mutex>
#include <string>
#include <cstdint>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <thread>
#include <sstream>
#include <Windows.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace ws_server {
    // Window focus flag shared with the WndProc hook (hooks.hpp). Kept here so
    // ws_bridge stays dependency-free; the log reader pauses while unfocused to
    // avoid contending with the client's own writes (voice path included).
    inline std::atomic<bool> g_window_focused{true};
    inline HWND g_window = nullptr;

    inline void set_window_focused(bool focused) {
        g_window_focused.store(focused, std::memory_order_release);
    }

    inline void set_window(HWND hwnd) {
        g_window = hwnd;
    }

    struct EspPlayer {
        int handle; std::string name, login, gender, social;
        int static_id, level, admin_level;
        std::string fraction; int fraction_id, family_id, leader_id;
        float hp, armor; uint32_t weapon_hash;
        bool is_admin, is_dead, is_media, is_tester, is_afk;
        bool is_cuffed = false, has_wear_armor = false;
        int wanted_level = 0;
        bool visible = true;
        uint16_t netid; float pos_x, pos_y, pos_z; uint64_t sequence;
        uint8_t bone_count, auto_relation; float bones[24][3];
    };

    struct MenuItem {
        std::string script, id, group, kind, label, tooltip, parent;
        std::vector<std::string> path, options, option_ids;
        json value; double min = 0, max = 0;
        bool visible = true, disabled = false; uint64_t order = 0;
    };
    struct MenuUpdate { std::string id; json patch; };

    inline std::mutex g_mutex;
    inline std::vector<EspPlayer> g_players;       // from GTA memory (bones/pos/hp)
    inline std::vector<EspPlayer> g_log_players;   // from client log (names/flags)
    inline std::string g_server_id;
    inline std::atomic<bool> g_log_running{false};
    inline std::thread g_log_thread;
    inline std::string g_line_buf;

    inline void set_players(std::vector<EspPlayer>&& players) {
        std::lock_guard<std::mutex> lk(g_mutex);
        g_players = std::move(players);
    }

    inline void copy_players(std::vector<EspPlayer>& out) {
        std::lock_guard<std::mutex> lk(g_mutex);

        out = g_players;

        if (!g_log_players.empty()) {
            for (auto& p : out) {
                const EspPlayer* match = nullptr;
                for (const auto& lp : g_log_players) {
                    if (p.netid > 0 && lp.netid > 0 && p.netid == lp.netid) { match = &lp; break; }
                }
                if (!match) {
                    for (const auto& lp : g_log_players) {
                        if (p.handle > 0 && lp.handle > 0 && p.handle == lp.handle) { match = &lp; break; }
                    }
                }
                if (!match) {
                    for (const auto& lp : g_log_players) {
                        if (p.static_id > 0 && lp.static_id > 0 && p.static_id == lp.static_id) { match = &lp; break; }
                    }
                }
                if (!match) continue;
                const EspPlayer& lp = *match;
                p.name        = lp.name;
                p.login       = lp.login;
                p.social      = lp.social;
                p.static_id   = lp.static_id;
                p.level       = lp.level;
                p.admin_level = lp.admin_level;
                p.fraction    = lp.fraction;
                p.fraction_id = lp.fraction_id;
                p.family_id   = lp.family_id;
                p.leader_id   = lp.leader_id;
                p.weapon_hash = lp.weapon_hash ? lp.weapon_hash : p.weapon_hash;
                p.is_admin    = lp.is_admin;
                p.is_dead     = lp.is_dead;
                p.is_media    = lp.is_media;
                p.is_tester   = lp.is_tester;
                p.is_afk      = lp.is_afk;
                p.is_cuffed   = lp.is_cuffed;
                p.has_wear_armor = lp.has_wear_armor;
                p.wanted_level = lp.wanted_level;
                p.auto_relation = lp.auto_relation;
            }
        }
    }

    inline std::vector<EspPlayer> get_players() {
        std::vector<EspPlayer> out;
        copy_players(out);
        return out;
    }

    inline bool has_resolved_server_id() { return !g_server_id.empty(); }
    inline std::string get_server_id() {
        std::lock_guard<std::mutex> lk(g_mutex);
        return g_server_id;
    }

    inline std::filesystem::path find_client_log() {
        if (const char* appdata = std::getenv("APPDATA")) {
            std::filesystem::path dir = std::filesystem::path(appdata) /
                "majestic-launcher" / "Multiplayer" / "logs";
            if (!std::filesystem::exists(dir)) return {};
            std::filesystem::path newest;
            std::filesystem::file_time_type newest_time;
            for (const auto& e : std::filesystem::directory_iterator(dir)) {
                const auto& p = e.path();
                if (p.filename().string().find("client_") == 0 && p.extension() == ".log") {
                    auto ft = std::filesystem::last_write_time(p);
                    if (newest.empty() || ft > newest_time) { newest = p; newest_time = ft; }
                }
            }
            return newest;
        }
        return {};
    }

    inline void parse_log_line(const std::string& line) {
        try {
            size_t brace = line.find('{');
            if (brace == std::string::npos) return;
            std::string json_part = line.substr(brace);

            auto j = json::parse(json_part, nullptr, false);
            if (j.is_discarded() || !j.contains("type")) return;
            std::string type = j["type"];

            if (type == "ESP_DATA" && j.contains("objects") && j["objects"].is_array()) {
                std::vector<EspPlayer> players;
                for (auto& obj : j["objects"]) {
                    EspPlayer p{};
                    const int raw_handle = obj.value("handle", 0);
                    p.handle      = raw_handle;
                    p.netid       = static_cast<uint16_t>(obj.value("netID", 0) & 0xFFFF);
                    p.name        = obj.value("name", std::string("Unknown"));
                    p.social      = obj.value("social", std::string());
                    p.static_id   = obj.value("staticID", 0);
                    p.level       = obj.value("level", 0);
                    p.admin_level = obj.value("adminLevel", 0);
                    p.fraction    = obj.value("fraction", std::string());
                    p.fraction_id = obj.value("fractionID", 0);
                    p.family_id   = obj.value("familyID", 0);
                    p.leader_id   = obj.value("leaderID", 0);
                    p.hp          = obj.value("hp", 100.0f);
                    p.armor       = obj.value("armor", 0.0f);
                    p.weapon_hash = obj.value("weaponHash", 0u);
                    p.is_admin    = obj.value("isAdmin", false);
                    p.is_dead     = obj.value("isDead", false);
                    p.is_media    = obj.value("isMedia", false);
                    p.is_tester   = obj.value("isTester", false);
                    p.is_afk      = obj.value("isAfk", false);
                    p.is_cuffed   = obj.value("cuffed", false);
                    p.has_wear_armor = obj.value("hasWearArmor", false);
                    p.wanted_level = obj.value("wantedLevel", 0);
                    p.pos_x       = obj.value("posX", 0.0f);
                    p.pos_y       = obj.value("posY", 0.0f);
                    p.pos_z       = obj.value("posZ", 0.0f);
                    p.bone_count  = static_cast<uint8_t>(obj.value("boneCount", 0));
                    p.auto_relation = static_cast<uint8_t>(obj.value("autoRel", 0));
                    if (obj.contains("bones") && obj["bones"].is_array()) {
                        size_t i = 0;
                        for (auto& b : obj["bones"]) {
                            if (i >= 24 || !b.is_array() || b.size() < 3) break;
                            p.bones[i][0] = b[0].get<float>();
                            p.bones[i][1] = b[1].get<float>();
                            p.bones[i][2] = b[2].get<float>();
                            ++i;
                        }
                    }
                    players.push_back(std::move(p));
                }
                std::lock_guard<std::mutex> lk(g_mutex);
                g_log_players = std::move(players);
            }

            if (type == "SERVER_INFO") {
                std::string srv = j.value("server", std::string());
                bool rec = j.value("recognized", false);
                if (rec && !srv.empty()) {
                    std::lock_guard<std::mutex> lk(g_mutex);
                    g_server_id = srv;
                }
            }
        } catch (...) {}
    }

    inline void log_thread_proc() {
        auto path = find_client_log();
        if (path.empty()) return;
        uintmax_t last = std::filesystem::file_size(path);

        while (g_log_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            // Skip file IO while the game window is not focused: the client is
            // still writing this log (voice events included) and frequent
            // open/read/close contends with it, which can stall the voice path
            // after minimizing (reported: mic stops working after minimize).
            if (!g_window_focused.load(std::memory_order_acquire)) {
                continue;
            }
            auto np = find_client_log();
            if (np != path) { path = np; last = 0; if (path.empty()) continue; }
            uintmax_t sz = std::filesystem::file_size(path);
            if (sz <= last) continue;

            std::ifstream f(path, std::ios::binary);
            if (!f.is_open()) continue;
            f.seekg(static_cast<std::streamoff>(last));
            std::string chunk(static_cast<size_t>(sz - last), '\0');
            f.read(chunk.data(), chunk.size());
            last = sz;

            std::istringstream ss(chunk);
            std::string line;
            while (std::getline(ss, line)) {
                if (line.empty()) continue;
                if (line.back() == '\r') line.pop_back();
                parse_log_line(line);
            }
        }
    }

    inline void start(int = 0) {
        g_log_running = true;
        g_log_thread = std::thread(log_thread_proc);
    }

    inline void stop() {
        g_log_running = false;
        if (g_log_thread.joinable()) g_log_thread.join();
    }

    inline bool is_running() {
        return g_log_running.load();
    }

    inline void send_config(bool,bool,bool,bool,bool,bool,bool,bool,bool,bool,bool,bool,bool) {}
    inline void send_menu_event(const std::string&, const std::string&, const json&) {}
    inline void unload_user_scripts_for_shutdown() {}
    inline void register_builtin_menu_item(MenuItem&&) {}
    inline std::vector<MenuUpdate> consume_builtin_menu_updates() { return {}; }
    inline bool send_execute(const std::string&, const std::string&, bool = false) { return false; }
    inline bool send_unload(const std::string&) { return false; }
    inline void set_menu_item_value(const std::string&, const std::string&, bool) {}
    inline std::vector<MenuItem> copy_menu_items() { return {}; }
    inline void flush_builtin_menu_snapshot() {}
}
