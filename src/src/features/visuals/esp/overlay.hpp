#pragma once

#include "config/interface.hpp"
#include "render/renderer.h"
#include "features/visuals/weapons_highlight.hpp"
#include <Windows.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>
#include <string>
#include <vector>

extern imgui_render renderer;

namespace esp {
#include "weapon_icon_map.hpp"

	enum class projected_esp_side : int {
		top = 0,
		bottom = 1,
		left = 2,
		right = 3,
		count
	};

	enum class projected_esp_element_id : int {
		none = -1,
		relation = 0,
		name,
		dormant,
		static_id,
		faction,
		admin,
		tester,
		media,
		level,
		afk,
		dead,
		health,
		armor,
		weapon_icon,
		weapon_text,
		distance,
		social,
		visible,
		cuffed,
		wanted,
		armor_tag,
		count
	};

	static constexpr int k_projected_esp_element_count = static_cast<int>(projected_esp_element_id::count);
	static constexpr float k_projected_esp_size_step = 1.f;

	struct projected_esp_layout {
		projected_esp_side side = projected_esp_side::top;
		int order = 0;
	};

	enum class projected_esp_item_type {
		label,
		weapon_icon,
		health,
		armor
	};

	enum class projected_esp_font_style : int {
		mini = 0,
		bold = 1
	};

	enum class projected_esp_text_case : int {
		default_style = 0,
		lowercase = 1,
		uppercase = 2
	};

	struct projected_esp_element_style {
		projected_esp_font_style font = projected_esp_font_style::mini;
		projected_esp_text_case text_case = projected_esp_text_case::default_style;
		float size = 10.f;
		bool show_value = true;
	};

	struct projected_esp_settings {
		float max_range;
		bool draw_skeleton;
		bool dim_dead_players;
		int health_mode;
		int health_position;
		bool draw_box;
		int box_style;
		float box_radius;
		float box_thickness;
		int name_pos;
		int dist_pos;
		int weapon_pos;
		std::array<projected_esp_layout, k_projected_esp_element_count> layouts{};
		std::array<projected_esp_element_style, k_projected_esp_element_count> element_styles{};
		bool layout_override_active = false;
		projected_esp_element_id layout_override_element = projected_esp_element_id::none;
		projected_esp_layout layout_override{};
		bool drag_visual_active = false;
		projected_esp_element_id drag_visual_element = projected_esp_element_id::none;
		ImVec2 drag_visual_pos{};
		bool clamp_top_stack_y = false;
		float top_stack_min_y = 0.f;
		bool draw_interaction_backgrounds = false;
		std::array<float, k_projected_esp_element_count> interaction_alpha{};
		std::array<ImVec2, k_projected_esp_element_count>* animated_positions = nullptr;
		std::array<bool, k_projected_esp_element_count>* animated_position_valid = nullptr;
		float layout_lerp = 1.f;
		bool show_name;
		bool show_static;
		bool show_faction;
		bool show_relation;
		bool show_admin;
		bool show_tester;
		bool show_media;
		bool show_afk;
		bool show_dead;
		bool force_dead_label = false;
		bool show_level;
		bool show_distance;
		bool show_weapon;
		bool show_weapon_text;
		bool show_weapon_icon;
		bool show_cuffed;
		bool show_wanted;
		bool show_armor_tag;
		bool show_social;
		bool show_visible;
		bool cheap_render_text;
		float skeleton_thickness;
		bool health_static_color;
		ImU32 health_static_color_u32;
		ImU32 armor_color;
		int fraction_color_mode;
		ImU32 fraction_static_color;
		std::array<ImU32, 13> fraction_custom_colors{};
		std::array<ImU32, k_projected_esp_element_count> element_colors{};
		std::array<bool, k_projected_esp_element_count> element_color_overrides{};
		ImU32 nickname_color;
		ImU32 static_color;
		ImU32 faction_color;
		ImU32 admin_color;
		ImU32 media_color;
		ImU32 afk_color;
		ImU32 dead_color;
		ImU32 level_color;
		ImU32 cuffed_color;
		ImU32 wanted_color;
		ImU32 armor_tag_color;
		ImU32 social_color;
		ImU32 visible_flag_color;
		RGBA invisible_box_color = RGBA(255, 0, 0, 255);
		ImU32 enemy_color;
		ImU32 friend_color;
		ImU32 family_relation_color;
		ImU32 fraction_relation_color;
		ImU32 distance_color;
		ImU32 weapon_color;
		RGBA visible_color = RGBA(0, 255, 0, 255);
		RGBA skeleton_color = RGBA(255, 255, 255, 255);
		RGBA skeleton_invisible_color = RGBA(255, 255, 255, 255);
		bool skeleton_invisible_enabled = true;
		RGBA box_color = RGBA(255, 255, 255, 255);
		RGBA box_fill_color = RGBA(51, 51, 51, 51);
	};

	struct projected_esp_player {
		const char* name = nullptr;
		const char* faction = nullptr;
		const char* weapon_label = nullptr;
		const char* social = nullptr;
		int static_id = 0;
		int dynamic_id = 0;
		int fraction_id = 0;
		int leader_id = 0;
		int level = 0;
		int admin_level = 0;
		int wanted_level = 0;
		float health = 100.f;
		float armor = 0.f;
		DWORD weapon_hash = 0;
		bool is_admin = false;
		bool is_dead = false;
		bool is_media = false;
		bool is_tester = false;
		bool is_afk = false;
		bool is_cuffed = false;
		bool has_wear_armor = false;
		bool visible = true;
		bool is_friend = false;
		bool is_enemy = false;
		bool is_family = false;
		bool is_fraction = false;
		bool dormant = false;
		bool weapon_highlight_active = false;
		bool weapon_force_label = false;
		ImU32 weapon_highlight_color = 0;
	};

	struct projected_esp_element_rect {
		bool visible = false;
		ImRect rect{};
		projected_esp_side side = projected_esp_side::top;
		int order = 0;
	};

	struct projected_esp_overlay_rects {
		bool name_visible = false;
		bool distance_visible = false;
		bool weapon_visible = false;
		bool health_visible = false;
		ImRect name;
		ImRect distance;
		ImRect weapon;
		ImRect health;
		std::array<projected_esp_element_rect, k_projected_esp_element_count> elements{};
	};

	inline int projected_esp_element_index(projected_esp_element_id id) {
		const int index = static_cast<int>(id);
		return index >= 0 && index < k_projected_esp_element_count ? index : -1;
	}

	inline bool is_projected_esp_element(projected_esp_element_id id) {
		return projected_esp_element_index(id) >= 0;
	}

	inline bool is_projected_esp_flag_element(projected_esp_element_id id) {
		switch (id) {
		case projected_esp_element_id::relation:
		case projected_esp_element_id::static_id:
		case projected_esp_element_id::faction:
		case projected_esp_element_id::admin:
		case projected_esp_element_id::tester:
		case projected_esp_element_id::media:
		case projected_esp_element_id::level:
		case projected_esp_element_id::afk:
		case projected_esp_element_id::dead:
		case projected_esp_element_id::cuffed:
		case projected_esp_element_id::wanted:
		case projected_esp_element_id::armor_tag:
		case projected_esp_element_id::social:
		case projected_esp_element_id::visible:
			return true;
		default:
			return false;
		}
	}

	inline bool is_projected_esp_identity_element(projected_esp_element_id id) {
		return id == projected_esp_element_id::relation ||
			id == projected_esp_element_id::static_id ||
			id == projected_esp_element_id::name;
	}

	inline bool is_projected_esp_text_element(projected_esp_element_id id) {
		switch (id) {
		case projected_esp_element_id::relation:
		case projected_esp_element_id::name:
		case projected_esp_element_id::static_id:
		case projected_esp_element_id::faction:
		case projected_esp_element_id::admin:
		case projected_esp_element_id::tester:
		case projected_esp_element_id::media:
		case projected_esp_element_id::level:
		case projected_esp_element_id::afk:
		case projected_esp_element_id::dead:
		case projected_esp_element_id::cuffed:
		case projected_esp_element_id::wanted:
		case projected_esp_element_id::armor_tag:
		case projected_esp_element_id::social:
		case projected_esp_element_id::visible:
		case projected_esp_element_id::weapon_text:
		case projected_esp_element_id::distance:
			return true;
		default:
			return false;
		}
	}

	inline bool is_projected_esp_scalable_element(projected_esp_element_id id) {
		return is_projected_esp_text_element(id) ||
			id == projected_esp_element_id::weapon_icon ||
			id == projected_esp_element_id::health ||
			id == projected_esp_element_id::armor;
	}

	inline const char* projected_esp_element_key(projected_esp_element_id id) {
		switch (id) {
		case projected_esp_element_id::relation: return "relation";
		case projected_esp_element_id::name: return "name";
		case projected_esp_element_id::dormant: return "dormant";
		case projected_esp_element_id::static_id: return "static";
		case projected_esp_element_id::faction: return "faction";
		case projected_esp_element_id::admin: return "admin";
		case projected_esp_element_id::tester: return "tester";
		case projected_esp_element_id::media: return "media";
		case projected_esp_element_id::level: return "level";
		case projected_esp_element_id::afk: return "afk";
		case projected_esp_element_id::dead: return "dead";
		case projected_esp_element_id::health: return "health";
		case projected_esp_element_id::armor: return "armor";
		case projected_esp_element_id::weapon_icon: return "weap_icon";
		case projected_esp_element_id::weapon_text: return "weap_text";
		case projected_esp_element_id::distance: return "dist";
		case projected_esp_element_id::social: return "social";
		case projected_esp_element_id::visible: return "visible";
		case projected_esp_element_id::cuffed: return "cuffed";
		case projected_esp_element_id::wanted: return "wanted";
		case projected_esp_element_id::armor_tag: return "armor_tag";
		default: return "";
		}
	}

	inline const char* projected_esp_element_label(projected_esp_element_id id) {
		switch (id) {
		case projected_esp_element_id::relation: return "relation tags";
		case projected_esp_element_id::name: return "name";
		case projected_esp_element_id::dormant: return "dormant";
		case projected_esp_element_id::static_id: return "static";
		case projected_esp_element_id::faction: return "fraction";
		case projected_esp_element_id::admin: return "admin";
		case projected_esp_element_id::tester: return "tester";
		case projected_esp_element_id::media: return "media";
		case projected_esp_element_id::level: return "level";
		case projected_esp_element_id::afk: return "afk";
		case projected_esp_element_id::dead: return "dead";
		case projected_esp_element_id::health: return "health";
		case projected_esp_element_id::armor: return "armor";
		case projected_esp_element_id::weapon_icon: return "weapon icon";
		case projected_esp_element_id::weapon_text: return "weapon text";
		case projected_esp_element_id::distance: return "distance";
		case projected_esp_element_id::social: return "social club";
		case projected_esp_element_id::visible: return "visible";
		case projected_esp_element_id::cuffed: return "cuffed";
		case projected_esp_element_id::wanted: return "wanted";
		case projected_esp_element_id::armor_tag: return "armor worn";
		default: return "";
		}
	}

	inline projected_esp_side clamp_projected_esp_side(int value) {
		if (value < 0) return projected_esp_side::top;
		if (value > static_cast<int>(projected_esp_side::right)) return projected_esp_side::right;
		return static_cast<projected_esp_side>(value);
	}

