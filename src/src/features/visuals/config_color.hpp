#pragma once

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <imgui.h>

namespace visual_config {
    struct rgba {
        float r = 1.f;
        float g = 1.f;
        float b = 1.f;
        float a = 1.f;
    };

    inline float clamp01(float value) {
        return std::clamp(value, 0.f, 1.f);
    }

    inline rgba normalize(rgba color) {
        color.r = clamp01(color.r);
        color.g = clamp01(color.g);
        color.b = clamp01(color.b);
        color.a = clamp01(color.a);
        return color;
    }

    inline std::string format_rgba(rgba color) {
        color = normalize(color);
        char buffer[96];
        std::snprintf(buffer, sizeof(buffer), "%.6g|%.6g|%.6g|%.6g", color.r, color.g, color.b, color.a);
        return buffer;
    }

    inline bool parse_part(const std::string& value, size_t& pos, float& out) {
        if (pos == std::string::npos || pos >= value.size()) {
            return false;
        }

        char* end = nullptr;
        const char* begin = value.c_str() + pos;
        const float parsed = std::strtof(begin, &end);
        if (end == begin) {
            return false;
        }

        const char* value_end = value.c_str() + value.size();
        if (end < value_end && *end != '|') {
            return false;
        }

        out = parsed;
        pos = end == value_end ? std::string::npos : static_cast<size_t>(end - value.c_str()) + 1;
        return true;
    }

    inline bool parse_rgba(const std::string& value, rgba& out) {
        size_t pos = 0;
        rgba parsed{};
        if (!parse_part(value, pos, parsed.r) || pos == std::string::npos) return false;
        if (!parse_part(value, pos, parsed.g) || pos == std::string::npos) return false;
        if (!parse_part(value, pos, parsed.b) || pos == std::string::npos) return false;
        if (!parse_part(value, pos, parsed.a) || pos != std::string::npos) return false;

        out = normalize(parsed);
        return true;
    }

    inline rgba from_float4(const float color[4]) {
        return normalize({ color[0], color[1], color[2], color[3] });
    }

    inline void to_float4(rgba color, float out[4]) {
        color = normalize(color);
        out[0] = color.r;
        out[1] = color.g;
        out[2] = color.b;
        out[3] = color.a;
    }

    inline rgba from_u32(ImU32 color) {
        return {
            ((color >> IM_COL32_R_SHIFT) & 0xFF) / 255.f,
            ((color >> IM_COL32_G_SHIFT) & 0xFF) / 255.f,
            ((color >> IM_COL32_B_SHIFT) & 0xFF) / 255.f,
            ((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.f
        };
    }

    inline ImU32 to_u32(rgba color) {
        color = normalize(color);
        return IM_COL32(
            static_cast<int>(color.r * 255.f),
            static_cast<int>(color.g * 255.f),
            static_cast<int>(color.b * 255.f),
            static_cast<int>(color.a * 255.f)
        );
    }

    template <typename Key>
    inline rgba map_color(const std::map<Key, std::string>& colors, Key key, rgba fallback) {
        const auto it = colors.find(key);
        if (it == colors.end()) {
            return normalize(fallback);
        }

        rgba parsed{};
        return parse_rgba(it->second, parsed) ? parsed : normalize(fallback);
    }
}
