#pragma once
#include "core/imports.h"
#include "config/bind_state.hpp"
#include "ui/menu/gui.h"
#include "assets/heavy_warning_sound.hpp"
#include "weapons_highlight.hpp"
#include <ctime>
#include <sstream>
#include <iomanip>


extern imgui_render renderer;

namespace hud { // solus so cool xddd =DD (honestly it's ugly as fuck but I cba to make it pretty)
    static constexpr float HUD_FONT_SZ = 13.f;
    static constexpr float SOLUS_FONT_SZ = 12.f;
    static constexpr float SOLUS_FONT_SECONDARY_SZ = 10.f;
    static constexpr float SOLUS_PANEL_RADIUS = 12.f;
    static constexpr float SOLUS_FADE_SPEED = 10.f;

    static ID3D11Device* s_device = nullptr;

    static ImFont* get_hud_font() {
        if (renderer.hudFont) return renderer.hudFont;
        return ImGui::GetFont();
    }

    static void hud_text(ImDrawList* dl, ImVec2 pos, ImU32 col, const char* text) {
        dl->AddText(get_hud_font(), HUD_FONT_SZ, pos, col, text);
    }
    static ImVec2 hud_measure(const char* text) {
        return get_hud_font()->CalcTextSizeA(HUD_FONT_SZ, FLT_MAX, 0.f, text);
    }
    static void hud_text_sized(ImDrawList* dl, float size, ImVec2 pos, ImU32 col, const char* text) {
        dl->AddText(get_hud_font(), size, pos, col, text);
    }
    static ImVec2 hud_measure_sized(float size, const char* text) {
        return get_hud_font()->CalcTextSizeA(size, FLT_MAX, 0.f, text);
    }

    static float solus_clamp01(float value) {
        if (value < 0.f) return 0.f;
        if (value > 1.f) return 1.f;
        return value;
    }

    static int solus_to_byte(float value) {
        return (int)(solus_clamp01(value) * 255.f + 0.5f);
    }

    static ImU32 solus_accent(float alpha_scale = 1.f) {
        return IM_COL32(
            solus_to_byte(config::get("hud", "accent_r", 180.f / 255.f)),
            solus_to_byte(config::get("hud", "accent_g", 167.f / 255.f)),
            solus_to_byte(config::get("hud", "accent_b", 245.f / 255.f)),
            (int)(255.f * solus_clamp01(alpha_scale))
        );
    }

    static float solus_lerp(float start, float end, float time) {
        time = ImClamp(ImGui::GetIO().DeltaTime * time * 175.f, 0.01f, 1.f);
        float value = start + (end - start) * time;
        if (end == 0.f && value < 0.01f && value > -0.01f) value = 0.f;
        else if (end == 1.f && value < 1.01f && value > 0.99f) value = 1.f;
        return value;
    }

    static ImU32 solus_menu_color(float alpha) {
        return IM_COL32(
            (int)(ui::menu_col[0] * 255.f),
            (int)(ui::menu_col[1] * 255.f),
            (int)(ui::menu_col[2] * 255.f),
            (int)(255.f * solus_clamp01(alpha))
        );
    }

    static const char* get_bind_key_name(const char* cfg_cat, const char* cfg_key) {
        if (!cfg_cat || !cfg_key) return nullptr;
        if (!strcmp(cfg_cat, "aimbot") && !strcmp(cfg_key, "aim_key"))           return "vector_aim_key";
        if (!strcmp(cfg_cat, "aimbot") && !strcmp(cfg_key, "triggerbot_key"))    return "triggerbot_key";
        if (!strcmp(cfg_cat, "hacks")  && !strcmp(cfg_key, "skip_anim_key"))     return "skip_anim_key";
        if (!strcmp(cfg_cat, "hacks")  && !strcmp(cfg_key, "double_shoot_key"))  return "double_shoot_key";
        return nullptr;
    }

    static const char* get_bind_mode_name(const char* cfg_cat, const char* cfg_mode) {
        if (!cfg_cat || !cfg_mode) return nullptr;
        if (!strcmp(cfg_cat, "aimbot") && !strcmp(cfg_mode, "aim_key_mode"))           return "vector_aim_mode";
        if (!strcmp(cfg_cat, "aimbot") && !strcmp(cfg_mode, "triggerbot_key_mode"))    return "triggerbot_mode";
        if (!strcmp(cfg_cat, "hacks")  && !strcmp(cfg_mode, "skip_anim_key_mode"))     return "skip_anim_mode";
        if (!strcmp(cfg_cat, "hacks")  && !strcmp(cfg_mode, "double_shoot_key_mode"))  return "double_shoot_mode";
        return nullptr;
    }

    static bind_state::bind_config make_bind_config(const char* cfg_cat, const char* cfg_enable, const char* cfg_key, const char* cfg_mode) {
        return {
            cfg_cat,
            cfg_enable,
            cfg_key,
            cfg_mode,
            get_bind_key_name(cfg_cat, cfg_key),
            get_bind_mode_name(cfg_cat, cfg_mode)
        };
    }

    static void get_wm_color(int& r, int& g, int& b, int& a) {
        r = (int)(config::get("hud", "accent_r", 0.35f) * 255);
        g = (int)(config::get("hud", "accent_g", 0.47f) * 255);
        b = (int)(config::get("hud", "accent_b", 0.94f) * 255);
        a = 255;
    }
    static void get_kb_color(int& r, int& g, int& b, int& a) {
        r = (int)(config::get("hud", "kb_col_r", 0.35f) * 255);
        g = (int)(config::get("hud", "kb_col_g", 0.47f) * 255);
        b = (int)(config::get("hud", "kb_col_b", 0.94f) * 255);
        a = 255;
    }
    static void get_info_color(int& r, int& g, int& b, int& a) {
        r = (int)(config::get("hud", "info_col_r", 0.35f) * 255);
        g = (int)(config::get("hud", "info_col_g", 0.47f) * 255);
        b = (int)(config::get("hud", "info_col_b", 0.94f) * 255);
        a = 255;
    }