	inline projected_esp_font_style clamp_projected_esp_font_style(int value) {
		return value == static_cast<int>(projected_esp_font_style::bold) ? projected_esp_font_style::bold : projected_esp_font_style::mini;
	}

	inline projected_esp_text_case clamp_projected_esp_text_case(int value) {
		if (value < 0) return projected_esp_text_case::default_style;
		if (value > static_cast<int>(projected_esp_text_case::uppercase)) return projected_esp_text_case::uppercase;
		return static_cast<projected_esp_text_case>(value);
	}

	inline float projected_esp_size_min(projected_esp_element_id id) {
		if (id == projected_esp_element_id::health || id == projected_esp_element_id::armor) return 1.f;
		if (id == projected_esp_element_id::weapon_icon) return 8.f;
		return 7.f;
	}

	inline float projected_esp_size_max(projected_esp_element_id id) {
		if (id == projected_esp_element_id::health || id == projected_esp_element_id::armor) return 6.f;
		if (id == projected_esp_element_id::weapon_icon) return 42.f;
		return 24.f;
	}

	inline float default_projected_esp_size(projected_esp_element_id id) {
		switch (id) {
		case projected_esp_element_id::relation:
			return 12.f;
		case projected_esp_element_id::name:
			return 12.f;
		case projected_esp_element_id::static_id:
		case projected_esp_element_id::admin:
		case projected_esp_element_id::tester:
			return 11.f;
		case projected_esp_element_id::dead:
			return 10.f;
		case projected_esp_element_id::weapon_icon:
			return 34.f;
		case projected_esp_element_id::weapon_text:
			return 11.f;
		case projected_esp_element_id::health:
		case projected_esp_element_id::armor:
			return 2.f;
		default:
			return is_projected_esp_text_element(id) ? 9.f : 10.f;
		}
	}

	inline float clamp_projected_esp_size(projected_esp_element_id id, float value) {
		if (!std::isfinite(value)) return default_projected_esp_size(id);
		const float snapped = std::round(value / k_projected_esp_size_step) * k_projected_esp_size_step;
		return std::clamp(snapped, projected_esp_size_min(id), projected_esp_size_max(id));
	}

	inline std::string projected_esp_pos_key(projected_esp_element_id id) {
		return std::string("esp_pos_") + projected_esp_element_key(id);
	}

	inline std::string projected_esp_order_key(projected_esp_element_id id) {
		return std::string("esp_order_") + projected_esp_element_key(id);
	}

	inline std::string projected_esp_font_key(projected_esp_element_id id) {
		return std::string("esp_font_") + projected_esp_element_key(id);
	}

	inline std::string projected_esp_case_key(projected_esp_element_id id) {
		return std::string("esp_case_") + projected_esp_element_key(id);
	}

	inline std::string projected_esp_size_key(projected_esp_element_id id) {
		return std::string("esp_size_") + projected_esp_element_key(id);
	}

	inline std::string projected_esp_show_value_key(projected_esp_element_id id) {
		return std::string("esp_show_value_") + projected_esp_element_key(id);
	}

	inline std::string projected_esp_color_key(projected_esp_element_id id, const char* channel) {
		return std::string("esp_color_") + projected_esp_element_key(id) + "_" + channel;
	}

	inline projected_esp_font_style default_projected_esp_font_style(projected_esp_element_id id) {
		return id == projected_esp_element_id::relation || id == projected_esp_element_id::name || id == projected_esp_element_id::static_id ? projected_esp_font_style::bold : projected_esp_font_style::mini;
	}

	inline projected_esp_text_case default_projected_esp_text_case(projected_esp_element_id id) {
		if (id == projected_esp_element_id::faction) return projected_esp_text_case::uppercase;
		return projected_esp_text_case::default_style;
	}

	inline bool projected_esp_default_uppercase(projected_esp_element_id id) {
		switch (id) {
		case projected_esp_element_id::relation:
		case projected_esp_element_id::admin:
		case projected_esp_element_id::tester:
		case projected_esp_element_id::media:
		case projected_esp_element_id::level:
		case projected_esp_element_id::afk:
		case projected_esp_element_id::dead:
		case projected_esp_element_id::cuffed:
		case projected_esp_element_id::wanted:
		case projected_esp_element_id::armor_tag:
		case projected_esp_element_id::social:
		case projected_esp_element_id::visible:
			return true;
		default:
			return false;
		}
	}

	inline projected_esp_layout read_projected_esp_layout(projected_esp_element_id id, projected_esp_side default_side, int default_order) {
		projected_esp_layout layout{};
		layout.side = clamp_projected_esp_side(config::get("visual", projected_esp_pos_key(id), static_cast<int>(default_side)));
		layout.order = config::get("visual", projected_esp_order_key(id), default_order);
		return layout;
	}

	inline projected_esp_element_style read_projected_esp_element_style(projected_esp_element_id id) {
		projected_esp_element_style style{};
		style.font = clamp_projected_esp_font_style(config::get("visual", projected_esp_font_key(id), static_cast<int>(default_projected_esp_font_style(id))));
		style.text_case = clamp_projected_esp_text_case(config::get("visual", projected_esp_case_key(id), static_cast<int>(default_projected_esp_text_case(id))));
		style.size = clamp_projected_esp_size(id, config::get("visual", projected_esp_size_key(id), default_projected_esp_size(id)));
		style.show_value = config::get("visual", projected_esp_show_value_key(id), 1) != 0;
		return style;
	}

	inline void save_projected_esp_layout(projected_esp_element_id id, projected_esp_side side, int order) {
		if (!is_projected_esp_element(id)) return;
		config::set("visual", projected_esp_pos_key(id), std::to_string(static_cast<int>(side)));
		config::set("visual", projected_esp_order_key(id), std::to_string(order));
	}

	inline projected_esp_layout get_projected_esp_layout(const projected_esp_settings& settings, projected_esp_element_id id) {
		const int index = projected_esp_element_index(id);
		if (index < 0) return {};
		projected_esp_layout layout = settings.layouts[index];
		if (settings.layout_override_active && settings.layout_override_element == id) {
			layout = settings.layout_override;
		}
		return layout;
	}

	inline projected_esp_element_id hit_test_projected_esp_overlay(const projected_esp_overlay_rects& rects, const ImVec2& point, const ImVec2& padding) {
		for (int i = k_projected_esp_element_count - 1; i >= 0; --i) {
			const auto& element = rects.elements[i];
			if (!element.visible) continue;
			const ImRect hit(element.rect.Min - padding, element.rect.Max + padding);
			if (hit.Contains(point)) return static_cast<projected_esp_element_id>(i);
		}
		return projected_esp_element_id::none;
	}

	inline float esp_clamp_ratio(float value) {
		if (value < 0.f) return 0.f;
		if (value > 1.f) return 1.f;
		return value;
	}

	inline ImU32 esp_apply_opacity(ImU32 color, float opacity) {
		const int alpha = (int)(((color >> IM_COL32_A_SHIFT) & 0xFF) * opacity);
		return (color & ~IM_COL32_A_MASK) | ((ImU32)alpha << IM_COL32_A_SHIFT);
	}

	inline RGBA esp_apply_opacity(const RGBA& color, float opacity) {
		return RGBA(color.r, color.g, color.b, (int)(color.a * opacity));
	}

	inline ImU32 rgba_to_u32(RGBA color) {
		return IM_COL32(color.r, color.g, color.b, color.a);
	}

	inline ImU32 read_visual_color_u32(const char* r_key, const char* g_key, const char* b_key, const char* a_key, float default_r, float default_g, float default_b, float default_a) {
		const float r = std::clamp(config::get("visual", r_key, default_r), 0.f, 1.f);
		const float g = std::clamp(config::get("visual", g_key, default_g), 0.f, 1.f);
		const float b = std::clamp(config::get("visual", b_key, default_b), 0.f, 1.f);
		const float a = std::clamp(config::get("visual", a_key, default_a), 0.f, 1.f);
		return IM_COL32((int)(r * 255.f), (int)(g * 255.f), (int)(b * 255.f), (int)(a * 255.f));
	}

	inline bool read_projected_esp_element_color_override(projected_esp_element_id id, ImU32* out) {
		if (!out || id == projected_esp_element_id::faction || id == projected_esp_element_id::relation) return false;
		const std::string r_key = projected_esp_color_key(id, "r");
		const std::string g_key = projected_esp_color_key(id, "g");
		const std::string b_key = projected_esp_color_key(id, "b");
		const std::string a_key = projected_esp_color_key(id, "a");
		const float r = config::get("visual", r_key, -1.f);
		const float g = config::get("visual", g_key, -1.f);
		const float b = config::get("visual", b_key, -1.f);
		const float a = config::get("visual", a_key, -1.f);
		if (r < 0.f || g < 0.f || b < 0.f || a < 0.f) return false;
		*out = IM_COL32(
			(int)(std::clamp(r, 0.f, 1.f) * 255.f),
			(int)(std::clamp(g, 0.f, 1.f) * 255.f),
			(int)(std::clamp(b, 0.f, 1.f) * 255.f),
			(int)(std::clamp(a, 0.f, 1.f) * 255.f)
		);
		return true;
	}

	inline float im_col32_channel(ImU32 color, int shift) {
		return (float)((color >> shift) & 0xFF) / 255.f;
	}

	inline ImU32 read_projected_esp_element_color(projected_esp_element_id id, ImU32 fallback) {
		if (id == projected_esp_element_id::faction || id == projected_esp_element_id::relation) return fallback;
		const std::string r_key = projected_esp_color_key(id, "r");
		const std::string g_key = projected_esp_color_key(id, "g");
		const std::string b_key = projected_esp_color_key(id, "b");
		const std::string a_key = projected_esp_color_key(id, "a");
		return read_visual_color_u32(
			r_key.c_str(),
			g_key.c_str(),
			b_key.c_str(),
			a_key.c_str(),
			im_col32_channel(fallback, IM_COL32_R_SHIFT),
			im_col32_channel(fallback, IM_COL32_G_SHIFT),
			im_col32_channel(fallback, IM_COL32_B_SHIFT),
			im_col32_channel(fallback, IM_COL32_A_SHIFT)
		);
	}

	inline ImU32 resolve_projected_esp_element_color(const projected_esp_settings& settings, projected_esp_element_id id, ImU32 fallback) {
		const int index = projected_esp_element_index(id);
		if (index >= 0 && settings.element_color_overrides[index]) return settings.element_colors[index];
		return fallback;
	}

	inline ImU32 get_default_friend_color_u32() {
		return IM_COL32(110, 255, 110, 255);
	}

	inline ImU32 get_default_enemy_color_u32() {
		return IM_COL32(255, 90, 90, 255);
	}

	inline ImU32 get_default_family_relation_color_u32() {
		return IM_COL32(255, 150, 35, 255);
	}

	inline ImU32 get_default_fraction_relation_color_u32() {
		return IM_COL32(170, 90, 255, 255);
	}

	inline bool use_custom_relation_tag_colors() {
		return std::clamp(config::get("visual", "relation_color_mode", 0), 0, 1) == 1;
	}

