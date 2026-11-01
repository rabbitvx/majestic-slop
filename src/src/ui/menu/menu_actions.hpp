#pragma once
#include <atomic>

namespace menu_actions {
    inline std::atomic<bool> pending_set_hp{false};
    inline std::atomic<bool> pending_set_armor{false};
    inline std::atomic<bool> pending_tp_waypoint{false};
}