    static void create_window(ImDrawList* dl, float x, float y, float w, float h,
                              int r, int g, int b, int a,
                              bool left_bar, bool top_outline) {
        const float rnd = 5.f;

        dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), IM_COL32(0, 0, 0, 50), rnd);

        dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), IM_COL32(r, g, b, 80), rnd);

        for (int i = 1; i <= 10; i++) {
            int ga = (int)((60.f - (60.f / 10.f) * i) * (a / 255.f));
            if (ga <= 0) continue;
            dl->AddRect(
                ImVec2(x - i, y - i), ImVec2(x + w + i, y + h + i),
                IM_COL32(r, g, b, ga), rnd + 2.f
            );
        }

        if (top_outline) {
            dl->AddLine(ImVec2(x + 4, y), ImVec2(x + w - 4, y), IM_COL32(r, g, b, a), 1.f);
            dl->PathArcTo(ImVec2(x + 4, y + 4), 3.f, IM_PI, IM_PI * 1.5f, 8);
            dl->PathStroke(IM_COL32(r, g, b, a), 0, 1.f);
            dl->PathArcTo(ImVec2(x + w - 5, y + 4), 3.f, IM_PI * 1.5f, IM_PI * 2.f, 8);
            dl->PathStroke(IM_COL32(r, g, b, a), 0, 1.f);
            dl->AddRectFilledMultiColor(
                ImVec2(x, y + 4), ImVec2(x + 1, y + h - 2),
                IM_COL32(r, g, b, a), IM_COL32(r, g, b, a),
                IM_COL32(r, g, b, 0), IM_COL32(r, g, b, 0));
            dl->AddRectFilledMultiColor(
                ImVec2(x + w - 1, y + 4), ImVec2(x + w, y + h - 2),
                IM_COL32(r, g, b, a), IM_COL32(r, g, b, a),
                IM_COL32(r, g, b, 0), IM_COL32(r, g, b, 0));
        }

        if (left_bar) {
            dl->AddLine(ImVec2(x, y + 4), ImVec2(x, y + h - 4), IM_COL32(r, g, b, a), 1.f);
            dl->PathArcTo(ImVec2(x + 4, y + 4), 3.f, IM_PI, IM_PI * 1.5f, 8);
            dl->PathStroke(IM_COL32(r, g, b, a), 0, 1.f);
            dl->PathArcTo(ImVec2(x + 4, y + h - 5), 3.f, IM_PI * 0.5f, IM_PI, 8);
            dl->PathStroke(IM_COL32(r, g, b, a), 0, 1.f);
            dl->AddRectFilledMultiColor(
                ImVec2(x + 4, y), ImVec2(x + 19, y + 1),
                IM_COL32(r, g, b, a), IM_COL32(r, g, b, 0),
                IM_COL32(r, g, b, 0), IM_COL32(r, g, b, a));
            dl->AddRectFilledMultiColor(
                ImVec2(x + 4, y + h - 1), ImVec2(x + 19, y + h),
                IM_COL32(r, g, b, a), IM_COL32(r, g, b, 0),
                IM_COL32(r, g, b, 0), IM_COL32(r, g, b, a));
        }
    }

    static void draw_solus_container(ImDrawList* dl, float x, float y, float w, float h, float alpha);

    struct hud_widget_state {
        ImVec2 pos = ImVec2(-1.f, -1.f);
        ImVec2 loaded_pos = ImVec2(FLT_MAX, FLT_MAX);
        ImVec2 last_size = ImVec2(0.f, 0.f);
        ImVec2 drag_delta = ImVec2(0.f, 0.f);
        bool initialized = false;
        bool dragging = false;
        bool snap_x = false;
        bool snap_y = false;
        bool snap_top = false;
        float line_x = 0.f;
        float line_y = 0.f;
        float line_top = 0.f;
        float press_anim = 0.f;
        float snap_x_anim = 0.f;
        float snap_y_anim = 0.f;
        float snap_top_anim = 0.f;
        float line_x_anim = 0.f;
        float line_y_anim = 0.f;
        float line_top_anim = 0.f;
    };

    static constexpr float HUD_WIDGET_SNAP_THRESHOLD = 10.f;
    static constexpr float HUD_WIDGET_SNAP_LINE_VISIBILITY_THRESHOLD = 80.f;
    static constexpr float HUD_WIDGET_TOP_SNAP_Y = 20.f;
    static hud_widget_state* hud_drag_owner = nullptr;
    static bool hud_drag_feedback_drawn = false;
    static float hud_drag_dim_anim = 0.f;

    static bool hud_widget_any_dragging() {
        return hud_drag_owner && hud_drag_owner->dragging;
    }

    static void hud_widget_cancel_drag(hud_widget_state& widget) {
        widget.dragging = false;
        widget.snap_x = false;
        widget.snap_y = false;
        widget.snap_top = false;
        widget.line_x = 0.f;
        widget.line_y = 0.f;
        widget.line_top = 0.f;
        if (hud_drag_owner == &widget) {
            hud_drag_owner = nullptr;
        }
    }

    static ImVec2 hud_widget_clamp_pos(ImVec2 pos, const ImVec2& size) {
        const float max_x = (std::max)(0.f, Game.screen.x - size.x);
        const float max_y = (std::max)(0.f, Game.screen.y - size.y);
        pos.x = ImClamp(pos.x, 0.f, max_x);
        pos.y = ImClamp(pos.y, 0.f, max_y);
        return pos;
    }

    static bool hud_widget_pos_changed(const ImVec2& a, const ImVec2& b) {
        return fabsf(a.x - b.x) > 0.01f || fabsf(a.y - b.y) > 0.01f;
    }

    static void hud_widget_sync_pos(
        hud_widget_state& widget,
        const char* cfg_x,
        const char* cfg_y,
        const ImVec2& default_pos,
        const ImVec2& size
    ) {
        const ImVec2 cfg_pos(
            config::get("hud", cfg_x, default_pos.x),
            config::get("hud", cfg_y, default_pos.y)
        );

        if (!widget.initialized || (!widget.dragging && hud_widget_pos_changed(widget.loaded_pos, cfg_pos))) {
            widget.pos = cfg_pos;
            widget.loaded_pos = cfg_pos;
            widget.initialized = true;
        }

        widget.pos = hud_widget_clamp_pos(widget.pos, size);

        if (!widget.dragging && widget.last_size.x > 0.f && fabsf(widget.last_size.x - size.x) > 0.01f) {
            const float screen_center_x = Game.screen.x * 0.5f;
            const float previous_center_x = widget.pos.x + widget.last_size.x * 0.5f;
            if (fabsf(previous_center_x - screen_center_x) <= HUD_WIDGET_SNAP_THRESHOLD) {
                widget.pos.x = screen_center_x - size.x * 0.5f;
                widget.pos = hud_widget_clamp_pos(widget.pos, size);
            }
        }
    }

    static void hud_widget_save_pos(
        hud_widget_state& widget,
        const char* cfg_x,
        const char* cfg_y,
        const ImVec2& default_pos
    ) {
        config::update(widget.pos.x, "hud", cfg_x, default_pos.x);
        config::update(widget.pos.y, "hud", cfg_y, default_pos.y);
        widget.loaded_pos = widget.pos;
    }

    static bool hud_widget_hovered(const hud_widget_state& widget, const ImVec2& size) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        return mouse.x >= widget.pos.x && mouse.y >= widget.pos.y &&
            mouse.x <= widget.pos.x + size.x && mouse.y <= widget.pos.y + size.y;
    }

    static void hud_widget_update_drag(
        hud_widget_state& widget,
        const char* cfg_x,
        const char* cfg_y,
        const ImVec2& default_pos,
        const ImVec2& size
    ) {
        widget.snap_x = false;
        widget.snap_y = false;
        widget.snap_top = false;
        widget.line_x = 0.f;
        widget.line_y = 0.f;
        widget.line_top = 0.f;
        hud_widget_sync_pos(widget, cfg_x, cfg_y, default_pos, size);
        widget.last_size = size;

        if (hud_drag_owner && !hud_drag_owner->dragging) {
            hud_drag_owner = nullptr;
        }

        if (!Game.menuOpen) {
            if (widget.dragging) {
                widget.dragging = false;
                hud_widget_save_pos(widget, cfg_x, cfg_y, default_pos);
                if (hud_drag_owner == &widget) {
                    hud_drag_owner = nullptr;
                }
            }
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        if (!hud_widget_any_dragging() && hud_widget_hovered(widget, size) && ImGui::IsMouseClicked(0)) {
            widget.dragging = true;
            widget.drag_delta = ImVec2(widget.pos.x - io.MousePos.x, widget.pos.y - io.MousePos.y);
            hud_drag_owner = &widget;
        }

        if (!widget.dragging) {
            return;
        }

        if (!ImGui::IsMouseDown(0)) {
            widget.dragging = false;
            hud_widget_save_pos(widget, cfg_x, cfg_y, default_pos);
            if (hud_drag_owner == &widget) {
                hud_drag_owner = nullptr;
            }
            return;
        }

        ImVec2 next_pos(io.MousePos.x + widget.drag_delta.x, io.MousePos.y + widget.drag_delta.y);
        next_pos = hud_widget_clamp_pos(next_pos, size);

        const float screen_center_x = Game.screen.x * 0.5f;
        const float screen_center_y = Game.screen.y * 0.5f;
        const float widget_center_x = next_pos.x + size.x * 0.5f;
        const float widget_center_y = next_pos.y + size.y * 0.5f;
        const float center_x_distance = fabsf(widget_center_x - screen_center_x);
        const float center_y_distance = fabsf(widget_center_y - screen_center_y);
        const float top_distance = fabsf(next_pos.y - HUD_WIDGET_TOP_SNAP_Y);

        auto snap_line_alpha = [](float distance) {
            if (distance >= HUD_WIDGET_SNAP_LINE_VISIBILITY_THRESHOLD) return 0.f;
            return 1.f - distance / HUD_WIDGET_SNAP_LINE_VISIBILITY_THRESHOLD;
        };

        widget.line_x = snap_line_alpha(center_x_distance);
        widget.line_y = snap_line_alpha(center_y_distance);
        widget.line_top = snap_line_alpha(top_distance);

        if (center_x_distance <= HUD_WIDGET_SNAP_THRESHOLD) {
            next_pos.x = screen_center_x - size.x * 0.5f;
            widget.snap_x = true;
        }

        if (center_y_distance <= HUD_WIDGET_SNAP_THRESHOLD) {
            next_pos.y = screen_center_y - size.y * 0.5f;
            widget.snap_y = true;
        }
        else if (top_distance <= HUD_WIDGET_SNAP_THRESHOLD) {
            next_pos.y = HUD_WIDGET_TOP_SNAP_Y;
            widget.snap_top = true;
        }

        widget.pos = hud_widget_clamp_pos(next_pos, size);
    }

    static void hud_widget_update_anim(hud_widget_state& widget) {
        widget.press_anim = solus_lerp(widget.press_anim, widget.dragging ? 1.f : 0.f, 0.12f);
        widget.snap_x_anim = solus_lerp(widget.snap_x_anim, widget.snap_x ? 1.f : 0.f, 0.16f);
        widget.snap_y_anim = solus_lerp(widget.snap_y_anim, widget.snap_y ? 1.f : 0.f, 0.16f);
        widget.snap_top_anim = solus_lerp(widget.snap_top_anim, widget.snap_top ? 1.f : 0.f, 0.16f);
        widget.line_x_anim = solus_lerp(widget.line_x_anim, widget.line_x, 0.16f);
        widget.line_y_anim = solus_lerp(widget.line_y_anim, widget.line_y, 0.16f);
        widget.line_top_anim = solus_lerp(widget.line_top_anim, widget.line_top, 0.16f);
    }

    static float hud_widget_draw_alpha(const hud_widget_state& widget, float alpha) {
        return alpha * (1.f - 0.3f * widget.press_anim);
    }

    static ImU32 hud_apply_alpha(ImU32 color, float alpha_scale) {
        const int alpha = (int)(((color >> IM_COL32_A_SHIFT) & 0xFF) * solus_clamp01(alpha_scale));
        return (color & ~IM_COL32_A_MASK) | ((ImU32)alpha << IM_COL32_A_SHIFT);
    }

    static void hud_reset_drag_feedback() {
        hud_drag_feedback_drawn = false;
    }

    static void hud_draw_drag_feedback(ImDrawList* dl) {
        hud_drag_dim_anim = solus_lerp(hud_drag_dim_anim, hud_widget_any_dragging() ? 1.f : 0.f, 0.10f);

        if (!dl || hud_drag_feedback_drawn || hud_drag_dim_anim <= 0.01f) {
            return;
        }

        hud_drag_feedback_drawn = true;
        const float screen_center_x = Game.screen.x * 0.5f;
        const float screen_center_y = Game.screen.y * 0.5f;
        const float snap_x = hud_drag_owner ? hud_drag_owner->snap_x_anim : 0.f;
        const float snap_y = hud_drag_owner ? hud_drag_owner->snap_y_anim : 0.f;
        const float snap_top = hud_drag_owner ? hud_drag_owner->snap_top_anim : 0.f;
        const float line_x_anim = hud_drag_owner ? hud_drag_owner->line_x_anim : 0.f;
        const float line_y_anim = hud_drag_owner ? hud_drag_owner->line_y_anim : 0.f;
        const float line_top_anim = hud_drag_owner ? hud_drag_owner->line_top_anim : 0.f;
        const int dim_alpha = (int)(55.f * hud_drag_dim_anim);
        const ImU32 dim_col = IM_COL32(0, 0, 0, dim_alpha);
        const ImU32 line_x = IM_COL32(255, 255, 255, (int)(ImLerp(80.f, 230.f, snap_x) * line_x_anim * hud_drag_dim_anim));
        const ImU32 line_y = IM_COL32(255, 255, 255, (int)(ImLerp(80.f, 230.f, snap_y) * line_y_anim * hud_drag_dim_anim));
        const ImU32 line_top = IM_COL32(255, 255, 255, (int)(ImLerp(80.f, 230.f, snap_top) * line_top_anim * hud_drag_dim_anim));

        dl->AddRectFilled(ImVec2(0.f, 0.f), ImVec2(Game.screen.x, Game.screen.y), dim_col);
        if (line_x_anim > 0.01f) dl->AddLine(ImVec2(screen_center_x, 0.f), ImVec2(screen_center_x, Game.screen.y), line_x, 1.f);
        if (line_y_anim > 0.01f) dl->AddLine(ImVec2(0.f, screen_center_y), ImVec2(Game.screen.x, screen_center_y), line_y, 1.f);
        if (line_top_anim > 0.01f) dl->AddLine(ImVec2(0.f, HUD_WIDGET_TOP_SNAP_Y), ImVec2(Game.screen.x, HUD_WIDGET_TOP_SNAP_Y), line_top, 1.f);
    }

    static hud_widget_state watermark_widget;
    static hud_widget_state keybind_widget;
    static hud_widget_state weapons_widget;

    struct watermark_part {
        std::string text;
        ImU32 color;
        bool compact_gap = false;
    };

    struct watermark_render_state {
        bool visible = false;
        float width = 0.f;
        char info[64] = {};
    };

    static watermark_render_state watermark_render;

    static std::string watermark_get_date() {
        const std::time_t now = std::time(nullptr);
        std::tm local_tm {};
        localtime_s(&local_tm, &now);

        std::ostringstream date_stream;
        date_stream << std::put_time(&local_tm, "%d.%m.%Y");
        return date_stream.str();
    }

    static std::vector<watermark_part> watermark_build_parts() {
        const ImU32 accent = solus_accent();
        const ImU32 white = IM_COL32(255, 255, 255, 255);
        const ImU32 muted = IM_COL32(150, 150, 150, 255);

        std::vector<watermark_part> parts;
        parts.push_back({ "hello", accent, false });
        parts.push_back({ std::to_string((int)std::lround(ImGui::GetIO().Framerate)), white, false });
        parts.push_back({ "fps", muted, true });
        parts.push_back({ watermark_get_date(), white, false });
        return parts;
    }

    static ImVec2 watermark_measure_parts(const std::vector<watermark_part>& parts) {
        constexpr float gap = 12.f;
        constexpr float compact_gap = 6.f;

        float width = 0.f;
        float height = 0.f;

        for (size_t i = 0; i < parts.size(); ++i) {
            const ImVec2 size = hud_measure(parts[i].text.c_str());
            if (i > 0) {
                width += parts[i].compact_gap ? compact_gap : gap;
            }

            width += size.x;
            height = IM_MAX(height, size.y);
        }

        if (height <= 0.f) {
            height = HUD_FONT_SZ;
        }

        return ImVec2(width, height);
    }

    static void watermark_draw_parts(ImDrawList* dl, float x, float y, const std::vector<watermark_part>& parts) {
        constexpr float gap = 12.f;
        constexpr float compact_gap = 6.f;

        float draw_x = x;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) {
                draw_x += parts[i].compact_gap ? compact_gap : gap;
            }

            hud_text(dl, ImVec2(draw_x, y), parts[i].color, parts[i].text.c_str());
            draw_x += hud_measure(parts[i].text.c_str()).x;
        }
    }

    static void prepare_watermark() {
        watermark_render.visible = false;
        if (!config::get("hack", "watermark", 1)) {
            hud_widget_cancel_drag(watermark_widget);
            return;
        }
        ImFont* font = get_hud_font();
        if (!font) return;

        const std::vector<watermark_part> parts = watermark_build_parts();
        if (parts.empty()) return;

        constexpr float font_size = 12.f;
        constexpr float gap = 8.f;
        constexpr float pad_x = 8.f;
        const char* title = "hello";
        char info[64];
        sprintf_s(info, "%d fps  %s", (int)std::lround(ImGui::GetIO().Framerate), watermark_get_date().c_str());

        const ImVec2 title_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, title);
        const ImVec2 info_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, info);
        const float width = (std::max)(title_size.x + info_size.x + gap + pad_x * 2.f, 120.f);
        const ImVec2 size(width, 24.f);
        const ImVec2 default_pos(Game.screen.x - width - 20.f, 18.f);

        sprintf_s(watermark_render.info, "%s", info);
        watermark_render.width = width;
        watermark_render.visible = true;
        hud_widget_update_drag(watermark_widget, "watermark_x", "watermark_y", default_pos, size);
        hud_widget_update_anim(watermark_widget);
    }

    static void draw_watermark() {
        if (!watermark_render.visible) return;
        ImFont* font = get_hud_font();
        if (!font) return;
        auto* dl = ImGui::GetBackgroundDrawList();
        if (!dl) return;

        constexpr float font_size = 12.f;
        constexpr float pad_x = 8.f;
        const char* title = "hello";
        const float width = watermark_render.width;
        const ImVec2 info_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, watermark_render.info);
        const float alpha = hud_widget_draw_alpha(watermark_widget, 1.f);
        const float x = watermark_widget.pos.x;
        const float y = watermark_widget.pos.y;

        draw_solus_container(dl, x, y + 2.f, width, 19.f, alpha);
        dl->AddText(font, font_size, ImVec2(x + pad_x, y + 9.f), solus_accent(alpha), title);
        dl->AddText(font, font_size, ImVec2(x + width - pad_x - info_size.x, y + 9.f), IM_COL32(255, 255, 255, (int)(220.f * alpha)), watermark_render.info);
    }

    struct bind_def {
        const char* name;
        const char* cfg_cat;
        const char* cfg_enable;
        const char* cfg_key;
        const char* cfg_mode;
        const char* indicator;
        bool enabled_indicator = false;
    };

    static bind_def binds[] = {
        { "vector aim",    "aimbot", "vector_enable",    "aim_key",            "aim_key_mode",            "AIM"     },
    };

    struct solus_keybind_entry {
        const bind_def* bind = nullptr;
        float alpha = 0.f;
        float offset = 0.f;
        const char* mode = "";
        bool active = false;
    };
    struct solus_keybind_state {
        float alpha = 0.f;
        float width = 0.f;
        float height = 24.f;
        int visible_rows = 0;
        bool visible = false;
        solus_keybind_entry entries[sizeof(binds) / sizeof(binds[0])];
    };

    static solus_keybind_state keybind_state;
    static int keybind_cache_frame = -1;

    static const char* get_keybind_mode_text(const bind_state::bind_config& bind) {
        const int mode = bind_state::read_mode(bind);
        if (mode == 1) return "holding";
        if (mode == 2) return "always on";
        return "toggled";
    }
    static ImVec2 get_default_keybind_pos() {
        return ImVec2(Game.screen.x / 1.385f, Game.screen.y / 2.5f);
    }

    static void update_keybind_cache() {
        const int frame = ImGui::GetFrameCount();
        if (keybind_cache_frame == frame) return;
        keybind_cache_frame = frame;

        for (size_t i = 0; i < sizeof(binds) / sizeof(binds[0]); ++i) {
            const bind_def& b = binds[i];
            solus_keybind_entry& entry = keybind_state.entries[i];
            entry.bind = &b;

            const bind_state::bind_config bind = make_bind_config(b.cfg_cat, b.cfg_enable, b.cfg_key, b.cfg_mode);
            if (b.enabled_indicator) {
                entry.active = config::get(b.cfg_cat, b.cfg_enable, 0) != 0;
            }
            else {
                entry.active = bind_state::is_active(bind);
            }
            if (entry.active) {
                entry.mode = b.enabled_indicator ? "enabled" : get_keybind_mode_text(bind);
            }
        }
    }

    static void draw_solus_glow_outline(ImDrawList* dl, float x, float y, float w, float h, float radius, ImU32 color) {
        dl->AddRect(ImVec2(x + 2.f, y + radius + 6.f), ImVec2(x + 3.f, y + h - radius - 6.f), color);
        dl->AddRect(ImVec2(x + w - 3.f, y + radius + 6.f), ImVec2(x + w - 2.f, y + h - radius - 6.f), color);
        dl->AddRect(ImVec2(x + radius + 6.f, y + 2.f), ImVec2(x + w - radius - 6.f, y + 3.f), color);
        dl->AddRect(ImVec2(x + radius + 6.f, y + h - 3.f), ImVec2(x + w - radius - 6.f, y + h - 2.f), color);
        dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), color, radius + 4.f, 0, 1.f);
    }

    static void draw_solus_container(ImDrawList* dl, float x, float y, float w, float h, float alpha) {
        const float visible_alpha = alpha * ui::menu_col[3];
        const ImU32 accent = solus_menu_color(visible_alpha);
        const ImU32 accent_clear = solus_menu_color(0.f);

        const float rounding = 7.f;
        const float line_thickness = 2.f;
        const float left = x + 5.f;
        const float right = x + w - 5.f;

        auto draw_fading_arc = [&](const ImVec2& center, float radius, float start_angle, float end_angle, bool fade_from_start, float base_alpha, float max_thickness) {
            constexpr int segments = 10;
            ImVec2 previous(
                center.x + cosf(start_angle) * radius,
                center.y + sinf(start_angle) * radius
            );

            for (int i = 1; i <= segments; ++i) {
                const float ratio = (float)i / (float)segments;
                const float angle = start_angle + (end_angle - start_angle) * ratio;
                const ImVec2 current(
                    center.x + cosf(angle) * radius,
                    center.y + sinf(angle) * radius
                );
                const float fade = fade_from_start ? ratio : (1.f - ratio);
                if (fade > 0.01f) {
                    dl->AddLine(previous, current, solus_menu_color(base_alpha * fade), max_thickness * fade);
                }
                previous = current;
            }
        };

        dl->AddLine(ImVec2(left + rounding, y), ImVec2(right - rounding, y), accent, line_thickness);
        draw_fading_arc(ImVec2(left + rounding, y + rounding), rounding, IM_PI, IM_PI * 1.5f, true, visible_alpha, line_thickness);
        draw_fading_arc(ImVec2(right - rounding, y + rounding), rounding, -IM_PI * 0.5f, 0.f, false, visible_alpha, line_thickness);

        for (int glow = 4; glow <= 20; ++glow) {
            const float radius = glow * 0.5f;
            const float glow_alpha = (20.f - radius * 2.f) / 255.f * alpha;
            if (glow_alpha <= 0.f) continue;
            const ImU32 glow_col = solus_menu_color(glow_alpha);
            dl->AddLine(ImVec2(left + rounding, y - radius), ImVec2(right - rounding, y - radius), glow_col, 1.f);
            draw_fading_arc(ImVec2(left + rounding, y + rounding), rounding + radius, IM_PI, IM_PI * 1.5f, true, glow_alpha, 1.f);
            draw_fading_arc(ImVec2(right - rounding, y + rounding), rounding + radius, -IM_PI * 0.5f, 0.f, false, glow_alpha, 1.f);
        }
    }

    static void prepare_keybinds() {
        keybind_state.visible = false;
        if (!config::get("hud", "keybinds", 1)) {
            hud_widget_cancel_drag(keybind_widget);
            return;
        }

        ImFont* font = renderer.calibriFont;
        if (!font) return;
        constexpr float font_size = 12.f;

        const float frames = 8.f * ImGui::GetIO().DeltaTime;
        bool latest_item = false;
        float maximum_offset = 66.f;
        int active_count = 0;

        update_keybind_cache();

        for (size_t i = 0; i < sizeof(binds) / sizeof(binds[0]); ++i) {
            const bind_def& b = binds[i];
            solus_keybind_entry& entry = keybind_state.entries[i];

            if (entry.active) {
                latest_item = true;
                entry.alpha += frames;
                if (entry.alpha > 1.f) entry.alpha = 1.f;
            }
            else {
                entry.alpha -= frames;
                if (entry.alpha < 0.f) entry.alpha = 0.f;
            }

            if (entry.alpha > 0.f) {
                const ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, b.name);
                entry.offset = text_size.x;
                if (entry.offset > maximum_offset) maximum_offset = entry.offset;
                ++active_count;
            }
        }

        const float target_width = 75.f + maximum_offset;
        if (keybind_state.width <= 0.f) keybind_state.width = target_width;
        keybind_state.width = solus_lerp(keybind_state.width, target_width, 0.115f);
        const float width = roundf(keybind_state.width);

        keybind_state.visible_rows = active_count;
        keybind_state.height = active_count > 0 ? 27.f + 15.f * active_count : 24.f;
        hud_widget_update_drag(keybind_widget, "keybinds_x", "keybinds_y", get_default_keybind_pos(), ImVec2(width, keybind_state.height));
        hud_widget_update_anim(keybind_widget);

        if ((active_count > 0 && latest_item) || Game.menuOpen) {
            keybind_state.alpha += frames;
            if (keybind_state.alpha > 1.f) keybind_state.alpha = 1.f;
        }
        else {
            keybind_state.alpha -= frames;
            if (keybind_state.alpha < 0.f) keybind_state.alpha = 0.f;
        }

        keybind_state.visible = keybind_state.alpha > 0.f;
    }

    static void draw_keybinds() {
        if (!keybind_state.visible) return;

        ImFont* font = renderer.calibriFont;
        if (!font) return;
        constexpr float font_size = 12.f;

        auto* dl = ImGui::GetBackgroundDrawList();
        if (!dl) return;

        const float alpha = hud_widget_draw_alpha(keybind_widget, keybind_state.alpha);
        const float width = roundf(keybind_state.width);
        const float x = keybind_widget.pos.x;
        const float y = keybind_widget.pos.y;
        draw_solus_container(dl, x, y + 2.f, width, 19.f, alpha);

        const char* title = "keybinds";
        const ImVec2 title_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, title);
        dl->AddText(font, font_size, ImVec2(x - title_size.x * 0.5f + width * 0.5f, y + 9.f), IM_COL32(255, 255, 255, (int)(alpha * 255.f)), title);

        float height_offset = 27.f;
        auto draw_row = [&](const char* name, const char* mode, float row_alpha) {
            const char* key_type = mode ? mode : "?";
            const ImVec2 key_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, key_type);
            const int row_text_alpha = (int)(alpha * row_alpha * 255.f);
            const int state_alpha = (int)(row_text_alpha * 0.5f);
            const ImVec2 name_pos(x + 5.f, y + height_offset);
            const ImVec2 state_pos(x + width - key_size.x - 5.f, y + height_offset);
            const ImU32 outline_col = IM_COL32(0, 0, 0, (int)(row_text_alpha * 0.65f));
            const ImU32 state_outline_col = IM_COL32(0, 0, 0, (int)(state_alpha * 0.65f));

            dl->AddText(font, font_size, ImVec2(name_pos.x + 1.f, name_pos.y + 1.f), outline_col, name);
            dl->AddText(font, font_size, name_pos, IM_COL32(255, 255, 255, row_text_alpha), name);
            dl->AddText(font, font_size, ImVec2(state_pos.x + 1.f, state_pos.y + 1.f), state_outline_col, key_type);
            dl->AddText(font, font_size, state_pos, IM_COL32(255, 255, 255, state_alpha), key_type);

            height_offset += roundf(15.f * row_alpha);
        };

        for (size_t i = 0; i < sizeof(binds) / sizeof(binds[0]); ++i) {
            const solus_keybind_entry& entry = keybind_state.entries[i];
            if (entry.alpha <= 0.f || !entry.bind) continue;
            draw_row(entry.bind->name, entry.mode, entry.alpha);
        }
    }

    static void draw_skeet_indicator(ImDrawList* dl, ImFont* font, const char* text, ImU32 color, float& adjust_position) {
        constexpr float font_size = 22.f;
        const float x = Game.screen.x / 100.f;
        const float y = Game.screen.y / 1.4f;
        const ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, text);
        const float width = text_size.x + 20.f;

        adjust_position += 40.f;
        const float rect_y = y - adjust_position + 2.f;
        const float text_y = y - adjust_position;
        const ImU32 fade_clear = IM_COL32(0, 0, 0, 0);
        const ImU32 fade_dark = IM_COL32(0, 0, 0, 75);

        dl->AddRectFilledMultiColor(ImVec2(13.f, rect_y), ImVec2(13.f + width * 0.5f, rect_y + 31.f), fade_clear, fade_dark, fade_dark, fade_clear);
        dl->AddRectFilledMultiColor(ImVec2(13.f + width * 0.5f, rect_y), ImVec2(13.f + width, rect_y + 31.f), fade_dark, fade_clear, fade_clear, fade_dark);
        dl->AddText(font, font_size, ImVec2(x - 1.f, text_y + 9.f), IM_COL32(33, 33, 33, 180), text);
        dl->AddText(font, font_size, ImVec2(x - 1.f, text_y + 8.f), color, text);
    }

    static int get_admins_around_count();

    static void draw_indicators() {
        if (!config::get("hud", "indicators", 1)) return;

        ImFont* font = renderer.calibriIndicatorFont;
        if (!font) return;

        auto* dl = ImGui::GetBackgroundDrawList();
        if (!dl) return;

        update_keybind_cache();

        float adjust_position = 0.f;
        const ImU32 white = IM_COL32(200, 200, 200, 255);
        const ImU32 alert_color = IM_COL32(0xE7, 0x4F, 0x4F, 255);

        for (size_t i = 0; i < sizeof(binds) / sizeof(binds[0]); ++i) {
            const solus_keybind_entry& entry = keybind_state.entries[i];
            if (!entry.active || !entry.bind) continue;

            const ImU32 indicator_color = !strcmp(entry.bind->indicator, "GM") ? alert_color : white;
            draw_skeet_indicator(dl, font, entry.bind->indicator, indicator_color, adjust_position);
        }

        if (config::get("visual", "admins_around_indicator", 1)) {
            const int admin_count = get_admins_around_count();
            if (admin_count > 0) {
                char admins_buf[32];
                sprintf_s(admins_buf, "ADMINS: %d", admin_count);
                draw_skeet_indicator(dl, font, admins_buf, alert_color, adjust_position);
            }
        }
    }


    static int get_admins_around_count() {
        static int cached_count = 0;

        if (game::ped_list_mutex.try_lock()) {
            int admin_count = 0;

            for (const auto& [ped, data] : game::ped_list) {
                if (!IsValidPtr(ped) || ped == local.player) continue;

                PedCache ped_cache;
                if (!read_ped_cache(ped, &ped_cache) || ped_cache.hp <= 0) continue;

                if (data.has_ws_data && (data.altv_is_admin || data.altv_admin_level > 0))
                    ++admin_count;
            }

            cached_count = admin_count;
            game::ped_list_mutex.unlock();
        }

        return cached_count;
    }

    struct weapons_detected_row {
        char name[80] = {};
        ImU32 color;
        float distance;
        bool highlighted;
    };

    struct weapons_cache_state {
        std::vector<weapons_detected_row> rows;
        int last_update_frame = -1;
        int previous_highlighted_total = 0;
    };

    static weapons_cache_state weapons_cache;

    struct weapons_render_state {
        bool visible = false;
        bool has_rows = false;
        int visible_rows = 0;
        float alpha = 0.f;
        float width = 0.f;
        float height = 0.f;
    };

    static weapons_render_state weapons_render;

    static bool weapons_valid_hash(DWORD weapon_hash) {
        return weapon_hash != 0 && weapon_hash != 0xA2719263u;
    }

    static const char* weapons_label(DWORD weapon_hash) {
        static thread_local std::string highlight_label;
        highlight_label = weapons_highlight::label(weapon_hash);
        if (!highlight_label.empty()) return highlight_label.c_str();

        const char* name = esp::get_readable_weapon_name(weapon_hash);
        if (name && name[0]) return name;
        name = esp::get_weapon_icon_title(weapon_hash);
        return name && name[0] ? name : nullptr;
    }

    static void prepare_weapons() {
        weapons_render.visible = false;
        weapons_render.has_rows = false;
        weapons_render.visible_rows = 0;

        const bool weapons_enabled = weapons_highlight::enabled();
        const bool widget_enabled = weapons_enabled && config::get("visual", "weapons_widget", 0) != 0;
        const bool sound_enabled = weapons_enabled && config::get("visual", "weapons_sound", 0) != 0;
        if (!widget_enabled) {
            hud_widget_cancel_drag(weapons_widget);
        }

        if (!widget_enabled && !sound_enabled) {
            weapons_cache.rows.clear();
            weapons_cache.previous_highlighted_total = 0;
            weapons_render.alpha = 0.f;
            return;
        }

        const bool local_ready = IsValidPtr(local.player) && local.player->HP > 0;
        const int frame = ImGui::GetFrameCount();
        const bool ignore_friends = config::get("visual", "weapons_ignore_friends", 1) != 0;

        if (!local_ready) {
            weapons_cache.rows.clear();
            weapons_cache.previous_highlighted_total = 0;
        }
        else if (weapons_cache.last_update_frame != frame) {
            std::unique_lock<std::mutex> lock(game::ped_list_mutex, std::try_to_lock);
            if (lock.owns_lock()) {
                weapons_cache.last_update_frame = frame;

                std::vector<weapons_detected_row> rows;
                rows.reserve(game::ped_list.size());
                int highlighted_total = 0;

                for (auto& [ped, data] : game::ped_list) {
                    if (!IsValidPtr(ped) || ped == local.player) continue;
                    if (ignore_friends && data.is_friend) continue;

                    PedCache ped_cache;
                    if (!read_ped_cache(ped, &ped_cache) || ped_cache.hp <= 0) continue;
                    const float distance = get_distance(local.player->fPosition, ped_cache.pos);
                    if (distance > 1000.f) continue;

                    DWORD weapon_hash = data.altv_weapon_hash;
                    if (weapon_hash == 0) {
                        weapon_hash = esp::weapon_reader::get_weapon_hash(ped);
                    }
                    if (!weapons_valid_hash(weapon_hash)) continue;

                    const char* weapon_name = weapons_label(weapon_hash);
                    if (!weapon_name) continue;

                    const bool highlighted = weapons_highlight::highlighted(weapon_hash);
                    if (!highlighted) continue;
                    ++highlighted_total;

                    weapons_detected_row row;
                    sprintf_s(row.name, "%s", weapon_name);
                    row.color = weapons_highlight::color(weapon_hash, IM_COL32(255, 255, 255, 255));
                    row.distance = distance;
                    row.highlighted = highlighted;
                    rows.push_back(row);
                }

                std::sort(rows.begin(), rows.end(), [](const weapons_detected_row& a, const weapons_detected_row& b) {
                    return a.distance < b.distance;
                });

                if (highlighted_total > 0 && weapons_cache.previous_highlighted_total == 0 && sound_enabled) {
                    PlaySoundA(
                        reinterpret_cast<LPCSTR>(embedded_audio::heavy_warning_wav),
                        nullptr,
                        SND_MEMORY | SND_ASYNC | SND_NODEFAULT
                    );
                }

                weapons_cache.previous_highlighted_total = highlighted_total;
                weapons_cache.rows = std::move(rows);
            }
        }

        const bool has_rows = local_ready && !weapons_cache.rows.empty();
        if (!widget_enabled) {
            weapons_render.alpha = 0.f;
            return;
        }

        const float frames = 8.f * ImGui::GetIO().DeltaTime;
        if (has_rows || Game.menuOpen || weapons_widget.dragging) {
            weapons_render.alpha += frames;
            if (weapons_render.alpha > 1.f) weapons_render.alpha = 1.f;
        }
        else {
            weapons_render.alpha -= frames;
            if (weapons_render.alpha < 0.f) weapons_render.alpha = 0.f;
        }

        ImFont* row_font = renderer.calibriFont;
        if (!row_font) row_font = renderer.espFont;
        if (!row_font) return;

        constexpr float font_size = 12.f;
        float maximum_offset = 66.f;
        const int visible_rows = has_rows ? (int)weapons_cache.rows.size() : 0;
        for (int i = 0; i < visible_rows; ++i) {
            const ImVec2 left_size = row_font->CalcTextSizeA(font_size, FLT_MAX, 0.f, weapons_cache.rows[i].name);
            if (left_size.x > maximum_offset) maximum_offset = left_size.x;
        }

        const float target_width = 75.f + maximum_offset;
        if (weapons_render.width <= 0.f) weapons_render.width = target_width;
        weapons_render.width = solus_lerp(weapons_render.width, target_width, 0.115f);

        const float width = roundf(weapons_render.width);
        const float height = visible_rows > 0 ? 27.f + 15.f * visible_rows : 24.f;
        const ImVec2 default_pos(Game.screen.x * 0.5f - width * 0.5f, 16.f);

        weapons_render.has_rows = has_rows;
        weapons_render.visible_rows = visible_rows;
        weapons_render.width = width;
        weapons_render.height = height;

        hud_widget_update_drag(weapons_widget, "weapons_x", "weapons_y", default_pos, ImVec2(width, height));
        hud_widget_update_anim(weapons_widget);

        weapons_render.visible = weapons_render.alpha > 0.f;
    }

    static void draw_weapons() {
        if (!weapons_render.visible) return;

        auto* dl = ImGui::GetBackgroundDrawList();
        if (!dl) return;

        ImFont* title_font = renderer.calibriFont;
        ImFont* row_font = renderer.calibriFont;
        if (!row_font) row_font = renderer.espFont;
        if (!title_font || !row_font) return;

        const float alpha = hud_widget_draw_alpha(weapons_widget, weapons_render.alpha);
        constexpr float font_size = 12.f;
        const float width = weapons_render.width;
        const float x = weapons_widget.pos.x;
        const float y = weapons_widget.pos.y;

        draw_solus_container(dl, x, y + 2.f, width, 19.f, alpha);

        const char* title = "weapons";
        const ImVec2 title_size = title_font->CalcTextSizeA(font_size, FLT_MAX, 0.f, title);
        dl->AddText(title_font, font_size, ImVec2(x - title_size.x * 0.5f + width * 0.5f, y + 9.f), IM_COL32(255, 255, 255, (int)(alpha * 255.f)), title);

        if (!weapons_render.has_rows) {
            return;
        }

        float height_offset = 27.f;
        for (int i = 0; i < weapons_render.visible_rows; ++i) {
            const weapons_detected_row& row = weapons_cache.rows[i];
            char dist_buf[24];
            sprintf_s(dist_buf, "%dm", (int)(row.distance + 0.5f));
            const ImVec2 dist_size = row_font->CalcTextSizeA(font_size, FLT_MAX, 0.f, dist_buf);
            const ImU32 name_color = row.highlighted ? row.color : IM_COL32(255, 255, 255, 255);
            const int row_text_alpha = (int)(alpha * 255.f);
            const int distance_alpha = (int)(row_text_alpha * 0.5f);
            const ImU32 name_outline_col = IM_COL32(0, 0, 0, (int)(row_text_alpha * 0.65f));
            const ImU32 distance_outline_col = IM_COL32(0, 0, 0, (int)(distance_alpha * 0.65f));
            const ImVec2 name_pos(x + 5.f, y + height_offset);
            const ImVec2 distance_pos(x + width - dist_size.x - 5.f, y + height_offset);

            dl->AddText(row_font, font_size, ImVec2(name_pos.x + 1.f, name_pos.y + 1.f), name_outline_col, row.name);
            dl->AddText(row_font, font_size, name_pos, hud_apply_alpha(name_color, alpha), row.name);
            dl->AddText(row_font, font_size, ImVec2(distance_pos.x + 1.f, distance_pos.y + 1.f), distance_outline_col, dist_buf);
            dl->AddText(row_font, font_size, distance_pos, IM_COL32(255, 255, 255, distance_alpha), dist_buf);

            height_offset += 15.f;
        }
    }

    static void _render_inner() {
        if (Game.screen.x < 100 || Game.screen.y < 100) return;
        if (!ImGui::GetCurrentContext()) return;
        auto* dl = ImGui::GetBackgroundDrawList();
        if (!dl) return;

        hud_reset_drag_feedback();
        prepare_watermark();
        prepare_keybinds();
        prepare_weapons();
        hud_draw_drag_feedback(dl);

        draw_watermark();
        draw_keybinds();
        draw_indicators();
        draw_weapons();
    }

    static void render() {
        if (!Game.renderReady || !Game.running) return;
        __try { _render_inner(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
    }
}