	inline ImU32 get_friend_color_u32() {
		if (!use_custom_relation_tag_colors()) return get_default_friend_color_u32();
		return read_visual_color_u32("friend_color_r", "friend_color_g", "friend_color_b", "friend_color_a", 110.f / 255.f, 1.f, 110.f / 255.f, 1.f);
	}

	inline ImU32 get_enemy_color_u32() {
		if (!use_custom_relation_tag_colors()) return get_default_enemy_color_u32();
		return read_visual_color_u32("enemy_color_r", "enemy_color_g", "enemy_color_b", "enemy_color_a", 1.f, 90.f / 255.f, 90.f / 255.f, 1.f);
	}

	inline ImU32 get_family_relation_color_u32() {
		if (!use_custom_relation_tag_colors()) return get_default_family_relation_color_u32();
		return read_visual_color_u32("family_relation_color_r", "family_relation_color_g", "family_relation_color_b", "family_relation_color_a", 1.f, 150.f / 255.f, 35.f / 255.f, 1.f);
	}

	inline ImU32 get_fraction_relation_color_u32() {
		if (!use_custom_relation_tag_colors()) return get_default_fraction_relation_color_u32();
		return read_visual_color_u32("fraction_relation_color_r", "fraction_relation_color_g", "fraction_relation_color_b", "fraction_relation_color_a", 170.f / 255.f, 90.f / 255.f, 1.f, 1.f);
	}

	inline ImU32 get_relation_color_u32(bool enemy_marked, bool friend_marked) {
		if (enemy_marked) return get_enemy_color_u32();
		if (friend_marked) return get_friend_color_u32();
		return IM_COL32(255, 255, 255, 255);
	}

	inline ImU32 get_altv_nickname_color_u32(bool enemy_marked, bool friend_marked) {
		if (enemy_marked || friend_marked) return get_relation_color_u32(enemy_marked, friend_marked);
		ImU32 projected_color = 0;
		if (read_projected_esp_element_color_override(projected_esp_element_id::name, &projected_color)) return projected_color;
		return read_visual_color_u32("altv_nickname_color_r", "altv_nickname_color_g", "altv_nickname_color_b", "altv_nickname_color_a", 1.f, 1.f, 1.f, 1.f);
	}

	inline ImU32 get_altv_static_color_u32() {
		return read_visual_color_u32("altv_static_color_r", "altv_static_color_g", "altv_static_color_b", "altv_static_color_a", 206.f / 255.f, 204.f / 255.f, 204.f / 255.f, 1.f);
	}

	inline ImU32 get_altv_faction_color_u32() {
		return read_visual_color_u32("altv_faction_color_r", "altv_faction_color_g", "altv_faction_color_b", "altv_faction_color_a", 180.f / 255.f, 1.f, 240.f / 255.f, 1.f);
	}

	inline ImU32 get_default_fraction_color_u32(int fraction_id, ImU32 fallback) {
		switch (fraction_id) {
		case 1: return IM_COL32(56, 128, 255, 255);
		case 2: return IM_COL32(255, 92, 92, 255);
		case 3: return IM_COL32(194, 124, 54, 255);
		case 4: return IM_COL32(98, 150, 78, 255);
		case 5: return IM_COL32(64, 112, 255, 255);
		case 6: return IM_COL32(255, 198, 66, 255);
		case 7: return IM_COL32(40, 78, 120, 255);
		case 8: return IM_COL32(152, 84, 255, 255);
		case 9: return IM_COL32(255, 214, 73, 255);
		case 10: return IM_COL32(78, 210, 104, 255);
		case 11: return IM_COL32(216, 45, 45, 255);
		case 12: return IM_COL32(65, 145, 255, 255);
		default: return fallback;
		}
	}

	inline ImU32 get_altv_fraction_color_u32(int fraction_id, ImU32 fallback) {
		const int color_mode = std::clamp(config::get("visual", "fraction_color_mode", 0), 0, 2);
		if (color_mode == 1) return get_altv_static_color_u32();
		if (color_mode == 0) return get_default_fraction_color_u32(fraction_id, fallback);
		switch (fraction_id) {
		case 1: return read_visual_color_u32("fraction_1_color_r", "fraction_1_color_g", "fraction_1_color_b", "fraction_1_color_a", 56.f / 255.f, 128.f / 255.f, 1.f, 1.f);
		case 2: return read_visual_color_u32("fraction_2_color_r", "fraction_2_color_g", "fraction_2_color_b", "fraction_2_color_a", 1.f, 92.f / 255.f, 92.f / 255.f, 1.f);
		case 3: return read_visual_color_u32("fraction_3_color_r", "fraction_3_color_g", "fraction_3_color_b", "fraction_3_color_a", 194.f / 255.f, 124.f / 255.f, 54.f / 255.f, 1.f);
		case 4: return read_visual_color_u32("fraction_4_color_r", "fraction_4_color_g", "fraction_4_color_b", "fraction_4_color_a", 98.f / 255.f, 150.f / 255.f, 78.f / 255.f, 1.f);
		case 5: return read_visual_color_u32("fraction_5_color_r", "fraction_5_color_g", "fraction_5_color_b", "fraction_5_color_a", 64.f / 255.f, 112.f / 255.f, 1.f, 1.f);
		case 6: return read_visual_color_u32("fraction_6_color_r", "fraction_6_color_g", "fraction_6_color_b", "fraction_6_color_a", 1.f, 198.f / 255.f, 66.f / 255.f, 1.f);
		case 7: return read_visual_color_u32("fraction_7_color_r", "fraction_7_color_g", "fraction_7_color_b", "fraction_7_color_a", 40.f / 255.f, 78.f / 255.f, 120.f / 255.f, 1.f);
		case 8: return read_visual_color_u32("fraction_8_color_r", "fraction_8_color_g", "fraction_8_color_b", "fraction_8_color_a", 152.f / 255.f, 84.f / 255.f, 1.f, 1.f);
		case 9: return read_visual_color_u32("fraction_9_color_r", "fraction_9_color_g", "fraction_9_color_b", "fraction_9_color_a", 1.f, 214.f / 255.f, 73.f / 255.f, 1.f);
		case 10: return read_visual_color_u32("fraction_10_color_r", "fraction_10_color_g", "fraction_10_color_b", "fraction_10_color_a", 78.f / 255.f, 210.f / 255.f, 104.f / 255.f, 1.f);
		case 11: return read_visual_color_u32("fraction_11_color_r", "fraction_11_color_g", "fraction_11_color_b", "fraction_11_color_a", 216.f / 255.f, 45.f / 255.f, 45.f / 255.f, 1.f);
		case 12: return read_visual_color_u32("fraction_12_color_r", "fraction_12_color_g", "fraction_12_color_b", "fraction_12_color_a", 65.f / 255.f, 145.f / 255.f, 1.f, 1.f);
		default: return fallback;
		}
	}

	inline ImU32 get_altv_fraction_color_u32(const projected_esp_settings& settings, int fraction_id, ImU32 fallback) {
		if (settings.fraction_color_mode == 1) return settings.fraction_static_color;
		if (settings.fraction_color_mode == 0) return get_default_fraction_color_u32(fraction_id, fallback);
		if (fraction_id >= 1 && fraction_id < static_cast<int>(settings.fraction_custom_colors.size())) {
			return settings.fraction_custom_colors[fraction_id];
		}
		return fallback;
	}

	inline ImU32 get_altv_admin_color_u32() {
		return read_visual_color_u32("altv_admin_color_r", "altv_admin_color_g", "altv_admin_color_b", "altv_admin_color_a", 1.f, 70.f / 255.f, 70.f / 255.f, 1.f);
	}

	inline ImU32 get_altv_tester_color_u32() {
		return IM_COL32(210, 170, 255, 255);
	}

	inline ImU32 get_altv_level_color_u32() {
		return read_visual_color_u32("altv_level_color_r", "altv_level_color_g", "altv_level_color_b", "altv_level_color_a", 180.f / 255.f, 202.f / 255.f, 1.f, 1.f);
	}

	inline ImU32 get_altv_dead_color_u32() {
		return read_visual_color_u32("altv_dead_color_r", "altv_dead_color_g", "altv_dead_color_b", "altv_dead_color_a", 1.f, 70.f / 255.f, 70.f / 255.f, 1.f);
	}

	inline ImU32 get_altv_afk_color_u32() {
		return read_visual_color_u32("altv_afk_color_r", "altv_afk_color_g", "altv_afk_color_b", "altv_afk_color_a", 253.f / 255.f, 184.f / 255.f, 69.f / 255.f, 1.f);
	}

	inline ImU32 get_altv_media_color_u32() {
		return read_visual_color_u32("altv_media_color_r", "altv_media_color_g", "altv_media_color_b", "altv_media_color_a", 183.f / 255.f, 1.f, 127.f / 255.f, 1.f);
	}

	inline ImU32 get_altv_cuffed_color_u32() {
		return read_visual_color_u32("altv_cuffed_color_r", "altv_cuffed_color_g", "altv_cuffed_color_b", "altv_cuffed_color_a", 1.f, 1.f, 1.f, 1.f);
	}

	inline ImU32 get_altv_wanted_color_u32() {
		return read_visual_color_u32("altv_wanted_color_r", "altv_wanted_color_g", "altv_wanted_color_b", "altv_wanted_color_a", 1.f, 122.f / 255.f, 122.f / 255.f, 1.f);
	}

	inline ImU32 get_altv_armor_color_u32() {
		return read_visual_color_u32("altv_armor_color_r", "altv_armor_color_g", "altv_armor_color_b", "altv_armor_color_a", 122.f / 255.f, 180.f / 255.f, 1.f, 1.f);
	}

	inline ImU32 get_altv_social_color_u32() {
		return read_visual_color_u32("altv_social_color_r", "altv_social_color_g", "altv_social_color_b", "altv_social_color_a", 1.f, 1.f, 1.f, 1.f);
	}

	inline ImU32 get_altv_visible_color_u32() {
		return read_visual_color_u32("altv_visible_color_r", "altv_visible_color_g", "altv_visible_color_b", "altv_visible_color_a", 0.f, 1.f, 0.f, 1.f);
	}


	inline std::string uppercase_flag_text(const char* text) {
		std::string value = text ? text : "";
		for (char& ch : value) {
			ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
		}
		return value;
	}

	inline std::string apply_projected_esp_text_case(projected_esp_element_id id, const char* text, projected_esp_text_case text_case) {
		std::string value = text ? text : "";
		const bool uppercase = text_case == projected_esp_text_case::uppercase || (text_case == projected_esp_text_case::default_style && projected_esp_default_uppercase(id));
		const bool lowercase = text_case == projected_esp_text_case::lowercase;
		if (!uppercase && !lowercase) return value;

		for (char& ch : value) {
			const unsigned char uch = static_cast<unsigned char>(ch);
			ch = static_cast<char>(uppercase ? std::toupper(uch) : std::tolower(uch));
		}
		return value;
	}

