#pragma once

namespace hacks {
    inline void hacks_tick() {
        // nop
    }

    void game_tick() {
        update_hacks();
        if (IsValidPtr(local.player) && _SafeCheckPlayerAlive()) {
            runtime_debug::last_section = "gt_hacks_misc_tick";
            hacks_tick();
        }
    }
}