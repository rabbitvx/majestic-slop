#pragma once
#include "core/imports.h"
#include "platform/debug.hpp"
#include "native/database.hpp"
#include "features/misc/binds.hpp"
#include "features/misc/core.hpp"
#include "features/misc/game_tick.hpp"
#include "config/bind_state.hpp"
#include "features/visuals/widgets.hpp"
#include "features/aimbot/aim.hpp"
#include "ui/menu/menu.hpp"
#include "network/nick_cache.h"
#include "network/ws_bridge.hpp"
#include "network/playerlist_snapshot.hpp"
#include "runtime/player_marks.hpp"
#include "native/native_invoke.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <limits>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <cstdlib>

namespace game {
std::unordered_map<uintptr_t, ws_server::EspPlayer> network_cache;
    int menu_key = VK_END;
    int game_ticks = 0;
    float tick_ms = 0.f;
    bool custom_fov_active = false;
    float custom_fov_default = 0.f;
    bool custom_aspect_active = false;
    float custom_aspect_default = 0.f;
    constexpr float k_custom_fov_fallback = 60.f;
    constexpr float k_custom_fov_min = 30.f;
    constexpr float k_custom_fov_max = 120.f;
    bool visible_peds[1024] = {};
    DWORD freemode_f = 0x9C9EFFD8;
    DWORD freemode_m = 0x705E61F2;
    bool isValidPlayer(DWORD hash, CObject* player) {
        if (!config::get("misc", "misc_show_npcs", 0) && !config::get("misc", "misc_npc_only", 0)) {
            return hash == freemode_f || hash == freemode_m;
        }
        else if (config::get("misc", "misc_show_npcs", 0) && !config::get("misc", "misc_npc_only", 0)) {
            return true;
        }
        else if (config::get("misc", "misc_npc_only", 0)) {
            return hash != freemode_f && hash != freemode_m;
        }
        return false;
    }
    int update_calls = 0;
    