	inline projected_esp_settings read_projected_esp_settings() {
		static thread_local projected_esp_settings cached{};
		static thread_local int cached_frame = -1;
		const int frame = ImGui::GetFrameCount();
		if (cached_frame == frame) {
			return cached;
		}
		cached_frame = frame;

		projected_esp_settings& s = cached;
		s.max_range = config::get("hack", "max_range", 1000.f);
		s.draw_skeleton = config::get("visual", "draw_skeleton", 0) != 0;
		s.dim_dead_players = config::get("visual", "dim_dead_players", 1) != 0;
		s.health_mode = config::get("visual", "health_mode", config::get("visual", "draw_healthbar", 0) != 0 ? 1 : 0);
		s.health_position = std::clamp(config::get("visual", "healthbar_position", 0), 0, 1);
		s.draw_box = config::get("visual", "draw_box", 0) != 0;
		s.box_style = std::clamp(config::get("visual", "draw_box_style", 0), 0, 2);
		s.box_radius = config::get("visual", "box_radius", 0.f);
		s.box_thickness = config::get("visual", "box_thickness", 0.f);
		s.name_pos = config::get("visual", "esp_pos_name", 0);
		s.dist_pos = config::get("visual", "esp_pos_dist", 1);
		s.weapon_pos = config::get("visual", "esp_pos_weap", 1);
		const projected_esp_side weapon_default_side = clamp_projected_esp_side(s.weapon_pos);
		const projected_esp_side health_default_side = config::get("visual", "healthbar_position", 0) == 1 ? projected_esp_side::bottom : projected_esp_side::left;
		s.layouts[projected_esp_element_index(projected_esp_element_id::dead)] = read_projected_esp_layout(projected_esp_element_id::dead, projected_esp_side::top, 0);
		s.layouts[projected_esp_element_index(projected_esp_element_id::admin)] = read_projected_esp_layout(projected_esp_element_id::admin, projected_esp_side::top, 1);
		s.layouts[projected_esp_element_index(projected_esp_element_id::tester)] = read_projected_esp_layout(projected_esp_element_id::tester, projected_esp_side::top, 2);
		s.layouts[projected_esp_element_index(projected_esp_element_id::static_id)] = read_projected_esp_layout(projected_esp_element_id::static_id, projected_esp_side::top, 3);
		s.layouts[projected_esp_element_index(projected_esp_element_id::relation)] = read_projected_esp_layout(projected_esp_element_id::relation, projected_esp_side::top, 4);
		s.layouts[projected_esp_element_index(projected_esp_element_id::name)] = read_projected_esp_layout(projected_esp_element_id::name, clamp_projected_esp_side(s.name_pos), 5);
		s.layouts[projected_esp_element_index(projected_esp_element_id::dormant)] = read_projected_esp_layout(projected_esp_element_id::dormant, projected_esp_side::top, 5);
		s.layouts[projected_esp_element_index(projected_esp_element_id::level)] = read_projected_esp_layout(projected_esp_element_id::level, projected_esp_side::right, 0);
		s.layouts[projected_esp_element_index(projected_esp_element_id::faction)] = read_projected_esp_layout(projected_esp_element_id::faction, projected_esp_side::right, 1);
		s.layouts[projected_esp_element_index(projected_esp_element_id::media)] = read_projected_esp_layout(projected_esp_element_id::media, projected_esp_side::right, 2);
		s.layouts[projected_esp_element_index(projected_esp_element_id::afk)] = read_projected_esp_layout(projected_esp_element_id::afk, projected_esp_side::right, 3);
		s.layouts[projected_esp_element_index(projected_esp_element_id::cuffed)] = read_projected_esp_layout(projected_esp_element_id::cuffed, projected_esp_side::right, 4);
		s.layouts[projected_esp_element_index(projected_esp_element_id::wanted)] = read_projected_esp_layout(projected_esp_element_id::wanted, projected_esp_side::right, 5);
		s.layouts[projected_esp_element_index(projected_esp_element_id::armor_tag)] = read_projected_esp_layout(projected_esp_element_id::armor_tag, projected_esp_side::right, 6);
		s.layouts[projected_esp_element_index(projected_esp_element_id::social)] = read_projected_esp_layout(projected_esp_element_id::social, projected_esp_side::right, 7);
		s.layouts[projected_esp_element_index(projected_esp_element_id::visible)] = read_projected_esp_layout(projected_esp_element_id::visible, projected_esp_side::right, 8);
		s.layouts[projected_esp_element_index(projected_esp_element_id::health)] = read_projected_esp_layout(projected_esp_element_id::health, health_default_side, 0);
		s.layouts[projected_esp_element_index(projected_esp_element_id::armor)] = read_projected_esp_layout(projected_esp_element_id::armor, health_default_side, 1);
		s.layouts[projected_esp_element_index(projected_esp_element_id::weapon_text)] = read_projected_esp_layout(projected_esp_element_id::weapon_text, weapon_default_side, 1);
		s.layouts[projected_esp_element_index(projected_esp_element_id::weapon_icon)] = read_projected_esp_layout(projected_esp_element_id::weapon_icon, weapon_default_side, 2);
		s.layouts[projected_esp_element_index(projected_esp_element_id::distance)] = read_projected_esp_layout(projected_esp_element_id::distance, clamp_projected_esp_side(s.dist_pos), 100);
		for (int i = 0; i < k_projected_esp_element_count; ++i) {
			const auto id = static_cast<projected_esp_element_id>(i);
			s.element_styles[i] = read_projected_esp_element_style(id);
			s.element_color_overrides[i] = read_projected_esp_element_color_override(id, &s.element_colors[i]);
		}
		s.show_name = config::get("visual", "altv_nickname", 0) != 0;
		s.show_static = config::get("visual", "altv_static", 0) != 0;
		s.show_faction = config::get("visual", "altv_faction", 0) != 0;
		s.show_relation = config::get("visual", "altv_relation", 1) != 0;
		s.show_admin = config::get("visual", "altv_admin", 0) != 0;
		s.show_tester = config::get("visual", "altv_tester", 0) != 0;
		s.show_media = config::get("visual", "altv_media", 0) != 0;
		s.show_afk = config::get("visual", "altv_afk", 0) != 0;
		s.show_dead = config::get("visual", "altv_dead", 0) != 0;
		s.show_level = config::get("visual", "altv_level", 0) != 0;
		s.show_distance = config::get("visual", "draw_distance", 0) != 0;
		s.show_cuffed = config::get("visual", "altv_cuffed", 0) != 0;
		s.show_wanted = config::get("visual", "altv_wanted", 0) != 0;
		s.show_armor_tag = config::get("visual", "altv_armor_flag", 0) != 0;
		s.show_social = config::get("visual", "altv_social", 0) != 0;
		s.show_visible = config::get("visual", "altv_visible", 0) != 0;
		const bool legacy_show_weapon = config::get("visual", "draw_weapons", 0) != 0;
		const bool show_weapon_text = config::get("visual", "draw_weapon_text", 0) != 0;
		const bool show_weapon_icon = config::get("visual", "draw_weapon_icon", 0) != 0;
		s.show_weapon_text = show_weapon_text || (legacy_show_weapon && !show_weapon_icon);
		s.show_weapon_icon = show_weapon_icon;
		s.show_weapon = s.show_weapon_text || s.show_weapon_icon;
		s.cheap_render_text = config::get("visual", "cheap_render_text", 0) != 0;
		s.skeleton_thickness = config::get("visual", "skeleton_thickness", 0.1f);
		s.skeleton_invisible_enabled = config::get("visual", "skel_invisible_enabled", 1) != 0;
		s.nickname_color = get_altv_nickname_color_u32(false, false);
		s.static_color = get_altv_static_color_u32();
		s.faction_color = get_altv_faction_color_u32();
		s.admin_color = get_altv_admin_color_u32();
		s.media_color = get_altv_media_color_u32();
		s.afk_color = get_altv_afk_color_u32();
		s.dead_color = get_altv_dead_color_u32();
		s.level_color = get_altv_level_color_u32();
		s.cuffed_color = get_altv_cuffed_color_u32();
		s.wanted_color = get_altv_wanted_color_u32();
		s.armor_tag_color = get_altv_armor_color_u32();
		s.social_color = get_altv_social_color_u32();
		s.visible_flag_color = get_altv_visible_color_u32();
		s.invisible_box_color = RGBA(
			(int)(config::get("visual", "invisible_box_color_r", 1.f) * 255),
			(int)(config::get("visual", "invisible_box_color_g", 0.f) * 255),
			(int)(config::get("visual", "invisible_box_color_b", 0.f) * 255),
			(int)(config::get("visual", "invisible_box_color_a", 1.f) * 255)
		);
		s.enemy_color = get_enemy_color_u32();
		s.friend_color = get_friend_color_u32();
		s.family_relation_color = get_family_relation_color_u32();
		s.fraction_relation_color = get_fraction_relation_color_u32();
		s.health_static_color = config::get("visual", "healthbar_static_color", 0) != 0;
		s.health_static_color_u32 = IM_COL32(
			(int)(config::get("visual", "healthbar_color_r", 0.f) * 255),
			(int)(config::get("visual", "healthbar_color_g", 0.9f) * 255),
			(int)(config::get("visual", "healthbar_color_b", 0.18f) * 255),
			(int)(config::get("visual", "healthbar_color_a", 1.f) * 255)
		);
		s.armor_color = IM_COL32(
			(int)(config::get("visual", "armorbar_color_r", 0.f) * 255),
			(int)(config::get("visual", "armorbar_color_g", 122.f / 255.f) * 255),
			(int)(config::get("visual", "armorbar_color_b", 1.f) * 255),
			(int)(config::get("visual", "armorbar_color_a", 1.f) * 255)
		);
		s.fraction_color_mode = std::clamp(config::get("visual", "fraction_color_mode", 0), 0, 2);
		s.fraction_static_color = s.static_color;
		if (s.fraction_color_mode == 2) {
			s.fraction_custom_colors[1] = read_visual_color_u32("fraction_1_color_r", "fraction_1_color_g", "fraction_1_color_b", "fraction_1_color_a", 56.f / 255.f, 128.f / 255.f, 1.f, 1.f);
			s.fraction_custom_colors[2] = read_visual_color_u32("fraction_2_color_r", "fraction_2_color_g", "fraction_2_color_b", "fraction_2_color_a", 1.f, 92.f / 255.f, 92.f / 255.f, 1.f);
			s.fraction_custom_colors[3] = read_visual_color_u32("fraction_3_color_r", "fraction_3_color_g", "fraction_3_color_b", "fraction_3_color_a", 194.f / 255.f, 124.f / 255.f, 54.f / 255.f, 1.f);
			s.fraction_custom_colors[4] = read_visual_color_u32("fraction_4_color_r", "fraction_4_color_g", "fraction_4_color_b", "fraction_4_color_a", 98.f / 255.f, 150.f / 255.f, 78.f / 255.f, 1.f);
			s.fraction_custom_colors[5] = read_visual_color_u32("fraction_5_color_r", "fraction_5_color_g", "fraction_5_color_b", "fraction_5_color_a", 64.f / 255.f, 112.f / 255.f, 1.f, 1.f);
			s.fraction_custom_colors[6] = read_visual_color_u32("fraction_6_color_r", "fraction_6_color_g", "fraction_6_color_b", "fraction_6_color_a", 1.f, 198.f / 255.f, 66.f / 255.f, 1.f);
			s.fraction_custom_colors[7] = read_visual_color_u32("fraction_7_color_r", "fraction_7_color_g", "fraction_7_color_b", "fraction_7_color_a", 40.f / 255.f, 78.f / 255.f, 120.f / 255.f, 1.f);
			s.fraction_custom_colors[8] = read_visual_color_u32("fraction_8_color_r", "fraction_8_color_g", "fraction_8_color_b", "fraction_8_color_a", 152.f / 255.f, 84.f / 255.f, 1.f, 1.f);
			s.fraction_custom_colors[9] = read_visual_color_u32("fraction_9_color_r", "fraction_9_color_g", "fraction_9_color_b", "fraction_9_color_a", 1.f, 214.f / 255.f, 73.f / 255.f, 1.f);
			s.fraction_custom_colors[10] = read_visual_color_u32("fraction_10_color_r", "fraction_10_color_g", "fraction_10_color_b", "fraction_10_color_a", 78.f / 255.f, 210.f / 255.f, 104.f / 255.f, 1.f);
			s.fraction_custom_colors[11] = read_visual_color_u32("fraction_11_color_r", "fraction_11_color_g", "fraction_11_color_b", "fraction_11_color_a", 216.f / 255.f, 45.f / 255.f, 45.f / 255.f, 1.f);
			s.fraction_custom_colors[12] = read_visual_color_u32("fraction_12_color_r", "fraction_12_color_g", "fraction_12_color_b", "fraction_12_color_a", 65.f / 255.f, 145.f / 255.f, 1.f, 1.f);
		}
		s.distance_color = IM_COL32((int)(config::get("visual", "dist_color_r", 201.f / 255.f) * 255), (int)(config::get("visual", "dist_color_g", 199.f / 255.f) * 255), (int)(config::get("visual", "dist_color_b", 199.f / 255.f) * 255), 220);
		s.weapon_color = IM_COL32((int)(config::get("visual", "weap_color_r", 241.f / 255.f) * 255), (int)(config::get("visual", "weap_color_g", 241.f / 255.f) * 255), (int)(config::get("visual", "weap_color_b", 241.f / 255.f) * 255), 220);
		s.visible_color = RGBA(config::get("visual", "visible_r", 0.f) * 255, config::get("visual", "visible_g", 1.f) * 255, config::get("visual", "visible_b", 0.f) * 255, config::get("visual", "visible_a", 1.f) * 255);
		s.skeleton_color = RGBA(config::get("visual", "skel_visible_r", 1.f) * 255, config::get("visual", "skel_visible_g", 1.f) * 255, config::get("visual", "skel_visible_b", 1.f) * 255, config::get("visual", "skel_visible_a", 1.f) * 255);
		s.skeleton_invisible_color = RGBA(config::get("visual", "skel_invisible_r", 1.f) * 255, config::get("visual", "skel_invisible_g", 1.f) * 255, config::get("visual", "skel_invisible_b", 1.f) * 255, config::get("visual", "skel_invisible_a", 1.f) * 255);
		s.box_color = RGBA(config::get("visual", "box_color_r", 1.f) * 255, config::get("visual", "box_color_g", 1.f) * 255, config::get("visual", "box_color_b", 1.f) * 255, config::get("visual", "box_color_a", 1.f) * 255);
		s.box_fill_color = RGBA(config::get("visual", "draw_box_fill_color_r", 0.2f) * 255, config::get("visual", "draw_box_fill_color_g", 0.2f) * 255, config::get("visual", "draw_box_fill_color_b", 0.2f) * 255, config::get("visual", "draw_box_fill_color_a", 0.2f) * 255);
		cached = s;
		return cached;
	}

