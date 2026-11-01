#pragma once

#include <string>

namespace hello_notify {
    enum class status {
        success,
        error,
        info,
    };

    void push(const std::string& message, status type = status::info, float duration = 3.f);
}
