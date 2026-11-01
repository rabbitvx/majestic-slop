#include "runtime/tasks.hpp"

#include "core/imports.h"
#include "features/misc/core.hpp"
#include "network/ws_bridge.hpp"
#include "platform/debug.hpp"
#include "runtime/session.hpp"

namespace runtime_tasks {
    void start_ws_server() {
    }

    void start_aimbot() {
        aimbot::init();
    }
}