    inline bool get_ped_hp(CObject* ped, float* outHP) {
        __try {
            if (!ped) return false;
            *outHP = ped->HP;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool get_entity_distance(CObject* a, CObject* b, float* outDist) {
        __try {
            if (!a || !b) return false;
            *outDist = get_distance(a->fPosition, b->fPosition);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool get_entity_pos(CObject* ped, Vector3* outPos) {
        __try {
            if (!ped) return false;
            *outPos = ped->fPosition;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool get_hp(CObject* ped, float* outHP) {
        __try {
            if (!ped) return false;
            *outHP = ped->HP;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool get_alpha(CObject* ped, float* out) {
        __try {
            if (!ped) return false;
            *out = ped->GetAlpha();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool set_alpha(CObject* ped, float val) {
        __try {
            if (!ped) return false;
            ped->SetAlpha(val);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool get_model_hash(CObject* ped, DWORD* outHash) {
        __try {
            if (!ped) return false;
            CModelInfo* modelInfo = ped->ModelInfo();
            if (!modelInfo) return false;
            *outHash = modelInfo->GetHash();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline int pointer_to_entity_handle(uintptr_t ptr) {
        __try {
            if (!pointer_to_handle || !ptr) {
                return 0;
            }

            return pointer_to_handle(ptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    inline int get_network_id_from_entity_handle(int entityHandle) {
        __try {
            if (entityHandle <= 0) {
                return 0;
            }

            const int netId = native::network::network_get_network_id_from_entity(entityHandle);
            return netId > 0 ? netId : 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    inline int get_entity_handle_from_network_id(int netId) {
        __try {
            if (netId <= 0) {
                return 0;
            }

            return native::network::network_get_entity_from_network_id(netId);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    inline void ResetAltvPedData(DataPed& pedData, float pedHP) {
        pedData.altv_nick[0] = '\0';
        pedData.altv_social[0] = '\0';
        pedData.altv_fraction[0] = '\0';
        pedData.altv_static_id = 0;
        pedData.altv_dynamic_id = 0;
        pedData.altv_fraction_id = 0;
        pedData.altv_family_id = 0;
        pedData.altv_leader_id = 0;
        pedData.altv_level = 0;
        pedData.altv_admin_level = 0;
        pedData.altv_hp = pedHP - 100.f;
        pedData.altv_armor = 0.f;
        pedData.altv_weapon_hash = 0;
        pedData.altv_is_admin = false;
        pedData.altv_is_dead = (pedHP <= 0.f);
        pedData.altv_is_media = false;
        pedData.altv_is_tester = false;
        pedData.altv_is_afk = false;
        pedData.altv_is_cuffed = false;
        pedData.altv_has_wear_armor = false;
        pedData.altv_wanted_level = 0;
        pedData.has_ws_data = false;
        pedData.has_current_ws_data = false;
    }

    inline void ApplyWsPlayerData(const ws_server::EspPlayer& netObj, DataPed& pedData, bool current_snapshot) {
        size_t cplen = netObj.name.size() < 63 ? netObj.name.size() : 63;
        memcpy(pedData.altv_nick, netObj.name.c_str(), cplen);
        pedData.altv_nick[cplen] = '\0';

        size_t slen = netObj.social.size() < 63 ? netObj.social.size() : 63;
        memcpy(pedData.altv_social, netObj.social.c_str(), slen);
        pedData.altv_social[slen] = '\0';

        pedData.altv_static_id = netObj.static_id;
        pedData.altv_dynamic_id = static_cast<int>(netObj.netid);
        pedData.altv_level = netObj.level;
        pedData.altv_admin_level = netObj.admin_level;

        size_t flen = netObj.fraction.size() < 31 ? netObj.fraction.size() : 31;
        memcpy(pedData.altv_fraction, netObj.fraction.c_str(), flen);
        pedData.altv_fraction[flen] = '\0';
        pedData.altv_fraction_id = netObj.fraction_id;
        pedData.altv_family_id = netObj.family_id;
        pedData.altv_leader_id = netObj.leader_id;

        pedData.altv_is_dead = netObj.is_dead;
        pedData.altv_is_admin = netObj.is_admin;
        pedData.altv_is_media = netObj.is_media;
        pedData.altv_is_tester = netObj.is_tester;
        pedData.altv_is_afk = netObj.is_afk;
        pedData.altv_is_cuffed = netObj.is_cuffed;
        pedData.altv_has_wear_armor = netObj.has_wear_armor;
        pedData.altv_wanted_level = netObj.wanted_level;
        pedData.altv_hp = netObj.hp;
        pedData.altv_armor = netObj.armor;
        pedData.altv_weapon_hash = netObj.weapon_hash;
        pedData.has_ws_data = true;
        pedData.has_current_ws_data = current_snapshot;
    }

    void update_visiblity() {
        // Throttle LOS checks like the original code throttled raycasts
        if (!config::get("misc", "visibility_update_enable", 1)) {
            update_calls = 0;
            std::fill(std::begin(visible_peds), std::end(visible_peds), true);
            return;
        }

        update_calls++;
        const int update_interval = std::max(1, config::get("misc", "visibility_update_time", 1));
        if (update_calls < update_interval) return;
        update_calls = 0;

        if (!IsValidPtr(Game.ReplayInterface)) return;
        if (!IsValidPtr(Game.ReplayInterface->ped_interface)) return;
        if (!IsValidPtr(local.player)) return;
        { float _hp=0; if (!get_hp(local.player,&_hp)||_hp<=0) return; }

        const int local_handle = pointer_to_entity_handle(reinterpret_cast<uintptr_t>(local.player));

        int max_peds = Game.ReplayInterface->ped_interface->max_peds;
        if (max_peds <= 0 || max_peds > 1024) max_peds = 256;

        for (int i = 0; i < max_peds; i++) {
            if (!IsValidPtr(local.player)) break;
            { float _lhp=0; if (!get_hp(local.player,&_lhp)||_lhp<=0) break; }

            auto tmpEntity = reinterpret_cast<CObject*>(Game.ReplayInterface->ped_interface->get_ped(i));
            if (!IsValidPtr(tmpEntity)) continue;

            float tmpHP = 0;
            if (!get_ped_hp(tmpEntity, &tmpHP) || tmpHP <= 0) continue;

            float dist = 0;
            if (!get_entity_distance(local.player, tmpEntity, &dist)) continue;
            if (dist > config::get("hack", "max_range", 1000.f)) continue;

            DWORD hash = 0;
            if (!get_model_hash(tmpEntity, &hash)) continue;
            if (!game::isValidPlayer(hash, tmpEntity)) continue;

            const int target_handle = pointer_to_entity_handle(reinterpret_cast<uintptr_t>(tmpEntity));
            if (target_handle <= 0) continue;

            // Native line-of-sight check — synchronous, no raycast plumbing.
            // 17 = standard trace type (ignores the source ped itself).
            const bool clear_los = native::entity::has_entity_clear_los_to_entity(target_handle, local_handle, 17);
            visible_peds[i] = clear_los;
        }
        return;
    }
    void crosshair() {
        RGBA CrosshairColorRGBA = RGBA(
            (float)config::get("visual", "crosshair_color_r", 1.f) * 255,
            (float)config::get("visual", "crosshair_color_g", 1.f) * 255,
            (float)config::get("visual", "crosshair_color_b", 1.f) * 255,
            (float)config::get("visual", "crosshair_color_a", 1.f) * 255
        );
        int ScreenWidth = ImGui::GetIO().DisplaySize.x;
        int ScreenHeight = ImGui::GetIO().DisplaySize.y;
        ImVec2 Center = ImVec2(ScreenWidth / 2, ScreenHeight / 2);
        float gap = config::get("visual", "visual_crosshair_gap", 0.f);
        float thickness = config::get("visual", "visual_crosshair_thickness", 0.f);
        float width = config::get("visual", "visual_crosshair_width", 0.f);
        int border = config::get("visual", "visual_crosshair_border", 0);
        ImVec2 Center_Left = ImVec2(Center.x - gap, Center.y);
        ImVec2 Center_Left_End = ImVec2(Center.x - width, Center.y);
        if (border) {
            renderer.RenderLine(ImVec2(Center_Left.x + 1, Center_Left.y), ImVec2(Center_Left_End.x - 1, Center_Left_End.y), RGBA(0, 0, 0, 255), thickness + 2.0f);
        }
        renderer.RenderLine(Center_Left, Center_Left_End, CrosshairColorRGBA, thickness);
        ImVec2 Center_Right = ImVec2(Center.x + gap, Center.y);
        ImVec2 Center_Right_End = ImVec2(Center.x + width, Center.y);
        if (border) {
            renderer.RenderLine(ImVec2(Center_Right.x - 1, Center_Right.y), ImVec2(Center_Right_End.x + 1, Center_Right_End.y), RGBA(0, 0, 0, 255), thickness + 2.0f);
        }
        renderer.RenderLine(Center_Right, Center_Right_End, CrosshairColorRGBA, thickness);
        ImVec2 Center_Top = ImVec2(Center.x, Center.y - gap);
        ImVec2 Center_Top_End = ImVec2(Center.x, Center.y - width);
        if (border) {
            renderer.RenderLine(ImVec2(Center_Top.x, Center_Top.y + 1), ImVec2(Center_Top_End.x, Center_Top_End.y - 1), RGBA(0, 0, 0, 255), thickness + 2.0f);
        }
        renderer.RenderLine(Center_Top, Center_Top_End, CrosshairColorRGBA, thickness);
        ImVec2 Center_Bottom = ImVec2(Center.x, Center.y + gap);
        ImVec2 Center_Bottom_End = ImVec2(Center.x, Center.y + width);
        if (border) {
            renderer.RenderLine(ImVec2(Center_Bottom.x, Center_Bottom.y - 1), ImVec2(Center_Bottom_End.x, Center_Bottom_End.y + 1), RGBA(0, 0, 0, 255), thickness + 2.0f);
        }
        renderer.RenderLine(Center_Bottom, Center_Bottom_End, CrosshairColorRGBA, thickness);
    }
    static bool is_valid_custom_fov(float fov) {
        return std::isfinite(fov) && fov >= k_custom_fov_min && fov <= k_custom_fov_max;
    }

    static float clamp_custom_fov(float fov) {
        if (!std::isfinite(fov)) return k_custom_fov_fallback;
        return std::clamp(fov, k_custom_fov_min, k_custom_fov_max);
    }

    static bool readFOV(float& fov) {
        CPlayerAngles* cam = Game.getCam();
        if (!IsValidPtr(cam) || !IsValidPtr(cam->m_cam_data)) {
            return false;
        }

        __try {
            fov = cam->m_cam_data->Fov_Zoom;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        return is_valid_custom_fov(fov);
    }

    static bool setFOV(float fov) {
        CPlayerAngles* cam = Game.getCam();
        if (!IsValidPtr(cam) || !IsValidPtr(cam->m_cam_data)) {
            return false;
        }

        const float clamped_fov = clamp_custom_fov(fov);
        __try {
            cam->m_cam_data->Fov_Zoom = clamped_fov;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static void restore_custom_fov() {
        if (custom_fov_active && is_valid_custom_fov(custom_fov_default)) {
            setFOV(custom_fov_default);
        }
        custom_fov_active = false;
        custom_fov_default = 0.f;
    }

    static void update_custom_fov() {
        if (!config::get("hacks", "custom_fov", 0)) {
            restore_custom_fov();
            return;
        }

        if (!custom_fov_active) {
            float current_fov = 0.f;
            custom_fov_default = readFOV(current_fov) ? current_fov : k_custom_fov_fallback;
            custom_fov_active = true;
        }

        setFOV(config::get("hacks", "custom_fov_value", k_custom_fov_fallback));
    }

    static bool is_valid_aspect_ratio(float value) {
        return std::isfinite(value) && value > 0.5f && value < 4.f;
    }

    static bool aspect_ratio_memory_writable() {
        MEMORY_BASIC_INFORMATION mbi = {};
        if (!Game.AspectRatioAddr || !VirtualQuery(Game.AspectRatioAddr, &mbi, sizeof(mbi))) {
            return false;
        }

        const uintptr_t start = reinterpret_cast<uintptr_t>(Game.AspectRatioAddr);
        const uintptr_t end = start + sizeof(float);
        const uintptr_t region_end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        if (mbi.State != MEM_COMMIT || end > region_end || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
            return false;
        }

        const DWORD writable =
            PAGE_READWRITE |
            PAGE_WRITECOPY |
            PAGE_EXECUTE_READWRITE |
            PAGE_EXECUTE_WRITECOPY;
        return (mbi.Protect & writable) != 0;
    }

    static bool read_aspect_ratio(float& value) {
        if (!aspect_ratio_memory_writable()) {
            return false;
        }

        __try {
            value = *Game.AspectRatioAddr;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        return is_valid_aspect_ratio(value);
    }

    static bool write_aspect_ratio(float value) {
        if (!is_valid_aspect_ratio(value) || !aspect_ratio_memory_writable()) {
            return false;
        }

        __try {
            *Game.AspectRatioAddr = value;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static void restore_custom_aspect_ratio(bool write_restore) {
        if (custom_aspect_active && write_restore) {
            const float restore_value = is_valid_aspect_ratio(custom_aspect_default)
                ? custom_aspect_default
                : Game.AspectRatioDefault;
            if (is_valid_aspect_ratio(restore_value)) {
                write_aspect_ratio(restore_value);
            }
        }

        custom_aspect_active = false;
        custom_aspect_default = 0.f;
    }

    static void update_custom_aspect_ratio() {
        if (!Game.renderReady || !Game.gameDataReady || !Game.AspectRatioAddr) {
            restore_custom_aspect_ratio(false);
            return;
        }

        if (!config::get("hacks", "custom_aspect", 0)) {
            restore_custom_aspect_ratio(true);
            return;
        }

        float current = 0.f;
        if (!read_aspect_ratio(current)) {
            restore_custom_aspect_ratio(false);
            return;
        }

        if (!custom_aspect_active) {
            custom_aspect_default = current;
            custom_aspect_active = true;
        }

        const float desired = std::clamp(config::get("hacks", "custom_aspect_value", 1.7777778f), 0.5f, 3.f);
        if (std::fabs(current - desired) > 0.001f) {
            write_aspect_ratio(desired);
        }
    }

    vector<pair<CObject*, DataPed>> ped_list;
    std::mutex ped_list_mutex;
    vector<CObject*> object_list;
    vector<CVehicle*> vehicle_list;
    vector<CVehicle*> nearby_vehicle_list;
    map<string, string> dummy_group;
    map<DWORD, string> dummy_group1;
    static std::string trim_friend_name(std::string value) {
        value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), value.end());
        return value;
    }
    static std::string trim_friend_name(const char* value) {
        if (!value) {
return {};
        }

        return trim_friend_name(std::string(value));
    }
    static std::string normalize_friend_name(const char* value) {
        std::string normalized = trim_friend_name(value);
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return normalized;
    }
    static std::string make_friend_key(int static_id, const char* altv_nick, const char* gta_name) {
        if (static_id > 0) {
            return "id:" + std::to_string(static_id);
        }

        std::string altv_key = normalize_friend_name(altv_nick);
        if (!altv_key.empty()) {
            return "altv:" + altv_key;
        }

        std::string gta_key = normalize_friend_name(gta_name);
        if (!gta_key.empty()) {
            return "gta:" + gta_key;
        }

        return {};
    }
    static bool is_friend_player(int static_id) {
        return player_marks::is_friend(static_id);
    }
    static bool is_enemy_player(int static_id) {
        return player_marks::is_enemy(static_id);
    }
    static bool group_find_value(const std::string& text, const char* key, size_t& begin, size_t& end, bool& quoted) {
        const std::string needle = std::string("\"") + key + "\"";
        size_t pos = text.find(needle);
        if (pos == std::string::npos) return false;

        pos = text.find(':', pos + needle.size());
        if (pos == std::string::npos) return false;

        ++pos;
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) ++pos;
        if (pos >= text.size()) return false;

        quoted = text[pos] == '"';
        if (quoted) {
            begin = ++pos;
            bool escaped = false;
            while (pos < text.size()) {
                const char ch = text[pos];
                if (ch == '"' && !escaped) break;
                escaped = ch == '\\' && !escaped;
                if (ch != '\\') escaped = false;
                ++pos;
            }
            end = pos;
            return end >= begin;
        }

        begin = pos;
        while (pos < text.size() && text[pos] != ',' && text[pos] != '}') ++pos;
        end = pos;
        while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
        return end > begin;
    }
    static std::string group_string_value(const std::string& text, const char* key, const std::string& fallback = {}) {
        size_t begin = 0;
        size_t end = 0;
        bool quoted = false;
        if (!group_find_value(text, key, begin, end, quoted)) return fallback;
        return text.substr(begin, end - begin);
    }
    static bool group_bool_value(const std::string& text, const char* key, bool fallback = false) {
        size_t begin = 0;
        size_t end = 0;
        bool quoted = false;
        if (!group_find_value(text, key, begin, end, quoted)) return fallback;

        std::string value = text.substr(begin, end - begin);
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value == "true" || value == "1";
    }
    static float group_float_value(const std::string& text, const char* key, float fallback = 0.f) {
        size_t begin = 0;
        size_t end = 0;
        bool quoted = false;
        if (!group_find_value(text, key, begin, end, quoted)) return fallback;

        const std::string value = text.substr(begin, end - begin);
        char* stop = nullptr;
        const float parsed = std::strtof(value.c_str(), &stop);
        return stop != value.c_str() ? parsed : fallback;
    }
    static void apply_group_data(DataPed& pedData, const std::string& dataString) {
        pedData.group.inGroup = true;
        pedData.group.name = group_string_value(dataString, "name");
        pedData.group.ally = group_bool_value(dataString, "ally");

        const char* r_key = pedData.visible ? "visible_color_r" : "invisible_color_r";
        const char* g_key = pedData.visible ? "visible_color_g" : "invisible_color_g";
        const char* b_key = pedData.visible ? "visible_color_b" : "invisible_color_b";
        const char* a_key = pedData.visible ? "visible_color_a" : "invisible_color_a";
        pedData.group.color = RGBA(
            static_cast<int>(group_float_value(dataString, r_key, 1.f) * 255.f),
            static_cast<int>(group_float_value(dataString, g_key, 1.f) * 255.f),
            static_cast<int>(group_float_value(dataString, b_key, 1.f) * 255.f),
            static_cast<int>(group_float_value(dataString, a_key, 1.f) * 255.f));
    }
    std::vector<player_list_ui_entry> get_player_list_snapshot() {
        std::vector<player_list_ui_entry> entries;

        std::lock_guard<std::mutex> lock(ped_list_mutex);
        entries.reserve(ped_list.size());

        for (const auto& ped_entry : ped_list) {
            const DataPed& ped = ped_entry.second;

            player_list_ui_entry entry;
            entry.altv_name = trim_friend_name(ped.altv_nick);
            entry.gta_name = trim_friend_name(ped.name);
            entry.faction_name = trim_friend_name(ped.altv_fraction);
            entry.fraction_id = ped.altv_fraction_id;
            entry.family_id = ped.altv_family_id;
            entry.is_family = player_marks::is_family(ped.altv_static_id);
            entry.is_fraction = player_marks::is_fraction(ped.altv_static_id);
            entry.static_id = ped.altv_static_id;
            entry.dynamic_id = ped.altv_dynamic_id;
            entry.index = ped.index;
            entry.has_ws_data = ped.has_ws_data;
            entry.is_admin = ped.altv_is_admin;
            entry.is_friend = ped.is_friend;
            entry.friend_key = make_friend_key(ped.altv_static_id, ped.altv_nick, ped.name);

            if (!entry.altv_name.empty()) {
                entry.display_name = entry.altv_name;
            }
            else if (!entry.gta_name.empty()) {
                entry.display_name = entry.gta_name;
            }
            else if (entry.static_id > 0) {
                entry.display_name = "ID " + std::to_string(entry.static_id);
            }
            else {
                entry.display_name = "player #" + std::to_string(entry.index);
            }

            entries.push_back(entry);
        }

        return entries;
    }
    void update_vehicles() {
        if (IsValidPtr(Game.ReplayInterface) && IsValidPtr(Game.ReplayInterface->vehicle_interface)) {
            if (IsValidPtr(local.player)) {
                map<DWORD, string> veh_list_find = config::get("misc", "vehicle_esp_vehicle_list", dummy_group1);
                vector<CVehicle*> new_vehicle_list;
                vector<CVehicle*> new_nearby_vehicle_list;
                for (int i = 0; i < Game.ReplayInterface->vehicle_interface->max_vehicles; i++) {
                    CVehicle* veh = reinterpret_cast<CVehicle*>(Game.ReplayInterface->vehicle_interface->get_vehicle(i));
                    if (IsValidPtr(veh)) {
                        float dist = 0;
                        if (!get_entity_distance(local.player, reinterpret_cast<CObject*>(veh), &dist)) continue;
                        if (dist > config::get("hack", "max_range", 1000.f)) continue;
                        if (dist < 50.f) {
                            new_nearby_vehicle_list.push_back(veh);
                        }
                        DWORD hash = 0;
                        if (!get_model_hash(reinterpret_cast<CObject*>(veh), &hash)) continue;
                        map<DWORD, string>::iterator it = veh_list_find.find(hash);
                        if (config::get("misc", "vehicle_esp_all", 0) || (it != veh_list_find.end())) {
                            new_vehicle_list.push_back(veh);
                        }
                    }
                }
                vehicle_list.clear();
                vehicle_list = new_vehicle_list;
                nearby_vehicle_list.clear();
                nearby_vehicle_list = new_nearby_vehicle_list;
            }
        }
    }
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;

    static void __declspec(noinline) read_player_name(CObject* ped, char* out, size_t out_size) {
        out[0] = '\0';
        __try {
            CPlayerInfo* pInfo = ped->player_info();
            if (IsValidPtr(pInfo)) {
                size_t copy_len = (out_size - 1 < 20) ? out_size - 1 : 20;
                memcpy(out, pInfo->sName, copy_len);
                out[copy_len] = '\0';
                for (size_t i = 0; out[i]; i++) {
                    if ((uint8_t)out[i] < 0x20 || (uint8_t)out[i] > 0x7E) {
                        out[0] = '\0';
                        break;
                    }
                }
                if (strcmp(out, "MajesticPlayer") == 0 || strcmp(out, "Player") == 0)
                    out[0] = '\0';
            }
        } __except(1) {}
    }

    static uint32_t __declspec(noinline) read_net_object_id(CObject* ped) {
        __try {
            static const uintptr_t net_obj_offsets[] = { 0xD0, 0xD8, 0xE0 };
            static const uintptr_t owner_id_offsets[] = { 0xA, 0x8, 0xC };

            for (auto net_off : net_obj_offsets) {
                uintptr_t ped_addr = (uintptr_t)ped;
                if (!IsValidPtr(*(void**)(ped_addr + net_off))) continue;
                uintptr_t net_obj = *(uintptr_t*)(ped_addr + net_off);
                if (net_obj < 0x10000 || net_obj > 0x7FFFFFFFFFFF) continue;
                if (IsBadReadPtr((void*)net_obj, 0x20)) continue;

                for (auto id_off : owner_id_offsets) {
                    uint16_t owner_id = *(uint16_t*)(net_obj + id_off);
                    if (owner_id > 0 && owner_id < 2048) {
                        uintptr_t vtable = *(uintptr_t*)net_obj;
                        if (vtable > 0x10000 && vtable < 0x7FFFFFFFFFFF) {
                            return (uint32_t)owner_id;
                        }
                    }
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        return 0;
    }

    void update_peds() {
        // everyone visible until the visibility raycast has run
        static bool s_visible_peds_initialized = false;
        if (!s_visible_peds_initialized) {
            std::fill(std::begin(visible_peds), std::end(visible_peds), true);
            s_visible_peds_initialized = true;
        }

        if (!IsValidPtr(Game.ReplayInterface)) return;
        if (!IsValidPtr(Game.ReplayInterface->ped_interface)) return;
        if (!IsValidPtr(local.player)) return;
        { float _hp = 0; if (!get_hp(local.player, &_hp) || _hp <= 0) return; }

        if (network_cache.size() > 512) network_cache.clear();

        map<string, string> group_list = dummy_group;
        std::vector<ws_server::EspPlayer> net_data;
        ws_server::copy_players(net_data);
        const float max_range = config::get("hack", "max_range", 1000.f);
        const bool esp_self = config::get("visual", "esp_self", 0) != 0;
        const bool alpha_check_enabled = config::get("misc", "ignore_invisible", 0) != 0;
        const float alpha_threshold = config::get("misc", "alpha_threshold", 0.f);
        const bool force_visible = config::get("misc", "force_visible", 0) != 0;
        const bool hide_anticheat_npcs = config::get("misc", "hide_anticheat_npcs", 0) != 0;
        const bool use_group_data = !group_list.empty() && (config::get("hack", "group_window", 0) || config::get("visual", "display_groups", 0));
        static const bind_state::bind_config k_vector_aim_bind {
            "aimbot",
            "vector_enable",
            "aim_key",
            "vector_aim_key",
            "aim_key_mode",
            "vector_aim_mode"
        };
        static const bind_state::bind_config k_visual_esp_bind {
            "visual",
            "enable",
            "esp_key",
            nullptr,
            "esp_key_mode",
            nullptr
        };
        const bool needs_bones = bind_state::is_active(k_vector_aim_bind) ||
            bind_state::is_active_or_enabled_when_unbound(k_visual_esp_bind) ||
            config::get("visual", "draw_skeleton", 0) != 0 ||
            config::get("aimbot", "vector_enable", 0) != 0;

        const size_t net_count = net_data.size();
        std::unordered_map<int, size_t> ws_by_handle;
        std::unordered_map<int, size_t> ws_by_static_id;
        std::unordered_map<int, size_t> ws_by_netid;
        std::vector<char> ws_claimed(net_count, 0);
        ws_by_handle.reserve(net_count);
        ws_by_static_id.reserve(net_count);
        ws_by_netid.reserve(net_count);

        for (size_t idx = 0; idx < net_data.size(); ++idx) {
            const auto& net_obj = net_data[idx];

            if (net_obj.handle > 0) {
                ws_by_handle[net_obj.handle] = idx;
            }

            if (net_obj.static_id > 0) {
                ws_by_static_id[net_obj.static_id] = idx;
            }

            if (net_obj.netid > 0) {
                ws_by_netid[net_obj.netid] = idx;
            }
        }

        int max_peds = Game.ReplayInterface->ped_interface->max_peds;
        if (max_peds <= 0 || max_peds > 1024) max_peds = 256;

        vector<pair<CObject*, DataPed>> new_list;
        std::unordered_set<uintptr_t> seen_peds;
        new_list.reserve(static_cast<size_t>(max_peds));
        seen_peds.reserve(static_cast<size_t>(max_peds));

        for (int i = 0; i < max_peds; i++) {
            if (!IsValidPtr(local.player)) break;
            { float _lhp = 0; if (!get_hp(local.player, &_lhp) || _lhp <= 0) break; }

            CObject* ped = reinterpret_cast<CObject*>(Game.ReplayInterface->ped_interface->get_ped(i));
            if (!IsValidPtr(ped)) continue;

            const uintptr_t ped_ptr = (uintptr_t)ped;
            seen_peds.insert(ped_ptr);

            float pedHP = 0.f;
            if (!get_ped_hp(ped, &pedHP) || pedHP <= 0.f) continue;

            if (!esp_self && local.player == ped) continue;

            float dist = 0.f;
            if (!get_entity_distance(local.player, ped, &dist)) continue;
            if (dist > max_range) continue;

            DWORD hash = 0;
            if (!get_model_hash(ped, &hash)) continue;
            if (!game::isValidPlayer(hash, ped)) continue;

            float _alpha = 255.f;
            get_alpha(ped, &_alpha);

            bool invisible_by_alpha = (_alpha < alpha_threshold);
            if (force_visible && invisible_by_alpha) {
                set_alpha(ped, alpha_threshold);
                invisible_by_alpha = false;
            }

            const int pedHandle = pointer_to_entity_handle(ped_ptr);
            if (hide_anticheat_npcs && pedHandle) {
                if (native::brain::get_is_task_active(pedHandle, 221) ||
                    native::brain::get_is_task_active(pedHandle, 222) ||
                    native::brain::get_is_task_active(pedHandle, 241) ||
                    native::brain::get_is_task_active(pedHandle, 151) ||
                    native::brain::get_is_task_active(pedHandle, 138) ||
                    native::brain::get_is_task_active(pedHandle, 100) ||
                    native::brain::get_is_task_active(pedHandle, 218) ||
                    native::brain::get_is_task_active(pedHandle, 220) ||
                    native::brain::get_is_task_active(pedHandle, 34) ||
                    native::brain::get_is_task_active(pedHandle, 101)) {
                    continue;
                }
            }

            DataPed PedData{};
            PedData.index = i;
            PedData.visible = (i < 1024) ? visible_peds[i] : false;
            PedData.name[0] = '\0';
            read_player_name(ped, PedData.name, sizeof(PedData.name));
            ResetAltvPedData(PedData, pedHP);

            {
                Vector3 pedPos = { 0, 0, 0 };
                get_entity_pos(ped, &pedPos);

                const int pedAltvId = static_cast<int>(read_net_object_id(ped));
                size_t matched_index = std::numeric_limits<size_t>::max();

                auto claim_match = [&](size_t index) -> bool {
                    if (index >= net_data.size() || ws_claimed[index]) {
                        return false;
                    }

                    matched_index = index;
                    ws_claimed[index] = 1;
                    return true;
                };

                auto cache_it = network_cache.find(ped_ptr);
                if (cache_it != network_cache.end()) {
                    const auto& cached = cache_it->second;

                    if (matched_index == std::numeric_limits<size_t>::max() && cached.handle > 0) {
                        auto it = ws_by_handle.find(cached.handle);
                        if (it != ws_by_handle.end()) {
                            claim_match(it->second);
                        }
                    }

                    if (matched_index == std::numeric_limits<size_t>::max() && cached.netid > 0) {
                        auto it = ws_by_netid.find(cached.netid);
                        if (it != ws_by_netid.end()) {
                            claim_match(it->second);
                        }
                    }

                    if (matched_index == std::numeric_limits<size_t>::max() && cached.static_id > 0) {
                        auto it = ws_by_static_id.find(cached.static_id);
                        if (it != ws_by_static_id.end()) {
                            claim_match(it->second);
                        }
                    }
                }

                if (matched_index == std::numeric_limits<size_t>::max() && pedAltvId > 0) {
                    auto it = ws_by_netid.find(pedAltvId);
                    if (it != ws_by_netid.end()) {
                        claim_match(it->second);
                    }
                }

                if (matched_index == std::numeric_limits<size_t>::max() && pedHandle > 0) {
                    auto it = ws_by_handle.find(pedHandle);
                    if (it != ws_by_handle.end()) {
                        claim_match(it->second);
                    }
                }

                if (matched_index == std::numeric_limits<size_t>::max()) {
                    constexpr float k_strict_pos_match = 1.5f;
                    constexpr float k_extended_pos_match = 4.f;
                    constexpr float k_extended_z_match = 3.f;

                    size_t nearest_index = std::numeric_limits<size_t>::max();
                    float nearest_dist = std::numeric_limits<float>::max();
                    int nearby_candidates = 0;

                    for (size_t idx = 0; idx < net_data.size(); ++idx) {
                        if (ws_claimed[idx]) {
                            continue;
                        }

                        const auto& net_obj = net_data[idx];
                        if (net_obj.pos_x == 0.f && net_obj.pos_y == 0.f && net_obj.pos_z == 0.f) {
                            continue;
                        }

                        float dx = pedPos.x - net_obj.pos_x;
                        float dy = pedPos.y - net_obj.pos_y;
                        float dz = fabsf(pedPos.z - net_obj.pos_z);
                        float net_dist = sqrtf(dx * dx + dy * dy);

                        if (net_dist > k_extended_pos_match || dz > k_extended_z_match) {
                            continue;
                        }

                        nearby_candidates++;
                        if (net_dist < nearest_dist) {
                            nearest_dist = net_dist;
                            nearest_index = idx;
                        }
                    }

                    // TEMP diag removed
                    if (nearest_index != std::numeric_limits<size_t>::max() &&
                        (nearest_dist <= k_strict_pos_match || nearby_candidates == 1)) {
                        claim_match(nearest_index);
                    }
                }

                if (matched_index != std::numeric_limits<size_t>::max()) {
                    const auto& matched_net = net_data[matched_index];
                    network_cache[ped_ptr] = matched_net;
                    ApplyWsPlayerData(matched_net, PedData, true);
                }
                else if (cache_it != network_cache.end()) {
                    ApplyWsPlayerData(cache_it->second, PedData, false);
                }
            }

            if (alpha_check_enabled && invisible_by_alpha) {
                continue;
            }

            if (use_group_data) {
                string componentId = GetPedComponentHash(ped);
                if (!componentId.empty()) {
                    map<string, string>::iterator it = group_list.find(componentId);
                    if (it != group_list.end() && !it->second.empty()) {
                        apply_group_data(PedData, it->second);
                    }
                }
            }

            if (needs_bones) {
                // Entity handle: POINTER_TO_ENTITY native is the authoritative
                // handle for GET_PED_BONE_COORDS. Pool iHandle is NOT a valid
                // native handle on this build (GET_ENTITY_COORDS returns 0)
                int bone_handle = 0;
                if (pointer_to_handle) {
                    __try { bone_handle = pointer_to_handle(reinterpret_cast<uintptr_t>(ped)); }
                    __except (EXCEPTION_EXECUTE_HANDLER) { bone_handle = 0; }
                }
                if (bone_handle <= 0 && IsValidPtr(Game.ReplayInterface) && IsValidPtr(Game.ReplayInterface->ped_interface) &&
                    IsValidPtr(Game.ReplayInterface->ped_interface->ped_list)) {
                    bone_handle = Game.ReplayInterface->ped_interface->ped_list->peds[i].iHandle;
                }
                aimbot::FetchPedBones(ped, bone_handle, &PedData.bones);
            }

            PedData.is_friend = is_friend_player(PedData.altv_static_id);

            new_list.emplace_back(ped, PedData);
        }

        for (auto it = network_cache.begin(); it != network_cache.end(); ) {
            if (seen_peds.find(it->first) == seen_peds.end()) it = network_cache.erase(it);
            else ++it;
        }

        {
            std::lock_guard<std::mutex> lock(ped_list_mutex);
            ped_list.swap(new_list);
        }

        // Build WS-compatible player list from GTA memory for ESP flags
        {
            std::vector<ws_server::EspPlayer> ws_players;
            ws_players.reserve(ped_list.size());
            for (const auto& [ped, data] : ped_list) {
                ws_server::EspPlayer p{};
                p.handle   = pointer_to_entity_handle(reinterpret_cast<uintptr_t>(ped));
                p.netid    = static_cast<uint16_t>(read_net_object_id(ped));
                p.name     = data.name[0] ? data.name : "Player";
                p.static_id = data.altv_static_id;
                p.level    = data.altv_level;
                p.admin_level = data.altv_admin_level;
                p.fraction = data.altv_fraction;
                p.fraction_id = data.altv_fraction_id;
                p.hp       = data.altv_hp;
                p.armor    = data.altv_armor;
                p.weapon_hash = data.altv_weapon_hash;
                p.is_admin = data.altv_is_admin;
                p.is_dead  = data.altv_is_dead;
                p.is_media = data.altv_is_media;
                p.is_tester= data.altv_is_tester;
                p.is_afk   = data.altv_is_afk;
                p.visible  = data.visible != 0;
                p.pos_x    = ped->fPosition.x;
                p.pos_y    = ped->fPosition.y;
                p.pos_z    = ped->fPosition.z;
                p.bone_count = 24;
                p.auto_relation = 0;
                // Copy bones — order MUST match ws_player_bones() (player.hpp):
                // 0 HEAD, 1 NECK, 2 R_HAND, 3 R_FORE, 4 R_UPPER, 5 R_CLAV,
                // 6 L_HAND, 7 L_FORE, 8 L_UPPER, 9 L_CLAV, 10 PELVIS, 11 SP_ROOT,
                // 12 SP0, 13 SP1, 14 SP2, 15 SP3, 16 R_TOE, 17 R_FOOT, 18 R_CALF,
                // 19 R_THIGH, 20 L_TOE, 21 L_FOOT, 22 L_CALF, 23 L_THIGH
                p.bones[0][0]  = data.bones.HEAD.x;          p.bones[0][1] = data.bones.HEAD.y;          p.bones[0][2] = data.bones.HEAD.z;
                p.bones[1][0]  = data.bones.NECK.x;          p.bones[1][1] = data.bones.NECK.y;          p.bones[1][2] = data.bones.NECK.z;
                p.bones[2][0]  = data.bones.RIGHT_HAND.x;    p.bones[2][1] = data.bones.RIGHT_HAND.y;    p.bones[2][2] = data.bones.RIGHT_HAND.z;
                p.bones[3][0]  = data.bones.RIGHT_FOREARM.x; p.bones[3][1] = data.bones.RIGHT_FOREARM.y; p.bones[3][2] = data.bones.RIGHT_FOREARM.z;
                p.bones[4][0]  = data.bones.RIGHT_UPPER_ARM.x; p.bones[4][1] = data.bones.RIGHT_UPPER_ARM.y; p.bones[4][2] = data.bones.RIGHT_UPPER_ARM.z;
                p.bones[5][0]  = data.bones.RIGHT_CLAVICLE.x;  p.bones[5][1] = data.bones.RIGHT_CLAVICLE.y;  p.bones[5][2] = data.bones.RIGHT_CLAVICLE.z;
                p.bones[6][0]  = data.bones.LEFT_HAND.x;     p.bones[6][1] = data.bones.LEFT_HAND.y;     p.bones[6][2] = data.bones.LEFT_HAND.z;
                p.bones[7][0]  = data.bones.LEFT_FOREARM.x;  p.bones[7][1] = data.bones.LEFT_FOREARM.y;  p.bones[7][2] = data.bones.LEFT_FOREARM.z;
                p.bones[8][0]  = data.bones.LEFT_UPPER_ARM.x; p.bones[8][1] = data.bones.LEFT_UPPER_ARM.y; p.bones[8][2] = data.bones.LEFT_UPPER_ARM.z;
                p.bones[9][0]  = data.bones.LEFT_CLAVICLE.x;  p.bones[9][1] = data.bones.LEFT_CLAVICLE.y;  p.bones[9][2] = data.bones.LEFT_CLAVICLE.z;
                p.bones[10][0] = data.bones.PELVIS.x;         p.bones[10][1]= data.bones.PELVIS.y;         p.bones[10][2]= data.bones.PELVIS.z;
                p.bones[11][0] = data.bones.SPINE_ROOT.x;     p.bones[11][1]= data.bones.SPINE_ROOT.y;     p.bones[11][2]= data.bones.SPINE_ROOT.z;
                p.bones[12][0] = data.bones.SPINE0.x;         p.bones[12][1]= data.bones.SPINE0.y;         p.bones[12][2]= data.bones.SPINE0.z;
                p.bones[13][0] = data.bones.SPINE1.x;         p.bones[13][1]= data.bones.SPINE1.y;         p.bones[13][2]= data.bones.SPINE1.z;
                p.bones[14][0] = data.bones.SPINE2.x;         p.bones[14][1]= data.bones.SPINE2.y;         p.bones[14][2]= data.bones.SPINE2.z;
                p.bones[15][0] = data.bones.SPINE3.x;         p.bones[15][1]= data.bones.SPINE3.y;         p.bones[15][2]= data.bones.SPINE3.z;
                p.bones[16][0] = data.bones.RIGHT_TOE.x;      p.bones[16][1]= data.bones.RIGHT_TOE.y;      p.bones[16][2]= data.bones.RIGHT_TOE.z;
                p.bones[17][0] = data.bones.RIGHT_FOOT.x;     p.bones[17][1]= data.bones.RIGHT_FOOT.y;     p.bones[17][2]= data.bones.RIGHT_FOOT.z;
                p.bones[18][0] = data.bones.RIGHT_CALF.x;     p.bones[18][1]= data.bones.RIGHT_CALF.y;     p.bones[18][2]= data.bones.RIGHT_CALF.z;
                p.bones[19][0] = data.bones.RIGHT_THIGH.x;    p.bones[19][1]= data.bones.RIGHT_THIGH.y;    p.bones[19][2]= data.bones.RIGHT_THIGH.z;
                p.bones[20][0] = data.bones.LEFT_TOE.x;       p.bones[20][1]= data.bones.LEFT_TOE.y;       p.bones[20][2]= data.bones.LEFT_TOE.z;
                p.bones[21][0] = data.bones.LEFT_FOOT.x;      p.bones[21][1]= data.bones.LEFT_FOOT.y;      p.bones[21][2]= data.bones.LEFT_FOOT.z;
                p.bones[22][0] = data.bones.LEFT_CALF.x;      p.bones[22][1]= data.bones.LEFT_CALF.y;      p.bones[22][2]= data.bones.LEFT_CALF.z;
                p.bones[23][0] = data.bones.LEFT_THIGH.x;     p.bones[23][1]= data.bones.LEFT_THIGH.y;     p.bones[23][2]= data.bones.LEFT_THIGH.z;
                ws_players.push_back(std::move(p));
            }
            ws_server::set_players(std::move(ws_players));
        }
    }
    static void _seh_native_init() { __try { runtime_debug::last_section = "gt_native_init"; native_invoke::init(); } __except(EXCEPTION_EXECUTE_HANDLER) { HELLO_RUNTIME_LOG("SEH: native_init 0x%08X", GetExceptionCode()); } }
    static CObject* _seh_get_local() {
        CObject* lp = nullptr;
        runtime_debug::last_section = "gt_get_local";
        __try {
            lp = Game.World->getLocalPlayer();
        } __except(EXCEPTION_EXECUTE_HANDLER) { HELLO_RUNTIME_LOG("SEH: getLocalPlayer 0x%08X", GetExceptionCode()); }
        return lp;
    }
    static void _seh_update_visibility() { __try { runtime_debug::last_section = "gt_update_visibility"; update_visiblity(); } __except(EXCEPTION_EXECUTE_HANDLER) { HELLO_RUNTIME_LOG("SEH: update_visibility 0x%08X", GetExceptionCode()); } }
    static void _seh_update_peds() { __try { runtime_debug::last_section = "gt_update_peds"; update_peds(); } __except(EXCEPTION_EXECUTE_HANDLER) { HELLO_RUNTIME_LOG("SEH: update_peds 0x%08X", GetExceptionCode()); } }
    static void _seh_hacks_tick() { __try { runtime_debug::clear_native_context(); runtime_debug::last_section = "gt_hacks_tick"; hacks::game_tick(); } __except(EXCEPTION_EXECUTE_HANDLER) { HELLO_RUNTIME_LOG("SEH: hacks_tick 0x%08X", GetExceptionCode()); } }
    static void _seh_waypoint_tp() {}
    static void _seh_wp_tp_tick() {}
    static int aimbot_tick_exception_filter(EXCEPTION_POINTERS* exception) {
        crash_logger::log_exception("aimbot_tick", exception);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    static void _seh_aimbot_tick() { __try { runtime_debug::last_section = "gt_aimbot_tick"; aimbot::tick(); } __except(aimbot_tick_exception_filter(GetExceptionInformation())) { HELLO_RUNTIME_LOG("SEH: aimbot_tick 0x%08X", GetExceptionCode()); } }

    inline bool local_player_stable_for_esp() {
        static CObject* stable_player = nullptr;
        static DWORD64 stable_since = 0;

        if (!Game.renderReady || !IsValidPtr(local.player)) {
            stable_player = nullptr;
            stable_since = 0;
            return false;
        }

        float hp = 0.f;
        if (!get_ped_hp(local.player, &hp) || hp <= 0.f) {
            stable_player = nullptr;
            stable_since = 0;
            return false;
        }

        const DWORD64 now = GetTickCount64();
        if (stable_player != local.player) {
            stable_player = local.player;
            stable_since = now;
            return false;
        }

        return stable_since != 0 && now - stable_since >= 5000;
    }

    void game_tick() {
        _seh_native_init();
if (!Game.running) return;
        if (!IsValidPtr(Game.ReplayInterface)) return;
        if (!IsValidPtr(Game.ReplayInterface->ped_interface)) return;
        if (!IsValidPtr(Game.World)) return;

        CObject* localPlayer = _seh_get_local();
        if (!IsValidPtr(localPlayer)) return;
        local.player = localPlayer;

        runtime_debug::last_section = "gt_local_hp";
        { float _ghp=0; if (!get_hp(local.player,&_ghp)||_ghp<=0) return; }

        if (!Game.gameDataReady) {
            Game.gameDataReady = true;
        }

        static const bind_state::bind_config k_vector_aim_bind {
            "aimbot",
            "vector_enable",
            "aim_key",
            "vector_aim_key",
            "aim_key_mode",
            "vector_aim_mode"
        };
        static const bind_state::bind_config k_visual_esp_bind {
            "visual",
            "enable",
            "esp_key",
            nullptr,
            "esp_key_mode",
            nullptr
        };
        const bool visual_esp_active = bind_state::is_active_or_enabled_when_unbound(k_visual_esp_bind);
        const bool visual_visibility_ready = visual_esp_active && local_player_stable_for_esp();
        const bool silent_pipeline_active = false;
        const bool weapons_enabled = weapons_highlight::enabled();
        const bool draw_weapons = config::get("visual", "draw_weapons", 0) != 0;
        const bool weapons_data_active = draw_weapons || (weapons_enabled && (config::get("visual", "weapons_widget", 0) != 0 || config::get("visual", "weapons_highlight", 0) != 0 || config::get("visual", "weapons_sound", 0) != 0)) || config::get("visual", "weapon_arrows", 0) != 0;
        bool aimbot_active = bind_state::is_active(k_vector_aim_bind) || silent_pipeline_active || config::get("aimbot", "vector_enable", 0) != 0;

        if (visual_visibility_ready || aimbot_active) {
            _seh_update_visibility();
        }
        if (visual_esp_active || aimbot_active || weapons_data_active) {
            // When the window is not focused (minimized / alt-tab), skip the
            // heavy ped scan entirely — it shares the game thread with the
            // majestic voice manager, and background activity starves voice.
            // Data from the last focused tick stays in ped_list / ws snapshot.
            if (!Game.windowFocused) {
                return;
            }
            // update_peds scans up to 256 pool slots + fetches 24 native bones per
            // ped + rebuilds the ws snapshot. It runs inside the game-thread hook
            // (hooked_native_thread → game::game_tick), and heavy work here delays
            // the whole game tick — including the majestic voice network loop
            // ("[voicemgr] Network tick was N" appears when it is starved).
            // Throttle to every 5th tick (~80-100ms): ESP/aimbot stay smooth and
            // the voice loop is no longer starved.
            static int peds_tick = 0;
            if ((peds_tick++ % 5) == 0) {
                _seh_update_peds();
            }
        }

        _seh_hacks_tick();

        {
            bool esp_on = visual_esp_active;
            bool js_overlay_on = false;
            bool nick_on = config::get("visual", "altv_nickname", 0);
            bool static_on = config::get("visual", "altv_static", 0);
            bool dynamic_on = config::get("visual", "altv_dynamic", 0);
            bool admin_on = config::get("visual", "altv_admin", 0);
            bool faction_on = config::get("visual", "altv_faction", 0);
            bool level_on = config::get("visual", "altv_level", 0);
            bool admins_around_on = config::get("visual", "admins_around_indicator", 1);
            ws_server::send_config(
                esp_on,
                js_overlay_on,
                nick_on,
                static_on,
                dynamic_on,
                admin_on,
                faction_on,
                level_on,
                admins_around_on,
                false,
                draw_weapons || weapons_data_active,
                true,
                true
            );
        }

        if (config::get("hacks", "waypoint_tp_pending", 0)) {
            config::update(0, "hacks", "waypoint_tp_pending", 0);
            _seh_waypoint_tp();
        }

        if (aimbot_active) {
            _seh_aimbot_tick();
        }

        // silent is gone (crab rave)

        return;
    }
    static void update_vehicles_render_pass() { __try { update_vehicles(); } __except(EXCEPTION_EXECUTE_HANDLER) { HELLO_RUNTIME_LOG("SEH: update_vehicles 0x%08X", GetExceptionCode()); } }
    static int render_esp_exception_filter(EXCEPTION_POINTERS* exception) {
        crash_logger::log_exception("esp_render", exception);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    static void render_esp_pass() { __try { esp::render(); } __except(render_esp_exception_filter(GetExceptionInformation())) { HELLO_RUNTIME_LOG("SEH: esp_render 0x%08X", GetExceptionCode()); } }
    static void render_hud_pass() { __try { hud::render(); } __except(EXCEPTION_EXECUTE_HANDLER) { HELLO_RUNTIME_LOG("SEH: hud_render 0x%08X", GetExceptionCode()); } }
    static void render_crosshair_pass() { __try { crosshair(); } __except(EXCEPTION_EXECUTE_HANDLER) { HELLO_RUNTIME_LOG("SEH: crosshair 0x%08X", GetExceptionCode()); } }
    static void render_body_inner(CObject* localPlayer) {
        float lhp = 0;
        if (!get_ped_hp(localPlayer, &lhp) || lhp <= 0) return;

        if (config::get("hacks", "unlock_nearby", 0) || config::get("hacks", "no_collision_veh", 0)) {
            update_vehicles_render_pass();
        }

        if (IsValidPtr(local.player)) {
            float rhp = 0;
            get_ped_hp(local.player, &rhp);
        }
    }

    void game_render_visuals() {
        if (!Game.running) return;
        if (!IsValidPtr(Game.World)) return;

        render_hud_pass();

        CObject* localPlayer = _seh_get_local();
        if (IsValidPtr(localPlayer)) {
            local.player = localPlayer;
            render_body_inner(localPlayer);
        }

if (config::get("visual", "visual_crosshair", 0)) {
            render_crosshair_pass();
        }
        static const bind_state::bind_config k_vector_aim_bind {
            "aimbot",
            "vector_enable",
            "aim_key",
            "vector_aim_key",
            "aim_key_mode",
            "vector_aim_mode"
        };
        if (config::get("aimbot", "vector_show_radius", 0) && bind_state::is_active(k_vector_aim_bind)) {
            int w = ImGui::GetIO().DisplaySize.x;
            int h = ImGui::GetIO().DisplaySize.y;
            ImVec4 color = ImVec4(
                config::get("aimbot", "vector_fov_r", 1.f),
                config::get("aimbot", "vector_fov_g", 1.f),
                config::get("aimbot", "vector_fov_b", 1.f),
                config::get("aimbot", "vector_fov_a", 1.f)
            );
            float fov = config::get("aimbot", "vector_radius", 50.f);
            renderer.RenderCircle(ImVec2(w / 2, h / 2), fov, RGBA(
                (int)(color.x * 255), (int)(color.y * 255), (int)(color.z * 255), (int)(color.w * 255)), 1.0f, 50);
        }
        
        

        update_custom_fov();
        update_custom_aspect_ratio();

    }

    static void shutdown_runtime() {
        restore_custom_aspect_ratio(true);

        restore_custom_fov();

    }

    void game_render_menu() {
        if (!Game.running) return;
        runtime_debug::last_section = "menu_render";
        try { menu::render(Game.menuOpen); } catch (...) { HELLO_RUNTIME_LOG("menu_render cxx exception"); }
    }

    void game_render() {
        game_render_visuals();
        game_render_menu();
        if (Game.running && IsValidPtr(local.player)) {
            float rhp = 0;
            if (get_ped_hp(local.player, &rhp) && rhp > 0) {
                render_esp_pass();
            }
        }
    }
}
