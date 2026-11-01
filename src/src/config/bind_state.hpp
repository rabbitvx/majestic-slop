#pragma once

#include "../core/imports.h"
#include <cstdio>
#include <cstring>

namespace bind_state {
    struct bind_config {
        const char* category;
        const char* enable_key;
        const char* legacy_key;
        const char* binds_key;
        const char* legacy_mode_key;
        const char* binds_mode_key;
    };

    struct runtime_bind_state {
        bool toggled_on = false;
        bool last_down = false;
        bool waiting_for_release = false;
    };

    struct runtime_bind_slot {
        bool used = false;
        char id[128] = {};
        runtime_bind_state state;
    };

    inline runtime_bind_slot runtime_slots[128];

    inline void make_id(const bind_config& bind, char* out, size_t out_size) {
        snprintf(
            out,
            out_size,
            "%s:%s:%s:%s:%s:%s",
            bind.category ? bind.category : "",
            bind.enable_key ? bind.enable_key : "",
            bind.legacy_key ? bind.legacy_key : "",
            bind.binds_key ? bind.binds_key : "",
            bind.legacy_mode_key ? bind.legacy_mode_key : "",
            bind.binds_mode_key ? bind.binds_mode_key : "");
    }

    inline runtime_bind_state& state_for(const bind_config& bind) {
        char id[128] = {};
        make_id(bind, id, sizeof(id));

        for (auto& slot : runtime_slots) {
            if (slot.used && strcmp(slot.id, id) == 0) {
                return slot.state;
            }
        }

        for (auto& slot : runtime_slots) {
            if (!slot.used) {
                strncpy_s(slot.id, id, _TRUNCATE);
                slot.used = true;
                return slot.state;
            }
        }

        return runtime_slots[0].state;
    }

    inline int read_key(const bind_config& bind) {
        if (bind.binds_key && bind.binds_key[0]) {
            const int key = config::get("binds", bind.binds_key, 0);
            if (key != 0) {
                return key;
            }
        }

        if (bind.category && bind.legacy_key && bind.legacy_key[0]) {
            const int key = config::get(bind.category, bind.legacy_key, 0);
            if (key != 0) {
                return key;
            }

            if (!strcmp(bind.category, "hacks") && !strcmp(bind.legacy_key, "skip_anim_key")) {
                return config::get(bind.category, "Skip_anim", 0);
            }
        }

        return 0;
    }

    inline int read_mode(const bind_config& bind) {
        int mode = 0;

        if (bind.binds_mode_key && bind.binds_mode_key[0]) {
            mode = config::get("binds", bind.binds_mode_key, 0);
        }

        if (mode == 0 && bind.category && bind.legacy_mode_key && bind.legacy_mode_key[0]) {
            mode = config::get(bind.category, bind.legacy_mode_key, 0);
        }

        if (mode < 0 || mode > 2) {
            mode = 0;
        }

        return mode;
    }

    inline bool is_enabled(const bind_config& bind) {
        if (!bind.category || !bind.enable_key) {
            return false;
        }

        return config::get(bind.category, bind.enable_key, 0) != 0;
    }

    inline bool has_game_input_focus() {
        if (!Game.window) {
            return true;
        }

        const HWND foreground = GetForegroundWindow();
        if (!foreground) {
            return false;
        }

        const HWND game_root = GetAncestor(Game.window, GA_ROOT);
        const HWND foreground_root = GetAncestor(foreground, GA_ROOT);

        if (!game_root) {
            return foreground == Game.window;
        }

        return foreground == Game.window || foreground_root == game_root;
    }

    inline bool is_key_down_raw(int key) {
        if (key <= 0 || key >= 256) {
            return false;
        }

        return Game.keyStates[key] || ((GetAsyncKeyState(key) & 0x8000) != 0);
    }

    inline bool has_game_bind_input() {
        return has_game_input_focus() && !Game.menuOpen;
    }

    inline bool is_key_down(int key) {
        return has_game_bind_input() && is_key_down_raw(key);
    }

    inline void clear_runtime_state(const bind_config& bind, bool down = false) {
        runtime_bind_state& state = state_for(bind);
        state.toggled_on = false;
        state.last_down = down;
        state.waiting_for_release = false;
    }

    inline bool is_active(const bind_config& bind) {
        const int key = read_key(bind);
        const int mode = read_mode(bind);
        const bool focused = has_game_bind_input();
        const bool raw_down = is_key_down_raw(key);

        if (!is_enabled(bind)) {
            clear_runtime_state(bind, false);
            return false;
        }

        if (key == 0) {
            clear_runtime_state(bind, false);
            return mode == 2;
        }

        runtime_bind_state& state = state_for(bind);

        if (!focused) {
            state.last_down = false;
            if (raw_down) {
                state.waiting_for_release = true;
            }

            if (mode == 0) {
                return state.toggled_on;
            }

            return mode == 2;
        }

        if (state.waiting_for_release) {
            if (raw_down) {
                state.last_down = false;

                if (mode == 0) {
                    return state.toggled_on;
                }

                return mode == 2;
            }

            state.waiting_for_release = false;
        }

        if (mode == 2) {
            state.last_down = raw_down;
            state.toggled_on = false;
            return true;
        }

        if (mode == 0) {
            if (raw_down && !state.last_down) {
                state.toggled_on = !state.toggled_on;
            }

            state.last_down = raw_down;
            return state.toggled_on;
        }

        state.last_down = raw_down;
        state.toggled_on = false;
        return raw_down;
    }

    inline bool is_active_or_enabled_when_unbound(const bind_config& bind) {
        if (!is_enabled(bind)) {
            clear_runtime_state(bind, false);
            return false;
        }

        if (read_key(bind) == 0) {
            clear_runtime_state(bind, false);
            return true;
        }

        return is_active(bind);
    }
}
