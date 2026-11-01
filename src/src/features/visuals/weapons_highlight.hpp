#pragma once

#include "config/interface.hpp"
#include "config_color.hpp"
#include <Windows.h>
#include <imgui.h>
#include <map>
#include <string>

namespace weapons_highlight {
    struct weapon_entry {
        DWORD hash;
        const char* label;
    };

    struct state {
        bool any_enabled = false;
        bool highlight_enabled = false;
        bool ignore_friends = true;
        std::map<DWORD, std::string> selected;
        std::map<DWORD, std::string> custom;
        std::map<DWORD, std::string> custom_names;
        std::map<DWORD, std::string> custom_colors;
        std::map<DWORD, std::string> lines;
        ImU32 accent = IM_COL32(180, 167, 245, 255);
    };

    static constexpr DWORD k_none_hash = 0u;
    static constexpr weapon_entry k_entries[] = {
        { 0x0C472FE2u, "Heavy Sniper" },
        { 0x0A914799u, "Heavy Sniper Mk2" },
        { 0x6E7DDDECu, "Precision Rifle" },
    };

    inline std::map<DWORD, std::string> default_selected() {
        std::map<DWORD, std::string> value;
        for (const auto& entry : k_entries) {
            value[entry.hash] = "1";
        }
        return value;
    }

    inline std::map<DWORD, std::string> default_lines() {
        std::map<DWORD, std::string> value;
        for (const auto& entry : k_entries) {
            value[entry.hash] = "1";
        }
        return value;
    }

    inline const weapon_entry* find(DWORD hash) {
        for (const auto& entry : k_entries) {
            if (entry.hash == hash) {
                return &entry;
            }
        }
        return nullptr;
    }

    inline std::map<DWORD, std::string> custom_entries() {
        return config::get("visual", "weapons_highlight_custom", std::map<DWORD, std::string>{});
    }

    inline std::map<DWORD, std::string> custom_names() {
        return config::get("visual", "weapons_highlight_custom_names", std::map<DWORD, std::string>{});
    }

    inline std::map<DWORD, std::string> custom_colors() {
        return config::get("visual", "weapons_highlight_custom_colors", std::map<DWORD, std::string>{});
    }

    inline std::map<DWORD, std::string> line_settings() {
        return config::get("visual", "weapons_highlight_lines", std::map<DWORD, std::string>{});
    }

    inline bool default_line_enabled(DWORD hash) {
        return find(hash) != nullptr;
    }

    inline bool line_enabled(const std::map<DWORD, std::string>& lines, DWORD hash) {
        const auto it = lines.find(hash);
        if (it != lines.end()) {
            return it->second == "1";
        }
        return default_line_enabled(hash);
    }

    inline bool custom_exists(DWORD hash) {
        const std::map<DWORD, std::string> custom = custom_entries();
        const auto it = custom.find(hash);
        return it != custom.end() && !it->second.empty();
    }

    inline bool exists(DWORD hash) {
        return find(hash) || custom_exists(hash);
    }

    inline std::string base_label(DWORD hash) {
        if (const weapon_entry* entry = find(hash)) {
            return entry->label;
        }

        const std::map<DWORD, std::string> custom = custom_entries();
        const auto it = custom.find(hash);
        return it != custom.end() ? it->second : std::string{};
    }

    inline std::string label(DWORD hash) {
        const std::map<DWORD, std::string> names = custom_names();
        const auto name_it = names.find(hash);
        if (name_it != names.end() && !name_it->second.empty()) {
            return name_it->second;
        }

        return base_label(hash);
    }

    inline std::map<DWORD, std::string> selected() {
        return config::get("visual", "weapons_highlighted", default_selected());
    }

    inline bool selected(DWORD hash) {
        const std::map<DWORD, std::string> selected_weapons = selected();
        auto it = selected_weapons.find(hash);
        return it != selected_weapons.end() && it->second == "1";
    }

    inline bool highlighted(DWORD hash) {
        return exists(hash) && selected(hash);
    }

    inline bool custom_highlighted(DWORD hash) {
        return custom_exists(hash) && selected(hash);
    }

    inline ImU32 accent_color() {
        return IM_COL32(
            (int)(config::get("hud", "accent_r", 180.f / 255.f) * 255.f),
            (int)(config::get("hud", "accent_g", 167.f / 255.f) * 255.f),
            (int)(config::get("hud", "accent_b", 245.f / 255.f) * 255.f),
            255
        );
    }

    inline ImU32 default_color(DWORD hash, ImU32 fallback, ImU32 accent) {
        switch (hash) {
        case 0x0C472FE2u:
            return IM_COL32(0, 255, 0, 255);
        case 0x0A914799u:
            return accent;
        case 0x6E7DDDECu:
            return IM_COL32(66, 170, 255, 255);
        default:
            return fallback;
        }
    }

