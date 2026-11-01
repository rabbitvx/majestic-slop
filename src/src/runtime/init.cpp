#include "runtime/init.hpp"

#include "core/imports.h"
#include "network/ws_bridge.hpp"
#include "platform/build_profile.hpp"
#include "platform/crash_logger.hpp"
#include "platform/debug.hpp"
#include "platform/vmp.hpp"
#include "runtime/context.hpp"
#include "runtime/session.hpp"
#include "runtime/tasks.hpp"
#include "runtime/tls_gate.hpp"

#include <cstdio>
#include <ctime>

#pragma comment(lib, "winmm.lib")

namespace {
    bool g_initialized = false;

    void patch_wnd_proc() {
        __try {
            hook::patch_wndproc();
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            HELLO_RUNTIME_LOG("SEH: patch_wndproc 0x%08X", GetExceptionCode());
        }
    }

    bool initialize_release_runtime() {
        HELLO_VMP_SCOPE_ULTRA("initialize_release_runtime");
        runtime_context::context_view context{};
        if (!runtime_context::read(context)) {
            if constexpr (build_profile::debug) {
                HELLO_RUNTIME_LOG("debug runtime: offline direct-inject mode");
                return true;
            }
            HELLO_RUNTIME_LOG("release runtime failed: context read failed");
            return false;
        }

        HELLO_RUNTIME_LOG(
            "runtime context: session_len=%zu product=%s tg_len=%zu hwid_len=%zu seed_empty=%d",
            context.product_session.size(),
            context.product_code.c_str(),
            context.tg_id.size(),
            context.hwid.size(),
            runtime_context::seed_is_empty(context.context_seed) ? 1 : 0
        );

        runtime_session::state session{};
        session.ready = true;
        strncpy_s(session.product_session, context.product_session.c_str(), _TRUNCATE);
        strncpy_s(session.product_code, context.product_code.empty() ? "altv" : context.product_code.c_str(), _TRUNCATE);
        strncpy_s(session.hwid, context.hwid.c_str(), _TRUNCATE);
        session.runtime_seed = context.context_seed;
        session.pointer_xor_seed = context.context_seed;
        runtime_session::set(session);

        if constexpr (build_profile::production) {
            tls_gate::set_stage(1);
        }

        config::deserialize_from_json("{\"normal\":{},\"string_map\":{},\"dword_map\":{},\"vector3_map\":{}}");
        return true;
    }

    void initialize() {
        runtime_debug::last_section = "initialize_start";
        HELLO_RUNTIME_LOG("initialize started");

        HELLO_VMP_BEGIN_ULTRA("initialize.release_gate");
        if (!initialize_release_runtime()) {
            HELLO_VMP_END();
            return;
        }
        HELLO_VMP_END();
        if (!g_initialized) {
            g_initialized = true;
        }
        __try { runtime_debug::last_section = "config_load"; config::load(); }
        __except(EXCEPTION_EXECUTE_HANDLER) { HELLO_RUNTIME_LOG("SEH: config::load 0x%08X", GetExceptionCode()); }

        runtime_debug::last_section = "findAllPatterns";
        HELLO_RUNTIME_LOG("calling findAllPatterns...");
        if (pointers::findAllPatterns()) {
            runtime_debug::last_section = "hook_enable";
            HELLO_RUNTIME_LOG("calling hook::enable...");
            if (hook::enable()) {
                Game.running = true;
                HELLO_RUNTIME_LOG("hook::enable OK, Game.running=true");

                runtime_tasks::start_ws_server();
                // Payload collection: 500ms interval (fast flags). A/B proved
                // payload and the log reader are NOT the voice-drop cause.
                if (config::get("misc", "payload_enable", 1) != 0) {
                    __try { runtime_debug::last_section = "executor_start"; executor::start(); }
                    __except(EXCEPTION_EXECUTE_HANDLER) { HELLO_RUNTIME_LOG("SEH: executor::start 0x%08X", GetExceptionCode()); }
                }
            }
        }

        runtime_debug::last_section = "main_loop";
        HELLO_RUNTIME_LOG("entering main loop");
        while (Game.running) {
            if (runtime_session::unload_requested()) {
                Game.running = false;
                break;
            }
            // NOTE: patch_wnd_proc() used to re-install our WndProc hook every
            // 500ms. That overwrote the majestic client's own WndProc hook,
            // breaking its window-message handling (voice device switching on
            // minimize) — the mic died after minimizing. The hook is installed
            // once in hook::enable(); do not re-apply it here.
            std::this_thread::sleep_for(500ms);
        }

        Game.running = false;

        // Restore player control before tearing down: the menu freeze
        // (SET_PLAYER_CONTROL false while menuOpen) would otherwise leave the
        // player unable to move after unload.
        Game.menuOpen = false;
        __try {
            if (IsValidPtr(local.player)) {
                const int pid = native::player::player_id();
                if (pid >= 0) {
                    native::player::set_player_control(static_cast<native::type::player>(pid), true, 0);
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}

        HELLO_RUNTIME_LOG("shutdown: aimbot");
        __try { aimbot::shutdown(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        HELLO_RUNTIME_LOG("shutdown: unload scripts");
        __try { ws_server::unload_user_scripts_for_shutdown(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        HELLO_RUNTIME_LOG("shutdown: panic features");
        __try { hacks::disable_panic_features(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        HELLO_RUNTIME_LOG("shutdown: session clear");
        __try { runtime_session::clear(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        HELLO_RUNTIME_LOG("shutdown: game runtime");
        __try { game::shutdown_runtime(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        HELLO_RUNTIME_LOG("shutdown: executor");
        __try { executor::shutdown(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        HELLO_RUNTIME_LOG("shutdown: ws stop");
        __try { ws_server::stop(); } __except(EXCEPTION_EXECUTE_HANDLER) {}

        Sleep(1000);

        HELLO_RUNTIME_LOG("shutdown: hooks");
        __try { hook::disable(); } __except(EXCEPTION_EXECUTE_HANDLER) {}

        Sleep(500);

        HELLO_RUNTIME_LOG("shutdown: done");
    }
}

namespace runtime_init {
    DWORD __stdcall initialize_thread_proc(PVOID) {
        HELLO_VMP_BEGIN_ULTRA("initialize_thread_proc");
        crash_logger::install();
        __try {
            initialize();
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            HELLO_RUNTIME_LOG("SEH: initialize top-level 0x%08X section=%s", GetExceptionCode(), runtime_debug::last_section);
        }
        HELLO_VMP_END();

        if (!Game.running) {
            HMODULE self = Game.hModule ? Game.hModule : GetModuleHandleA(nullptr);
            if (self) {
                FreeLibraryAndExitThread(self, 0);
            }
        }
        return 1;
    }
}