	inline ImFont* get_projected_name_font(const projected_esp_settings& settings, float box_height, float& name_size, float& small_size, float& label_size, float& flag_label_size) {
		ImFont* base_font = renderer.espFont ? renderer.espFont : (renderer.hudFont ? renderer.hudFont : ImGui::GetFont());
		const float base_info_size = base_font ? base_font->FontSize : 13.f;
		const float info_scale = settings.cheap_render_text ? 1.f : std::clamp((box_height - base_info_size) / (box_height > 0.f ? box_height : 1.f), 0.60f, 1.f);
		name_size = base_info_size * info_scale;
		small_size = 10.f;
		label_size = 12.f;
		flag_label_size = (std::max)(1.f, small_size - 1.f);

		ImFont* font = renderer.EspNameFont(name_size, &name_size);
		if (!font) font = base_font;
		return font;
	}

	inline ImFont* get_projected_small_font(const projected_esp_settings& settings, float& small_size) {
		(void)settings;
		ImFont* font = renderer.EspSmallFont(&small_size);
		if (!font) font = renderer.espFont ? renderer.espFont : ImGui::GetFont();
		return font;
	}

	inline float get_projected_distance_scale(float distance) {
		if (!std::isfinite(distance)) return 1.f;
		if (distance <= 35.f) return 1.f;
		if (distance >= 250.f) return 0.55f;

		const float t = (distance - 35.f) / (250.f - 35.f);
		return 1.f - (0.45f * t);
	}

	inline float get_projected_bottom_health_bar_width(float box_width, float distance, float label_size) {
		(void)distance;
		(void)label_size;
		return box_width;
	}

	inline ImVec2 align_projected_text_pos(const ImVec2& value) {
		return ImVec2(floorf(value.x), floorf(value.y));
	}

	inline void draw_projected_shadow_text(ImDrawList* dl, ImFont* text_font, const char* text, const ImVec2& pos, float font_size, ImU32 color, float opacity, const projected_esp_settings& settings) {
		if (!dl || !text_font || !text || !text[0]) return;
		const ImVec2 text_pos = align_projected_text_pos(pos);
		const ImU32 text_color = esp_apply_opacity(color, opacity);
		const ImU32 outline_color = IM_COL32(0, 0, 0, (int)(185.f * opacity));
		const float outline_offset = settings.cheap_render_text ? 1.f : 0.75f;
		dl->AddText(text_font, font_size, ImVec2(text_pos.x - outline_offset, text_pos.y), outline_color, text);
		dl->AddText(text_font, font_size, ImVec2(text_pos.x + outline_offset, text_pos.y), outline_color, text);
		dl->AddText(text_font, font_size, ImVec2(text_pos.x, text_pos.y - outline_offset), outline_color, text);
		dl->AddText(text_font, font_size, ImVec2(text_pos.x, text_pos.y + outline_offset), outline_color, text);
		dl->AddText(text_font, font_size, text_pos, text_color, text);
	}

	inline ImU32 get_dynamic_health_color(float ratio) {
		ratio = esp_clamp_ratio(ratio);
		const int r = (int)((1.f - ratio) * 255.f);
		const int g = (int)(ratio * 255.f);
		return IM_COL32(r, g, 0, 255);
	}

	inline ImU32 get_health_status_color(float hp_ratio) {
		if (config::get("visual", "healthbar_static_color", 0)) {
			return IM_COL32(
				(int)(config::get("visual", "healthbar_color_r", 0.f) * 255),
				(int)(config::get("visual", "healthbar_color_g", 0.9f) * 255),
				(int)(config::get("visual", "healthbar_color_b", 0.18f) * 255),
				(int)(config::get("visual", "healthbar_color_a", 1.f) * 255)
			);
		}
		return get_dynamic_health_color(hp_ratio);
	}

	inline ImU32 get_armor_status_color() {
		return IM_COL32(
			(int)(config::get("visual", "armorbar_color_r", 0.f) * 255),
			(int)(config::get("visual", "armorbar_color_g", 122.f / 255.f) * 255),
			(int)(config::get("visual", "armorbar_color_b", 1.f) * 255),
			(int)(config::get("visual", "armorbar_color_a", 1.f) * 255)
		);
	}

	inline void draw_bar_value_text(ImDrawList* dl, float bar_x, float bar_width, float bar_y1, float bar_y2, float filled_y, int value, float opacity) {
		if (!dl || value >= 100) return;

		char value_buf[16];
		sprintf_s(value_buf, "%d", value);

		const float font_size_px = 10.f;
		ImFont* font = renderer.tahomaBoldFont ? renderer.tahomaBoldFont : (renderer.espFont ? renderer.espFont : ImGui::GetFont());
		if (!font) return;
		const ImVec2 ts = font ? font->CalcTextSizeA(font_size_px, FLT_MAX, 0.f, value_buf) : ImGui::CalcTextSize(value_buf);
		float text_x = bar_x + bar_width * 0.5f - ts.x * 0.5f;
		float text_y = filled_y - ts.y * 0.5f;
		const float max_y = bar_y2 - ts.y;
		if (text_y < bar_y1) text_y = bar_y1;
		if (text_y > max_y) text_y = max_y;

		const ImU32 text_col = IM_COL32(255, 255, 255, (int)(255.f * opacity));
		const ImU32 outline_col = IM_COL32(0, 0, 0, (int)(185.f * opacity));
		const float outline_offset = 0.75f;
		dl->AddText(font, font_size_px, ImVec2(text_x - outline_offset, text_y), outline_col, value_buf);
		dl->AddText(font, font_size_px, ImVec2(text_x + outline_offset, text_y), outline_col, value_buf);
		dl->AddText(font, font_size_px, ImVec2(text_x, text_y - outline_offset), outline_col, value_buf);
		dl->AddText(font, font_size_px, ImVec2(text_x, text_y + outline_offset), outline_col, value_buf);
		dl->AddText(font, font_size_px, ImVec2(text_x, text_y), text_col, value_buf);
	}

	inline void draw_horizontal_bar_value_text(ImDrawList* dl, float bar_x1, float bar_x2, float bar_y1, float bar_y2, float filled_x, int value, float opacity) {
		if (!dl || value >= 100) return;

		char value_buf[16];
		sprintf_s(value_buf, "%d", value);

		const float font_size_px = 10.f;
		ImFont* font = renderer.tahomaBoldFont ? renderer.tahomaBoldFont : (renderer.espFont ? renderer.espFont : ImGui::GetFont());
		if (!font) return;
		const ImVec2 ts = font ? font->CalcTextSizeA(font_size_px, FLT_MAX, 0.f, value_buf) : ImGui::CalcTextSize(value_buf);
		float text_x = filled_x - ts.x * 0.5f;
		const float max_x = (std::max)(bar_x1, bar_x2 - ts.x);
		if (text_x < bar_x1) text_x = bar_x1;
		if (text_x > max_x) text_x = max_x;
		const float text_y = bar_y1 + ((bar_y2 - bar_y1) - ts.y) * 0.5f;

		const ImU32 text_col = IM_COL32(255, 255, 255, (int)(255.f * opacity));
		const ImU32 outline_col = IM_COL32(0, 0, 0, (int)(185.f * opacity));
		const float outline_offset = 0.75f;
		dl->AddText(font, font_size_px, ImVec2(text_x - outline_offset, text_y), outline_col, value_buf);
		dl->AddText(font, font_size_px, ImVec2(text_x + outline_offset, text_y), outline_col, value_buf);
		dl->AddText(font, font_size_px, ImVec2(text_x, text_y - outline_offset), outline_col, value_buf);
		dl->AddText(font, font_size_px, ImVec2(text_x, text_y + outline_offset), outline_col, value_buf);
		dl->AddText(font, font_size_px, ImVec2(text_x, text_y), text_col, value_buf);
	}

