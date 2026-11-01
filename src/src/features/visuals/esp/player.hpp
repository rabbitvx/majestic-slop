	inline ImFont* get_esp_font() {
		if (renderer.espFont) return renderer.espFont;
		if (renderer.hudFont) return renderer.hudFont;
		if (ImGui::GetFont()) return ImGui::GetFont();
		return nullptr;
	}

	inline ImFont* get_esp_small_font() {
		if (renderer.smallFont) return renderer.smallFont;
		return get_esp_font();
	}

	inline ImFont* get_esp_weapon_icon_font(float desired_size, float* render_size) {
		return renderer.EspWeaponIconFont(desired_size, render_size);
	}

	inline float get_esp_info_scale(float box_height, float base_text_height) {
		const float height_difference = box_height - base_text_height;
		float scale = box_height > 0.f ? height_difference / box_height : 1.f;
		if (scale > 1.f) scale = 1.f;
		if (scale < 0.60f) scale = 0.60f;
		return scale;
	}

	inline float get_esp_distance_scale(float distance) {
		if (!std::isfinite(distance)) return 1.f;
		if (distance <= 35.f) return 1.f;
		if (distance >= 250.f) return 0.55f;

		const float t = (distance - 35.f) / (250.f - 35.f);
		return 1.f - (0.45f * t);
	}

	inline float get_bottom_health_bar_width(float box_width, float distance, float label_size) {
		const float scaled_width = box_width * get_esp_distance_scale(distance);
		const float min_width = 24.f;
		const float max_width = label_size * 6.f;
		return std::clamp(scaled_width, min_width, max_width);
	}

	struct weapon_name_entry {
		DWORD hash;
		const char* name;
	};

	inline const char* get_readable_weapon_name(DWORD hash) {
		static constexpr weapon_name_entry names[] = {
			{0x92A27487, "Кинжал"}, {0x958A4A8F, "Бита"}, {0xF9E6AA4B, "Бутылка"},
			{0x84BD7BFD, "Лом"}, {0xA2719263, "Без оружия"}, {0x8BB05FD7, "Фонарик"},
			{0x440E4788, "Клюшка для гольфа"}, {0x4E875F73, "Молоток"}, {0xF9DCBF2D, "Топор"},
			{0xD8DF3C3C, "Кастет"}, {0x99B507EA, "Нож"}, {0xDD5DF8D9, "Мачете"},
			{0xDFE37640, "Складной нож"}, {0x678B81B1, "Полицейская дубинка"}, {0x19044EE0, "Ключ"},
			{0xCD274149, "Боевой топор"}, {0x94117305, "Кий"},
			{0x1B06D571, "Пистолет"}, {0xBFE256D4, "Пистолет Mk2"}, {0x5EF9FEC4, "Боевой пистолет"},
			{0x22D8FE39, "Бронебойный пистолет"}, {0x3656C8C1, "Тазер"}, {0x99AEEB3B, "Пистолет .50"},
			{0xBFD21232, "Карманный пистолет"}, {0x88374054, "Карманный пистолет Mk2"}, {0xD205520E, "Тяжелый пистолет"},
			{0x083839C4, "Винтажный пистолет"}, {0x47757124, "Сигнальный пистолет"}, {0xDC4DB296, "Пистолет Marksman"},
			{0xC1B3C3D1, "Револьвер"}, {0xCB96392F, "Револьвер Mk2"}, {0x97EA20B8, "Наградной револьвер"},
			{0x917F6C8C, "Военно-морской револьвер"}, {0x57A4368C, "Пистолет Перико"},
			{0x13532244, "Микро ПП"}, {0x2BE6766B, "ПП"}, {0x78A97CD0, "ПП Mk2"},
			{0xEFE7E2DF, "Штурмовой ПП"}, {0x0A3D4D34, "Боевой ПОС"}, {0xDB1AA450, "Тактический ПП"},
			{0xBD248B55, "Мини ПП"},
			{0x1D073A89, "Помповый дробовик"}, {0x555AF99A, "Помповый дробовик Mk2"},
			{0x7846A318, "Короткий дробовик"}, {0xE284C527, "Штурмовой дробовик"},
			{0x9D61E50F, "Дробовик Буллпап"}, {0xA89CB99E, "Мушкет"},
			{0x3AABBBAA, "Тяжелый дробовик"}, {0xEF951FBB, "Двуствольный дробовик"}, {0x12E82D3D, "Автоматический дробовик"},
			{0x5A96BA4, "Боевой дробовик"},
			{0xBFEFFF6D, "Штурмовая винтовка"}, {0x394F415C, "Штурмовая винтовка Mk2"},
			{0x83BF0278, "Карабинная винтовка"}, {0xFAD1F1C9, "Карабинная винтовка Mk2"},
			{0xAF113F99, "Улучшенная винтовка"}, {0xC0A3098D, "Специальный карабин"},
			{0x969C3D67, "Специальный карабин Mk2"}, {0x7F229F94, "Винтовка Буллпап"},
			{0x84D6FAFD, "Винтовка Буллпап Mk2"}, {0x624FE830, "Компактная винтовка"},
			{0x9D1F17E6, "Военная винтовка"}, {0x84EA1D5E, "Тяжёлая винтовка"},
			{0xD1D5F52B, "Тактическая винтовка"}, {0x6E7DDDEC, "Прецизионная винтовка"},
			{0x9D07F764, "Ручной пулемет"}, {0x7FD62962, "Пулемёт Калашникова"}, {0xDBBD7280, "Ручной пулемет Mk2"},
			{0x61012683, "Пистолет-пулемёт Томпсона"},
			{0x05FC3C11, "Снайперская винтовка"}, {0x0C472FE2, "Тяжелая снайперская винтовка"},
			{0x0A914799, "Тяжелая снайперская винтовка Mk II"}, {0xC734385A, "Винтовка Marksman"},
			{0x6A6C02E0, "Винтовка Marksman Mk2"},
			{0xB1CA77B1, "РПГ"}, {0xA284510B, "Гранатомет"},
			{0x4DD2DC56, "Дымовой гранатомет"}, {0x42BF8A85, "Миниган"},
			{0x7F7497E5, "Фейерверк"}, {0x6D544C99, "Рельсотрон"},
			{0x63AB0442, "Самонаводящаяся РПГ"}, {0x0781FE4A, "Компактный гранатомет"},
			{0x93E220BD, "Граната"}, {0xA0973D5E, "Слезоточивый газ"}, {0xFDBC8A50, "Дымовая граната"},
			{0x497FACC3, "Фальшфейер"}, {0x24B17070, "Коктейль Молотова"}, {0x2C3731D9, "Липкая бомба"},
			{0xAB564B93, "Мина"}, {0x0787F0BB, "Снежок"},
			{0xBA45E8B8, "Самодельная бомба"}, {0x23C9F95C, "Мяч"},
			{0x34A67B97, "Канистра"}, {0x060EC506, "Огнетушитель"},
			{0xFBAB5776, "Парашют"},
			{0x3813FC08, "Древний топор"}, {0x86589186, "Карамельная трость"}, {0xDAC00025, "Электрическая дубинка"},
			{0xAF3696A1, "Атомайзер"}, {0x2B5EF5EC, "Керамический пистолет"}, {0x45CD9CF3, "Тазер"},
			{0x1BC4FDB9, "Глок P80"}, {0xF7F1E25E, "Кислотный пакет"}, {0x476BF155, "Адская пушка"},
			{0x14E56510, "Тактический ПП"}, {0xC78D71B4, "Тяжёлая винтовка"}, {0x6D544C85, "Рельсотрон"},
			{0xB62D1F67, "Вдоводел"}, {0xDB26713A, "Компактный ЭМИ-гранатомет"}, {0xFEA23564, "Рельсотрон"},
			{0xBA536372, "Канистра с опасным веществом"}, {0x184140A1, "Канистра удобрений"},
		};
		for (const auto& entry : names) {
			if (entry.hash == hash) return entry.name;
		}

		static thread_local std::string custom_weapon_name;
		custom_weapon_name = ::weapons_highlight::label(hash);
		if (!custom_weapon_name.empty()) return custom_weapon_name.c_str();

		return nullptr;
	}

	inline const char* get_weapon_display_name(DWORD hash) {
		const int language = std::clamp(config::get("visual", "esp_weapon_text_language", 1), 0, 1);
		if (language == 0) {
			const char* english_name = get_weapon_icon_title(hash);
			if (english_name && english_name[0]) return english_name;
		}

		return get_readable_weapon_name(hash);
	}


	namespace weapon_reader {
		inline int get_entity_handle_safe(CObject* ped) {
			if (!IsValidPtr(ped))
				return 0;

			__try {
				if (!pointer_to_handle)
					return 0;

				return pointer_to_handle(static_cast<intptr_t>(reinterpret_cast<uintptr_t>(ped)));
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				return 0;
			}
		}

		inline DWORD get_native_weapon_hash(CObject* ped) {
			const int entity_handle = get_entity_handle_safe(ped);
			if (entity_handle == 0)
				return 0;

			__try {
				native::type::hash weapon_hash = 0;
				if (!native::weapon::get_current_ped_weapon(entity_handle, &weapon_hash, true))
					return 0;

				return static_cast<DWORD>(weapon_hash);
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				return 0;
			}
		}

		inline DWORD get_weapon_hash(CObject* ped) {
			if (!IsValidPtr(ped))
				return 0;

			DWORD result = 0;
			__try {
				CWeaponManager* wep = ped->weapon();
				if (!IsValidPtr(wep))
					return 0;

				if (!IsValidPtr(wep->_WeaponInfo))
					return 0;

				uintptr_t info_ptr = reinterpret_cast<uintptr_t>(wep->_WeaponInfo);
				if (info_ptr > 0x10000 && info_ptr < 0x7FFFFFFFFFFF) {
					result = *(DWORD*)(info_ptr + 0x10);
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				return 0;
			}
			return result;
		}

		inline DWORD get_weapon_model_hash_safe(CObject* ped) {
			if (!IsValidPtr(ped))
				return 0;

			DWORD result = 0;
			__try {
				CWeaponManager* wep = ped->weapon();
				if (!IsValidPtr(wep))
					return 0;

				if (!IsValidPtr(wep->_CurrentWeapon))
					return 0;

				CWeapon* current_weapon = reinterpret_cast<CWeapon*>(wep->_CurrentWeapon);
				if (!IsValidPtr(current_weapon))
					return 0;

				result = current_weapon->dwModelHash;
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				return 0;
			}
			return result;
		}
		
		inline bool get_weapon_name_safe(CObject* ped, char* out_buf, int buf_size) {
			if (!IsValidPtr(ped) || !out_buf || buf_size <= 0)
				return false;
			
			out_buf[0] = '\0';
			
			__try {
				CWeaponManager* wep = ped->weapon();
				if (!IsValidPtr(wep))
					return false;
				
				if (!IsValidPtr(wep->_WeaponInfo))
					return false;
				
				const char* str = wep->_WeaponInfo->GetSzWeaponName();
				if (!str)
					return false;
				
				uintptr_t str_ptr = reinterpret_cast<uintptr_t>(str);
				if (str_ptr < 0x10000 || str_ptr > 0x7FFFFFFFFFFF)
					return false;
				
				for (int i = 0; i < buf_size - 1 && str[i] != '\0'; i++) {
					out_buf[i] = str[i];
					out_buf[i + 1] = '\0';
				}
				return true;
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				out_buf[0] = '\0';
				return false;
			}
		}
		
		inline DWORD get_model_hash_safe(CObject* ped) {
			if (!IsValidPtr(ped))
				return 0;
			
			DWORD result = 0;
			__try {
				CModelInfo* model_info = ped->ModelInfo();
				if (!IsValidPtr(model_info))
					return 0;
				result = model_info->GetHash();
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				return 0;
			}
			return result;
		}
	}
	

	namespace utils {
		ImVec2 WorldToRadar(Vector3 Location, FLOAT RadarX, FLOAT RadarY, float RadarSize, float RadarZoom) {
			Vector2 Coord(0, 0);
			ImVec2 Return(0, 0);
			CPlayerAngles* cam = Game.getCam();
			if(IsValidPtr(cam) && IsValidPtr(local.player)) {

				float rot = acosf(cam->m_fps_angles.x) * 180.0f / PI;
				if(asinf(cam->m_fps_angles.y) * 180.0f / PI < 0.0f) rot *= -1.0f;

				Vector2 forwardVec(0, 0);
				float ForwardDirection = DirectX::XMConvertToRadians(rot);
				forwardVec.x = cos(ForwardDirection);
				forwardVec.y = sin(ForwardDirection);

				FLOAT CosYaw = forwardVec.x;
				FLOAT SinYaw = forwardVec.y;

				FLOAT DeltaX = Location.x - local.player->fPosition.x;
				FLOAT DeltaY = Location.y - local.player->fPosition.y;

				FLOAT LocationX = (DeltaY * CosYaw - DeltaX * SinYaw) / RadarZoom;
				FLOAT LocationY = (DeltaX * CosYaw + DeltaY * SinYaw) / RadarZoom;

				if(LocationX > RadarSize / 2.0f - 2.5f)
					LocationX = RadarSize / 2.0f - 2.5f;
				else if(LocationX < -(RadarSize / 2.0f - 2.5f))
					LocationX = -(RadarSize / 2.0f - 2.5f);

				if(LocationY > RadarSize / 2.0f - 2.5f)
					LocationY = RadarSize / 2.0f - 2.5f;
				else if(LocationY < -(RadarSize / 2.0f - 2.5f))
					LocationY = -(RadarSize / 2.0f - 2.5f);

				Return.x = -LocationX + RadarX;
				Return.y = -LocationY + RadarY;

			}

			return Return;
		}

		inline ImU32 get_dynamic_health_color(float ratio, float alpha = 1.f) {
			const float clamped_ratio = ratio < 0.f ? 0.f : (ratio > 1.f ? 1.f : ratio);
			const float t = clamped_ratio * clamped_ratio * (3.f - 2.f * clamped_ratio);
			const float clamped_alpha = alpha < 0.f ? 0.f : (alpha > 1.f ? 1.f : alpha);
			const float hue = 0.31f * t;
			const float saturation = 0.86f - (0.14f * t);
			const float value = 0.96f;

			float r = 0.f;
			float g = 0.f;
			float b = 0.f;
			ImGui::ColorConvertHSVtoRGB(hue, saturation, value, r, g, b);
			return IM_COL32((int)(r * 255.f), (int)(g * 255.f), (int)(b * 255.f), (int)(clamped_alpha * 255.f));
		}

		void draw_hp_bar(float x, float y, float w, float h, float health, float max, bool armor, bool horizontal = false) {
			if(!max) return;
			if(health < 0) health = 0;

			float ratio = health / max;
			if (ratio < 0.f) ratio = 0.f;
			if (ratio > 1.f) ratio = 1.f;

			auto* dl = ImGui::GetBackgroundDrawList();
			const ImU32 outline_col = IM_COL32(0, 0, 0, 165);
			const ImU32 bg_col = IM_COL32(0, 0, 0, 72);
			const float outline_pad = 0.5f;
			const float outline_thickness = 0.75f;

			ImU32 bar_col;
			if (armor) {
				float cr = config::get("visual", "armorbar_color_r", 0.18f);
				float cg = config::get("visual", "armorbar_color_g", 0.69f);
				float cb = config::get("visual", "armorbar_color_b", 0.9f);
				float ca = config::get("visual", "armorbar_color_a", 1.f);
				bar_col = IM_COL32((int)(cr*255), (int)(cg*255), (int)(cb*255), (int)(ca*255));
			} else {
				float cr = config::get("visual", "healthbar_color_r", -1.f);
				if (cr < 0.f) {
					bar_col = get_dynamic_health_color(ratio);
				} else {
					float cg = config::get("visual", "healthbar_color_g", 0.9f);
					float cb = config::get("visual", "healthbar_color_b", 0.18f);
					float ca = config::get("visual", "healthbar_color_a", 1.f);
					bar_col = IM_COL32((int)(cr*255), (int)(cg*255), (int)(cb*255), (int)(ca*255));
				}
			}

			if (!horizontal) {
				float bar_w = w * ratio;
				dl->AddRect(ImVec2(x - outline_pad, y - outline_pad), ImVec2(x + w + outline_pad, y + h + outline_pad), outline_col, 0.f, 0, outline_thickness);
				dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), bg_col);
				dl->AddRectFilled(ImVec2(x, y), ImVec2(x + bar_w, y + h), bar_col);
			} else {
				float bar_width = 2.f;
				float bar_filled = y + h - (h * ratio);
				dl->AddRect(ImVec2(x - outline_pad, y - outline_pad), ImVec2(x + bar_width + outline_pad, y + h + outline_pad), outline_col, 0.f, 0, outline_thickness);
				dl->AddRectFilled(ImVec2(x, y), ImVec2(x + bar_width, y + h), bg_col);
				dl->AddRectFilled(ImVec2(x, bar_filled), ImVec2(x + bar_width, y + h), bar_col);

			}
		}

		inline float clamp_ratio(float value) {
			if (value < 0.f) return 0.f;
			if (value > 1.f) return 1.f;
			return value;
		}

		inline ImU32 apply_opacity(ImU32 color, float opacity) {
			const int alpha = (int)(((color >> IM_COL32_A_SHIFT) & 0xFF) * opacity);
			return (color & ~IM_COL32_A_MASK) | ((ImU32)alpha << IM_COL32_A_SHIFT);
		}

		inline RGBA apply_opacity(const RGBA& color, float opacity) {
			return RGBA(color.r, color.g, color.b, (int)(color.a * opacity));
		}

		void draw_bar_value_text(ImDrawList* dl, float bar_x, float bar_width, float bar_y1, float bar_y2, float filled_y, int value, float opacity = 1.f) {
			if (!dl || value >= 100) return;

			char value_buf[16];
			sprintf_s(value_buf, "%d", value);

			float bar_height = bar_y2 - bar_y1;
			float font_size_px = 10.f;

			ImFont* font = renderer.tahomaBoldFont ? renderer.tahomaBoldFont : get_esp_font();
			if (!font && ImGui::GetIO().Fonts && ImGui::GetIO().Fonts->Fonts.Size > 0)
				font = ImGui::GetIO().Fonts->Fonts[0];

			ImVec2 ts = font ? font->CalcTextSizeA(font_size_px, FLT_MAX, 0.f, value_buf) : ImGui::CalcTextSize(value_buf);
			float text_x = bar_x + bar_width * 0.5f - ts.x * 0.5f;
			float text_y = filled_y - ts.y * 0.5f;
			float min_y = bar_y1;
			float max_y = bar_y2 - ts.y;
			if (text_y < min_y) text_y = min_y;
			if (text_y > max_y) text_y = max_y;

			const ImU32 text_col = IM_COL32(255, 255, 255, (int)(255.f * opacity));
			const ImU32 outline_col = IM_COL32(0, 0, 0, (int)(185.f * opacity));
			const float outline_offset = 0.75f;
			if (font) {
				dl->AddText(font, font_size_px, ImVec2(text_x - outline_offset, text_y), outline_col, value_buf);
				dl->AddText(font, font_size_px, ImVec2(text_x + outline_offset, text_y), outline_col, value_buf);
				dl->AddText(font, font_size_px, ImVec2(text_x, text_y - outline_offset), outline_col, value_buf);
				dl->AddText(font, font_size_px, ImVec2(text_x, text_y + outline_offset), outline_col, value_buf);
				dl->AddText(font, font_size_px, ImVec2(text_x, text_y), text_col, value_buf);
			} else {
				dl->AddText(ImVec2(text_x - outline_offset, text_y), outline_col, value_buf);
				dl->AddText(ImVec2(text_x + outline_offset, text_y), outline_col, value_buf);
				dl->AddText(ImVec2(text_x, text_y - outline_offset), outline_col, value_buf);
				dl->AddText(ImVec2(text_x, text_y + outline_offset), outline_col, value_buf);
				dl->AddText(ImVec2(text_x, text_y), text_col, value_buf);
			}
		}

		void draw_horizontal_bar_value_text(ImDrawList* dl, float bar_x1, float bar_x2, float bar_y1, float bar_y2, float filled_x, int value, float opacity = 1.f) {
			if (!dl || value >= 100) return;

			char value_buf[16];
			sprintf_s(value_buf, "%d", value);

			float font_size_px = 10.f;
			ImFont* font = renderer.tahomaBoldFont ? renderer.tahomaBoldFont : get_esp_font();
			if (!font && ImGui::GetIO().Fonts && ImGui::GetIO().Fonts->Fonts.Size > 0)
				font = ImGui::GetIO().Fonts->Fonts[0];

			ImVec2 ts = font ? font->CalcTextSizeA(font_size_px, FLT_MAX, 0.f, value_buf) : ImGui::CalcTextSize(value_buf);
			float text_x = filled_x - ts.x * 0.5f;
			float min_x = bar_x1;
			float max_x = bar_x2 - ts.x;
			if (max_x < min_x) max_x = min_x;
			if (text_x < min_x) text_x = min_x;
			if (text_x > max_x) text_x = max_x;
			float text_y = bar_y1 + ((bar_y2 - bar_y1) - ts.y) * 0.5f;

			const ImU32 text_col = IM_COL32(255, 255, 255, (int)(255.f * opacity));
			const ImU32 outline_col = IM_COL32(0, 0, 0, (int)(185.f * opacity));
			const float outline_offset = 0.75f;
			if (font) {
				dl->AddText(font, font_size_px, ImVec2(text_x - outline_offset, text_y), outline_col, value_buf);
				dl->AddText(font, font_size_px, ImVec2(text_x + outline_offset, text_y), outline_col, value_buf);
				dl->AddText(font, font_size_px, ImVec2(text_x, text_y - outline_offset), outline_col, value_buf);
				dl->AddText(font, font_size_px, ImVec2(text_x, text_y + outline_offset), outline_col, value_buf);
				dl->AddText(font, font_size_px, ImVec2(text_x, text_y), text_col, value_buf);
			}
			else {
				dl->AddText(ImVec2(text_x - outline_offset, text_y), outline_col, value_buf);
				dl->AddText(ImVec2(text_x + outline_offset, text_y), outline_col, value_buf);
				dl->AddText(ImVec2(text_x, text_y - outline_offset), outline_col, value_buf);
				dl->AddText(ImVec2(text_x, text_y + outline_offset), outline_col, value_buf);
				dl->AddText(ImVec2(text_x, text_y), text_col, value_buf);
			}
		}

		void draw_vertical_status_bar(float bar_x, float bar_y1, float bar_y2, float ratio, ImU32 fill_color, bool show_value, float opacity = 1.f) {
			auto* dl = ImGui::GetBackgroundDrawList();
			if (!dl) return;

			const float bar_width = 1.5f;
			const float outline_pad = 0.5f;
			const ImU32 outline = IM_COL32(0, 0, 0, (int)(55.f * opacity));
			const ImU32 bg = IM_COL32(0, 0, 0, (int)(255.f * 0.28f * opacity));
			fill_color = apply_opacity(fill_color, opacity);
			bar_x = floorf(bar_x) + 0.5f;
			bar_y1 = floorf(bar_y1) + 0.5f;
			bar_y2 = floorf(bar_y2) + 0.5f;
			const float filled_y = floorf(bar_y2 - (bar_y2 - bar_y1) * ratio) + 0.5f;

			dl->AddRect(ImVec2(bar_x - outline_pad, bar_y1 - outline_pad), ImVec2(bar_x + bar_width + outline_pad, bar_y2 + outline_pad), outline, 0.f, 0, 0.5f);
			dl->AddRectFilled(ImVec2(bar_x, bar_y1), ImVec2(bar_x + bar_width, bar_y2), bg, 0.f, 0);
			dl->AddRectFilled(ImVec2(bar_x, filled_y), ImVec2(bar_x + bar_width, bar_y2), fill_color, 0.f, 0);

			if (show_value) {
				int value = (int)(ratio * 100.f + 0.5f);
				draw_bar_value_text(dl, bar_x, bar_width, bar_y1, bar_y2, filled_y, value, opacity);
			}
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

		void draw_horizontal_status_bar(float x, float y, float w, float h, float ratio, ImU32 fill_color, bool show_value, float opacity = 1.f) {
			auto* dl = ImGui::GetBackgroundDrawList();
			if (!dl || w <= 0.f || h <= 0.f) return;

			const float outline_pad = 0.5f;
			const ImU32 outline = IM_COL32(0, 0, 0, (int)(55.f * opacity));
			const ImU32 bg = IM_COL32(0, 0, 0, (int)(255.f * 0.28f * opacity));
			fill_color = apply_opacity(fill_color, opacity);

			const float x1 = floorf(x) + 0.5f;
			const float y1 = floorf(y) + 0.5f;
			const float x2 = floorf(x + w) + 0.5f;
			const float y2 = floorf(y + h) + 0.5f;
			const float filled_x = x1 + (x2 - x1) * clamp_ratio(ratio);

			dl->AddRect(ImVec2(x1 - outline_pad, y1 - outline_pad), ImVec2(x2 + outline_pad, y2 + outline_pad), outline, 0.f, 0, 0.5f);
			dl->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), bg, 0.f, 0);
			dl->AddRectFilled(ImVec2(x1, y1), ImVec2(filled_x, y2), fill_color, 0.f, 0);

			if (show_value) {
				int value = (int)(ratio * 100.f + 0.5f);
				draw_horizontal_bar_value_text(dl, x1, x2, y1, y2, filled_x, value, opacity);
			}
		}

		void draw_health_stack(const ImVec2& p0, const ImVec2& p1, float health, float max_health, float armor, bool show_values, float opacity = 1.f) {
			int mode = config::get("visual", "health_mode", config::get("visual", "draw_healthbar", 0) != 0 ? 1 : 0);
			if (mode <= 0 || max_health <= 0.f) return;

			float hp_ratio = clamp_ratio(health / max_health);
			float armor_ratio = clamp_ratio(armor / 100.f);
			float bar_y1 = p0.y;
			float bar_y2 = p1.y;
			const float bar_width = 1.5f;
			float bar_x = p0.x - bar_width - 2.f;

			ImU32 hp_color = get_health_status_color(hp_ratio);
			ImU32 armor_color = get_armor_status_color();
			if (mode == 1) {
				draw_vertical_status_bar(bar_x, bar_y1, bar_y2, hp_ratio, hp_color, show_values, opacity);
				return;
			}

			if (mode == 2) {
				draw_vertical_status_bar(bar_x, bar_y1, bar_y2, hp_ratio, hp_color, show_values, opacity);
				if (armor_ratio > 0.f)
					draw_vertical_status_bar(bar_x - bar_width - 2.f, bar_y1, bar_y2, armor_ratio, armor_color, show_values, opacity);
				return;
			}

			float use_ratio = armor_ratio > 0.f ? armor_ratio : hp_ratio;
			ImU32 fill_color = armor_ratio > 0.f ? armor_color : hp_color;
			draw_vertical_status_bar(bar_x, bar_y1, bar_y2, use_ratio, fill_color, show_values, opacity);
		}
	}

	inline int skeleton_line_clip_code(const ImVec2& point, const ImVec2& display_size) {
		int code = 0;
		if (point.x < 0.f) code |= 1;
		else if (point.x > display_size.x) code |= 2;
		if (point.y < 0.f) code |= 4;
		else if (point.y > display_size.y) code |= 8;
		return code;
	}

	inline bool clip_skeleton_line_to_screen(ImVec2& from, ImVec2& to) {
		const ImVec2 display_size = ImGui::GetIO().DisplaySize;
		if (display_size.x <= 1.f || display_size.y <= 1.f) return false;

		int from_code = skeleton_line_clip_code(from, display_size);
		int to_code = skeleton_line_clip_code(to, display_size);

		while (true) {
			if ((from_code | to_code) == 0) return true;
			if ((from_code & to_code) != 0) return false;

			const int clip_code = from_code != 0 ? from_code : to_code;
			float x = 0.f;
			float y = 0.f;

			if (clip_code & 8) {
				const float dy = to.y - from.y;
				if (fabsf(dy) < 0.001f) return false;
				x = from.x + (to.x - from.x) * (display_size.y - from.y) / dy;
				y = display_size.y;
			}
			else if (clip_code & 4) {
				const float dy = to.y - from.y;
				if (fabsf(dy) < 0.001f) return false;
				x = from.x + (to.x - from.x) * (0.f - from.y) / dy;
				y = 0.f;
			}
			else if (clip_code & 2) {
				const float dx = to.x - from.x;
				if (fabsf(dx) < 0.001f) return false;
				y = from.y + (to.y - from.y) * (display_size.x - from.x) / dx;
				x = display_size.x;
			}
			else {
				const float dx = to.x - from.x;
				if (fabsf(dx) < 0.001f) return false;
				y = from.y + (to.y - from.y) * (0.f - from.x) / dx;
				x = 0.f;
			}

			if (clip_code == from_code) {
				from = ImVec2(x, y);
				from_code = skeleton_line_clip_code(from, display_size);
			}
			else {
				to = ImVec2(x, y);
				to_code = skeleton_line_clip_code(to, display_size);
			}
		}
	}

	void drawBones(PlayerBones bones, RGBA current_color, float thickness, const WorldToScreenSnapshot* view = nullptr) {
		const float line_thickness = std::clamp(thickness, 0.1f, 1.f);
		ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
		if (!draw_list) return;

		Vector3 bone_positions[][2] = {
			{ bones.NECK, bones.RIGHT_CLAVICLE },
			{ bones.RIGHT_CLAVICLE, bones.RIGHT_UPPER_ARM },
			{ bones.RIGHT_UPPER_ARM, bones.RIGHT_FOREARM },
			{ bones.RIGHT_FOREARM, bones.RIGHT_HAND },

			{ bones.NECK, bones.LEFT_CLAVICLE },
			{ bones.LEFT_CLAVICLE, bones.LEFT_UPPER_ARM },
			{ bones.LEFT_UPPER_ARM, bones.LEFT_FOREARM },
			{ bones.LEFT_FOREARM, bones.LEFT_HAND },

			{ bones.NECK, bones.SPINE3 },
			{ bones.SPINE3, bones.SPINE2 },
			{ bones.SPINE2, bones.SPINE1 },
			{ bones.SPINE1, bones.SPINE_ROOT },

			{ bones.SPINE_ROOT, bones.RIGHT_THIGH },
			{ bones.RIGHT_THIGH ,bones.RIGHT_CALF},
			{ bones.RIGHT_CALF,bones.RIGHT_FOOT },

			{ bones.SPINE_ROOT, bones.LEFT_THIGH },
			{ bones.LEFT_THIGH,bones.LEFT_CALF },
			{ bones.LEFT_CALF,bones.LEFT_FOOT },

		};

		int arrSize = sizeof(bone_positions) / sizeof(bone_positions[0]);

		for(size_t i = 0; i < arrSize; i++) {
			ImVec2 from_point;
			ImVec2 to_point;
			const Vector3& from = bone_positions[i][0];
			const Vector3& to = bone_positions[i][1];
			if ((from.x == 0.f && from.y == 0.f && from.z == 0.f) || (to.x == 0.f && to.y == 0.f && to.z == 0.f)) continue;
			const bool from_ok = view ? WorldToScreenMatrix(*view, from, &from_point) : WorldToScreenMatrix(from, &from_point);
			const bool to_ok = view ? WorldToScreenMatrix(*view, to, &to_point) : WorldToScreenMatrix(to, &to_point);
			if (!from_ok || !to_ok) continue;
			if (!clip_skeleton_line_to_screen(from_point, to_point)) continue;

			draw_list->AddLine(from_point, to_point, IM_COL32(current_color.r, current_color.g, current_color.b, current_color.a), line_thickness);
		}

		

	}

	inline ImU32 get_default_arrow_color_u32() {
		const float r = std::clamp(config::get("visual", "arrFovCol_r", 0.f), 0.f, 1.f);
		const float g = std::clamp(config::get("visual", "arrFovCol_g", 1.f), 0.f, 1.f);
		const float b = std::clamp(config::get("visual", "arrFovCol_b", 0.f), 0.f, 1.f);
		const float a = std::clamp(config::get("visual", "arrFovCol_a", 1.f), 0.f, 1.f);
		return IM_COL32((int)(r * 255.f), (int)(g * 255.f), (int)(b * 255.f), (int)(a * 255.f));
	}

	void drawArrow(ImVec2 center, float angle, float radius, float size, ImU32 color) {
		const int r = (color >> IM_COL32_R_SHIFT) & 0xFF;
		const int g = (color >> IM_COL32_G_SHIFT) & 0xFF;
		const int b = (color >> IM_COL32_B_SHIFT) & 0xFF;
		const int a = (color >> IM_COL32_A_SHIFT) & 0xFF;

		auto* draw_list = ImGui::GetBackgroundDrawList();
		ImColor arrow_color(r, g, b, a);
		ImColor outline_color(0, 0, 0, (a > 160) ? a : 160);

		const float width = 0.05f;
		const float inner_cut = 0.85f;

		ImVec2 p1 = { center.x + cosf(angle) * radius, center.y + sinf(angle) * radius };
		ImVec2 p2 = { center.x + cosf(angle - width) * (radius - size), center.y + sinf(angle - width) * (radius - size) };
		ImVec2 p3 = { center.x + cosf(angle + width) * (radius - size), center.y + sinf(angle + width) * (radius - size) };
		ImVec2 p4 = { center.x + cosf(angle) * (radius - size * inner_cut), center.y + sinf(angle) * (radius - size * inner_cut) };

		draw_list->AddTriangleFilled(p1, p2, p4, arrow_color);
		draw_list->AddTriangleFilled(p1, p3, p4, arrow_color);

		draw_list->AddLine(p1, p2, outline_color, 0.8f);
		draw_list->AddLine(p2, p4, outline_color, 0.8f);
		draw_list->AddLine(p4, p3, outline_color, 0.8f);
		draw_list->AddLine(p3, p1, outline_color, 0.8f);
	}

	void drawBracketBox(const ImVec2& p0, const ImVec2& p1, float thickness, float opacity = 1.f) {
		float len = (p1.x - p0.x) * 0.28f;
		if (len < 7.0f) len = 7.0f;
		if (len > 16.0f) len = 16.0f;

		float lenh = (p1.y - p0.y) * 0.18f;
		if (lenh < 9.0f) lenh = 9.0f;
		if (lenh > 20.0f) lenh = 20.0f;

		float th = thickness < 1.0f ? 1.0f : thickness;
		float outline_th = th + 2.0f;

		RGBA main = RGBA(235, 235, 235, (int)(255.f * opacity));
		RGBA outline = RGBA(0, 0, 0, (int)(220.f * opacity));
		auto* dl = ImGui::GetBackgroundDrawList();
		if (!dl) return;
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

			stroke(IM_COL32(outline.r, outline.g, outline.b, outline.a), outline_th);
			stroke(IM_COL32(main.r, main.g, main.b, main.a), th);
		};

		draw_corner(ImVec2(p0.x, p0.y), +1.f, +1.f, -IM_PI * 0.5f, -IM_PI);
		draw_corner(ImVec2(p1.x, p0.y), -1.f, +1.f, -IM_PI * 0.5f, 0.0f);
		draw_corner(ImVec2(p0.x, p1.y), +1.f, -1.f, IM_PI * 0.5f, IM_PI);
		draw_corner(ImVec2(p1.x, p1.y), -1.f, -1.f, IM_PI * 0.5f, 0.0f);
	}
	void drawCornerFilledBox(const ImVec2& p0, const ImVec2& p1, ImU32 line_color, float thickness, ImU32 fill_color) {
		auto* dl = ImGui::GetBackgroundDrawList();
		if (!dl) return;

		const float width = p1.x - p0.x;
		const float height = p1.y - p0.y;

		float len = width * 0.25f;
		if (len < 7.0f) len = 7.0f;
		if (len > 16.0f) len = 16.0f;

		float lenh = height * 0.22f;
		if (lenh < 9.0f) lenh = 9.0f;
		if (lenh > 20.0f) lenh = 20.0f;

		const float th = thickness < 1.0f ? 1.0f : thickness;
		const ImU32 outline = IM_COL32(0, 0, 0, 200);

		dl->AddRectFilled(p0, p1, fill_color, 0.0f, 0);

		auto draw_line = [&](const ImVec2& a, const ImVec2& b) {
			dl->AddLine(a, b, outline, th + 1.0f);
			dl->AddLine(a, b, line_color, th);
		};

		draw_line(ImVec2(p0.x, p0.y), ImVec2(p0.x + len, p0.y));
		draw_line(ImVec2(p0.x, p0.y), ImVec2(p0.x, p0.y + lenh));

		draw_line(ImVec2(p1.x - len, p0.y), ImVec2(p1.x, p0.y));
		draw_line(ImVec2(p1.x, p0.y), ImVec2(p1.x, p0.y + lenh));

		draw_line(ImVec2(p0.x, p1.y - lenh), ImVec2(p0.x, p1.y));
		draw_line(ImVec2(p0.x, p1.y), ImVec2(p0.x + len, p1.y));

		draw_line(ImVec2(p1.x - len, p1.y), ImVec2(p1.x, p1.y));
		draw_line(ImVec2(p1.x, p1.y - lenh), ImVec2(p1.x, p1.y));
	}
	inline std::string trim_relation_name(std::string value) {
		value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) {
			return !std::isspace(ch);
		}));
		value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
		}).base(), value.end());
		return value;
	}
	inline std::string lower_relation_name(std::string value) {
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
			return static_cast<char>(std::tolower(ch));
		});
		return value;
	}
	inline std::string make_relation_key(int static_id, const char* altv_nick, const char* gta_name) {
		if (static_id > 0) {
			return "id:" + std::to_string(static_id);
		}

		std::string altv_key = lower_relation_name(trim_relation_name(altv_nick ? std::string(altv_nick) : std::string()));
		if (!altv_key.empty()) {
			return "altv:" + altv_key;
		}

		std::string gta_key = lower_relation_name(trim_relation_name(gta_name ? std::string(gta_name) : std::string()));
		if (!gta_key.empty()) {
			return "gta:" + gta_key;
		}

		return {};
	}
	inline bool is_enemy_marked(const DataPed& data) {
		return player_marks::is_enemy(data.altv_static_id);
	}
	inline bool is_fraction_marked(const DataPed& data) {
		return player_marks::is_fraction(data.altv_static_id);
	}
	inline bool is_family_marked(const DataPed& data) {
		return player_marks::is_family(data.altv_static_id);
	}
	inline ImU32 get_weapon_highlight_color_u32(DWORD weapon_hash, ImU32 fallback, bool is_friend) {
		return ::weapons_highlight::esp_color(weapon_hash, fallback, is_friend);
	}
	inline void draw_weapon_highlight_line(const ImVec2& target, ImU32 color, float opacity = 1.f) {
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		if (!dl) return;

		const ImVec2 center(Game.screen.x * 0.5f, Game.screen.y * 0.5f);
		const ImU32 line = utils::apply_opacity(color, opacity);
		const ImU32 outline = IM_COL32(0, 0, 0, (int)(170.f * opacity));
		dl->AddLine(center, target, outline, 2.f);
		dl->AddLine(center, target, line, 1.f);
	}
	inline bool get_screen_edge_line_target(const ImVec2& center, ImVec2 dir, const ImVec2& display_size, ImVec2* out) {
		if (!out) return false;
		const float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
		if (len < 0.001f) return false;
		dir.x /= len;
		dir.y /= len;

		const float margin = 8.f;
		float tx = FLT_MAX;
		float ty = FLT_MAX;
		if (fabsf(dir.x) > 0.001f) {
			tx = dir.x > 0.f ? (display_size.x - margin - center.x) / dir.x : (margin - center.x) / dir.x;
		}
		if (fabsf(dir.y) > 0.001f) {
			ty = dir.y > 0.f ? (display_size.y - margin - center.y) / dir.y : (margin - center.y) / dir.y;
		}

		const float t = (std::min)(tx, ty);
		if (!std::isfinite(t) || t <= 0.f) return false;
		*out = ImVec2(center.x + dir.x * t, center.y + dir.y * t);
		return true;
	}

	inline bool get_projected_screen_direction(const WorldToScreenSnapshot& view, const Vector3& target_pos, const ImVec2& center, ImVec2* out_dir) {
		if (!out_dir) return false;
		ImVec2 projected{};
		if (!WorldToScreenMatrix(view, target_pos, &projected)) return false;

		ImVec2 dir(projected.x - center.x, projected.y - center.y);
		const float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
		if (len < 0.001f) return false;
		dir.x /= len;
		dir.y /= len;
		*out_dir = dir;
		return true;
	}

	inline bool get_offscreen_indicator_target(const Vector3& target_pos, const Vector3& local_pos, ImVec2* out) {
		if (!out) return false;

		WorldToScreenSnapshot view{};
		const ImVec2 display_size = ImGui::GetIO().DisplaySize;
		const ImVec2 center(display_size.x * 0.5f, display_size.y * 0.5f);
		ImVec2 dir{};
		if (CaptureWorldToScreenSnapshot(&view) && get_projected_screen_direction(view, target_pos, center, &dir)) {
			return get_screen_edge_line_target(center, dir, display_size, out);
		}

		CPlayerAngles* cam = Game.getCam();
		if (!IsValidPtr(cam)) return false;

		float rot = acosf(cam->m_fps_angles.x) * 180.0f / PI;
		if (asinf(cam->m_fps_angles.y) * 180.0f / PI < 0.0f) rot *= -1.0f;
		const float forward_direction = DirectX::XMConvertToRadians(rot);
		const float cos_yaw = cosf(forward_direction);
		const float sin_yaw = sinf(forward_direction);
		const float delta_x = target_pos.x - local_pos.x;
		const float delta_y = target_pos.y - local_pos.y;

		dir = ImVec2(-(delta_y * cos_yaw - delta_x * sin_yaw), -(delta_x * cos_yaw + delta_y * sin_yaw));
		return get_screen_edge_line_target(ImVec2(display_size.x * 0.5f, display_size.y * 0.5f), dir, display_size, out);
	}
	inline Vector3 ws_vec3(const ws_server::EspPlayer& player, uint8_t index) {
		if (index >= player.bone_count || index >= 24) return Vector3(0.f, 0.f, 0.f);
		return Vector3(player.bones[index][0], player.bones[index][1], player.bones[index][2]);
	}

	inline PlayerBones ws_player_bones(const ws_server::EspPlayer& player) {
		PlayerBones bones{};
		bones.HEAD = ws_vec3(player, 0);
		bones.NECK = ws_vec3(player, 1);
		bones.RIGHT_HAND = ws_vec3(player, 2);
		bones.RIGHT_FOREARM = ws_vec3(player, 3);
		bones.RIGHT_UPPER_ARM = ws_vec3(player, 4);
		bones.RIGHT_CLAVICLE = ws_vec3(player, 5);
		bones.LEFT_HAND = ws_vec3(player, 6);
		bones.LEFT_FOREARM = ws_vec3(player, 7);
		bones.LEFT_UPPER_ARM = ws_vec3(player, 8);
		bones.LEFT_CLAVICLE = ws_vec3(player, 9);
		bones.PELVIS = ws_vec3(player, 10);
		bones.SPINE_ROOT = ws_vec3(player, 11);
		bones.SPINE0 = ws_vec3(player, 12);
		bones.SPINE1 = ws_vec3(player, 13);
		bones.SPINE2 = ws_vec3(player, 14);
		bones.SPINE3 = ws_vec3(player, 15);
		bones.RIGHT_TOE = ws_vec3(player, 16);
		bones.RIGHT_FOOT = ws_vec3(player, 17);
		bones.RIGHT_CALF = ws_vec3(player, 18);
		bones.RIGHT_THIGH = ws_vec3(player, 19);
		bones.LEFT_TOE = ws_vec3(player, 20);
		bones.LEFT_FOOT = ws_vec3(player, 21);
		bones.LEFT_CALF = ws_vec3(player, 22);
		bones.LEFT_THIGH = ws_vec3(player, 23);
		return bones;
	}

	inline void copy_ws_text(char* dst, size_t dst_size, const std::string& src) {
		if (!dst || dst_size == 0) return;
		const size_t len = src.size() < dst_size - 1 ? src.size() : dst_size - 1;
		memcpy(dst, src.c_str(), len);
		dst[len] = '\0';
	}

	struct gta_skeleton_cache_entry {
		Vector3 pos;
		PlayerBones bones;
		bool visible;
		int static_id;
		int netid;
		DWORD weapon_hash;
	};

	inline bool has_skeleton_bones(const PlayerBones& bones) {
		return (bones.HEAD.x != 0.f || bones.HEAD.y != 0.f || bones.HEAD.z != 0.f) ||
			(bones.NECK.x != 0.f || bones.NECK.y != 0.f || bones.NECK.z != 0.f) ||
			(bones.SPINE3.x != 0.f || bones.SPINE3.y != 0.f || bones.SPINE3.z != 0.f);
	}

	inline bool skeleton_point_matches_anchor(const Vector3& point, const Vector3& anchor) {
		if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z)) return false;
		if (point.x == 0.f && point.y == 0.f && point.z == 0.f) return false;

		const float dx = point.x - anchor.x;
		const float dy = point.y - anchor.y;
		const float dz = fabsf(point.z - anchor.z);
		return (dx * dx + dy * dy) <= 16.f && dz <= 3.f;
	}

	inline bool skeleton_matches_altv_anchor(const PlayerBones& bones, const Vector3& anchor) {
		if (!has_skeleton_bones(bones)) return false;
		return skeleton_point_matches_anchor(bones.SPINE_ROOT, anchor) ||
			skeleton_point_matches_anchor(bones.SPINE3, anchor) ||
			skeleton_point_matches_anchor(bones.NECK, anchor) ||
			skeleton_point_matches_anchor(bones.LEFT_FOOT, anchor) ||
			skeleton_point_matches_anchor(bones.RIGHT_FOOT, anchor);
	}

	inline bool valid_world_pos(const Vector3& pos) {
		return std::isfinite(pos.x) && std::isfinite(pos.y) && std::isfinite(pos.z) &&
			(pos.x != 0.f || pos.y != 0.f || pos.z != 0.f);
	}

	inline void normalize_projected_bounds_center(ImVec2& top, ImVec2& bottom) {
		if (!std::isfinite(top.x) || !std::isfinite(bottom.x)) return;
		const float center_x = (top.x + bottom.x) * 0.5f;
		top.x = center_x;
		bottom.x = center_x;
	}

	inline std::vector<gta_skeleton_cache_entry> build_gta_player_cache() {
		std::vector<gta_skeleton_cache_entry> cache;
		std::lock_guard<std::mutex> lock(game::ped_list_mutex);
		cache.reserve(game::ped_list.size());

		for (const auto& entity : game::ped_list) {
			CObject* ped = entity.first;
			if (!IsValidPtr(ped) || ped == local.player) continue;

			PedCache ped_cache{};
			if (!read_ped_cache(ped, &ped_cache) || ped_cache.hp <= 0.f) continue;

			DWORD weapon_hash = weapon_reader::get_weapon_hash(ped);
			if (weapon_hash == 0) weapon_hash = entity.second.altv_weapon_hash;
			cache.push_back({ ped_cache.pos, entity.second.bones, entity.second.visible != 0, entity.second.altv_static_id, entity.second.altv_dynamic_id, weapon_hash });
		}

		return cache;
	}

	inline const gta_skeleton_cache_entry* find_gta_match(const std::vector<gta_skeleton_cache_entry>& cache, const ws_server::EspPlayer& player, const Vector3& anchor) {
		for (const auto& entity : cache) {
			if (player.static_id > 0 && entity.static_id == player.static_id) {
				return &entity;
			}
		}

		for (const auto& entity : cache) {
			if (player.netid > 0 && entity.netid == static_cast<int>(player.netid)) {
				return &entity;
			}
		}

		const gta_skeleton_cache_entry* best = nullptr;
		float best_dist_sq = 4.f * 4.f;
		int candidates = 0;

		for (const auto& entity : cache) {
			if (!has_skeleton_bones(entity.bones)) continue;

			const float dx = entity.pos.x - anchor.x;
			const float dy = entity.pos.y - anchor.y;
			const float dz = fabsf(entity.pos.z - anchor.z);
			if (dz > 3.f) continue;

			const float dist_sq = dx * dx + dy * dy;
			if (dist_sq > 4.f * 4.f) continue;

			++candidates;
			if (dist_sq <= best_dist_sq) {
				best_dist_sq = dist_sq;
				best = &entity;
			}
		}

		return candidates == 1 ? best : nullptr;
	}

	using altv_esp_render_settings = projected_esp_settings;

	inline ImU32 get_arrow_color_u32(const altv_esp_render_settings& settings, bool enemy_marked, bool friend_marked, bool family_marked, bool fraction_marked) {
		if (enemy_marked) return settings.enemy_color;
		if (friend_marked) return settings.friend_color;
		if (family_marked) return settings.family_relation_color;
		if (fraction_marked) return settings.fraction_relation_color;
		return get_default_arrow_color_u32();
	}

	struct altv_esp_fade_state {
		uint64_t key = 0;
		float opacity = 0.f;
		bool present = false;
		bool used = false;
		ws_server::EspPlayer last_player{};
		bool has_last_player = false;
		double missing_since = 0.0;
		double fade_out_since = 0.0;
	};

	struct altv_esp_render_item {
		ws_server::EspPlayer player{};
		bool dormant = false;
		bool fading_out = false;
	};
	constexpr double k_altv_dormant_ttl = 3.0;
	constexpr double k_altv_dormant_enter_duration = 0.12;
	constexpr double k_altv_dormant_fade_out_duration = 0.18;
	constexpr float k_altv_dormant_opacity = 0.45f;
	constexpr float k_altv_dormant_max_distance = 300.f;

	inline float altv_player_distance_to_local(const ws_server::EspPlayer& player, const PedCache& local_cache) {
		const float dx = player.pos_x - local_cache.pos.x;
		const float dy = player.pos_y - local_cache.pos.y;
		const float dz = player.pos_z - local_cache.pos.z;
		return sqrtf(dx * dx + dy * dy + dz * dz);
	}

	inline float smooth_step01(float value) {
		value = std::clamp(value, 0.f, 1.f);
		return value * value * (3.f - 2.f * value);
	}

	inline uint64_t make_altv_fade_key(const ws_server::EspPlayer& player) {
		if (player.netid > 0) return 0x1000000000000000ull | static_cast<uint64_t>(player.netid);
		if (player.static_id > 0) return 0x2000000000000000ull | static_cast<uint64_t>(static_cast<uint32_t>(player.static_id));
		return 0x3000000000000000ull | static_cast<uint64_t>(static_cast<uint32_t>(player.handle));
	}

	inline bool is_weapons_highlight_candidate(const ::weapons_highlight::state& highlight_state, DWORD weapon_hash) {
		return ::weapons_highlight::exists(highlight_state, weapon_hash);
	}

	inline bool weapon_hash_has_esp_display(const ::weapons_highlight::state& highlight_state, DWORD weapon_hash) {
		if (weapon_hash == 0 || weapon_hash == 0xA2719263) return false;
		return find_weapon_icon_entry(weapon_hash) != nullptr ||
			get_readable_weapon_name(weapon_hash) != nullptr ||
			::weapons_highlight::exists(highlight_state, weapon_hash);
	}

	struct weapon_label_cache_entry {
		uint64_t key = 0;
		DWORD hash = 0;
		bool used = false;
		char label[80] = {};
	};
	inline const char* stable_weapon_label(const ::weapons_highlight::state& highlight_state, uint64_t key, DWORD weapon_hash, bool use_highlight_label) {
		static weapon_label_cache_entry cache[128] = {};
		if (weapon_hash == 0 || weapon_hash == 0xA2719263) return nullptr;

		weapon_label_cache_entry* entry = nullptr;
		weapon_label_cache_entry* free_entry = nullptr;
		for (auto& item : cache) {
			if (item.used && item.key == key) {
				entry = &item;
				break;
			}
			if (!item.used && !free_entry) free_entry = &item;
		}
		if (!entry) {
			entry = free_entry ? free_entry : &cache[0];
			entry->key = key;
			entry->hash = 0;
			entry->used = true;
			entry->label[0] = '\0';
		}
		if (entry->hash != weapon_hash) {
			entry->hash = weapon_hash;
			entry->label[0] = '\0';
		}

		static thread_local std::string highlight_label;
		if (use_highlight_label) {
			highlight_label = ::weapons_highlight::label(highlight_state, weapon_hash);
		} else {
			highlight_label.clear();
		}

		const char* current_name = nullptr;
		if (use_highlight_label && ::weapons_highlight::has_custom_name(highlight_state, weapon_hash)) {
			current_name = highlight_label.c_str();
		}
		if (!current_name || !current_name[0]) {
			current_name = get_weapon_display_name(weapon_hash);
		}
		if ((!current_name || !current_name[0]) && use_highlight_label && !highlight_label.empty()) {
			current_name = highlight_label.c_str();
		}
		if (!current_name || !current_name[0]) return nullptr;

		if (!is_weapons_highlight_candidate(highlight_state, weapon_hash) || entry->label[0] == '\0' || strcmp(entry->label, current_name) != 0) {
			strncpy_s(entry->label, current_name, _TRUNCATE);
		}

		return entry->label;
	}

	inline altv_esp_render_settings read_altv_esp_render_settings() {
		return read_projected_esp_settings();
	}

	bool draw_altv_player_esp() {
		if (!IsValidPtr(local.player)) return false;
		PedCache local_cache{};
		if (!read_ped_cache(local.player, &local_cache) || local_cache.hp <= 0.f) return false;

		static thread_local std::vector<ws_server::EspPlayer> players;
		static thread_local std::vector<altv_esp_render_item> render_items;
		static thread_local std::vector<uint64_t> current_keys;
		static altv_esp_fade_state fade_states[128] = {};
		static double last_fade_time = 0.0;
		ws_server::copy_players(players);

		const altv_esp_render_settings settings = read_altv_esp_render_settings();
		bool has_cached_players = false;
		if (players.empty()) {
			for (const auto& state : fade_states) {
				if (state.used && state.has_last_player) {
					has_cached_players = true;
					break;
				}
			}
		}
		if (players.empty() && !has_cached_players) return false;

		const double now = ImGui::GetTime();
		const ::weapons_highlight::state weapon_highlight = ::weapons_highlight::read_state();
		float fade_step = 0.16f;
		if (last_fade_time > 0.0) {
			const float dt = static_cast<float>(now - last_fade_time);
			fade_step = std::clamp(dt / 0.18f, 0.02f, 0.22f);
		}
		last_fade_time = now;

		for (auto& state : fade_states) {
			if (state.used) state.present = false;
		}

		const RGBA skel_visible_color = settings.skeleton_color;
		const RGBA skel_invisible_color = settings.skeleton_invisible_color;

		auto* esp_dl = ImGui::GetBackgroundDrawList();
		if (!esp_dl) return true;
		WorldToScreenSnapshot w2s_view{};
		if (!CaptureWorldToScreenSnapshot(&w2s_view)) return true;

		const ImVec2 display_size = ImGui::GetIO().DisplaySize;
		bool arrows_enabled = config::get("visual", "arrows", 0) != 0;
		const float arrow_radius = config::get("visual", "arrow_fov", 50.f) + 10.0f;
		const ImVec2 arrow_center(display_size.x * 0.5f, display_size.y * 0.5f);
		float arrow_cos_yaw = 0.f;
		float arrow_sin_yaw = 0.f;
		if (arrows_enabled) {
			CPlayerAngles* cam = Game.getCam();
			if (IsValidPtr(cam)) {
				float rot = acosf(cam->m_fps_angles.x) * 180.0f / PI;
				if (asinf(cam->m_fps_angles.y) * 180.0f / PI < 0.0f) rot *= -1.0f;
				const float forward_direction = DirectX::XMConvertToRadians(rot);
				arrow_cos_yaw = cosf(forward_direction);
				arrow_sin_yaw = sinf(forward_direction);
			}
			else {
				arrows_enabled = false;
			}
		}

		float draw_opacity = 1.f;

		render_items.clear();
		current_keys.clear();
		render_items.reserve(players.size() + 16);
		current_keys.reserve(players.size());
		const uint64_t current_sequence = players.empty() ? 0 : players.front().sequence;
		for (const auto& player : players) {
			if (current_sequence != 0 && player.sequence != current_sequence) continue;

			const uint64_t fade_key = make_altv_fade_key(player);
			current_keys.push_back(fade_key);
			render_items.push_back(altv_esp_render_item{ player, false });
		}
		for (auto& state : fade_states) {
			if (!state.used || !state.has_last_player) continue;
			if (std::find(current_keys.begin(), current_keys.end(), state.key) != current_keys.end()) continue;
			const float dormant_distance = altv_player_distance_to_local(state.last_player, local_cache);
			if (!std::isfinite(dormant_distance) || dormant_distance >= k_altv_dormant_max_distance) {
				state = altv_esp_fade_state{};
				continue;
			}
			if (state.missing_since <= 0.0) {
				state.missing_since = now;
				state.fade_out_since = 0.0;
			}
			const bool dormant_expired = now - state.missing_since > k_altv_dormant_ttl;
			if (dormant_expired && state.fade_out_since <= 0.0) state.fade_out_since = now;
			if (state.fade_out_since > 0.0 && now - state.fade_out_since > k_altv_dormant_fade_out_duration) {
				state = altv_esp_fade_state{};
				continue;
			}
			render_items.push_back(altv_esp_render_item{ state.last_player, true, state.fade_out_since > 0.0 });
		}

		const bool weapon_fallback_enabled = settings.show_weapon || weapon_highlight.any_enabled;
		const float max_range_sq = settings.max_range * settings.max_range;
		bool needs_gta_player_cache = settings.draw_skeleton || settings.show_visible;
		if (!needs_gta_player_cache && weapon_fallback_enabled) {
			for (const auto& item : render_items) {
				const auto& player = item.player;
				if (item.dormant || player.is_dead) continue;
				if (player.pos_x == 0.f && player.pos_y == 0.f && player.pos_z == 0.f) continue;

				const float dx = player.pos_x - local_cache.pos.x;
				const float dy = player.pos_y - local_cache.pos.y;
				const float dz = player.pos_z - local_cache.pos.z;
				const float dist_sq = dx * dx + dy * dy + dz * dz;
				const bool weapon_fallback_in_range = weapon_fallback_enabled && !weapon_hash_has_esp_display(weapon_highlight, player.weapon_hash) && dist_sq <= max_range_sq;
				if (weapon_fallback_in_range) {
					needs_gta_player_cache = true;
					break;
				}
			}
		}
		const auto gta_player_cache = needs_gta_player_cache ? build_gta_player_cache() : std::vector<gta_skeleton_cache_entry>{};

		for (const auto& item : render_items) {
			const auto& player = item.player;

			const uint64_t fade_key = make_altv_fade_key(player);
			altv_esp_fade_state* fade_state = nullptr;
			altv_esp_fade_state* free_state = nullptr;
			for (auto& state : fade_states) {
				if (state.used && state.key == fade_key) {
					fade_state = &state;
					break;
				}
				if (!state.used && !free_state) free_state = &state;
			}
			if (!fade_state) {
				if (item.dormant) continue;
				fade_state = free_state ? free_state : &fade_states[0];
				*fade_state = altv_esp_fade_state{};
				fade_state->key = fade_key;
				fade_state->opacity = 0.f;
				fade_state->used = true;
			}
			if (!item.dormant) {
				fade_state->present = true;
				fade_state->missing_since = 0.0;
				fade_state->fade_out_since = 0.0;
				fade_state->opacity += (1.f - fade_state->opacity) * fade_step;
				if (fade_state->opacity > 0.995f) fade_state->opacity = 1.f;
			}

			draw_opacity = fade_state->opacity;
			if (item.dormant) {
				if (item.fading_out) {
					const float fade_out_t = static_cast<float>((now - fade_state->fade_out_since) / k_altv_dormant_fade_out_duration);
					draw_opacity *= k_altv_dormant_opacity * (1.f - smooth_step01(fade_out_t));
				}
				else {
					const float enter_t = static_cast<float>((now - fade_state->missing_since) / k_altv_dormant_enter_duration);
					const float enter_blend = smooth_step01(enter_t);
					draw_opacity *= 1.f - ((1.f - k_altv_dormant_opacity) * enter_blend);
				}
			}
			if (draw_opacity <= 0.005f) continue;
			if (player.is_dead && settings.dim_dead_players) draw_opacity *= 0.7f;
			if (player.pos_x == 0.f && player.pos_y == 0.f && player.pos_z == 0.f) continue;
			Vector3 pos(player.pos_x, player.pos_y, player.pos_z);
			const float dx = pos.x - local_cache.pos.x;
			const float dy = pos.y - local_cache.pos.y;
			const float dz = pos.z - local_cache.pos.z;
			const float dist_sq = dx * dx + dy * dy + dz * dz;
			if (dist_sq > max_range_sq) continue;

			const gta_skeleton_cache_entry* gta_match = !item.dormant && !player.is_dead ? find_gta_match(gta_player_cache, player, pos) : nullptr;

			DWORD player_weapon_hash = player.weapon_hash;
			if (!weapon_hash_has_esp_display(weapon_highlight, player_weapon_hash) && gta_match && weapon_hash_has_esp_display(weapon_highlight, gta_match->weapon_hash)) {
				player_weapon_hash = gta_match->weapon_hash;
			}
			if (!item.dormant) {
				ws_server::EspPlayer cached_player = player;
				cached_player.pos_x = pos.x;
				cached_player.pos_y = pos.y;
				cached_player.pos_z = pos.z;
				cached_player.weapon_hash = player_weapon_hash;
				fade_state->last_player = cached_player;
				fade_state->has_last_player = true;
			}
			float Distance = sqrtf(dist_sq);
			const bool player_is_friend = player_marks::is_friend(player.static_id);
			const bool is_enemy = player_marks::is_enemy(player.static_id);
			const player_marks::relation persistent_relation = player_marks::persistent_get(player.static_id);
			const bool auto_family_enabled = config::get("visual", "relation_auto_family", 1) != 0;
			const bool auto_fraction_enabled = config::get("visual", "relation_auto_fraction", 1) != 0;
			const bool is_family = persistent_relation == player_marks::relation::family_player || (auto_family_enabled && player.auto_relation == 3);
			const bool is_fraction = persistent_relation == player_marks::relation::fraction_player || (auto_fraction_enabled && player.auto_relation == 4);

			if (arrows_enabled) {
				constexpr float ARROW_SIZE = 14.f;
				const float delta_x = pos.x - local_cache.pos.x;
				const float delta_y = pos.y - local_cache.pos.y;

				ImVec2 dir(-(delta_y * arrow_cos_yaw - delta_x * arrow_sin_yaw), -(delta_x * arrow_cos_yaw + delta_y * arrow_sin_yaw));
				const float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
				if (len < 0.001f) continue;
				dir.x /= len;
				dir.y /= len;

				drawArrow(arrow_center, atan2f(dir.y, dir.x), arrow_radius, ARROW_SIZE, get_arrow_color_u32(settings, settings.show_relation && is_enemy, settings.show_relation && player_is_friend, settings.show_relation && is_family, settings.show_relation && is_fraction));
			}

			const bool skeleton_data_allowed = !item.dormant && !player.is_dead && Distance <= 300.f;
			const bool skeleton_allowed = settings.draw_skeleton && skeleton_data_allowed;
			PlayerBones ws_bones{};
			PlayerBones gta_bones{};
			bool skeleton_visible = true;
			bool has_ws_skeleton = false;
			if (skeleton_data_allowed && player.bone_count >= 24) {
				ws_bones = ws_player_bones(player);
				has_ws_skeleton = has_skeleton_bones(ws_bones);
			}
			const bool has_gta_skeleton = !has_ws_skeleton && skeleton_data_allowed && gta_match && has_skeleton_bones(gta_match->bones);
			if (has_gta_skeleton) {
				gta_bones = gta_match->bones;
				skeleton_visible = gta_match->visible;
			}

			ImVec2 top2d;
			ImVec2 bottom2d;
			bool top_ok = WorldToScreenMatrix(w2s_view, Vector3(pos.x, pos.y, pos.z + 0.85f), &top2d);
			bool bottom_ok = WorldToScreenMatrix(w2s_view, Vector3(pos.x, pos.y, pos.z - 1.0f), &bottom2d);
			if (!top_ok && !bottom_ok) {
				if (::weapons_highlight::esp_line(weapon_highlight, player_weapon_hash, player_is_friend)) {
					ImVec2 line_target;
					if (get_offscreen_indicator_target(pos, local_cache.pos, &line_target)) {
						draw_weapon_highlight_line(line_target, ::weapons_highlight::esp_color(weapon_highlight, player_weapon_hash, settings.weapon_color, player_is_friend), draw_opacity);
					}
				}
				continue;
			}
			if (!top_ok) top2d = ImVec2(bottom2d.x, bottom2d.y - 50.f);
			if (!bottom_ok) bottom2d = ImVec2(top2d.x, top2d.y + 50.f);
			normalize_projected_bounds_center(top2d, bottom2d);

			float BoxHeight = fabsf(top2d.y - bottom2d.y);
			if (BoxHeight < 12.f) BoxHeight = 12.f;
			if (::weapons_highlight::esp_line(weapon_highlight, player_weapon_hash, player_is_friend)) {
				const ImVec2 line_target(top2d.x, top2d.y + BoxHeight * 0.5f);
				draw_weapon_highlight_line(line_target, ::weapons_highlight::esp_color(weapon_highlight, player_weapon_hash, settings.weapon_color, player_is_friend), draw_opacity);
			}

			if (skeleton_allowed) {
				const RGBA skel_visible_final = skel_visible_color;
				const RGBA skel_invisible_final = settings.skeleton_invisible_enabled ? skel_invisible_color : skel_visible_color;
				if (has_ws_skeleton) {
					const bool skel_visible = player.visible;
					const RGBA skeleton_color = skel_visible ? skel_visible_final : skel_invisible_final;
					drawBones(ws_bones, utils::apply_opacity(skeleton_color, draw_opacity), settings.skeleton_thickness, &w2s_view);
				}
				else if (has_gta_skeleton) {
					const RGBA skeleton_color = skeleton_visible ? skel_visible_final : skel_invisible_final;
					drawBones(gta_bones, utils::apply_opacity(skeleton_color, draw_opacity), settings.skeleton_thickness, &w2s_view);
				}
			}

			projected_esp_player projected{};
			projected.name = player.name.c_str();
			projected.social = player.social.c_str();
			projected.faction = player.fraction.c_str();
			projected.static_id = player.static_id;
			projected.dynamic_id = static_cast<int>(player.netid);
			projected.fraction_id = player.fraction_id;
			projected.leader_id = player.leader_id;
			projected.level = player.level;
			projected.admin_level = player.admin_level;
			projected.health = player.hp;
			projected.armor = player.armor;
			projected.weapon_hash = player_weapon_hash;
			projected.weapon_force_label = ::weapons_highlight::esp_custom_label(weapon_highlight, player_weapon_hash, player_is_friend);
			projected.weapon_highlight_active = ::weapons_highlight::esp_highlight_active(weapon_highlight, player_weapon_hash, player_is_friend);
			projected.weapon_highlight_color = ::weapons_highlight::esp_color(weapon_highlight, player_weapon_hash, settings.weapon_color, player_is_friend);
			projected.weapon_label = stable_weapon_label(weapon_highlight, make_altv_fade_key(player), player_weapon_hash, projected.weapon_force_label);
			projected.is_admin = player.is_admin;
			projected.is_dead = player.is_dead;
			projected.is_media = player.is_media;
			projected.is_tester = player.is_tester;
			projected.is_afk = player.is_afk;
			projected.is_cuffed = player.is_cuffed;
			projected.has_wear_armor = player.has_wear_armor;
			projected.wanted_level = player.wanted_level;
			projected.visible = player.visible;
			projected.is_friend = player_is_friend;
			projected.is_enemy = is_enemy;
			projected.is_family = is_family;
			projected.is_fraction = is_fraction;
			projected.dormant = item.dormant;
			draw_projected_player_overlay(esp_dl, settings, projected, top2d, bottom2d, Distance, draw_opacity);

		}

		for (auto& state : fade_states) {
			if (!state.used || state.present) continue;
			if (state.fade_out_since > 0.0 && now - state.fade_out_since > k_altv_dormant_fade_out_duration) {
				state = altv_esp_fade_state{};
			}
		}

		return true;
	}

	void draw_player_esp() {
		if (!IsValidPtr(local.player) || local.player->HP <= 0)
			return;

		draw_altv_player_esp();
		return;

		int countPed = 0;
		int countPed_Admin = 0;

		RGBA skel_visible_color = RGBA(
			config::get("visual", "skel_visible_r", 1.f) * 255,
			config::get("visual", "skel_visible_g", 1.f) * 255,
			config::get("visual", "skel_visible_b", 1.f) * 255,
			config::get("visual", "skel_visible_a", 1.f) * 255
		);
		RGBA skel_invisible_color = RGBA(
			config::get("visual", "skel_invisible_r", 1.f) * 255,
			config::get("visual", "skel_invisible_g", 1.f) * 255,
			config::get("visual", "skel_invisible_b", 1.f) * 255,
			config::get("visual", "skel_invisible_a", 1.f) * 255
		);
		
		RGBA box_color = RGBA(
			config::get("visual", "box_color_r", 1.f) * 255,
			config::get("visual", "box_color_g", 1.f) * 255,
			config::get("visual", "box_color_b", 1.f) * 255,
			config::get("visual", "box_color_a", 1.f) * 255
		);
		
		RGBA distance_color = RGBA(
			config::get("visual", "distance_color_r", 0.f) * 255,
			config::get("visual", "distance_color_g", 1.f) * 255,
			config::get("visual", "distance_color_b", 0.f) * 255,
			config::get("visual", "distance_color_a", 1.f) * 255
		);
		
		RGBA weapons_color = RGBA(
			config::get("visual", "weapons_color_r", 0.f) * 255,
			config::get("visual", "weapons_color_g", 1.f) * 255,
			config::get("visual", "weapons_color_b", 0.f) * 255,
			config::get("visual", "weapons_color_a", 1.f) * 255
		);

		RGBA visible_color = RGBA(
			config::get("visual", "visible_r", 0.f) * 255,
			config::get("visual", "visible_g", 1.f) * 255,
			config::get("visual", "visible_b", 0.f) * 255,
			config::get("visual", "visible_a", 1.f) * 255
		);
		RGBA invisible_color = RGBA(
			config::get("visual", "invisible_r", 1.f) * 255,
			config::get("visual", "invisible_g", 0.f) * 255,
			config::get("visual", "invisible_b", 0.f) * 255,
			config::get("visual", "invisible_a", 1.f) * 255
		);
		{
			std::lock_guard<std::mutex> lock(game::ped_list_mutex);
			for (pair<CObject*, DataPed>& entity : game::ped_list) {
				PedCache _lpc;
				if (!IsValidPtr(local.player) || !read_ped_cache(local.player, &_lpc) || _lpc.hp <= 0)
					break;

				CObject* ped = entity.first;
				DataPed data = entity.second;
				const bool is_enemy = is_enemy_marked(data);

				if (!IsValidPtr(ped)) continue;
				PedCache pc;
				if (!read_ped_cache(ped, &pc) || pc.hp <= 0)
					continue;

				if (ped == local.player)
					continue;

				if (get_distance(_lpc.pos, pc.pos) > config::get("hack", "max_range", 1000.f)) continue;
				CModelInfo* _mi1 = ped->ModelInfo();
				if (!IsValidPtr(_mi1)) continue;
				DWORD hash = _mi1->GetHash();
				if (!game::isValidPlayer(hash, ped)) continue;
				countPed++;

				auto Distance = get_distance(_lpc.pos, pc.pos);

				RGBA current_color = RGBA(255, 255, 255, 255);
				RGBA skeleton_color = skel_visible_color;

				auto Top = Vector3(pc.pos.x, pc.pos.y, pc.pos.z + 0.85f);
				auto Bottom = Vector3(pc.pos.x, pc.pos.y, pc.pos.z - 1.0f);

				ImVec2 top2d;
				ImVec2 bottom2d;

				if (config::get("visual", "arrows", 0)) {
					if (!IsValidPtr(local.player) || local.player->HP <= 0)
						break;

					const auto ARROW_SIZE = 14.f;
					auto fov = config::get("visual", "arrow_fov", 50.f);
					CPlayerAngles* cam = Game.getCam();
					if (!IsValidPtr(cam))
						continue;

					float rot = acosf(cam->m_fps_angles.x) * 180.0f / PI;
					if (asinf(cam->m_fps_angles.y) * 180.0f / PI < 0.0f) rot *= -1.0f;
					float ForwardDirection = DirectX::XMConvertToRadians(rot);

					float CosYaw = cosf(ForwardDirection);
					float SinYaw = sinf(ForwardDirection);

					float DeltaX = pc.pos.x - _lpc.pos.x;
					float DeltaY = pc.pos.y - _lpc.pos.y;

					ImVec2 dir(-(DeltaY * CosYaw - DeltaX * SinYaw), -(DeltaX * CosYaw + DeltaY * SinYaw));
					float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
					if (len < 0.001f) continue;
					dir.x /= len;
					dir.y /= len;

					float angle = atan2f(dir.y, dir.x);

					ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
					float radius = fov + 10.0f;

					WorldToScreenSnapshot arrow_view{};
					ImVec2 projected_dir{};
					if (CaptureWorldToScreenSnapshot(&arrow_view) && get_projected_screen_direction(arrow_view, pc.pos, center, &projected_dir)) {
						angle = atan2f(projected_dir.y, projected_dir.x);
					}

					drawArrow(center, angle, radius, ARROW_SIZE, get_default_arrow_color_u32());
				}

				bool is_local_player = (ped == local.player);
				if (is_local_player && config::get("visual", "esp_self", 0)) {
					float local_esp_x = config::get("visual", "local_esp_x", 150.f);
					float local_esp_y = config::get("visual", "local_esp_y", 400.f);
					float local_esp_height = config::get("visual", "local_esp_height", 150.f);
					
					top2d = ImVec2(local_esp_x, local_esp_y);
					bottom2d = ImVec2(local_esp_x, local_esp_y + local_esp_height);
				} else {
					bool topOk = WorldToScreen2(Top, &top2d);
					bool botOk = WorldToScreen2(Bottom, &bottom2d);
					if (!topOk && !botOk) {
						const DWORD line_weapon_hash = data.has_ws_data && data.altv_weapon_hash != 0 ? data.altv_weapon_hash : weapon_reader::get_weapon_hash(ped);
						if (::weapons_highlight::esp_line(line_weapon_hash, data.is_friend)) {
							ImVec2 line_target;
							if (get_offscreen_indicator_target(pc.pos, _lpc.pos, &line_target)) {
								const ImU32 line_color = IM_COL32(
									(int)(config::get("visual", "weap_color_r", 241.f / 255.f) * 255),
									(int)(config::get("visual", "weap_color_g", 241.f / 255.f) * 255),
									(int)(config::get("visual", "weap_color_b", 241.f / 255.f) * 255),
									220
								);
								draw_weapon_highlight_line(line_target, get_weapon_highlight_color_u32(line_weapon_hash, line_color, data.is_friend));
							}
						}
						continue;
					}
					if (!topOk) top2d = ImVec2(bottom2d.x, bottom2d.y - 50.f);
					if (!botOk) bottom2d = ImVec2(top2d.x, top2d.y + 50.f);
				}
				normalize_projected_bounds_center(top2d, bottom2d);

				float BoxHeight = abs(top2d.y - bottom2d.y);
				float BoxWidth = BoxHeight / 2.0f;
				float TextAboveY = top2d.y;
				int textRowAbove = 0;
				float TextStartY = top2d.y + BoxHeight;
				int textRow = 0;

				if (data.visible) {
					current_color = visible_color;
					skeleton_color = skel_visible_color;
				}
				else {
					current_color = invisible_color;
					skeleton_color = skel_invisible_color;
				}
				
				if (pc.alpha < 250) {
					countPed_Admin++;
					current_color = RGBA(255, 0, 0, 255);
				}

	
				if ((config::get("visual", "display_groups", 0)) && (data.group.name != "\0")) {
					current_color = data.group.color;
				}

				if ((config::get("hack", "group_window", 0) && Game.menuOpen)) {
					if (ImGui::IsMouseClicked(0, false)) {
						ImVec2 mPos = ImGui::GetMousePos();
						if ((mPos.x >= top2d.x - (BoxWidth / 2.0f)) && (mPos.x <= top2d.x - (BoxWidth / 2.0f) + BoxWidth)) {
							if ((mPos.y >= top2d.y) && (mPos.y <= top2d.y + BoxHeight)) {
								strcpy(ui::toAddHashSets, GetPedComponentHash(ped).c_str());
							}
						}
					}

					ImVec2 mPos = ImGui::GetMousePos();
					if ((mPos.x >= top2d.x - (BoxWidth / 2.0f)) && (mPos.x <= top2d.x - (BoxWidth / 2.0f) + BoxWidth)) {
						if ((mPos.y >= top2d.y) && (mPos.y <= top2d.y + BoxHeight)) {
							renderer.RenderRectFilled(ImVec2(top2d.x - (BoxWidth / 2.0f), top2d.y), ImVec2(top2d.x - (BoxWidth / 2.0f) + BoxWidth, top2d.y + BoxHeight), RGBA(current_color.r, current_color.g, current_color.b, 150), 0.0f, 0);
							renderer.RenderText("Copy Style ID", ImVec2(top2d.x, top2d.y + BoxHeight / 2), 13, RGBA(255, 255, 255, 255), true, true);
						}
					}
				}

				float thickness_box = config::get("visual", "box_thickness", 0.f);
				const int name_pos = config::get("visual", "esp_pos_name", 0);
				const int dist_pos = config::get("visual", "esp_pos_dist", 1);
				const int weap_pos = config::get("visual", "esp_pos_weap", 1);
				const float base_info_size = renderer.espFont ? renderer.espFont->FontSize : 13.f;
				const float info_scale = get_esp_info_scale(BoxHeight, base_info_size);
				const float desired_info_size = base_info_size * info_scale;
				float name_size = desired_info_size;
				float small_size = 10.f;
				float label_size = 12.f;

				float current_top_offset = 2.f;
				float side_bottom_offset = 0.f;
				float side_left_offset = 0.f;
				float side_right_offset = 0.f;

				auto* esp_dl = ImGui::GetBackgroundDrawList();
				ImFont* esp_font = renderer.EspNameFont(desired_info_size, &name_size);
				ImFont* esp_small_font = renderer.EspSmallFont(&small_size);

				auto align_text_pos = [](const ImVec2& value) -> ImVec2 {
					return ImVec2(floorf(value.x), floorf(value.y));
				};

				auto draw_shadow_text = [&](ImFont* text_font, const char* text, const ImVec2& pos, float font_size, ImU32 color) {
					if (!esp_dl || !text_font || !text || !text[0]) return;

					const ImVec2 text_pos = align_text_pos(pos);
					const ImU32 soft_shadow = IM_COL32(0, 0, 0, 55);
					const ImU32 hard_shadow = IM_COL32(0, 0, 0, 135);
					esp_dl->AddText(text_font, font_size, ImVec2(text_pos.x + 2.f, text_pos.y + 2.f), soft_shadow, text);
					esp_dl->AddText(text_font, font_size, ImVec2(text_pos.x + 1.f, text_pos.y + 1.f), hard_shadow, text);
					esp_dl->AddText(text_font, font_size, text_pos, color, text);
				};

				auto draw_centered_shadow_text = [&](const char* text, const ImVec2& center, float font_size, ImU32 color) {
					if (!esp_font || !text || !text[0]) return;
					const ImVec2 text_size = esp_font->CalcTextSizeA(font_size, FLT_MAX, 0.f, text);
					draw_shadow_text(esp_font, text, ImVec2(center.x - text_size.x * 0.5f, center.y), font_size, color);
				};

				auto calc_text_pos = [&](ImFont* text_font, int pos, const char* text, float font_size) -> ImVec2 {
					if (!text_font || !text) return ImVec2(top2d.x, top2d.y);

					ImVec2 text_size = text_font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text);
					const float left = top2d.x - (BoxWidth / 2.0f);
					const float right = top2d.x + (BoxWidth / 2.0f);

					if (pos == 0) {
						const float y = top2d.y - current_top_offset - text_size.y - thickness_box;
						current_top_offset += text_size.y + 2.f;
						return ImVec2(top2d.x - text_size.x * 0.5f, y);
					}

					if (pos == 1) {
						const float y = TextStartY + thickness_box + (14.f * textRow) + side_bottom_offset;
						side_bottom_offset += text_size.y + 2.f;
						return ImVec2(top2d.x - text_size.x * 0.5f, y);
					}

					if (pos == 2) {
						const float y = top2d.y + (BoxHeight * 0.5f) - (text_size.y * 0.5f) + side_left_offset;
						side_left_offset += text_size.y + 2.f;
						return ImVec2(left - text_size.x - 6.f, y);
					}

					const float y = top2d.y + (BoxHeight * 0.5f) - (text_size.y * 0.5f) + side_right_offset;
					side_right_offset += text_size.y + 2.f;
					return ImVec2(right + 6.f, y);
				};

				const DWORD player_weapon_hash = data.has_ws_data && data.altv_weapon_hash != 0 ? data.altv_weapon_hash : weapon_reader::get_weapon_hash(ped);
				const ImU32 legacy_weapon_color = IM_COL32(
					(int)(config::get("visual", "weap_color_r", 241.f / 255.f) * 255),
					(int)(config::get("visual", "weap_color_g", 241.f / 255.f) * 255),
					(int)(config::get("visual", "weap_color_b", 241.f / 255.f) * 255),
					220
				);
				if (::weapons_highlight::esp_line(player_weapon_hash, data.is_friend)) {
					const ImVec2 line_target(top2d.x, top2d.y + BoxHeight * 0.5f);
					draw_weapon_highlight_line(line_target, get_weapon_highlight_color_u32(player_weapon_hash, legacy_weapon_color, data.is_friend));
				}

				if (is_enemy) {
					const char* enemy_text = "ENEMY";
					const ImU32 enemy_color = get_enemy_color_u32();
					ImVec2 textSize = esp_small_font->CalcTextSizeA(label_size, FLT_MAX, 0, enemy_text);
					ImVec2 textPos = ImVec2(top2d.x - textSize.x / 2.f, top2d.y - current_top_offset - textSize.y - thickness_box);
					draw_shadow_text(esp_small_font, enemy_text, textPos, label_size, enemy_color);
					current_top_offset += textSize.y + 2.f;
				}
				else if (data.is_friend) {
					const char* friend_text = "FRIEND";
					const ImU32 friend_color = get_friend_color_u32();
					ImVec2 textSize = esp_small_font->CalcTextSizeA(label_size, FLT_MAX, 0, friend_text);
					ImVec2 textPos = ImVec2(top2d.x - textSize.x / 2.f, top2d.y - current_top_offset - textSize.y - thickness_box);
					draw_shadow_text(esp_small_font, friend_text, textPos, label_size, friend_color);
					current_top_offset += textSize.y + 2.f;
				}

			if (data.has_ws_data) {
				if (config::get("visual", "altv_nickname", 0) && name_pos == 0 && data.altv_nick[0] != '\0') {
					const char* nick_text = data.altv_nick;
					const ImU32 nick_color = get_weapon_highlight_color_u32(data.altv_weapon_hash, get_altv_nickname_color_u32(is_enemy, data.is_friend), data.is_friend);
					ImVec2 textSize = esp_font->CalcTextSizeA(name_size, FLT_MAX, 0, nick_text);
					ImVec2 textPos = ImVec2(top2d.x - textSize.x / 2.f, top2d.y - current_top_offset - textSize.y - thickness_box);
					draw_shadow_text(esp_font, nick_text, textPos, name_size, nick_color);
					current_top_offset += textSize.y + 2.f;
				}

				if (config::get("visual", "altv_static", 0) && data.altv_static_id > 0) {
					char id_buf[48];
					if (data.altv_dynamic_id > 0) {
						sprintf_s(id_buf, "#%d [%d]", data.altv_static_id, data.altv_dynamic_id);
					}
					else {
						sprintf_s(id_buf, "#%d", data.altv_static_id);
					}
					const ImU32 static_color = get_altv_static_color_u32();
					ImVec2 textSize = esp_font->CalcTextSizeA(name_size, FLT_MAX, 0, id_buf);
					ImVec2 textPos = ImVec2(top2d.x - textSize.x / 2.f, top2d.y - current_top_offset - textSize.y - thickness_box);
					draw_shadow_text(esp_font, id_buf, textPos, name_size, static_color);
					current_top_offset += textSize.y + 2.f;
				}

				if (config::get("visual", "altv_faction", 0) && data.altv_fraction[0] != '\0' && strcmp(data.altv_fraction, "None") != 0) {
					const ImU32 faction_color = get_altv_fraction_color_u32(data.altv_fraction_id, get_altv_faction_color_u32());
					char faction_buf[64];
					const char* faction_text = data.altv_fraction;
					if (data.altv_leader_id > 0) {
						sprintf_s(faction_buf, "Leader %s", data.altv_fraction);
						faction_text = faction_buf;
					}
					ImVec2 textSize = esp_small_font->CalcTextSizeA(label_size, FLT_MAX, 0, faction_text);
					ImVec2 textPos = ImVec2(top2d.x - textSize.x / 2.f, top2d.y - current_top_offset - textSize.y - thickness_box);
					draw_shadow_text(esp_small_font, faction_text, textPos, label_size, faction_color);
					current_top_offset += textSize.y + 2.f;
				}

				if (config::get("visual", "altv_admin", 0) && data.altv_is_admin) {
					char admin_buf[32];
					if (data.altv_admin_level > 0) sprintf_s(admin_buf, "ADMIN [%d]", data.altv_admin_level);
					else sprintf_s(admin_buf, "ADMIN");
					const ImU32 admin_color = get_altv_admin_color_u32();
					ImVec2 textSize = esp_small_font->CalcTextSizeA(label_size, FLT_MAX, 0, admin_buf);
					ImVec2 textPos = ImVec2(top2d.x - textSize.x / 2.f, top2d.y - current_top_offset - textSize.y - thickness_box);
					draw_shadow_text(esp_small_font, admin_buf, textPos, label_size, admin_color);
					current_top_offset += textSize.y + 2.f;
				}

				if (config::get("visual", "altv_tester", 0) && data.altv_is_tester) {
					const char* tester_text = "TESTER";
					const ImU32 tester_color = get_altv_tester_color_u32();
					ImVec2 textSize = esp_small_font->CalcTextSizeA(label_size, FLT_MAX, 0, tester_text);
					ImVec2 textPos = ImVec2(top2d.x - textSize.x / 2.f, top2d.y - current_top_offset - textSize.y - thickness_box);
					draw_shadow_text(esp_small_font, tester_text, textPos, label_size, tester_color);
					current_top_offset += textSize.y + 2.f;
				}

				if (config::get("visual", "altv_media", 0) && data.altv_is_media) {
					const char* media_text = "MEDIA";
					const ImU32 media_color = get_altv_media_color_u32();
					ImVec2 textSize = esp_small_font->CalcTextSizeA(label_size, FLT_MAX, 0, media_text);
					ImVec2 textPos = ImVec2(top2d.x - textSize.x / 2.f, top2d.y - current_top_offset - textSize.y - thickness_box);
					draw_shadow_text(esp_small_font, media_text, textPos, label_size, media_color);
					current_top_offset += textSize.y + 2.f;
				}

				if (config::get("visual", "altv_afk", 0) && data.altv_is_afk) {
					const char* afk_text = "AFK";
					const ImU32 afk_color = get_altv_afk_color_u32();
					ImVec2 textSize = esp_small_font->CalcTextSizeA(label_size, FLT_MAX, 0, afk_text);
					ImVec2 textPos = ImVec2(top2d.x - textSize.x / 2.f, top2d.y - current_top_offset - textSize.y - thickness_box);
					draw_shadow_text(esp_small_font, afk_text, textPos, label_size, afk_color);
					current_top_offset += textSize.y + 2.f;
				}

				if (config::get("visual", "altv_dead", 0) && data.altv_is_dead) {
					const char* dead_text = "DEAD";
					const ImU32 dead_color = get_altv_dead_color_u32();
					ImVec2 textSize = esp_small_font->CalcTextSizeA(label_size, FLT_MAX, 0, dead_text);
					ImVec2 textPos = ImVec2(top2d.x - textSize.x / 2.f, top2d.y - current_top_offset - textSize.y - thickness_box);
					draw_shadow_text(esp_small_font, dead_text, textPos, label_size, dead_color);
					current_top_offset += textSize.y + 2.f;
				}

				if (config::get("visual", "altv_level", 0) && data.altv_level > 0) {
					char lvl_buf[32];
					sprintf_s(lvl_buf, "LVL: %d", data.altv_level);
					const ImU32 level_color = get_altv_level_color_u32();
					ImVec2 textSize = esp_small_font->CalcTextSizeA(label_size, FLT_MAX, 0, lvl_buf);
					ImVec2 textPos = ImVec2(top2d.x - textSize.x / 2.f, top2d.y - current_top_offset - textSize.y - thickness_box);
					draw_shadow_text(esp_small_font, lvl_buf, textPos, label_size, level_color);
					current_top_offset += textSize.y + 2.f;
				}
			}
			else if (config::get("visual", "altv_nickname", 0) && name_pos == 0 && data.name[0] != '\0') {
					const char* nick_text = data.name;
					const DWORD side_weapon_hash = data.has_ws_data && data.altv_weapon_hash != 0 ? data.altv_weapon_hash : weapon_reader::get_weapon_hash(ped);
						const ImU32 nick_color = get_weapon_highlight_color_u32(side_weapon_hash, get_altv_nickname_color_u32(is_enemy, data.is_friend), data.is_friend);
					ImVec2 textSize = esp_font->CalcTextSizeA(name_size, FLT_MAX, 0, nick_text);
					ImVec2 textPos = ImVec2(top2d.x - textSize.x / 2.f, top2d.y - current_top_offset - textSize.y - thickness_box);
					draw_shadow_text(esp_font, nick_text, textPos, name_size, nick_color);
					current_top_offset += textSize.y + 2.f;
				}

				if ((config::get("visual", "display_groups", 0)) && (data.group.name != "\0")) {
					renderer.RenderText("[" + data.group.name + "]", ImVec2(top2d.x, (TextStartY + thickness_box) + (14 * textRow)), 13, current_color, true, true);
					textRow += 1;
				}
				if (config::get("visual", "display_groups", 0)) {
					string pedHash = GetPedComponentHash(ped);
					if (pedHash != "\0" && !pedHash.empty()) {
						renderer.RenderText(pedHash, ImVec2(top2d.x, (TextStartY + thickness_box) + (14 * textRow)), 11, RGBA(180, 180, 180, 200), true, true);
						textRow += 1;
					}
				}
				
				if (config::get("visual", "draw_box", 0)) {
					int box_style = config::get("visual", "draw_box_style", 0);
					if (box_style < 0 || box_style > 2)
						box_style = 0;
					auto p0 = ImVec2(top2d.x - (BoxWidth / 2.0f), top2d.y);
					auto p1 = ImVec2(top2d.x - (BoxWidth / 2.0f) + BoxWidth, top2d.y + BoxHeight);
					float box_radius = config::get("visual", "box_radius", 0.f);
					if (box_radius < 0.f)
						box_radius = 0.f;
					RGBA box_fill_color = RGBA(
						config::get("visual", "draw_box_fill_color_r", 0.2f) * 255,
						config::get("visual", "draw_box_fill_color_g", 0.2f) * 255,
						config::get("visual", "draw_box_fill_color_b", 0.2f) * 255,
						config::get("visual", "draw_box_fill_color_a", 0.2f) * 255
					);

					if (box_style == 0) {
						RGBA outline = RGBA(0, 0, 0, 255);
						renderer.RenderRect(ImVec2(p0.x-1, p0.y-1), ImVec2(p1.x+1, p1.y+1), outline, box_radius, ImDrawFlags_RoundCornersAll, 1.f);
						renderer.RenderRect(p0, p1, box_color, box_radius, ImDrawFlags_RoundCornersAll, thickness_box);
					}
					if (box_style == 1) {
						drawBracketBox(p0, p1, thickness_box);
					}
					if (box_style == 2) {
						renderer.RenderRectFilled(p0, p1, box_fill_color, box_radius, ImDrawFlags_RoundCornersAll);
						renderer.RenderRect(p0, p1, box_color, box_radius, ImDrawFlags_RoundCornersAll, thickness_box);
					}
				}

				const int health_mode = config::get("visual", "health_mode", config::get("visual", "draw_healthbar", 0) != 0 ? 1 : 0);
				if (health_mode != 0) {
					utils::draw_health_stack(
						ImVec2(top2d.x - (BoxWidth / 2.0f), top2d.y),
						ImVec2(top2d.x - (BoxWidth / 2.0f) + BoxWidth, top2d.y + BoxHeight),
						pc.hp - 100.0f,
						pc.maxHp - 100.0f,
						pc.armor,
						Distance <= 50.f
					);
				}

				if (config::get("visual", "draw_skeleton", 0)) {
				 drawBones(data.bones, skeleton_color, config::get("visual", "skeleton_thickness", 0.1f));
				}
				
				if (config::get("visual", "draw_healthtext", 0)) {
					renderer.RenderText(("[HP " + std::to_string((int)(pc.hp - 100.0f)) + "]").c_str(), ImVec2(top2d.x, (TextStartY + thickness_box) + (14 * textRow)), 13, current_color, true, true);
					textRow += 1;
					if (pc.armor > 1.0f) {
						renderer.RenderText(("[A " + std::to_string((int)(pc.armor)) + "]").c_str(), ImVec2(top2d.x, (TextStartY + thickness_box) + (14 * textRow)), 13, current_color, true, true);
						textRow += 1;
					}
				}

				if (config::get("visual", "altv_nickname", 0) && name_pos != 0) {
					const char* side_name = nullptr;
					if (data.has_ws_data) {
						if (data.altv_nick[0] != '\0') {
							side_name = data.altv_nick;
						}
					}
					else if (data.name[0] != '\0') {
						side_name = data.name;
					}

					if (side_name && side_name[0]) {
						const DWORD side_weapon_hash = data.has_ws_data && data.altv_weapon_hash != 0 ? data.altv_weapon_hash : weapon_reader::get_weapon_hash(ped);
						const ImU32 nick_color = get_weapon_highlight_color_u32(side_weapon_hash, get_altv_nickname_color_u32(is_enemy, data.is_friend), data.is_friend);
						draw_shadow_text(esp_font, side_name, calc_text_pos(esp_font, name_pos, side_name, name_size), name_size, nick_color);
					}
				}

				if (config::get("visual", "draw_aim_dot", 0)) {
					ImVec2 aim2d;
					int b = config::get("aimbot", "aim_bone", 0) == 0 ? 0 : config::get("aimbot", "aim_bone", 0) == 1 ? 7 : 8;

					if (!WorldToScreen(ped->get_bone(b), &aim2d)) continue;

					renderer.RenderDot(ImVec2(aim2d.x - 5, aim2d.y - 5), ImVec2(aim2d.x + 5, aim2d.y + 5), current_color, 1.f);
				}

				if (config::get("visual", "draw_distance", 0)) {
					char dist_buf[32];
					sprintf_s(dist_buf, "%dM", (int)(Distance + 0.5f));
					ImU32 text_col = IM_COL32(
						(int)(config::get("visual","dist_color_r",201.f / 255.f)*255),
						(int)(config::get("visual","dist_color_g",199.f / 255.f)*255),
						(int)(config::get("visual","dist_color_b",199.f / 255.f)*255), 220);
					draw_shadow_text(esp_small_font, dist_buf, calc_text_pos(esp_small_font, dist_pos, dist_buf, small_size), small_size, text_col);
				}
				
				DWORD weapon_hash = player_weapon_hash;
				const bool force_custom_weapon_label = ::weapons_highlight::esp_custom_label(weapon_hash, data.is_friend);
				if (config::get("visual", "draw_weapons", 0) || force_custom_weapon_label) {
					if (weapon_hash != 0 && weapon_hash != 0xA2719263) {
						std::string highlighted_weapon_name;
						if (force_custom_weapon_label) {
							highlighted_weapon_name = ::weapons_highlight::label(weapon_hash);
						}
						const char* wname = force_custom_weapon_label && !highlighted_weapon_name.empty() ? highlighted_weapon_name.c_str() : esp::get_weapon_display_name(weapon_hash);
						if (wname && wname[0]) {
							char wbuf[80];
							sprintf_s(wbuf, "%s", wname);
							ImU32 wcol = IM_COL32(
								(int)(config::get("visual","weap_color_r",241.f / 255.f)*255),
								(int)(config::get("visual","weap_color_g",241.f / 255.f)*255),
								(int)(config::get("visual","weap_color_b",241.f / 255.f)*255), 220);
							wcol = get_weapon_highlight_color_u32(weapon_hash, wcol, data.is_friend);
							draw_shadow_text(esp_small_font, wbuf, calc_text_pos(esp_small_font, weap_pos, wbuf, label_size), label_size, wcol);
						}
					}
				}

			}
		}

		return;
	}
