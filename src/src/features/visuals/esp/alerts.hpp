	static bool read_local_heavy_alert_state(Vector3& local_pos, float& cy, float& sy) {
		__try {
			if (!IsValidPtr(local.player) || local.player->HP <= 0) return false;
			local_pos = local.player->fPosition;
			CPlayerAngles* cam = Game.getCam();
			if (!IsValidPtr(cam)) return false;
			const float x = std::clamp(cam->m_fps_angles.x, -1.f, 1.f);
			const float y = std::clamp(cam->m_fps_angles.y, -1.f, 1.f);
			float rot = acosf(x) * 180.0f / PI;
			if (asinf(y) * 180.0f / PI < 0.0f) rot *= -1.0f;
			const float yaw = rot * PI / 180.0f;
			cy = cosf(yaw);
			sy = sinf(yaw);
			return std::isfinite(cy) && std::isfinite(sy);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	static bool read_heavy_alert_ped(CObject* ped, Vector3& pos, float& hp) {
		__try {
			if (!IsValidPtr(ped)) return false;
			hp = ped->HP;
			if (hp <= 0.f) return false;
			pos = ped->fPosition;
			return std::isfinite(pos.x) && std::isfinite(pos.y) && std::isfinite(pos.z);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	struct weapon_arrow_settings {
		std::map<DWORD, std::string> selected_weapons;
		std::map<DWORD, std::string> custom_weapons;
		std::map<DWORD, std::string> highlighted_weapons;
		std::map<DWORD, std::string> custom_colors;
		ImU32 arrow_color = IM_COL32(180, 167, 245, 230);
		ImU32 accent_color = IM_COL32(180, 167, 245, 255);
	};

	static bool weapon_arrow_custom_allowed(const weapon_arrow_settings& settings, DWORD weapon_hash) {
		const auto it = settings.custom_weapons.find(weapon_hash);
		return it != settings.custom_weapons.end() && !it->second.empty();
	}

	static bool weapon_arrow_weapon_enabled(const weapon_arrow_settings& settings, DWORD weapon_hash) {
		if (weapon_hash == 0 || weapon_hash == 0xA2719263u) return false;
		if (!weapon_arrows::is_builtin_allowed(weapon_hash) && !weapon_arrow_custom_allowed(settings, weapon_hash)) return false;
		return weapon_arrows::selected(settings.selected_weapons, weapon_hash);
	}

	static bool weapon_arrow_highlighted(const weapon_arrow_settings& settings, DWORD weapon_hash) {
		const auto it = settings.highlighted_weapons.find(weapon_hash);
		return it != settings.highlighted_weapons.end() && it->second == "1";
	}

	static bool screen_point_visible(const ImVec2& point) {
		return point.x >= 0.f && point.y >= 0.f && point.x <= Game.screen.x && point.y <= Game.screen.y;
	}

	static bool world_point_visible(const Vector3& point) {
		ImVec2 screen{};
		return WorldToScreen(point, &screen) && screen_point_visible(screen);
	}

	static ImU32 weapon_arrow_color() {
		return IM_COL32(
			(int)(config::get("hud", "accent_r", 180.f / 255.f) * 255.f),
			(int)(config::get("hud", "accent_g", 167.f / 255.f) * 255.f),
			(int)(config::get("hud", "accent_b", 245.f / 255.f) * 255.f),
			230
		);
	}

	static ImU32 weapon_arrow_accent_color() {
		return IM_COL32(
			(int)(config::get("hud", "accent_r", 180.f / 255.f) * 255.f),
			(int)(config::get("hud", "accent_g", 167.f / 255.f) * 255.f),
			(int)(config::get("hud", "accent_b", 245.f / 255.f) * 255.f),
			255
		);
	}

	static ImU32 weapon_arrow_icon_color(DWORD weapon_hash, const weapon_arrow_settings& settings) {
		const ImU32 fallback = IM_COL32(245, 245, 245, 235);
		if (!weapon_arrow_highlighted(settings, weapon_hash)) {
			return fallback;
		}

		const ImU32 default_col = ::weapons_highlight::default_color(weapon_hash, fallback, settings.accent_color);
		const ImU32 color = visual_config::to_u32(
			visual_config::map_color(settings.custom_colors, weapon_hash, visual_config::from_u32(default_col))
		);

		return (color & ~IM_COL32_A_MASK) | (235u << IM_COL32_A_SHIFT);
	}

	static RGBA rgba_from_u32(ImU32 color) {
		return RGBA(
			(int)((color >> IM_COL32_R_SHIFT) & 0xFF),
			(int)((color >> IM_COL32_G_SHIFT) & 0xFF),
			(int)((color >> IM_COL32_B_SHIFT) & 0xFF),
			(int)((color >> IM_COL32_A_SHIFT) & 0xFF)
		);
	}

	static ImVec2 weapon_arrow_corner_point(const ImVec2& from, const ImVec2& to, float rounding) {
		const float dx = to.x - from.x;
		const float dy = to.y - from.y;
		const float len = sqrtf(dx * dx + dy * dy);
		if (len <= 0.001f) return from;
		const float amount = (std::min)(rounding, len * 0.42f);
		return ImVec2(from.x + dx * (amount / len), from.y + dy * (amount / len));
	}

	static void draw_rounded_weapon_arrow(ImDrawList* dl, const ImVec2& tip, const ImVec2& left, const ImVec2& right, float rounding, ImU32 color) {
		const ImVec2 tip_to_right = weapon_arrow_corner_point(tip, right, rounding);
		const ImVec2 right_to_tip = weapon_arrow_corner_point(right, tip, rounding);
		const ImVec2 right_to_left = weapon_arrow_corner_point(right, left, rounding);
		const ImVec2 left_to_right = weapon_arrow_corner_point(left, right, rounding);
		const ImVec2 left_to_tip = weapon_arrow_corner_point(left, tip, rounding);
		const ImVec2 tip_to_left = weapon_arrow_corner_point(tip, left, rounding);

		dl->PathLineTo(tip_to_right);
		dl->PathLineTo(right_to_tip);
		dl->PathBezierQuadraticCurveTo(right, right_to_left, 4);
		dl->PathLineTo(left_to_right);
		dl->PathBezierQuadraticCurveTo(left, left_to_tip, 4);
		dl->PathLineTo(tip_to_left);
		dl->PathBezierQuadraticCurveTo(tip, tip_to_right, 4);
		dl->PathFillConvex(color);
	}

	static void draw_weapon_arrow_marker(ImDrawList* dl, const ImVec2& anchor, const ImVec2& dir, DWORD weapon_hash, float arrow_size, float icon_size, ImU32 arrow_col, ImU32 icon_col) {
		if (!dl) return;

		const ImVec2 perp(-dir.y, dir.x);
		const ImVec2 tip(anchor.x + dir.x * arrow_size * 0.62f, anchor.y + dir.y * arrow_size * 0.62f);
		const ImVec2 base(anchor.x - dir.x * arrow_size * 0.38f, anchor.y - dir.y * arrow_size * 0.38f);
		const ImVec2 left(base.x + perp.x * arrow_size * 0.48f, base.y + perp.y * arrow_size * 0.48f);
		const ImVec2 right(base.x - perp.x * arrow_size * 0.48f, base.y - perp.y * arrow_size * 0.48f);

		const float rounding = (std::clamp)(arrow_size * 0.13f, 1.4f, 3.2f);

		draw_rounded_weapon_arrow(dl, tip, left, right, rounding, arrow_col);

		float render_icon_size = icon_size;
		ImFont* icon_font = renderer.EspWeaponIconFont(icon_size, &render_icon_size);
		char weapon_icon[5]{};
		if (!build_weapon_icon_utf8(weapon_hash, icon_font, weapon_icon)) return;

		const ImVec2 icon_center(
			anchor.x - dir.x * (arrow_size * 0.70f + icon_size * 0.16f),
			anchor.y - dir.y * (arrow_size * 0.70f + icon_size * 0.16f)
		);
		const ImVec2 icon_size_px = icon_font->CalcTextSizeA(render_icon_size, FLT_MAX, 0.f, weapon_icon);
		const ImVec2 icon_pos(icon_center.x - icon_size_px.x * 0.5f, icon_center.y - icon_size_px.y * 0.5f);
		const ImU32 icon_outline = IM_COL32(0, 0, 0, 180);

		dl->AddText(icon_font, render_icon_size, ImVec2(icon_pos.x - 1.f, icon_pos.y), icon_outline, weapon_icon);
		dl->AddText(icon_font, render_icon_size, ImVec2(icon_pos.x + 1.f, icon_pos.y), icon_outline, weapon_icon);
		dl->AddText(icon_font, render_icon_size, ImVec2(icon_pos.x, icon_pos.y - 1.f), icon_outline, weapon_icon);
		dl->AddText(icon_font, render_icon_size, ImVec2(icon_pos.x, icon_pos.y + 1.f), icon_outline, weapon_icon);
		dl->AddText(icon_font, render_icon_size, icon_pos, icon_col, weapon_icon);
	}

	void draw_weapon_arrows() {
		if (!config::get("visual", "weapon_arrows", 0)) return;
		if (!Game.gameDataReady || !Game.renderReady) return;
		if (Game.screen.x <= 0.f || Game.screen.y <= 0.f) return;

		weapon_arrow_settings settings;
		settings.custom_weapons = ::weapons_highlight::custom_entries();
		settings.selected_weapons = config::get("visual", "weapon_arrow_weapons", weapon_arrows::default_selected(settings.custom_weapons));
		settings.highlighted_weapons = config::get("visual", "weapons_highlighted", ::weapons_highlight::default_selected());
		settings.custom_colors = ::weapons_highlight::custom_colors();
		settings.arrow_color = weapon_arrow_color();
		settings.accent_color = weapon_arrow_accent_color();

		Vector3 local_pos{};
		float cy = 0.f;
		float sy = 0.f;
		if (!read_local_heavy_alert_state(local_pos, cy, sy)) return;

		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		if (!dl) return;

		const float arrow_size = config::get("visual", "weapon_arrow_size", 14.f);
		const float icon_size = config::get("visual", "weapon_arrow_icon_size", 42.f);
		const float distance = std::clamp(config::get("visual", "weapon_arrow_distance", 42.f), 1.f, 300.f) / 100.f;
		const float max_dist = config::get("hack", "max_range", 1000.f);
		const ImVec2 center(Game.screen.x * 0.5f, Game.screen.y * 0.5f);
		const float radius = (std::min)(Game.screen.x, Game.screen.y) * distance;
		const float margin = (std::max)(40.f, arrow_size + icon_size);
		WorldToScreenSnapshot w2s_view{};
		const bool has_w2s_view = CaptureWorldToScreenSnapshot(&w2s_view);

		const std::vector<ws_server::EspPlayer> players = ws_server::get_players();
		bool drew_ws_weapon_arrow = false;
		for (const auto& player : players) {
			if (player.is_dead) continue;
			if (!weapon_arrow_weapon_enabled(settings, player.weapon_hash)) continue;
			if (player.pos_x == 0.f && player.pos_y == 0.f && player.pos_z == 0.f) continue;

			Vector3 ped_pos(player.pos_x, player.pos_y, player.pos_z);
			if (!std::isfinite(ped_pos.x) || !std::isfinite(ped_pos.y) || !std::isfinite(ped_pos.z)) continue;
			if (get_distance(local_pos, ped_pos) > max_dist) continue;

			Vector3 indicator_pos = ped_pos;
			indicator_pos.z += 1.0f;
			if (world_point_visible(indicator_pos)) continue;

			ImVec2 dir{};
			if (!has_w2s_view || !get_projected_screen_direction(w2s_view, indicator_pos, center, &dir)) {
				const float dx = ped_pos.x - local_pos.x;
				const float dy = ped_pos.y - local_pos.y;
				dir = ImVec2(-(dy * cy - dx * sy), -(dx * cy + dy * sy));
				const float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
				if (len < 0.001f) continue;
				dir.x /= len;
				dir.y /= len;
			}

			ImVec2 anchor(center.x + dir.x * radius, center.y + dir.y * radius);
			anchor.x = (std::max)(margin, (std::min)(Game.screen.x - margin, anchor.x));
			anchor.y = (std::max)(margin, (std::min)(Game.screen.y - margin, anchor.y));
			draw_weapon_arrow_marker(dl, anchor, dir, player.weapon_hash, arrow_size, icon_size, settings.arrow_color, weapon_arrow_icon_color(player.weapon_hash, settings));
			drew_ws_weapon_arrow = true;
		}

		if (drew_ws_weapon_arrow) return;

		std::lock_guard<std::mutex> lock(game::ped_list_mutex);
		for (const auto& entity : game::ped_list) {
			CObject* ped = entity.first;
			if (!IsValidPtr(ped) || ped == local.player) continue;

			PedCache ped_cache{};
			if (!read_ped_cache(ped, &ped_cache) || ped_cache.hp <= 0.f) continue;

			const DWORD weapon_hash = weapon_reader::get_weapon_hash(ped);
			if (!weapon_arrow_weapon_enabled(settings, weapon_hash)) continue;
			if (get_distance(local_pos, ped_cache.pos) > max_dist) continue;

			Vector3 indicator_pos = ped_cache.pos;
			indicator_pos.z += 1.0f;
			if (world_point_visible(indicator_pos)) continue;

			ImVec2 dir{};
			if (!has_w2s_view || !get_projected_screen_direction(w2s_view, indicator_pos, center, &dir)) {
				const float dx = ped_cache.pos.x - local_pos.x;
				const float dy = ped_cache.pos.y - local_pos.y;
				dir = ImVec2(-(dy * cy - dx * sy), -(dx * cy + dy * sy));
				const float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
				if (len < 0.001f) continue;
				dir.x /= len;
				dir.y /= len;
			}

			ImVec2 anchor(center.x + dir.x * radius, center.y + dir.y * radius);
			anchor.x = (std::max)(margin, (std::min)(Game.screen.x - margin, anchor.x));
			anchor.y = (std::max)(margin, (std::min)(Game.screen.y - margin, anchor.y));
			draw_weapon_arrow_marker(dl, anchor, dir, weapon_hash, arrow_size, icon_size, settings.arrow_color, weapon_arrow_icon_color(weapon_hash, settings));
		}
	}

	void draw_heavy_sniper_detection() {
		if (!IsValidPtr(local.player) || local.player->HP <= 0) return;

		static const DWORD heavy_hashes[] = { 0x0C472FE2, 0x0A914799, 0x6E7DDDEC, 0x05FC3C11 };
		bool detected = false;

		{
			std::lock_guard<std::mutex> lock(game::ped_list_mutex);
			for (auto& entity : game::ped_list) {
				CObject* ped = entity.first;
				if (!IsValidPtr(ped) || ped->HP <= 0 || ped == local.player) continue;
				DWORD wh = weapon_reader::get_weapon_hash(ped);
				if (wh == 0) continue;
				for (int i = 0; i < 4; i++) {
					if (wh == heavy_hashes[i]) { detected = true; break; }
				}
				if (detected) break;
			}
		}

		if (detected) {
			ImVec2 screen_size = ImGui::GetIO().DisplaySize;
			float banner_w = 280.f;
			float banner_h = 32.f;
			float bx = (screen_size.x - banner_w) * 0.5f;
			float by = 25.f;

			renderer.RenderRectFilled(
				ImVec2(bx, by), ImVec2(bx + banner_w, by + banner_h),
				RGBA(9, 9, 9, 230), 8.f, 0);

			renderer.RenderText("HEAVY WEAPON DETECTED",
				ImVec2(screen_size.x * 0.5f, by + banner_h * 0.5f),
				12, RGBA(205, 72, 72, 255), true, true);
		}
	}

	inline int section_exception_filter(const char* name, EXCEPTION_POINTERS* exception) {
		const EXCEPTION_RECORD* record = exception ? exception->ExceptionRecord : nullptr;
		if (record && record->ExceptionCode == 0xE06D7363) {
			return EXCEPTION_CONTINUE_SEARCH;
		}

		crash_logger::log_exception(name ? name : "esp_section", exception);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	inline void run_section_seh(const char* name, void (*fn)()) {
		__try {
			fn();
		}
		__except (section_exception_filter(name, GetExceptionInformation())) {
			HELLO_RUNTIME_LOG("SEH: esp section %s 0x%08X", name ? name : "esp_unknown", GetExceptionCode());
		}
	}

	inline void run_section(const char* name, void (*fn)()) {
		runtime_debug::last_section = name ? name : "esp_unknown";
		try {
			run_section_seh(name, fn);
		} catch (const std::exception& e) {
			HELLO_RUNTIME_LOG("esp section %s exception: %s", name, e.what());
		} catch (...) {
			HELLO_RUNTIME_LOG("esp section %s exception", name);
		}
	}

	static bool draw_pickup_hash_box(const Vector3& center, float box_scale, const RGBA& color) {
		const Vector3 min_bounds(-1.35f * box_scale, -0.26f * box_scale, -0.24f * box_scale);
		const Vector3 max_bounds(1.35f * box_scale, 0.26f * box_scale, 0.24f * box_scale);
		const Vector3 corners[8] = {
			{ min_bounds.x, min_bounds.y, min_bounds.z },
			{ max_bounds.x, min_bounds.y, min_bounds.z },
			{ max_bounds.x, max_bounds.y, min_bounds.z },
			{ min_bounds.x, max_bounds.y, min_bounds.z },
			{ min_bounds.x, min_bounds.y, max_bounds.z },
			{ max_bounds.x, min_bounds.y, max_bounds.z },
			{ max_bounds.x, max_bounds.y, max_bounds.z },
			{ min_bounds.x, max_bounds.y, max_bounds.z },
		};

		ImVec2 screen_corners[8];
		for (int i = 0; i < 8; ++i) {
			const Vector3 world_corner(center.x + corners[i].x, center.y + corners[i].y, center.z + corners[i].z);
			if (!WorldToScreen(world_corner, &screen_corners[i]) || !screen_point_visible(screen_corners[i])) {
				return false;
			}
		}

		const int edges[12][2] = {
			{0, 1}, {1, 2}, {2, 3}, {3, 0},
			{4, 5}, {5, 6}, {6, 7}, {7, 4},
			{0, 4}, {1, 5}, {2, 6}, {3, 7}
		};

		for (int i = 0; i < 12; ++i) {
			renderer.RenderLine(screen_corners[edges[i][0]], screen_corners[edges[i][1]], color, 1.35f);
		}

		return true;
	}

	static void draw_pickup_hash_line(const ImVec2& target, const RGBA& color) {
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		if (!dl) return;

		const ImVec2 center(Game.screen.x * 0.5f, Game.screen.y * 0.5f);
		const ImU32 outline = IM_COL32(0, 0, 0, (std::min)(color.a, 170));
		const ImU32 line = IM_COL32(color.r, color.g, color.b, color.a);
		dl->AddLine(center, target, outline, 2.f);
		dl->AddLine(center, target, line, 1.f);
	}

	static int safe_object_count(CObjectInterface* objIface) {
		__try {
			if (!IsValidPtr(objIface)) return 0;
			const int max_objects = objIface->iMaxObjects;
			if (max_objects <= 0) return 0;
			return (std::min)(max_objects, 2300);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return 0;
		}
	}

	static bool read_object_hash_entry(CObjectInterface* objIface, int index, Vector3& pos, DWORD& hash, int& handle) {
		hash = 0;
		handle = 0;
		__try {
			if (!IsValidPtr(objIface) || index < 0) return false;

			CObject* obj = objIface->get_object(static_cast<unsigned int>(index));
			if (!IsValidPtr(obj)) return false;

			handle = objIface->getHandle(static_cast<unsigned int>(index));
			pos = obj->fPosition;
			CModelInfo* model = obj->ModelInfo();
			if (!IsValidPtr(model)) return false;

			hash = model->GetHash();
			return hash != 0;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			hash = 0;
			handle = 0;
			return false;
		}
	}

	static bool is_valid_world_pos(const Vector3& pos) {
		return std::isfinite(pos.x) && std::isfinite(pos.y) && std::isfinite(pos.z);
	}

	static bool is_world_weapon_inside_player_volume(const Vector3& weapon_pos, const Vector3& player_pos) {
		if (!is_valid_world_pos(weapon_pos) || !is_valid_world_pos(player_pos)) return false;

		const float dx = weapon_pos.x - player_pos.x;
		const float dy = weapon_pos.y - player_pos.y;
		if ((dx * dx + dy * dy) > (1.15f * 1.15f)) return false;

		const float dz = weapon_pos.z - player_pos.z;
		return dz > -0.45f && dz < 1.20f;
	}

	static void collect_world_weapon_player_positions(std::vector<Vector3>& positions) {
		positions.clear();
		positions.reserve(128);

		Vector3 pos{};
		float hp = 0.f;
		if (read_heavy_alert_ped(local.player, pos, hp)) {
			positions.push_back(pos);
		}

		std::lock_guard<std::mutex> lock(game::ped_list_mutex);
		for (const auto& entity : game::ped_list) {
			CObject* ped = entity.first;
			if (ped == local.player) continue;
			if (read_heavy_alert_ped(ped, pos, hp)) {
				positions.push_back(pos);
			}
		}
	}

	static bool is_world_weapon_carried_by_player(const Vector3& weapon_pos, const std::vector<Vector3>& player_positions) {
		for (const Vector3& player_pos : player_positions) {
			if (is_world_weapon_inside_player_volume(weapon_pos, player_pos)) {
				return true;
			}
		}

		return false;
	}

	static int safe_pickup_count(CPickupInterface* pickupIface) {
		__try {
			if (!IsValidPtr(pickupIface) || !IsValidPtr(pickupIface->pCPickupList)) return 0;
			const int max_pickups = pickupIface->iMaxPickups;
			if (max_pickups <= 0) return 0;
			return (std::min)(max_pickups, 73);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return 0;
		}
	}

	static bool read_pickup_entry(CPickupInterface* pickupIface, int index, Vector3& pos, int& handle) {
		handle = 0;
		__try {
			if (!IsValidPtr(pickupIface) || !IsValidPtr(pickupIface->pCPickupList)) return false;
			if (index < 0 || index >= 73 || index >= pickupIface->iMaxPickups) return false;

			const CPickupHandle& pickup_handle = pickupIface->pCPickupList->pickups[index];
			CPickup* pickup = pickup_handle.pCPickup;
			if (!IsValidPtr(pickup)) return false;

			pos = pickup->v3VisualPos;
			handle = pickup_handle.iHandle;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			handle = 0;
			return false;
		}
	}

	static void read_pickup_hashes(int handle, DWORD& pickup_hash, DWORD& weapon_hash) {
		pickup_hash = 0;
		weapon_hash = 0;
		__try {
			if (handle != 0) {
				pickup_hash = static_cast<DWORD>(native::object::_get_pickup_hash(handle));
				if (pickup_hash != 0) {
					weapon_hash = static_cast<DWORD>(native::object::_get_weapon_hash_from_pickup(pickup_hash));
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			pickup_hash = 0;
			weapon_hash = 0;
		}
	}

	static void draw_world_pickup_icon(ImDrawList* dl, const ImVec2& screen, DWORD weapon_hash, ImU32 color, float size) {
		if (!dl) return;

		float render_icon_size = size;
		ImFont* icon_font = renderer.EspWeaponIconFont(size, &render_icon_size);
		char weapon_icon[5]{};
		if (build_weapon_icon_utf8(weapon_hash, icon_font, weapon_icon)) {
			const ImVec2 icon_size_px = icon_font->CalcTextSizeA(render_icon_size, FLT_MAX, 0.f, weapon_icon);
			const ImVec2 icon_pos(screen.x - icon_size_px.x * 0.5f, screen.y - icon_size_px.y - 4.f);
			const ImU32 outline = IM_COL32(0, 0, 0, 190);
			dl->AddText(icon_font, render_icon_size, ImVec2(icon_pos.x - 1.f, icon_pos.y), outline, weapon_icon);
			dl->AddText(icon_font, render_icon_size, ImVec2(icon_pos.x + 1.f, icon_pos.y), outline, weapon_icon);
			dl->AddText(icon_font, render_icon_size, ImVec2(icon_pos.x, icon_pos.y - 1.f), outline, weapon_icon);
			dl->AddText(icon_font, render_icon_size, ImVec2(icon_pos.x, icon_pos.y + 1.f), outline, weapon_icon);
			dl->AddText(icon_font, render_icon_size, icon_pos, color, weapon_icon);
			return;
		}

		const float radius = (std::max)(3.f, size * 0.18f);
		const ImU32 outline = IM_COL32(0, 0, 0, 180);
		dl->AddCircleFilled(screen, radius + 1.f, outline, 12);
		dl->AddCircleFilled(screen, radius, color, 12);
	}

	static DWORD read_weapon_model_hash(DWORD weapon_hash) {
		__try {
			return static_cast<DWORD>(native::weapon::get_weapontype_model(weapon_hash));
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return 0;
		}
	}

	inline bool g_world_weapon_model_cache_ready = false;
	inline std::map<DWORD, DWORD> g_world_model_to_weapon_hash;

	static DWORD world_weapon_hash_from_model(DWORD model_hash) {
		if (model_hash == 0) return 0;

		if (!g_world_weapon_model_cache_ready) {
			std::map<DWORD, DWORD> next_model_to_weapon_hash;
			for (const auto& entry : k_weapon_icon_entries) {
				if (entry.hash == 0 || entry.hash == 0xA2719263u) continue;

				const DWORD model = read_weapon_model_hash(entry.hash);
				if (model != 0) {
					next_model_to_weapon_hash[model] = entry.hash;
				}
			}

			if (!next_model_to_weapon_hash.empty()) {
				g_world_model_to_weapon_hash = std::move(next_model_to_weapon_hash);
				g_world_weapon_model_cache_ready = true;
			}
		}

		const auto it = g_world_model_to_weapon_hash.find(model_hash);
		return it != g_world_model_to_weapon_hash.end() ? it->second : 0;
	}

	void draw_world_esp() {
		if (!config::get("visual", "world_esp", 0)) return;
		if (!IsValidPtr(Game.ReplayInterface) || !IsValidPtr(local.player)) return;

		CObjectInterface* objIface = Game.ReplayInterface->object_interface;
		CPickupInterface* pickupIface = Game.ReplayInterface->pickup_interface;
		const bool has_pickups = IsValidPtr(pickupIface) && IsValidPtr(pickupIface->pCPickupList);
		if (!IsValidPtr(objIface) && !has_pickups) return;

		const float max_dist = std::clamp(config::get("visual", "world_esp_dist", 300.f), 1.f, 300.f);
		const bool show_text = config::get("visual", "world_esp_text", 1) != 0;
		const bool show_icon = config::get("visual", "world_esp_icon", 1) != 0;
		const bool show_dist = config::get("visual", "world_esp_show_dist", 1) != 0;
		const bool ignore_local_held_objects = config::get("visual", "world_esp_ignore_local_held_objects", 1) != 0;
		const float icon_size = config::get("visual", "world_esp_icon_size", 28.f);
		const visual_config::rgba fallback_color{
			config::get("visual", "world_esp_color_r", 1.f),
			config::get("visual", "world_esp_color_g", 1.f),
			config::get("visual", "world_esp_color_b", 1.f),
			config::get("visual", "world_esp_color_a", 1.f)
		};
		const std::map<DWORD, std::string> custom_names = config::get("visual", "object_hash_names", std::map<DWORD, std::string>{});
		const std::map<DWORD, std::string> custom_colors = config::get("visual", "object_hash_colors", std::map<DWORD, std::string>{});

		if (!show_text && !show_icon) return;

		const Vector3 local_pos = local.player->fPosition;
		std::vector<Vector3> player_weapon_filter_positions;
		if (ignore_local_held_objects) {
			collect_world_weapon_player_positions(player_weapon_filter_positions);
		}
		ImDrawList* dl = ImGui::GetBackgroundDrawList();

		auto label_for_hash = [&](DWORD display_hash, DWORD weapon_hash) {
			const auto name_it = custom_names.find(display_hash);
			if (name_it != custom_names.end() && !name_it->second.empty()) {
				return name_it->second;
			}

			if (weapon_hash != 0) {
				const auto weapon_name_it = custom_names.find(weapon_hash);
				if (weapon_name_it != custom_names.end() && !weapon_name_it->second.empty()) {
					return weapon_name_it->second;
				}

				const char* weapon_title = get_weapon_icon_title(weapon_hash);
				if (weapon_title && weapon_title[0]) {
					return std::string(weapon_title);
				}
			}

			char hash_buf[32];
			sprintf_s(hash_buf, "0x%X", display_hash);
			return std::string(hash_buf);
		};

		auto color_for_hash = [&](DWORD display_hash, DWORD weapon_hash) {
			if (custom_colors.find(display_hash) != custom_colors.end()) {
				return visual_config::to_u32(visual_config::map_color(custom_colors, display_hash, fallback_color));
			}
			if (weapon_hash != 0 && custom_colors.find(weapon_hash) != custom_colors.end()) {
				return visual_config::to_u32(visual_config::map_color(custom_colors, weapon_hash, fallback_color));
			}
			return visual_config::to_u32(fallback_color);
		};

		auto render_world_entry = [&](DWORD display_hash, DWORD weapon_hash, const Vector3& pos, float dist) {
			if (display_hash == 0) return;

			ImVec2 screen;
			if (!WorldToScreen(pos, &screen) || !screen_point_visible(screen)) {
				return;
			}

			const ImU32 color = color_for_hash(display_hash, weapon_hash);
			if (show_icon) {
				draw_world_pickup_icon(dl, screen, weapon_hash, color, icon_size);
			}

			if (show_text) {
				std::string label = label_for_hash(display_hash, weapon_hash);
				if (show_dist) {
					label += " [" + std::to_string((int)(dist + 0.5f)) + "m]";
				}
				const float text_y = screen.y + (show_icon ? 5.f : 0.f);
				renderer.RenderText(label.c_str(), ImVec2(screen.x, text_y), 13.f, rgba_from_u32(color), true, true);
			}
		};

		if (IsValidPtr(objIface)) {
			const int max_objects = safe_object_count(objIface);
			for (int i = 0; i < max_objects; ++i) {
				Vector3 pos;
				DWORD model_hash = 0;
				int handle = 0;
				if (!read_object_hash_entry(objIface, i, pos, model_hash, handle)) continue;

				const DWORD weapon_hash = world_weapon_hash_from_model(model_hash);
				if (weapon_hash == 0) continue;
				if (ignore_local_held_objects && is_world_weapon_carried_by_player(pos, player_weapon_filter_positions)) {
					continue;
				}

				const float dist = get_distance(local_pos, pos);
				if (dist > max_dist) {
					continue;
				}

				render_world_entry(model_hash, weapon_hash, pos, dist);
			}
		}

		if (has_pickups) {
			const int max_pickups = safe_pickup_count(pickupIface);
			for (int i = 0; i < max_pickups; ++i) {
				Vector3 pos;
				int handle = 0;
				if (!read_pickup_entry(pickupIface, i, pos, handle)) continue;
				if (pos.x == 0.f && pos.y == 0.f && pos.z == 0.f) continue;

				const float dist = get_distance(local_pos, pos);
				if (dist > max_dist) continue;

				DWORD pickup_hash = 0;
				DWORD weapon_hash = 0;
				read_pickup_hashes(handle, pickup_hash, weapon_hash);
				const DWORD display_hash = weapon_hash != 0 ? weapon_hash : pickup_hash;
				if (display_hash == 0) continue;
				if (ignore_local_held_objects && is_world_weapon_carried_by_player(pos, player_weapon_filter_positions)) {
					continue;
				}

				render_world_entry(display_hash, weapon_hash, pos, dist);
			}
		}

	}

	void draw_pickup_esp() {
		if (!IsValidPtr(Game.ReplayInterface)) return;
		CObjectInterface* objIface = Game.ReplayInterface->object_interface;
		CPickupInterface* pickupIface = Game.ReplayInterface->pickup_interface;
		if (!IsValidPtr(objIface) && !IsValidPtr(pickupIface)) return;

		float max_dist = config::get("visual", "pickup_esp_dist", 150.f);
		float col_r = config::get("visual", "pickup_color_r", 1.f);
		float col_g = config::get("visual", "pickup_color_g", 1.f);
		float col_b = config::get("visual", "pickup_color_b", 1.f);
		bool show_dist = config::get("visual", "pickup_show_dist", 1);
		bool show_all = config::get("visual", "pickup_show_all", 0);
		bool ignore_local_held_objects = config::get("visual", "pickup_ignore_local_held_objects", 0);
		const std::map<DWORD, std::string> named_hashes = config::get("visual", "object_hash_names", std::map<DWORD, std::string>{});
		const std::map<DWORD, std::string> hash_colors = config::get("visual", "object_hash_colors", std::map<DWORD, std::string>{});
		const std::map<DWORD, std::string> hash_lines = config::get("visual", "object_hash_lines", std::map<DWORD, std::string>{});
		const std::map<DWORD, std::string> hash_boxes = config::get("visual", "object_hash_boxes", std::map<DWORD, std::string>{});
		const visual_config::rgba fallback_color{ col_r, col_g, col_b, 1.f };

		if (!IsValidPtr(local.player)) return;
		Vector3 localPos = local.player->fPosition;
		std::vector<Vector3> local_weapon_filter_positions;
		if (ignore_local_held_objects) {
			local_weapon_filter_positions.push_back(localPos);
		}

		auto display_name_for_hash = [&](DWORD hash) -> std::string {
			const auto it = named_hashes.find(hash);
			return it != named_hashes.end() ? it->second : std::string{};
		};

		auto hash_line_enabled = [&](DWORD hash) {
			const auto it = hash_lines.find(hash);
			return it != hash_lines.end() && it->second == "1";
		};

		auto hash_box_enabled = [&](DWORD hash) {
			const auto it = hash_boxes.find(hash);
			return it != hash_boxes.end() && it->second == "1";
		};

		auto hash_styled = [&](DWORD hash) {
			return !display_name_for_hash(hash).empty() ||
				hash_colors.find(hash) != hash_colors.end() ||
				hash_line_enabled(hash) ||
				hash_box_enabled(hash);
		};

		auto is_close_local_added_hash = [&](DWORD hash, float dist) {
			return ignore_local_held_objects && dist <= 1.0f && hash_styled(hash);
		};

		auto render_hash_entry = [&](DWORD hash, const Vector3& pos, float dist, float box_scale) {
			if (hash == 0) return;

			ImVec2 screen;
			if (!WorldToScreen(pos, &screen)) return;

			const std::string display_name = display_name_for_hash(hash);
			const bool has_name = !display_name.empty();
			const bool has_custom_color = hash_colors.find(hash) != hash_colors.end();
			const bool has_line = hash_line_enabled(hash);
			const bool has_box = hash_box_enabled(hash);
			const bool has_style = has_name || has_custom_color || has_line || has_box;
			const visual_config::rgba style_color = has_custom_color
				? visual_config::map_color(hash_colors, hash, fallback_color)
				: fallback_color;
			const RGBA pickup_col = rgba_from_u32(visual_config::to_u32(style_color));

			if (has_style) {
				if (has_box) {
					draw_pickup_hash_box(pos, box_scale, pickup_col);
				}
				if (has_line) {
					draw_pickup_hash_line(screen, pickup_col);
				}
			}

			if (!show_all && !has_style) {
				return;
			}

			char hash_buf[32];
			sprintf_s(hash_buf, "0x%X", hash);
			std::string str = has_name ? display_name : hash_buf;
			if (show_dist) {
				str += "  [" + std::to_string((int)dist) + "m]";
			}
			renderer.RenderText(str.c_str(), ImVec2(screen.x, screen.y), 14, pickup_col, true, true);
		};

		if (IsValidPtr(objIface)) {
			const int maxObj = safe_object_count(objIface);
			for (int i = 0; i < maxObj; i++) {
				Vector3 pos;
				DWORD hash = 0;
				int handle = 0;
				if (!read_object_hash_entry(objIface, i, pos, hash, handle)) continue;
				if (ignore_local_held_objects && is_world_weapon_carried_by_player(pos, local_weapon_filter_positions)) continue;

				float dist = get_distance(localPos, pos);
				if (dist > max_dist) continue;
				if (is_close_local_added_hash(hash, dist)) continue;

				::object_hash_registry::observe(hash);
				if (!show_all && !hash_styled(hash)) continue;
				render_hash_entry(hash, pos, dist, 0.75f);
			}
		}

		if (IsValidPtr(pickupIface) && IsValidPtr(pickupIface->pCPickupList)) {
			const int max_pickups = safe_pickup_count(pickupIface);
			for (int i = 0; i < max_pickups; ++i) {
				Vector3 pos;
				int handle = 0;
				if (!read_pickup_entry(pickupIface, i, pos, handle)) continue;
				if (ignore_local_held_objects && is_world_weapon_carried_by_player(pos, local_weapon_filter_positions)) continue;

				if (pos.x == 0.f && pos.y == 0.f && pos.z == 0.f) continue;
				const float dist = get_distance(localPos, pos);
				if (dist > max_dist) continue;

				DWORD pickup_hash = 0;
				DWORD weapon_hash = 0;
				read_pickup_hashes(handle, pickup_hash, weapon_hash);
				::object_hash_registry::observe(pickup_hash);
				::object_hash_registry::observe(weapon_hash);

				DWORD display_hash = 0;
				if (weapon_hash != 0 && hash_styled(weapon_hash)) {
					display_hash = weapon_hash;
				}
				else if (pickup_hash != 0 && hash_styled(pickup_hash)) {
					display_hash = pickup_hash;
				}
				else if (show_all) {
					display_hash = weapon_hash != 0 ? weapon_hash : pickup_hash;
				}
				if (is_close_local_added_hash(display_hash, dist)) continue;

				render_hash_entry(display_hash, pos, dist, 0.75f);
			}
		}
	}
