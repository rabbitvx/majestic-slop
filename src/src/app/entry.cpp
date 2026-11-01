#include "core/imports.h"
#include "platform/crash_logger.hpp"
#include "runtime/tls_gate.hpp"
#include "runtime/init.hpp"

#define IMGUI_DISABLE_DEBUG_TOOLS

#include "runtime/tasks.cpp"
#include "runtime/init.cpp"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        Game.base = (uintptr_t)GetModuleHandleA(0);
        Game.hModule = hModule;
        tls_gate::process_attach(hModule);
        crash_logger::remember_module(hModule);

        Sleep(100);

        CreateThread(NULL, 0, runtime_init::initialize_thread_proc, NULL, 0, NULL);

        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