	inline void draw_vertical_status_bar(ImDrawList* dl, float bar_x, float bar_y1, float bar_y2, float ratio, ImU32 fill_color, bool show_value, float opacity, float bar_width = 1.5f) {
		if (!dl) return;

		bar_width = (std::max)(1.f, bar_width);
		const float outline_pad = 0.5f;
		const ImU32 outline = IM_COL32(0, 0, 0, (int)(55.f * opacity));
		const ImU32 bg = IM_COL32(0, 0, 0, (int)(255.f * 0.28f * opacity));
		fill_color = esp_apply_opacity(fill_color, opacity);
		bar_x = floorf(bar_x) + 0.5f;
		bar_y1 = floorf(bar_y1) + 0.5f;
		bar_y2 = floorf(bar_y2) + 0.5f;
		const float filled_y = floorf(bar_y2 - (bar_y2 - bar_y1) * esp_clamp_ratio(ratio)) + 0.5f;

		dl->AddRect(ImVec2(bar_x - outline_pad, bar_y1 - outline_pad), ImVec2(bar_x + bar_width + outline_pad, bar_y2 + outline_pad), outline, 0.f, 0, 0.5f);
		dl->AddRectFilled(ImVec2(bar_x, bar_y1), ImVec2(bar_x + bar_width, bar_y2), bg, 0.f, 0);
		dl->AddRectFilled(ImVec2(bar_x, filled_y), ImVec2(bar_x + bar_width, bar_y2), fill_color, 0.f, 0);

		if (show_value) {
			const int value = (int)(ratio * 100.f + 0.5f);
			draw_bar_value_text(dl, bar_x, bar_width, bar_y1, bar_y2, filled_y, value, opacity);
		}
	}

