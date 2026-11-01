#pragma once

#include <winsock2.h>
#include <Windows.h>

namespace runtime_init {
    DWORD __stdcall initialize_thread_proc(PVOID);
}