    inline ImU32 color(DWORD hash, ImU32 fallback) {
        if (!highlighted(hash)) {
            return fallback;
        }

        const ImU32 default_col = default_color(hash, fallback, accent_color());
        return visual_config::to_u32(
            visual_config::map_color(custom_colors(), hash, visual_config::from_u32(default_col))
        );
    }

    inline bool ignore_friend(bool is_friend) {
        return is_friend && config::get("visual", "weapons_ignore_friends", 1) != 0;
    }

    inline bool enabled() {
        return config::get("visual", "weapons_enabled", 0) != 0 ||
            config::get("visual", "weapons_widget", 0) != 0 ||
            config::get("visual", "weapons_highlight", 0) != 0 ||
            config::get("visual", "weapons_sound", 0) != 0;
    }

    inline state read_state() {
        state value;
        value.any_enabled = enabled();
        value.highlight_enabled = config::get("visual", "weapons_highlight", 0) != 0;
        value.ignore_friends = config::get("visual", "weapons_ignore_friends", 1) != 0;
        value.selected = selected();
        value.custom = custom_entries();
        value.custom_names = custom_names();
        value.custom_colors = custom_colors();
        value.lines = line_settings();
        value.accent = accent_color();
        return value;
    }

    inline bool custom_exists(const state& state, DWORD hash) {
        const auto it = state.custom.find(hash);
        return it != state.custom.end() && !it->second.empty();
    }

    inline bool exists(const state& state, DWORD hash) {
        return find(hash) || custom_exists(state, hash);
    }

    inline std::string base_label(const state& state, DWORD hash) {
        if (const weapon_entry* entry = find(hash)) {
            return entry->label;
        }

        const auto it = state.custom.find(hash);
        return it != state.custom.end() ? it->second : std::string{};
    }

    inline bool has_custom_name(const state& state, DWORD hash) {
        const auto name_it = state.custom_names.find(hash);
        return name_it != state.custom_names.end() && !name_it->second.empty();
    }

    inline std::string label(const state& state, DWORD hash) {
        const auto name_it = state.custom_names.find(hash);
        if (name_it != state.custom_names.end() && !name_it->second.empty()) {
            return name_it->second;
        }

        return base_label(state, hash);
    }

    inline bool selected(const state& state, DWORD hash) {
        const auto it = state.selected.find(hash);
        return it != state.selected.end() && it->second == "1";
    }

    inline bool highlighted(const state& state, DWORD hash) {
        return exists(state, hash) && selected(state, hash);
    }

    inline ImU32 color(const state& state, DWORD hash, ImU32 fallback) {
        if (!highlighted(state, hash)) {
            return fallback;
        }

        const ImU32 default_col = default_color(hash, fallback, state.accent);
        return visual_config::to_u32(
            visual_config::map_color(state.custom_colors, hash, visual_config::from_u32(default_col))
        );
    }

    inline bool ignore_friend(const state& state, bool is_friend) {
        return is_friend && state.ignore_friends;
    }

    inline bool line_enabled(const state& state, DWORD hash) {
        return line_enabled(state.lines, hash);
    }

    inline bool esp_highlight_active(const state& state, DWORD hash, bool is_friend) {
        return state.any_enabled &&
            state.highlight_enabled &&
            !ignore_friend(state, is_friend) &&
            highlighted(state, hash);
    }

    inline ImU32 esp_color(const state& state, DWORD hash, ImU32 fallback, bool is_friend) {
        if (!esp_highlight_active(state, hash, is_friend)) {
            return fallback;
        }
        return color(state, hash, fallback);
    }

    inline bool esp_line(const state& state, DWORD hash, bool is_friend) {
        return esp_highlight_active(state, hash, is_friend) &&
            line_enabled(state, hash);
    }

    inline bool esp_custom_label(const state& state, DWORD hash, bool is_friend) {
        return esp_highlight_active(state, hash, is_friend) &&
            !label(state, hash).empty();
    }

    inline ImU32 esp_color(DWORD hash, ImU32 fallback, bool is_friend) {
        if (!enabled() || config::get("visual", "weapons_highlight", 0) == 0 || ignore_friend(is_friend)) {
            return fallback;
        }
        return color(hash, fallback);
    }

    inline bool line_enabled(DWORD hash) {
        return line_enabled(line_settings(), hash);
    }

    inline bool esp_line(DWORD hash, bool is_friend) {
        return enabled() &&
            config::get("visual", "weapons_highlight", 0) != 0 &&
            !ignore_friend(is_friend) &&
            highlighted(hash) &&
            line_enabled(hash);
    }

    inline bool esp_custom_label(DWORD hash, bool is_friend) {
        return enabled() &&
            config::get("visual", "weapons_highlight", 0) != 0 &&
            !ignore_friend(is_friend) &&
            highlighted(hash) &&
            !label(hash).empty();
    }
}
