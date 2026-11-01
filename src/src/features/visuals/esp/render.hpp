inline const char* render_stage = "start";
inline bool render_failed = false;

void render_impl() {
	runtime_debug::last_section = "esp_render";

	static const bind_state::bind_config k_visual_esp_bind {
		"visual",
		"enable",
		"esp_key",
		nullptr,
		"esp_key_mode",
		nullptr
	};
	static const bind_state::bind_config k_pickup_esp_bind {
		"visual",
		"pickup_esp",
		"pickup_esp_key",
		nullptr,
		"pickup_esp_key_mode",
		nullptr
	};

	render_stage = "visual bind";
	if (bind_state::is_active_or_enabled_when_unbound(k_visual_esp_bind)) {
		render_stage = "player";
		run_section("player", draw_player_esp);
	}

	render_stage = "radar config";
	if (config::get("visual", "radar", 0) && ws_server::has_resolved_server_id()) {
		render_stage = "radar";
		run_section("radar", radar);
	}

	render_stage = "weapon_arrows";
	run_section("weapon_arrows", draw_weapon_arrows);

	render_stage = "world";
	run_section("world", draw_world_esp);

	render_stage = "pickup bind";
	::object_hash_registry::clear_visible();
	if (bind_state::is_active_or_enabled_when_unbound(k_pickup_esp_bind)) {
		render_stage = "pickup";
		run_section("pickup", draw_pickup_esp);
	}

	render_stage = "waypoints config";
	if (config::get("misc", "waypoints_enable", 0) && config::get("misc", "waypoints_draw", 0)) {
		render_stage = "waypoints";
		run_section("waypoints", draw_waypoints);
	}

	render_stage = "done";
	runtime_debug::last_section = "esp_render_done";
}

void render() {
	if (render_failed) return;

	try {
		render_impl();
	} catch (const std::exception& e) {
		HELLO_RUNTIME_LOG("esp render exception at %s: %s", render_stage, e.what());
		render_failed = true;
	} catch (...) {
		HELLO_RUNTIME_LOG("esp render exception at %s", render_stage);
		render_failed = true;
	}
}