	inline void draw_horizontal_status_bar(ImDrawList* dl, float x, float y, float w, float h, float ratio, ImU32 fill_color, bool show_value, float opacity) {
		if (!dl || w <= 0.f || h <= 0.f) return;

		const float outline_pad = 0.5f;
		const ImU32 outline = IM_COL32(0, 0, 0, (int)(55.f * opacity));
		const ImU32 bg = IM_COL32(0, 0, 0, (int)(255.f * 0.28f * opacity));
		fill_color = esp_apply_opacity(fill_color, opacity);

		const float x1 = floorf(x) + 0.5f;
		const float y1 = floorf(y) + 0.5f;
		const float x2 = floorf(x + w) + 0.5f;
		const float y2 = floorf(y + h) + 0.5f;
		const float filled_x = x1 + (x2 - x1) * esp_clamp_ratio(ratio);

		dl->AddRect(ImVec2(x1 - outline_pad, y1 - outline_pad), ImVec2(x2 + outline_pad, y2 + outline_pad), outline, 0.f, 0, 0.5f);
		dl->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), bg, 0.f, 0);
		dl->AddRectFilled(ImVec2(x1, y1), ImVec2(filled_x, y2), fill_color, 0.f, 0);

		if (show_value) {
			const int value = (int)(ratio * 100.f + 0.5f);
			draw_horizontal_bar_value_text(dl, x1, x2, y1, y2, filled_x, value, opacity);
		}
	}

	inline void draw_projected_status_bar(ImDrawList* dl, const ImVec2& pos, const ImVec2& size, bool horizontal, float ratio, ImU32 fill_color, bool show_value, float opacity) {
		if (!dl || size.x <= 0.f || size.y <= 0.f) return;

		const float outline_pad = 0.5f;
		const ImU32 outline = IM_COL32(0, 0, 0, (int)(55.f * opacity));
		const ImU32 bg = IM_COL32(0, 0, 0, (int)(255.f * 0.28f * opacity));
		fill_color = esp_apply_opacity(fill_color, opacity);

		const float x1 = floorf(pos.x) + 0.5f;
		const float y1 = floorf(pos.y) + 0.5f;
		const float x2 = floorf(pos.x + size.x) + 0.5f;
		const float y2 = floorf(pos.y + size.y) + 0.5f;
		dl->AddRect(ImVec2(x1 - outline_pad, y1 - outline_pad), ImVec2(x2 + outline_pad, y2 + outline_pad), outline, 0.f, 0, 0.5f);
		dl->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), bg, 0.f, 0);

		if (horizontal) {
			const float filled_x = x1 + (x2 - x1) * esp_clamp_ratio(ratio);
			dl->AddRectFilled(ImVec2(x1, y1), ImVec2(filled_x, y2), fill_color, 0.f, 0);
			if (show_value) {
				draw_horizontal_bar_value_text(dl, x1, x2, y1, y2, filled_x, (int)(ratio * 100.f + 0.5f), opacity);
			}
			return;
		}

		const float filled_y = floorf(y2 - (y2 - y1) * esp_clamp_ratio(ratio)) + 0.5f;
		dl->AddRectFilled(ImVec2(x1, filled_y), ImVec2(x2, y2), fill_color, 0.f, 0);
		if (show_value) {
			draw_bar_value_text(dl, x1, x2 - x1, y1, y2, filled_y, (int)(ratio * 100.f + 0.5f), opacity);
		}
	}

	inline void draw_health_stack(ImDrawList* dl, const ImVec2& p0, const ImVec2& p1, float health, float max_health, float armor, bool show_values, float opacity, ImRect* out_rect = nullptr) {
		if (max_health <= 0.f) return;

		const float hp_ratio = esp_clamp_ratio(health / max_health);
		const float armor_ratio = esp_clamp_ratio(armor / 100.f);
		const float bar_y1 = p0.y;
		const float bar_y2 = p1.y;
		const float bar_width = 1.5f;
		const float bar_x = p0.x - bar_width - 2.f;

		if (out_rect) {
			const int mode = config::get("visual", "health_mode", config::get("visual", "draw_healthbar", 0) != 0 ? 1 : 0);
			const float total_width = mode == 2 && armor_ratio > 0.f ? bar_width * 2.f + 2.f : bar_width;
			*out_rect = ImRect(ImVec2(bar_x - (total_width - bar_width), bar_y1), ImVec2(bar_x + bar_width, bar_y2));
		}

		const ImU32 hp_color = get_health_status_color(hp_ratio);
		const ImU32 armor_color = get_armor_status_color();
		if (config::get("visual", "health_mode", config::get("visual", "draw_healthbar", 0) != 0 ? 1 : 0) == 1) {
			draw_vertical_status_bar(dl, bar_x, bar_y1, bar_y2, hp_ratio, hp_color, show_values, opacity);
			return;
		}

		if (config::get("visual", "health_mode", config::get("visual", "draw_healthbar", 0) != 0 ? 1 : 0) == 2) {
			draw_vertical_status_bar(dl, bar_x, bar_y1, bar_y2, hp_ratio, hp_color, show_values, opacity);
			if (armor_ratio > 0.f) draw_vertical_status_bar(dl, bar_x - bar_width - 2.f, bar_y1, bar_y2, armor_ratio, armor_color, show_values, opacity);
			return;
		}

		const float use_ratio = armor_ratio > 0.f ? armor_ratio : hp_ratio;
		const ImU32 fill_color = armor_ratio > 0.f ? armor_color : hp_color;
		draw_vertical_status_bar(dl, bar_x, bar_y1, bar_y2, use_ratio, fill_color, show_values, opacity);
	}

	inline void draw_projected_bracket_box(ImDrawList* dl, const ImVec2& p0, const ImVec2& p1, float thickness, float opacity, ImU32 main_color = IM_COL32(235, 235, 235, 255)) {
		if (!dl) return;
		float len = (p1.x - p0.x) * 0.28f;
		if (len < 7.0f) len = 7.0f;
		if (len > 16.0f) len = 16.0f;

		float lenh = (p1.y - p0.y) * 0.18f;
		if (lenh < 9.0f) lenh = 9.0f;
		if (lenh > 20.0f) lenh = 20.0f;

		const float th = thickness < 1.0f ? 1.0f : thickness;
		const float outline_th = th + 2.0f;
		const ImU32 main = main_color;
		const ImU32 outline = IM_COL32(0, 0, 0, (int)(220.f * opacity));
		const float radius = (std::min)(6.0f, (std::min)(len, lenh) * 0.45f);

		auto draw_corner = [&](ImVec2 corner, float sx, float sy, float a0, float a1) {
			auto stroke = [&](ImU32 col, float stroke_thickness) {
				dl->PathClear();
				dl->PathLineTo(ImVec2(corner.x + sx * len, corner.y));
				dl->PathLineTo(ImVec2(corner.x + sx * radius, corner.y));
				dl->PathArcTo(ImVec2(corner.x + sx * radius, corner.y + sy * radius), radius, a0, a1, 8);
				dl->PathLineTo(ImVec2(corner.x, corner.y + sy * lenh));
				dl->PathStroke(col, false, stroke_thickness);
			};

			stroke(outline, outline_th);
			stroke(main, th);
		};

		draw_corner(ImVec2(p0.x, p0.y), +1.f, +1.f, -IM_PI * 0.5f, -IM_PI);
		draw_corner(ImVec2(p1.x, p0.y), -1.f, +1.f, -IM_PI * 0.5f, 0.0f);
		draw_corner(ImVec2(p0.x, p1.y), +1.f, -1.f, IM_PI * 0.5f, IM_PI);
		draw_corner(ImVec2(p1.x, p1.y), -1.f, -1.f, IM_PI * 0.5f, 0.0f);
	}

	inline projected_esp_overlay_rects draw_projected_player_overlay(ImDrawList* dl, const projected_esp_settings& settings, const projected_esp_player& player, ImVec2 top2d, ImVec2 bottom2d, float distance, float opacity) {
		projected_esp_overlay_rects rects{};
		if (!dl || opacity <= 0.005f) return rects;

		float box_height = fabsf(top2d.y - bottom2d.y);
		if (box_height < 12.f) box_height = 12.f;
		const float box_width = box_height / 2.0f;
		const ImVec2 box_min(top2d.x - (box_width / 2.0f), top2d.y);
		const ImVec2 box_max(top2d.x + (box_width / 2.0f), top2d.y + box_height);
		const float thickness_box = settings.box_thickness;

		float name_size = 13.f;
		float small_size = 10.f;
		float label_size = 12.f;
		float flag_label_size = 11.f;
		ImFont* esp_font = get_projected_name_font(settings, box_height, name_size, small_size, label_size, flag_label_size);
		ImFont* esp_small_font = get_projected_small_font(settings, small_size);
		if (!esp_font || !esp_small_font) return rects;
		flag_label_size = (std::max)(1.f, small_size - 1.f);

		const RGBA box_color = player.visible ? settings.box_color : settings.invisible_box_color;
		if (settings.draw_box) {
			if (settings.box_style == 0) {
				dl->AddRect(ImVec2(box_min.x - 1, box_min.y - 1), ImVec2(box_max.x + 1, box_max.y + 1), IM_COL32(0, 0, 0, (int)(255.f * opacity)), settings.box_radius, ImDrawFlags_RoundCornersAll, 1.f);
				dl->AddRect(box_min, box_max, rgba_to_u32(esp_apply_opacity(box_color, opacity)), settings.box_radius, ImDrawFlags_RoundCornersAll, thickness_box);
			}
			else if (settings.box_style == 1) {
				draw_projected_bracket_box(dl, box_min, box_max, thickness_box, opacity, rgba_to_u32(box_color));
			}
			else {
				dl->AddRectFilled(box_min, box_max, rgba_to_u32(esp_apply_opacity(settings.box_fill_color, opacity)), settings.box_radius, ImDrawFlags_RoundCornersAll);
				dl->AddRect(box_min, box_max, rgba_to_u32(esp_apply_opacity(box_color, opacity)), settings.box_radius, ImDrawFlags_RoundCornersAll, thickness_box);
			}
		}

		struct projected_item {
			projected_esp_element_id id = projected_esp_element_id::none;
			projected_esp_side side = projected_esp_side::top;
			int order = 0;
			int runtime_order = 0;
			projected_esp_item_type type = projected_esp_item_type::label;
			ImVec2 size{};
			ImVec2 pos{};
			ImFont* font = nullptr;
			float font_size = 0.f;
			float icon_baseline_trim = 0.f;
			ImU32 color = 0;
			std::string text;
			float health_ratio = 1.f;
			float armor_ratio = 0.f;
			bool show_health_values = false;
			float health_bar_height = 0.f;
			float health_bar_gap = 0.f;
			bool adaptive_health_bar = false;
		};

		std::vector<projected_item> items;
		items.reserve(k_projected_esp_element_count + 1);

		auto element_style = [&](projected_esp_element_id id) {
			const int index = projected_esp_element_index(id);
			return index >= 0 ? settings.element_styles[index] : projected_esp_element_style{};
		};

		auto add_text_item = [&](projected_esp_element_id id, const char* text, float mini_font_size, ImU32 fallback_color, bool apply_weapon_highlight = false) {
			if (!is_projected_esp_element(id) || !text || !text[0]) return;
			const projected_esp_element_style style = element_style(id);
			const bool force_mini_font = id == projected_esp_element_id::weapon_text;
			ImFont* text_font = !force_mini_font && style.font == projected_esp_font_style::bold ? renderer.EspNameFont(style.size, nullptr) : renderer.EspSmallFont(style.size, nullptr);
			if (!text_font) return;
			projected_item item{};
			item.id = id;
			const projected_esp_layout layout = get_projected_esp_layout(settings, id);
			item.side = layout.side;
			item.order = layout.order;
			item.type = projected_esp_item_type::label;
			item.font = text_font;
			item.font_size = style.size;
			item.color = resolve_projected_esp_element_color(settings, id, fallback_color);
			if (apply_weapon_highlight && player.weapon_highlight_active) {
				item.color = player.weapon_highlight_color;
			}
			item.text = apply_projected_esp_text_case(id, text, style.text_case);
			if (item.text.empty()) return;
			item.size = text_font->CalcTextSizeA(item.font_size, FLT_MAX, 0.0f, item.text.c_str());
			items.push_back(item);
		};

		if (settings.show_relation) {
			if (player.is_enemy) {
				add_text_item(projected_esp_element_id::relation, "ENEMY", label_size, settings.enemy_color);
			}
			else if (player.is_friend) {
				add_text_item(projected_esp_element_id::relation, "FRIEND", label_size, settings.friend_color);
			}
			else if (player.is_family) {
				add_text_item(projected_esp_element_id::relation, "FAMILY", label_size, settings.family_relation_color);
			}
			else if (player.is_fraction) {
				add_text_item(projected_esp_element_id::relation, "FRACTION", label_size, settings.fraction_relation_color);
			}
		}

		if (player.dormant) {
			if (esp_font) {
				projected_item item{};
				item.id = projected_esp_element_id::none;
				item.side = projected_esp_side::top;
				item.order = -100;
				item.type = projected_esp_item_type::label;
				item.font = esp_font;
				item.font_size = label_size;
				item.color = IM_COL32(160, 160, 160, 220);
				item.text = "DORMANT";
				item.size = esp_font->CalcTextSizeA(item.font_size, FLT_MAX, 0.0f, item.text.c_str());
				items.push_back(item);
			}
		}
		if (settings.show_name && player.name && player.name[0] != '\0') {
			const ImU32 base_name_color = player.is_enemy ? settings.enemy_color : player.is_friend ? settings.friend_color : player.is_family ? settings.family_relation_color : settings.nickname_color;
			add_text_item(projected_esp_element_id::name, player.name, small_size, base_name_color, true);
		}
		if (settings.show_static && player.static_id > 0) {
			char id_buf[48];
			if (player.dynamic_id > 0) sprintf_s(id_buf, "#%d [%d]", player.static_id, player.dynamic_id);
			else sprintf_s(id_buf, "#%d", player.static_id);
			add_text_item(projected_esp_element_id::static_id, id_buf, flag_label_size, settings.static_color);
		}
		if (settings.show_faction && player.faction && player.faction[0] != '\0' && strcmp(player.faction, "None") != 0) {
			const ImU32 faction_color = get_altv_fraction_color_u32(settings, player.fraction_id, settings.faction_color);
			char faction_buf[64];
			const char* faction_text = player.faction;
			if (player.leader_id > 0) {
				sprintf_s(faction_buf, "Leader %s", player.faction);
				faction_text = faction_buf;
			}
			add_text_item(projected_esp_element_id::faction, faction_text, flag_label_size, faction_color);
		}
		if (settings.show_admin && player.is_admin) {
			char admin_buf[32];
			if (player.admin_level > 0) sprintf_s(admin_buf, "ADMIN [%d]", player.admin_level);
			else sprintf_s(admin_buf, "ADMIN");
			add_text_item(projected_esp_element_id::admin, admin_buf, flag_label_size, settings.admin_color);
		}
		if (settings.show_tester && player.is_tester) {
			add_text_item(projected_esp_element_id::tester, "TESTER", flag_label_size, get_altv_tester_color_u32());
		}
		if (settings.show_media && player.is_media) {
			add_text_item(projected_esp_element_id::media, "MEDIA", flag_label_size, settings.media_color);
		}
		if (settings.show_level && player.level > 0) {
			char lvl_buf[32];
			sprintf_s(lvl_buf, "LVL: %d", player.level);
			add_text_item(projected_esp_element_id::level, lvl_buf, flag_label_size, settings.level_color);
		}
		if (settings.show_afk && player.is_afk) {
			add_text_item(projected_esp_element_id::afk, "AFK", flag_label_size, settings.afk_color);
		}
		if (settings.show_dead && (player.is_dead || settings.force_dead_label)) {
			add_text_item(projected_esp_element_id::dead, "DEAD", flag_label_size, settings.dead_color);
		}
		if (settings.show_cuffed && player.is_cuffed) {
			add_text_item(projected_esp_element_id::cuffed, "CUFFED", flag_label_size, settings.cuffed_color);
		}
		if (settings.show_wanted && player.wanted_level > 0) {
			char wanted_buf[32];
			sprintf_s(wanted_buf, "WANTED [%d]", player.wanted_level);
			add_text_item(projected_esp_element_id::wanted, wanted_buf, flag_label_size, settings.wanted_color);
		}
		if (settings.show_armor_tag && player.has_wear_armor) {
			add_text_item(projected_esp_element_id::armor_tag, "ARMOR", flag_label_size, settings.armor_tag_color);
		}
		if (settings.show_social && player.social && player.social[0] != '\0') {
			add_text_item(projected_esp_element_id::social, player.social, flag_label_size, settings.social_color);
		}
		if (settings.show_visible) {
			add_text_item(projected_esp_element_id::visible, player.visible ? "VISIBLE" : "HIDDEN", flag_label_size, settings.visible_flag_color);
		}

		if (settings.health_mode != 0) {
			const float hp = player.health > 100.f ? player.health - 100.f : player.health;
			const float hp_ratio = esp_clamp_ratio(hp / 100.f);
			const float armor_ratio = esp_clamp_ratio(player.armor / 100.f);
			auto add_status_bar_item = [&](projected_esp_element_id id, projected_esp_item_type type, float health_ratio, float armor_bar_ratio, bool adaptive_health_bar) {
				const projected_esp_element_style style = element_style(id);
				const float bar_size = style.size;
				const projected_esp_layout layout = get_projected_esp_layout(settings, id);
				projected_item item{};
				item.id = id;
				item.side = layout.side;
				item.order = layout.order;
				item.type = type;
				item.health_ratio = health_ratio;
				item.armor_ratio = armor_bar_ratio;
				item.show_health_values = style.show_value && distance <= 50.f;
				item.adaptive_health_bar = adaptive_health_bar;
				const bool horizontal = item.side == projected_esp_side::top || item.side == projected_esp_side::bottom;
				if (horizontal) {
					const float bottom_bar_scale = get_projected_distance_scale(distance);
					item.health_bar_height = (std::max)(1.f, bar_size * bottom_bar_scale);
					item.health_bar_gap = (std::max)(1.f, 2.f * bottom_bar_scale);
					item.size = ImVec2(get_projected_bottom_health_bar_width(box_width, distance, label_size), item.health_bar_height);
				}
				else {
					const float bar_width = (std::max)(1.f, bar_size);
					item.health_bar_height = bar_width;
					item.health_bar_gap = 2.f;
					item.size = ImVec2(bar_width, box_height);
				}
				items.push_back(item);
			};

			add_status_bar_item(projected_esp_element_id::health, projected_esp_item_type::health, hp_ratio, armor_ratio, settings.health_mode == 3);
			if (settings.health_mode == 2 && armor_ratio > 0.f) {
				add_status_bar_item(projected_esp_element_id::armor, projected_esp_item_type::armor, armor_ratio, armor_ratio, false);
			}
		}

		const bool force_custom_weapon_label = player.weapon_force_label;
		if ((settings.show_weapon || force_custom_weapon_label) && player.weapon_hash != 0 && player.weapon_hash != 0xA2719263) {
			const bool draw_weapon_text = force_custom_weapon_label || settings.show_weapon_text;
			const bool draw_weapon_icon = settings.show_weapon_icon;
			const char* weapon_label = draw_weapon_text ? player.weapon_label : nullptr;
			const float desired_icon_size = element_style(projected_esp_element_id::weapon_icon).size;
			ImFont* icon_font = renderer.EspWeaponIconFont(desired_icon_size, nullptr);
			const float icon_size = desired_icon_size;
			char weapon_icon[5]{};
			const bool has_icon = draw_weapon_icon && build_weapon_icon_utf8(player.weapon_hash, icon_font, weapon_icon);
			if ((!weapon_label || !weapon_label[0]) && draw_weapon_text) weapon_label = get_weapon_icon_title(player.weapon_hash);
			const bool has_text = draw_weapon_text && weapon_label && weapon_label[0];
			const ImU32 weapon_icon_color = player.weapon_highlight_active ? player.weapon_highlight_color : resolve_projected_esp_element_color(settings, projected_esp_element_id::weapon_icon, settings.weapon_color);
			if (has_icon) {
				const projected_esp_layout layout = get_projected_esp_layout(settings, projected_esp_element_id::weapon_icon);
				const ImVec2 icon_text_extent = icon_font->CalcTextSizeA(icon_size, FLT_MAX, 0.0f, weapon_icon);
				projected_item item{};
				item.id = projected_esp_element_id::weapon_icon;
				item.side = layout.side;
				item.order = layout.order;
				item.type = projected_esp_item_type::weapon_icon;
				item.font = icon_font;
				item.font_size = icon_size;
				item.icon_baseline_trim = icon_size * 0.18f;
				item.color = weapon_icon_color;
				item.text = weapon_icon;
				item.size = ImVec2(icon_text_extent.x, icon_size * 0.40f);
				items.push_back(item);
			}
			if (has_text) {
				add_text_item(projected_esp_element_id::weapon_text, weapon_label, label_size, settings.weapon_color, true);
			}
		}

		if (settings.show_distance) {
			char dist_buf[32];
			sprintf_s(dist_buf, "%dM", (int)(distance + 0.5f));
			add_text_item(projected_esp_element_id::distance, dist_buf, small_size, settings.distance_color);
		}

		std::array<std::vector<int>, 4> side_items{};
		for (int i = 0; i < static_cast<int>(items.size()); ++i) {
			const int side_index = std::clamp(static_cast<int>(items[i].side), 0, 3);
			side_items[side_index].push_back(i);
		}

		auto order_less = [&](int lhs, int rhs) {
			if (items[lhs].order != items[rhs].order) return items[lhs].order < items[rhs].order;
			return static_cast<int>(items[lhs].id) < static_cast<int>(items[rhs].id);
		};

		for (int side = 0; side < 4; ++side) {
			auto& list = side_items[side];
			if (list.empty()) continue;

			int dragged_index = -1;
			if (settings.layout_override_active && static_cast<int>(settings.layout_override.side) == side) {
				for (int index : list) {
					if (items[index].id == settings.layout_override_element) {
						dragged_index = index;
						break;
					}
				}
			}

			if (dragged_index >= 0) {
				list.erase(std::remove(list.begin(), list.end(), dragged_index), list.end());
				std::stable_sort(list.begin(), list.end(), order_less);
				const int insert_at = std::clamp(settings.layout_override.order, 0, static_cast<int>(list.size()));
				list.insert(list.begin() + insert_at, dragged_index);
			}
			else {
				std::stable_sort(list.begin(), list.end(), order_less);
			}

			for (int order = 0; order < static_cast<int>(list.size()); ++order) {
				items[list[order]].runtime_order = order;
			}
		}

		constexpr float stack_gap = 2.f;
		constexpr float flag_stack_gap = 0.f;
		constexpr float side_gap = 6.f;

		auto item_stack_gap = [&](int previous_index, int next_index) {
			if (previous_index < 0 || next_index < 0) return stack_gap;
			if (is_projected_esp_identity_element(items[previous_index].id) && is_projected_esp_identity_element(items[next_index].id)) return flag_stack_gap;
			const bool compact_flags = is_projected_esp_flag_element(items[previous_index].id) && is_projected_esp_flag_element(items[next_index].id);
			return compact_flags ? flag_stack_gap : stack_gap;
		};

		auto side_total_height = [&](const std::vector<int>& list) {
			float total = 0.f;
			for (int i = 0; i < static_cast<int>(list.size()); ++i) {
				total += items[list[i]].size.y;
				if (i > 0) total += item_stack_gap(list[i - 1], list[i]);
			}
			return total;
		};

		for (int side = 0; side < 4; ++side) {
			auto& list = side_items[side];
			if (list.empty()) continue;

			float y = box_min.y;
			if (side == static_cast<int>(projected_esp_side::top)) {
				y = box_min.y - thickness_box - stack_gap - side_total_height(list);
				if (settings.clamp_top_stack_y) {
					y = (std::max)(y, settings.top_stack_min_y);
				}
			}
			else if (side == static_cast<int>(projected_esp_side::bottom)) {
				y = box_max.y + thickness_box + stack_gap;
			}

			const bool side_stack = side == static_cast<int>(projected_esp_side::left) || side == static_cast<int>(projected_esp_side::right);
			int side_status_indices[2] = {};
			int side_status_count = 0;
			if (side_stack) {
				for (int index : list) {
					if (items[index].type == projected_esp_item_type::health || items[index].type == projected_esp_item_type::armor) {
						if (side_status_count < 2) side_status_indices[side_status_count++] = index;
					}
				}

				if (side_status_count > 0) {
					if (side == static_cast<int>(projected_esp_side::left)) {
						float x = box_min.x - side_gap;
						for (int i = 0; i < side_status_count; ++i) {
							const int index = side_status_indices[i];
							projected_item& item = items[index];
							x -= item.size.x;
							item.pos = ImVec2(x, box_min.y);
							x -= side_gap;
						}
					}
					else {
						float x = box_max.x + side_gap;
						for (int i = 0; i < side_status_count; ++i) {
							const int index = side_status_indices[i];
							projected_item& item = items[index];
							item.pos = ImVec2(x, box_min.y);
							x += item.size.x + side_gap;
						}
					}
				}
			}

			float side_status_width = 0.f;
			for (int i = 0; i < side_status_count; ++i) {
				side_status_width += items[side_status_indices[i]].size.x;
				if (i > 0) side_status_width += side_gap;
			}
			const float side_text_gap = side_stack && side_status_width > 0.f ? side_status_width + side_gap * 2.f : side_gap;
			for (int item_pos = 0; item_pos < static_cast<int>(list.size()); ++item_pos) {
				const int index = list[item_pos];
				projected_item& item = items[index];
				if (side_stack && (item.type == projected_esp_item_type::health || item.type == projected_esp_item_type::armor)) {
					continue;
				}
				if (side == static_cast<int>(projected_esp_side::left)) {
					item.pos = ImVec2(box_min.x - side_text_gap - item.size.x, y);
				}
				else if (side == static_cast<int>(projected_esp_side::right)) {
					item.pos = ImVec2(box_max.x + side_text_gap, y);
				}
				else {
					item.pos = ImVec2(top2d.x - item.size.x * 0.5f, y);
				}
				const int next_index = item_pos + 1 < static_cast<int>(list.size()) ? list[item_pos + 1] : -1;
				y += item.size.y + item_stack_gap(index, next_index);
			}
		}

		std::array<bool, k_projected_esp_element_count> animated_seen{};
		for (auto& item : items) {
			const int index = projected_esp_element_index(item.id);
			if (index >= 0 && settings.animated_positions && settings.animated_position_valid) {
				const ImVec2 target = item.pos;
				if ((*settings.animated_position_valid)[index]) {
					item.pos = ImLerp((*settings.animated_positions)[index], target, std::clamp(settings.layout_lerp, 0.f, 1.f));
				}
				(*settings.animated_positions)[index] = item.pos;
				animated_seen[index] = true;
			}
		}
		if (settings.animated_position_valid) {
			for (int i = 0; i < k_projected_esp_element_count; ++i) {
				(*settings.animated_position_valid)[i] = animated_seen[i];
			}
		}

		for (const auto& item : items) {
			const int index = projected_esp_element_index(item.id);
			if (index < 0) continue;
			const ImRect rect(item.pos, item.pos + item.size);
			rects.elements[index].visible = true;
			rects.elements[index].rect = rect;
			rects.elements[index].side = item.side;
			rects.elements[index].order = item.runtime_order;

			auto merge_legacy_rect = [](ImRect& dst, bool& visible, const ImRect& src) {
				if (!visible) {
					dst = src;
					visible = true;
					return;
				}
				dst.Min.x = (std::min)(dst.Min.x, src.Min.x);
				dst.Min.y = (std::min)(dst.Min.y, src.Min.y);
				dst.Max.x = (std::max)(dst.Max.x, src.Max.x);
				dst.Max.y = (std::max)(dst.Max.y, src.Max.y);
			};

			if (item.id == projected_esp_element_id::name) merge_legacy_rect(rects.name, rects.name_visible, rect);
			else if (item.id == projected_esp_element_id::distance) merge_legacy_rect(rects.distance, rects.distance_visible, rect);
			else if (item.id == projected_esp_element_id::health) merge_legacy_rect(rects.health, rects.health_visible, rect);
			else if (item.id == projected_esp_element_id::weapon_icon || item.id == projected_esp_element_id::weapon_text) merge_legacy_rect(rects.weapon, rects.weapon_visible, rect);
		}

		if (settings.draw_interaction_backgrounds) {
			for (const auto& item : items) {
				const int index = projected_esp_element_index(item.id);
				if (index < 0) continue;
				const float alpha = std::clamp(settings.interaction_alpha[index], 0.f, 1.f);
				if (alpha <= 0.01f) continue;
				const ImRect rect(item.pos - ImVec2(3.f, 2.f), item.pos + item.size + ImVec2(3.f, 2.f));
				dl->AddRectFilled(rect.Min, rect.Max, IM_COL32(0, 0, 0, (int)(85.f * alpha * opacity)), 3.f);
				dl->AddRect(rect.Min, rect.Max, IM_COL32(255, 255, 255, (int)(30.f * alpha * opacity)), 3.f, 0, 1.f);
			}
		}

		auto draw_health_item = [&](const projected_item& item) {
			const bool horizontal = item.side == projected_esp_side::top || item.side == projected_esp_side::bottom;
			const bool use_armor = item.type == projected_esp_item_type::armor || (item.adaptive_health_bar && item.armor_ratio > 0.f);
			const float ratio = use_armor ? item.armor_ratio : item.health_ratio;
			const ImU32 color = use_armor ? settings.armor_color : (settings.health_static_color ? settings.health_static_color_u32 : get_dynamic_health_color(item.health_ratio));
			draw_projected_status_bar(dl, item.pos, item.size, horizontal, ratio, color, item.show_health_values, opacity);
		};

		auto draw_projected_item = [&](const projected_item& item) {
			if (item.type == projected_esp_item_type::health || item.type == projected_esp_item_type::armor) {
				draw_health_item(item);
			}
			else if (item.type == projected_esp_item_type::weapon_icon) {
				draw_projected_shadow_text(dl, item.font, item.text.c_str(), ImVec2(item.pos.x, item.pos.y - item.icon_baseline_trim), item.font_size, item.color, opacity, settings);
			}
			else {
				draw_projected_shadow_text(dl, item.font, item.text.c_str(), item.pos, item.font_size, item.color, opacity, settings);
			}
		};

		const projected_item* dragged_item = nullptr;
		for (const auto& item : items) {
			if (settings.drag_visual_active && item.id == settings.drag_visual_element) {
				dragged_item = &item;
				const ImRect target_rect(item.pos - ImVec2(3.f, 2.f), item.pos + item.size + ImVec2(3.f, 2.f));
				dl->AddRectFilled(target_rect.Min, target_rect.Max, IM_COL32(255, 255, 255, (int)(28.f * opacity)), 3.f);
				dl->AddRect(target_rect.Min, target_rect.Max, IM_COL32(255, 255, 255, (int)(58.f * opacity)), 3.f, 0, 1.f);
				continue;
			}
			draw_projected_item(item);
		}

		if (dragged_item) {
			projected_item item = *dragged_item;
			item.pos = settings.drag_visual_pos;
			const ImRect rect(item.pos - ImVec2(3.f, 2.f), item.pos + item.size + ImVec2(3.f, 2.f));
			dl->AddRectFilled(rect.Min, rect.Max, IM_COL32(0, 0, 0, (int)(105.f * opacity)), 3.f);
			dl->AddRect(rect.Min, rect.Max, IM_COL32(255, 255, 255, (int)(45.f * opacity)), 3.f, 0, 1.f);
			draw_projected_item(item);
		}

		return rects;
	}

}
