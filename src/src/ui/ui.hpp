#pragma once

#include "../core/imports.h"

namespace ui {
    inline char toAddHashSets[256] = {};
    inline char toAddHash[256] = {};
    inline char toAddHashVeh[256] = {};

    struct AppLog {
        void AddLog(const char* fmt, ...) {}
    };

    inline AppLog DEBUG;
}
