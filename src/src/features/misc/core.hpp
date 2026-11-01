#pragma once
#include "ui/menu/menu_actions.hpp"
#include <string>
#include <utility>
#include <vector>

namespace hacks {
    inline int local_ped_handle() {
        if (IsValidPtr(local.player)) {
            uintptr_t ptr = reinterpret_cast<uintptr_t>(local.player);
            ::pointer_to_handle_t handle_fn = ::pointer_to_handle;
            return handle_fn ? handle_fn(static_cast<intptr_t>(ptr)) : 0;
        }
        return 0;
    }

    inline bool _SafeCheckPlayerAlive() {
        return IsValidPtr(local.player) && local.player->HP > 0;
    }

    inline void _update_hacks_inner();

    void update_hacks() {
        _update_hacks_inner();
    }

    void disable_panic_features() {
        // nop
    }

    struct panic_snapshot {
        bool active = false;
        std::vector<std::pair<std::string, int>> saved; // category/key -> value
        bool enabled = false; // visual.enable
    };
    inline panic_snapshot g_panic{};

    inline const std::vector<std::pair<std::string, std::string>>& panic_keys() {
        static const std::vector<std::pair<std::string, std::string>> keys = {
            { "visual", "enable" },
            { "visual", "draw_box" },
            { "visual", "altv_nickname" },
            { "visual", "altv_relation" },
            { "visual", "altv_faction" },
            { "visual", "draw_skeleton" },
            { "visual", "skel_preview_invisible" },
            { "visual", "draw_distance" },
            { "visual", "altv_static" },
            { "visual", "altv_admin" },
            { "visual", "altv_tester" },
            { "visual", "altv_level" },
            { "visual", "altv_dead" },
            { "visual", "altv_afk" },
            { "visual", "altv_media" },
            { "visual", "altv_cuffed" },
            { "visual", "altv_wanted" },
            { "visual", "altv_armor_flag" },
            { "visual", "altv_social" },
            { "visual", "altv_visible" },
            { "visual", "admins_around_indicator" },
            { "visual", "dim_dead_players" },
            { "visual", "cheap_render_text" },
            { "visual", "weapons_enabled" },
            { "visual", "weapons_widget" },
            { "visual", "weapon_arrows" },
            { "visual", "weapons_highlight" },
            { "visual", "world_esp" },
            { "visual", "radar" },
            { "visual", "pickup_esp" },
        };
        return keys;
    }

    inline void panic_activate() {
        if (g_panic.active) return;
        g_panic.saved.clear();
        for (const auto& [cat, key] : panic_keys()) {
            g_panic.saved.emplace_back(cat + "/" + key, config::get(cat, key, 0));
        }
        g_panic.enabled = config::get("visual", "enable", 0) != 0;
        for (const auto& [cat, key] : panic_keys()) {
            config::update(0, cat, key, 0);
        }
        g_panic.active = true;
    }

    inline void panic_restore() {
        if (!g_panic.active) return;
        for (const auto& [full, value] : g_panic.saved) {
            const size_t sep = full.find('/');
            if (sep == std::string::npos) continue;
            config::update(value, full.substr(0, sep), full.substr(sep + 1), 0);
        }
        if (g_panic.enabled) config::update(1, "visual", "enable", 0);
        g_panic.saved.clear();
        g_panic.active = false;
    }

    inline bind_state::bind_config panic_bind() {
        return bind_state::bind_config{
            "misc",            // category (legacy keys live here)
            "panic_enabled",   // enable_key (checkbox)
            "panic_key",       // legacy_key
            "panic_key",       // binds_key
            "panic_mode",      // legacy_mode_key
            "panic_mode"       // binds_mode_key
        };
    }

    inline bool panic_bind_active() {
        const bind_state::bind_config bind = panic_bind();
        const int key = bind_state::read_key(bind);
        const int mode = bind_state::read_mode(bind); // 0=toggle 1=hold 2=always
        const bool raw_down = bind_state::is_key_down_raw(key);

        if (key == 0) {
            return mode == 2;
        }

        bind_state::runtime_bind_state& state = bind_state::state_for(bind);
        const bool focused = bind_state::has_game_bind_input();

        if (!focused) {
            state.last_down = false;
            if (raw_down) state.waiting_for_release = true;
            if (mode == 0) return state.toggled_on;
            return mode == 2;
        }

        if (state.waiting_for_release) {
            if (raw_down) {
                state.last_down = false;
                if (mode == 0) return state.toggled_on;
                return mode == 2;
            }
            state.waiting_for_release = false;
        }

        if (mode == 2) { // always on
            state.last_down = raw_down;
            state.toggled_on = false;
            return true;
        }

        if (mode == 0) { // toggle
            if (raw_down && !state.last_down) {
                state.toggled_on = !state.toggled_on;
            }
            state.last_down = raw_down;
            return state.toggled_on;
        }

        // hold
        state.last_down = raw_down;
        state.toggled_on = false;
        return raw_down;
    }

    inline void tick_panic_key() {
        const bool active = panic_bind_active();
        if (active && !g_panic.active) {
            panic_activate();
        }
        else if (!active && g_panic.active) {
            panic_restore();
        }
    }

    inline void _update_hacks_inner() {
        tick_panic_key();
        // Menu key handled here (not via WndProc): a WndProc hook breaks the
        // majestic client's own window chain and kills voice on minimize.
        {
            const int menu_key = config::get("menu", "menu_key", 35); // 35 = VK_END
            const bool menu_down = (GetAsyncKeyState(menu_key) & 0x8000) != 0;
            static bool old_menu = false;
            if (menu_down && !old_menu) {
                Game.menuOpen = !Game.menuOpen;
            }
            old_menu = menu_down;
        }
        // While the menu is open, freeze player control so mouse/keys don't move
        // the camera or attack (the ImGui menu captures the input instead).
        if (IsValidPtr(local.player)) {
            static bool menu_control_frozen = false;
            const bool want_frozen = Game.menuOpen;
            if (want_frozen != menu_control_frozen) {
                menu_control_frozen = want_frozen;
                const int pid = native::player::player_id();
                if (pid >= 0) {
                    __try {
                        native::player::set_player_control(
                            static_cast<native::type::player>(pid), !want_frozen, 0);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {}
                }
            }
        }
        if (IsValidPtr(local.player) && _SafeCheckPlayerAlive()) {
            bool toggle_key = config::get("visual", "toggle_key", 0);
            if (toggle_key) {
                runtime_debug::last_section = "gt_hacks_visual_toggle";
                int toggle_key_num = config::get("visual", "toggle_key_num", 1);
                const bool toggle_down = bind_state::is_key_down(toggle_key_num);
                static bool old_state = false;
                if (toggle_down && !old_state) {
                    bool visuals_enable = config::get("visual", "enable", 0);
                    config::update(!visuals_enable, "visual", "enable", 0);
                }
                old_state = toggle_down;
            }
        }
    }

    Vector3 get_cam_directions() {
        const auto ped = local_ped_handle();
        const float entity_heading = ped ? native::entity::get_entity_heading(ped) : 0.0f;
        float heading = native::cam::get_gameplay_cam_relative_heading() + entity_heading;
        float pitch = native::cam::get_gameplay_cam_relative_pitch();
        float x = -sin(heading * PI / 180.0f);
        float y = cos(heading * PI / 180.0f);
        float z = sin(pitch * PI / 180.0f);
        double len = sqrt(x * x + y * y + z * z);
        if (len != 0) {
            x = x / len;
            y = y / len;
            z = z / len;
        }
        return Vector3(x, y, z);
    }
}