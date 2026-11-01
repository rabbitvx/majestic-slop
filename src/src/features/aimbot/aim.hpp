#pragma once
#include "config/bind_state.hpp"
#include <atomic>
#include <set>
#include <cstdio>
#include <cstdlib>

namespace config {
    float get(std::string category, std::string key, float defaultVal);
    int get(std::string category, std::string key, int defaultVal);
    bool update(float newValue, std::string category, std::string key, float defaultVal);
    bool update(int newValue, std::string category, std::string key, int defaultVal);
}

namespace game {
    extern vector<pair<CObject*, DataPed>> ped_list;
    extern std::mutex ped_list_mutex;
}

namespace aimbot {
	std::mutex target_mutex;
	std::mutex execution_mutex;

	struct ExecutionContext {
		bool active = false;
		int mode = 0;
		Vector3 aim_pos;
		Vector3 damager_head_pos;
		CObject* target = 0;
	} exec_ctx;

	CObject* vector_target = 0;
	DataPed vector_target_data;
	static auto vector_targetTimeout = std::chrono::high_resolution_clock::now();
	static float vector_aimTargetDist = 99999.0f;

	CObject* current_target = 0;
	DataPed current_target_data;
	static Vector3 lastTargetVector = Vector3(0, 0, 0);
	static auto targetTimeout = std::chrono::high_resolution_clock::now();
	static float aimTargetDist = 99999.0f;
	static Vector3 target_aim_angle;
	using accurate_shot_fn_t = bool(__fastcall*)(__int64, __int64, Vector3, Vector3, float);
	static inline accurate_shot_fn_t original_get_is_accurate = nullptr;
	static inline std::atomic<bool> accurate_hook_installed = false;
	std::mutex accurate_hook_state_mutex;
	Vector3 accurate_hook_hit_pos = Vector3(0.f, 0.f, 0.f);

	struct vehicleState {
		bool state;
		uint64_t ticksSinceChange;
	};
	vehicleState vehicleAimbot;

	inline bool read_hp(CObject* target, float* outHP) {
		__try {
			if (!IsValidPtr(target) || !outHP) return false;
			*outHP = target->HP;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	inline bool get_distance_between(Vector3* pos1, Vector3* pos2, float* outDist) {
		if (!pos1 || !pos2 || !outDist) return false;
		*outDist = get_distance(*pos1, *pos2);
		return true;
	}

	inline bool get_model_hash(CObject* ped, DWORD* outHash) {
		__try {
			if (!IsValidPtr(ped) || !outHash) return false;
			auto modelInfo = ped->ModelInfo();
			if (!IsValidPtr(modelInfo)) return false;
			*outHash = modelInfo->GetHash();
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	inline bool is_in_vehicle(CObject* ped, bool* outResult) {
		__try {
			if (!IsValidPtr(ped) || !outResult) return false;
			*outResult = ped->IsInVehicle();
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	inline bool get_weapon_hash(CObject* ped, DWORD* outHash) {
		__try {
			if (!IsValidPtr(ped) || !outHash) return false;
			CWeaponManager* wep = ped->weapon();
			if (!IsValidPtr(wep) || !IsValidPtr(wep->_WeaponInfo)) return false;
			*outHash = *reinterpret_cast<DWORD*>(reinterpret_cast<uintptr_t>(wep->_WeaponInfo) + 0x10);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	inline bool get_local_player_handle(int* outHandle) {
		__try {
			if (!outHandle) return false;
			*outHandle = hacks::local_ped_handle();
			return *outHandle != 0;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	inline bool get_current_weapon_for_shot(CObject* ped, native::type::hash* outHash, int* outDamage) {
		__try {
			if (!IsValidPtr(ped) || !outHash || !outDamage) return false;

			CWeaponManager* wep = ped->weapon();
			if (!IsValidPtr(wep) || !IsValidPtr(wep->_WeaponInfo)) return false;

			CWeaponInfo* info = wep->_WeaponInfo;
			const uintptr_t info_ptr = reinterpret_cast<uintptr_t>(info);
			if (info_ptr <= 0x10000 || info_ptr >= 0x7FFFFFFFFFFF) return false;

			const DWORD hash = *reinterpret_cast<DWORD*>(info_ptr + 0x10);
			if (hash == 0 || hash == 0xA2719263) return false;

			const float damage = info->Damage;
			if (!std::isfinite(damage) || damage <= 0.f) return false;

			const int rounded_damage = static_cast<int>(std::round(damage));
			if (rounded_damage <= 0) return false;

			*outHash = static_cast<native::type::hash>(hash);
			*outDamage = rounded_damage;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			HELLO_RUNTIME_LOG("SEH: weapon shot state 0x%08X", GetExceptionCode());
			return false;
		}
	}

	inline bool get_velocity(CObject* ped, Vector3* outVel) {
		__try {
			if (!IsValidPtr(ped) || !outVel) return false;
			*outVel = ped->v3Velocity;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	inline bool get_vehicle_velocity(CObject* ped, Vector3* outVel) {
		__try {
			if (!IsValidPtr(ped) || !outVel) return false;
			CVehicle* veh = ped->vehicle();
			if (!IsValidPtr(veh)) return false;
			*outVel = veh->v3Velocity;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	inline bool get_position(CObject* ped, Vector3* outPos) {
		__try {
			if (!IsValidPtr(ped) || !outPos) return false;
			*outPos = ped->fPosition;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	static inline DWORD64 lastDamagerShot = 0;
	static inline DWORD64 lastSilentWallshotShot = 0;
	static inline int lastSilentClipAmmo = -1;
	static inline native::type::hash lastSilentClipWeapon = 0;
	static inline std::atomic<bool> bullet_instance_hook_installed = false;
	std::mutex bullet_instance_hook_mutex;

	inline int GetConfiguredBindKey(const char* category, const char* legacy_key, const char* binds_key) {
		int key = config::get("binds", binds_key, 0);
		if (key != 0) return key;
		return config::get(category, legacy_key, 0);
	}

	inline bool HasConfiguredBind(const char* category, const char* legacy_key, const char* binds_key) {
		return GetConfiguredBindKey(category, legacy_key, binds_key) != 0;
	}

	static const bind_state::bind_config k_vector_aim_bind {
		"aimbot",
		"vector_enable",
		"aim_key",
		"vector_aim_key",
		"aim_key_mode",
		"vector_aim_mode"
	};

inline bool IsTargetValid(CObject* target) {
		if (!target) return false;
if (!IsValidPtr(target)) return false;
		float hp = 0;
		if (!read_hp(target, &hp)) return false;
		if (hp <= 0) return false;
		return true;
	}

	inline bool has_clear_los_to_ped(CObject* target) {
		if (!IsValidPtr(local.player) || !IsValidPtr(target)) return false;
		int local_handle = 0;
		int target_handle = 0;
		__try {
			if (pointer_to_handle) {
				local_handle = pointer_to_handle(reinterpret_cast<uintptr_t>(local.player));
				target_handle = pointer_to_handle(reinterpret_cast<uintptr_t>(target));
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return true;
		}
		if (local_handle <= 0 || target_handle <= 0) return true; // unknown → assume visible
		__try {
			return native::entity::has_entity_clear_los_to_entity(target_handle, local_handle, 17);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return true;
		}
	}

	inline bool IsTargetDataDead(const DataPed& data) {
		return data.altv_is_dead;
	}

	inline bool should_ignore_marked_target(const DataPed& data) {
		if (config::get("aimbot", "ignore_friends", 1) && data.is_friend) return true;
		if (config::get("aimbot", "ignore_family", 0) && player_marks::is_family(data.altv_static_id)) return true;
		if (config::get("aimbot", "ignore_fraction", 0) && player_marks::is_fraction(data.altv_static_id)) return true;
		if (config::get("aimbot", "ignore_other_families", 0) && data.altv_family_id > 0) {
			const std::map<std::string, std::string> ignored_families = config::get("aimbot", "ignore_families", std::map<std::string, std::string>{});
			const auto it = ignored_families.find(std::to_string(data.altv_family_id));
			if (it != ignored_families.end() && it->second == "1") return true;
		}
		if (config::get("aimbot", "ignore_other_fractions", 0) && data.altv_fraction_id > 0) {
			const std::map<std::string, std::string> ignored_fractions = config::get("aimbot", "ignore_fractions", std::map<std::string, std::string>{});
			const auto it = ignored_fractions.find(std::to_string(data.altv_fraction_id));
			if (it != ignored_fractions.end() && it->second == "1") return true;
		}
		return false;
	}

	inline bool has_current_aim_target_data(const DataPed& data) {
		(void)data;
		return true;
	}

	inline bool refresh_current_aim_target_data(CObject* target, DataPed* out_data) {
		if (!target || !out_data) return false;

		std::lock_guard<std::mutex> lock(game::ped_list_mutex);
		for (const auto& entry : game::ped_list) {
			if (entry.first != target) continue;
			if (!has_current_aim_target_data(entry.second)) return false;
			*out_data = entry.second;
			return true;
		}

		return false;
	}

	inline bool has_current_aim_target(CObject* target) {
		DataPed data = {};
		return refresh_current_aim_target_data(target, &data);
	}

	inline bool IsBoneDataValid(const Vector3& bone) {
		if (!std::isfinite(bone.x) || !std::isfinite(bone.y) || !std::isfinite(bone.z)) return false;
		if (bone.x == 0.f && bone.y == 0.f && bone.z == 0.f) return false;
		return true;
	}

	inline bool GetBonePositionSafe(CObject* ped, int handle, int bone_id, Vector3* out) {
		if (!out) return false;
		if (handle <= 0 && pointer_to_handle && ped) {
			__try { handle = pointer_to_handle(reinterpret_cast<uintptr_t>(ped)); }
			__except (EXCEPTION_EXECUTE_HANDLER) { handle = 0; }
		}
		if (handle > 0) {
			__try {
				const PVector3 v = native::ped::get_ped_bone_coords(
					static_cast<native::type::ped>(handle), bone_id, 0.f, 0.f, 0.f);
				if (std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z)) {
					out->x = v.x;
					out->y = v.y;
					out->z = v.z;
					return true;
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				// native path failed on this build — fall back below
			}
		}
		*out = GetBonePosition(ped, bone_id);
		return IsBoneDataValid(*out);
	}

	inline Vector3 safe_bone(CObject* ped, int handle, int bone_id) {
		Vector3 v;
		if (GetBonePositionSafe(ped, handle, bone_id, &v)) return v;
		return Vector3();
	}

	inline void FetchPedBones(CObject* ped, int handle, PlayerBones* out) {
		if (!ped || !out) return;
		out->HEAD = safe_bone(ped, handle, HEAD);
		out->NECK = safe_bone(ped, handle, NECK);
		out->RIGHT_HAND = safe_bone(ped, handle, RIGHT_HAND);
		out->RIGHT_FOREARM = safe_bone(ped, handle, RIGHT_FOREARM);
		out->RIGHT_UPPER_ARM = safe_bone(ped, handle, RIGHT_UPPER_ARM);
		out->RIGHT_CLAVICLE = safe_bone(ped, handle, RIGHT_CLAVICLE);
		out->LEFT_HAND = safe_bone(ped, handle, LEFT_HAND);
		out->LEFT_FOREARM = safe_bone(ped, handle, LEFT_FOREARM);
		out->LEFT_UPPER_ARM = safe_bone(ped, handle, LEFT_UPPER_ARM);
		out->LEFT_CLAVICLE = safe_bone(ped, handle, LEFT_CLAVICLE);
		out->PELVIS = safe_bone(ped, handle, PELVIS);
		out->SPINE_ROOT = safe_bone(ped, handle, SPINE_ROOT);
		out->SPINE0 = safe_bone(ped, handle, SPINE0);
		out->SPINE1 = safe_bone(ped, handle, SPINE1);
		out->SPINE2 = safe_bone(ped, handle, SPINE2);
		out->SPINE3 = safe_bone(ped, handle, SPINE3);
		out->RIGHT_TOE = safe_bone(ped, handle, RIGHT_TOE);
		out->RIGHT_FOOT = safe_bone(ped, handle, RIGHT_FOOT);
		out->RIGHT_CALF = safe_bone(ped, handle, RIGHT_CALF);
		out->RIGHT_THIGH = safe_bone(ped, handle, RIGHT_THIGH);
		out->LEFT_TOE = safe_bone(ped, handle, LEFT_TOE);
		out->LEFT_FOOT = safe_bone(ped, handle, LEFT_FOOT);
		out->LEFT_CALF = safe_bone(ped, handle, LEFT_CALF);
		out->LEFT_THIGH = safe_bone(ped, handle, LEFT_THIGH);
	}

	Vector3 get_nearest_bone(DataPed data) {
		Vector3 aim = data.bones.HEAD;
		if (!IsBoneDataValid(aim)) {
			aim = data.bones.SPINE2;
			if (!IsBoneDataValid(aim)) {
				return Vector3(0, 0, 0);
			}
		}
		
		PlayerBones bones = data.bones;
		Vector3 bone_positions[] = {
			bones.HEAD,
			bones.NECK,
			bones.RIGHT_HAND,
			bones.RIGHT_FOREARM,
			bones.RIGHT_UPPER_ARM,
			bones.RIGHT_CLAVICLE,
			bones.LEFT_HAND,
			bones.LEFT_FOREARM,
			bones.LEFT_UPPER_ARM,
			bones.LEFT_CLAVICLE,
			bones.PELVIS,
			bones.SPINE_ROOT,
			bones.SPINE0,
			bones.SPINE1,
			bones.SPINE2,
			bones.SPINE3,
			bones.RIGHT_TOE,
			bones.RIGHT_FOOT,
			bones.RIGHT_CALF,
			bones.RIGHT_THIGH,
			bones.LEFT_TOE,
			bones.LEFT_FOOT,
			bones.LEFT_CALF,
			bones.LEFT_THIGH
		};
		float closest_distance = 999.f;
		int arrSize = sizeof(bone_positions) / sizeof(bone_positions[0]);
		
		for (size_t i = 0; i < arrSize; i++) {
			if (!IsBoneDataValid(bone_positions[i])) continue;
			
			ImVec2 screen_point;
			if (!WorldToScreen(bone_positions[i], &screen_point)) continue;
			float dist = screen_distance(Game.screen.x / 2, Game.screen.y / 2, screen_point.x, screen_point.y);
			if (closest_distance > dist) {
				closest_distance = dist;
				aim = bone_positions[i];
			}
		}

		return aim;
	}

	inline bool is_weapon_in_list(DWORD hash, const DWORD* values, size_t count) {
		for (size_t i = 0; i < count; ++i) {
			if (values[i] == hash) return true;
		}
		return false;
	}

	std::string get_weapon_category_prefix() {
		if (!IsValidPtr(local.player)) return "";
		DWORD wh = 0;
		if (!get_weapon_hash(local.player, &wh) || wh == 0) return "";

		static constexpr DWORD snipers[] = {
			weapon_hashes::WEAPON_SNIPERRIFLE, weapon_hashes::WEAPON_HEAVYSNIPER,
			weapon_hashes::WEAPON_HEAVYSNIPER_MK2, weapon_hashes::WEAPON_MARKSMANRIFLE,
			weapon_hashes::WEAPON_MARKSMANRIFLE_MK2
		};
		if (is_weapon_in_list(wh, snipers, std::size(snipers))) return "sniper_";

		static constexpr DWORD rifles[] = {
			weapon_hashes::WEAPON_ASSAULTRIFLE, weapon_hashes::WEAPON_ASSAULTRIFLE_MK2,
			weapon_hashes::WEAPON_CARBINERIFLE, weapon_hashes::WEAPON_CARBINERIFLE_MK2,
			weapon_hashes::WEAPON_ADVANCEDRIFLE, weapon_hashes::WEAPON_SPECIALCARBINE,
			weapon_hashes::WEAPON_SPECIALCARBINE_MK2, weapon_hashes::WEAPON_BULLPUPRIFLE,
			weapon_hashes::WEAPON_BULLPUPRIFLE_MK2, weapon_hashes::WEAPON_COMPACTRIFLE,
			weapon_hashes::WEAPON_MILITARYRIFLE, weapon_hashes::WEAPON_HEAVYRIFLE
		};
		if (is_weapon_in_list(wh, rifles, std::size(rifles))) return "rifle_";

		static constexpr DWORD shotguns[] = {
			weapon_hashes::WEAPON_PUMPSHOTGUN, weapon_hashes::WEAPON_PUMPSHOTGUN_MK2,
			weapon_hashes::WEAPON_SAWNOFFSHOTGUN, weapon_hashes::WEAPON_ASSAULTSHOTGUN,
			weapon_hashes::WEAPON_BULLPUPSHOTGUN, weapon_hashes::WEAPON_MUSKET,
			weapon_hashes::WEAPON_HEAVYSHOTGUN, weapon_hashes::WEAPON_DBSHOTGUN,
			weapon_hashes::WEAPON_AUTOSHOTGUN, weapon_hashes::WEAPON_COMBATSHOTGUN
		};
		if (is_weapon_in_list(wh, shotguns, std::size(shotguns))) return "sg_";

		static constexpr DWORD smgs[] = {
			weapon_hashes::WEAPON_MICROSMG, weapon_hashes::WEAPON_SMG,
			weapon_hashes::WEAPON_SMG_MK2, weapon_hashes::WEAPON_ASSAULTSMG,
			weapon_hashes::WEAPON_COMBATPDW, weapon_hashes::WEAPON_MACHINEPISTOL,
			weapon_hashes::WEAPON_MINISMG
		};
		if (is_weapon_in_list(wh, smgs, std::size(smgs))) return "smg_";

		static constexpr DWORD pistols[] = {
			weapon_hashes::WEAPON_PISTOL, weapon_hashes::WEAPON_PISTOL_MK2,
			weapon_hashes::WEAPON_COMBATPISTOL, weapon_hashes::WEAPON_APPISTOL,
			weapon_hashes::WEAPON_STUNGUN, weapon_hashes::WEAPON_PISTOL50,
			weapon_hashes::WEAPON_SNSPISTOL, weapon_hashes::WEAPON_SNSPISTOL_MK2,
			weapon_hashes::WEAPON_HEAVYPISTOL, weapon_hashes::WEAPON_VINTAGEPISTOL,
			weapon_hashes::WEAPON_FLAREGUN, weapon_hashes::WEAPON_MARKSMANPISTOL,
			weapon_hashes::WEAPON_REVOLVER, weapon_hashes::WEAPON_REVOLVER_MK2,
			weapon_hashes::WEAPON_DOUBLEACTION, weapon_hashes::WEAPON_CERAMICPISTOL,
			weapon_hashes::WEAPON_NAVYREVOLVER, weapon_hashes::WEAPON_GADGETPISTOL
		};
		if (is_weapon_in_list(wh, pistols, std::size(pistols))) return "pistol_";

		return "";
	}

	int get_aim_config_int(const std::string& key, int def) {
		std::string pfx = get_weapon_category_prefix();
		if (!pfx.empty()) {
			int val = config::get("aimbot", pfx + key, -999);
			if (val != -999) return val;
		}
		return config::get("aimbot", key, def);
	}

	float get_aim_config_float(const std::string& key, float def) {
		std::string pfx = get_weapon_category_prefix();
		if (!pfx.empty()) {
			float val = config::get("aimbot", pfx + key, -999.f);
			if (val != -999.f) return val;
		}
		return config::get("aimbot", key, def);
	}
	Vector3 get_cached_bone_by_id(const DataPed& data, const std::string& bone_id) {
		if (bone_id == "head") return data.bones.HEAD;
		if (bone_id == "neck") return data.bones.NECK;
		if (bone_id == "chest") return data.bones.SPINE2;
		if (bone_id == "spine") return data.bones.SPINE1;
		if (bone_id == "pelvis") return data.bones.PELVIS;
		return Vector3();
	}

	Vector3 get_live_bone_by_id(CObject* ped, const std::string& bone_id) {
		if (!ped || !IsValidPtr(ped)) return Vector3();
		int handle = 0;
		if (pointer_to_handle) {
			__try { handle = pointer_to_handle(reinterpret_cast<uintptr_t>(ped)); }
			__except (EXCEPTION_EXECUTE_HANDLER) { handle = 0; }
		}
		if (bone_id == "head") return safe_bone(ped, handle, HEAD);
		if (bone_id == "neck") return safe_bone(ped, handle, NECK);
		if (bone_id == "chest") return safe_bone(ped, handle, SPINE2);
		if (bone_id == "spine") return safe_bone(ped, handle, SPINE1);
		if (bone_id == "pelvis") return safe_bone(ped, handle, PELVIS);
		return Vector3();
	}

	void append_legacy_bones(int legacy_value, std::vector<std::string>& selected_bones) {
		if (legacy_value <= 0) {
			selected_bones.push_back("head");
			selected_bones.push_back("neck");
			selected_bones.push_back("chest");
			selected_bones.push_back("spine");
			selected_bones.push_back("pelvis");
			return;
		}

		if (legacy_value == 1) {
			selected_bones.push_back("head");
			return;
		}

		if (legacy_value == 2) {
			selected_bones.push_back("neck");
			return;
		}

		if (legacy_value == 3) {
			selected_bones.push_back("chest");
			return;
		}

		if (legacy_value == 4) {
			selected_bones.push_back("spine");
			return;
		}

		if (legacy_value == 5) {
			selected_bones.push_back("pelvis");
			return;
		}

		selected_bones.push_back("head");
	}

	std::vector<std::string> get_selected_bones(const char* multi_key, int legacy_value) {
		std::vector<std::string> selected_bones;
		const std::map<std::string, std::string> saved_bones =
			config::get("aimbot", multi_key, std::map<std::string, std::string>{});

		static const char* bone_order[] = { "head", "neck", "chest", "spine", "pelvis" };
		for (const char* bone_id : bone_order) {
			auto it = saved_bones.find(bone_id);
			if (it != saved_bones.end() && it->second == "1") {
				selected_bones.emplace_back(bone_id);
			}
		}

		if (selected_bones.empty()) {
			append_legacy_bones(legacy_value, selected_bones);
		}

		if (selected_bones.empty()) {
			selected_bones.push_back("head");
		}

		return selected_bones;
	}

	Vector3 pick_best_cached_bone(const DataPed& data, const std::vector<std::string>& selected_bones) {
		Vector3 best_bone = Vector3();
		float best_distance = 999999.f;

		for (const std::string& bone_id : selected_bones) {
			Vector3 bone_pos = get_cached_bone_by_id(data, bone_id);
			if (!IsBoneDataValid(bone_pos)) continue;

			ImVec2 screen_point;
			if (WorldToScreen(bone_pos, &screen_point)) {
				float distance = screen_distance(Game.screen.x / 2, Game.screen.y / 2, screen_point.x, screen_point.y);
				if (!IsBoneDataValid(best_bone) || distance < best_distance) {
					best_bone = bone_pos;
					best_distance = distance;
				}
				continue;
			}

			if (!IsBoneDataValid(best_bone)) {
				best_bone = bone_pos;
			}
		}

		return best_bone;
	}

	Vector3 pick_best_live_bone(CObject* ped, const std::vector<std::string>& selected_bones) {
		Vector3 best_bone = Vector3();
		float best_distance = 999999.f;

		for (const std::string& bone_id : selected_bones) {
			Vector3 bone_pos = get_live_bone_by_id(ped, bone_id);
			if (!IsBoneDataValid(bone_pos)) continue;

			ImVec2 screen_point;
			if (WorldToScreen(bone_pos, &screen_point)) {
				float distance = screen_distance(Game.screen.x / 2, Game.screen.y / 2, screen_point.x, screen_point.y);
				if (!IsBoneDataValid(best_bone) || distance < best_distance) {
					best_bone = bone_pos;
					best_distance = distance;
				}
				continue;
			}

			if (!IsBoneDataValid(best_bone)) {
				best_bone = bone_pos;
			}
		}

		return best_bone;
	}

	Vector3 pick_random_cached_bone(const DataPed& data, const std::vector<std::string>& selected_bones) {
		std::vector<Vector3> valid_bones;

		for (const std::string& bone_id : selected_bones) {
			Vector3 bone_pos = get_cached_bone_by_id(data, bone_id);
			if (IsBoneDataValid(bone_pos)) valid_bones.push_back(bone_pos);
		}

		if (valid_bones.empty()) return Vector3();
		return valid_bones[rand() % valid_bones.size()];
	}

	Vector3 pick_random_live_bone(CObject* ped, const std::vector<std::string>& selected_bones) {
		std::vector<Vector3> valid_bones;

		for (const std::string& bone_id : selected_bones) {
			Vector3 bone_pos = get_live_bone_by_id(ped, bone_id);
			if (IsBoneDataValid(bone_pos)) valid_bones.push_back(bone_pos);
		}

		if (valid_bones.empty()) return Vector3();
		return valid_bones[rand() % valid_bones.size()];
	}

	Vector3 get_aimbone_for_vector(DataPed data) {
		const int legacy_value = get_aim_config_int("vector_bone", config::get("aimbot", "vector_aim_bone", 0));
		return pick_best_cached_bone(data, get_selected_bones("vector_aim_bones", legacy_value));
	}

	inline bool get_fresh_head_bone(CObject* ped, Vector3* outBone) {
		__try {
			if (!ped || !IsValidPtr(ped) || !outBone) return false;
			int handle = 0;
			if (pointer_to_handle) {
				handle = pointer_to_handle(reinterpret_cast<uintptr_t>(ped));
			}
			*outBone = safe_bone(ped, handle, HEAD);
			return IsBoneDataValid(*outBone);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	Vector3 get_aimbone_complex(Vector3 current, DataPed data) {
		return get_nearest_bone(data);
	}

void get_vector_target(CPlayerAngles* cam, float radius) {
		if (!Game.renderReady || Game.screen.x < 100) return;
		bool target_acquired = false;
		if (!IsValidPtr(cam) || !IsValidPtr(local.player)) {
			std::lock_guard<std::mutex> lock(target_mutex);
			vector_target = 0;
			vector_aimTargetDist = 99999.f;
			return;
		}

		CObject* closest = 0;
		DataPed closest_data = { 0 };
		float closest_dist = 99999.0f;

		std::vector<std::pair<CObject*, DataPed>> snapshot;
		{
			std::lock_guard<std::mutex> lock(game::ped_list_mutex);
			snapshot = game::ped_list;
		}

		// 0 = closest to crosshair (screen distance), 1 = closest by 3D distance.
		const int priority = config::get("aimbot", "vector_priority", 0);
		// slider stores a float; reading through the int overload misparses it
		const int switch_delay = std::max(0, static_cast<int>(config::get("aimbot", "target_switch_delay", 0.f)));

		float best_metric = 999999.f;
		for (auto it = snapshot.begin(); it != snapshot.end(); ++it) {
			CObject* ped = it->first;
			if (!ped || !IsValidPtr(ped)) continue;

			DataPed data = it->second;

			if (!IsTargetValid(ped)) continue;
			if (IsTargetDataDead(data)) continue;
			if (local.player == ped) continue;

			Vector3 pedPos, localPos;
			if (!get_position(ped, &pedPos)) continue;
			if (!get_position(local.player, &localPos)) continue;

			float dist_to_player = 0.f;
			if (!get_distance_between(&localPos, &pedPos, &dist_to_player)) continue;

			if (dist_to_player > config::get("hack", "max_range", 1000.f)) continue;

			DWORD hash = 0;
			if (!get_model_hash(ped, &hash)) continue;

			if (!game::isValidPlayer(hash, ped)) continue;
			if (!has_current_aim_target_data(data)) continue;

			Vector3 aimpos = get_aimbone_for_vector(data);
			if (!IsBoneDataValid(aimpos)) continue;

			if (data.group.ally) continue;
			if (should_ignore_marked_target(data)) continue;
			if (config::get("aimbot", "visible_only", 0) && !has_clear_los_to_ped(ped)) continue;

			ImVec2 aim2d;
			if (!WorldToScreen(aimpos, &aim2d)) continue;

			float Distance2d = screen_distance(Game.screen.x / 2, Game.screen.y / 2, aim2d.x, aim2d.y);

			if (!(Distance2d > config::get("aimbot", "aim_stop_radius", 0.f)) || !(radius > Distance2d)) continue;

			const float metric = priority == 1 ? dist_to_player : Distance2d;

			CObject* current_vector_target_local = 0;
			{
				std::lock_guard<std::mutex> tlock(target_mutex);
				current_vector_target_local = vector_target;
			}

			if (current_vector_target_local == ped) {
				closest = ped;
				closest_data = data;
				closest_dist = metric;
				target_acquired = true;
				break;
			}

			if (metric < best_metric) {
				best_metric = metric;
				closest = ped;
				closest_data = data;
				closest_dist = metric;
				target_acquired = true;
			}
		}

		if (closest && target_acquired) {
			auto now = std::chrono::high_resolution_clock::now();

			std::lock_guard<std::mutex> lock(target_mutex);
			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - vector_targetTimeout).count();

			if (closest != vector_target || vector_target == 0) {
				if (dur > switch_delay) {
					vector_target = closest;
					vector_target_data = closest_data;
					vector_aimTargetDist = closest_dist;
					vector_targetTimeout = std::chrono::high_resolution_clock::now();
					return;
				}
			}
			else {
				vector_target_data = closest_data;
				vector_aimTargetDist = closest_dist;
				vector_targetTimeout = std::chrono::high_resolution_clock::now();
				return;
			}
		}

		if (!target_acquired) {
			std::lock_guard<std::mutex> lock(target_mutex);
			vector_target = 0;
			vector_aimTargetDist = 99999.f;
		}
	}

	void get_new_target_with_radius(CPlayerAngles* cam, float radius) {
		if (!Game.renderReady || Game.screen.x < 100) return;
		bool target_accquired = false;
		if (!IsValidPtr(cam) || !IsValidPtr(local.player)) {
			current_target = 0;
			aimTargetDist = 9999.f;
			target_aim_angle = { 0 };
			return;
		}

		Vector3 CrosshairPos = cam->m_cam_pos;
		CObject* closest = 0;
		DataPed closest_data = { 0 };

		std::lock_guard<std::mutex> lock(game::ped_list_mutex);
		for (auto it = game::ped_list.begin(); it != game::ped_list.end(); ++it) {
			CObject* ped = it->first;
			if (!ped || !IsValidPtr(ped)) continue;
			
			DataPed data = it->second;
			
			if (local.player == ped) continue;
			if (IsTargetDataDead(data)) continue;
			
			float hp = 0;
			if (!read_hp(ped, &hp) || hp <= 0) continue;
			
			Vector3 pedPos, localPos;
			if (!get_position(ped, &pedPos)) continue;
			if (!get_position(local.player, &localPos)) continue;
			
			float dist = 0;
			if (!get_distance_between(&localPos, &pedPos, &dist)) continue;
			if (dist > config::get("hack", "max_range", 1000.f)) continue;
			
			DWORD hash = 0;
			if (!get_model_hash(ped, &hash)) continue;
			if (!game::isValidPlayer(hash, ped)) continue;
			if (!has_current_aim_target_data(data)) continue;

			Vector3 aimpos = data.bones.HEAD;
			if (!IsBoneDataValid(aimpos)) continue;

			if (data.group.ally) continue;
			if (should_ignore_marked_target(data)) continue;
			if (config::get("aimbot", "visible_only", 0) && !has_clear_los_to_ped(ped)) continue;

			ImVec2 aim2d;
			if (!WorldToScreen(aimpos, &aim2d)) continue;

			float Distance2d = screen_distance(Game.screen.x / 2, Game.screen.y / 2, aim2d.x, aim2d.y);

			if (
				(Distance2d > config::get("aimbot", "aim_stop_radius", 0.f)) &&
				(radius > Distance2d)
				) {
				if ((aimTargetDist > Distance2d) || (current_target == ped)) {
					aimTargetDist = Distance2d;
					target_accquired = true;
					closest = ped;
					closest_data = data;
				}
			}
		}

		if (closest) {
			auto now = std::chrono::high_resolution_clock::now();
			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - targetTimeout).count();

			if (closest != current_target) {
				if (dur > config::get("aimbot", "target_timeout", 0.f)) {
					current_target = closest;
					current_target_data = closest_data;
					targetTimeout = std::chrono::high_resolution_clock::now();
					return;
				}
			}
			if (current_target == 0) {
				if (dur > config::get("aimbot", "target_timeout", 0.f)) {
					current_target = closest;
					current_target_data = closest_data;
					targetTimeout = std::chrono::high_resolution_clock::now();
					return;
				}
			}
		}

		if (!target_accquired) {
			current_target = 0;
			aimTargetDist = 9999.f;
			target_aim_angle = { 0 };
		}
	}

	void get_new_target(CPlayerAngles* cam) {
		bool target_accquired = false;
		if (!IsValidPtr(cam) || !IsValidPtr(local.player)) {
			current_target = 0;
			aimTargetDist = 9999.f;
			target_aim_angle = { 0 };
			return;
		}

		Vector3 CrosshairPos = cam->m_cam_pos;
		CObject* closest = 0;
		DataPed closest_data = { 0 };

		std::lock_guard<std::mutex> lock(game::ped_list_mutex);
		for (auto it = game::ped_list.begin(); it != game::ped_list.end(); ++it) {
			CObject* ped = it->first;
			if (!ped || !IsValidPtr(ped)) continue;
			
			DataPed data = it->second;
			
			if (local.player == ped) continue;
			if (IsTargetDataDead(data)) continue;
			
			float hp = 0;
			if (!read_hp(ped, &hp) || hp <= 0) continue;
			
			Vector3 pedPos, localPos;
			if (!get_position(ped, &pedPos)) continue;
			if (!get_position(local.player, &localPos)) continue;
			
			float dist = 0;
			if (!get_distance_between(&localPos, &pedPos, &dist)) continue;
			if (dist > config::get("hack", "max_range", 1000.f)) continue;
			
			DWORD hash = 0;
			if (!get_model_hash(ped, &hash)) continue;
			if (!game::isValidPlayer(hash, ped)) continue;
			if (!has_current_aim_target_data(data)) continue;

			Vector3 aimpos = config::get("aimbot", "aim_bone", 0) == 0 ? data.bones.HEAD : config::get("aimbot", "aim_bone", 0) == 1 ? data.bones.NECK : data.bones.SPINE2;

			if (config::get("aimbot", "aim_bone", 0) == 3) {
				aimpos = get_aimbone_complex(aimpos, data);
			}
			
			if (!IsBoneDataValid(aimpos)) continue;

			if (data.group.ally) continue;
			if (should_ignore_marked_target(data)) continue;
			if (config::get("aimbot", "visible_only", 0) && !has_clear_los_to_ped(ped)) continue;

			ImVec2 aim2d;
			if (!WorldToScreen(aimpos, &aim2d)) continue;

			float Distance2d = screen_distance(Game.screen.x / 2, Game.screen.y / 2, aim2d.x, aim2d.y);

			if (
				(Distance2d > config::get("aimbot", "aim_stop_radius", 0.f)) &&
				(config::get("aimbot", "aim_radius", 50.f) > Distance2d)
				) {
				if ((aimTargetDist > Distance2d) || (current_target == ped)) {
					aimTargetDist = Distance2d;
					target_accquired = true;
					closest = ped;
					closest_data = data;
				}
			}
		}

		if (closest) {
			auto now = std::chrono::high_resolution_clock::now();
			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - targetTimeout).count();

			if (closest != current_target) {
				if (dur > config::get("aimbot", "target_timeout", 0.f)) {
					current_target = closest;
					current_target_data = closest_data;
					targetTimeout = std::chrono::high_resolution_clock::now();
					return;
				}
			}
			if (current_target == 0) {
				if (dur > config::get("aimbot", "target_timeout", 0.f)) {
					current_target = closest;
					current_target_data = closest_data;
					targetTimeout = std::chrono::high_resolution_clock::now();
					return;
				}
			}
		}

		if (!target_accquired) {
			current_target = 0;
			aimTargetDist = 9999.f;
			target_aim_angle = { 0 };
		}
	}

inline bool run_damager(Vector3 aim_pos, Vector3 damager_aim_pos, bool autoFire) { (void)aim_pos; (void)damager_aim_pos; (void)autoFire; return false; }
void aimbot_sync_execution() {
	}

	static void tick_vector_aim_impl() {
		if (!Game.renderReady || Game.screen.x < 100) return;
		CPlayerAngles* cam = Game.getCam();
		if (!bind_state::is_active(k_vector_aim_bind)) {
			std::lock_guard<std::mutex> lock(target_mutex);
			vector_target = 0;
			return;
		}
		if (!IsValidPtr(cam) || !IsValidPtr(local.player)) return;

		float vector_radius = get_aim_config_float("vector_fov", config::get("aimbot", "vector_radius", 50.f));

		__try {
			get_vector_target(cam, vector_radius);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return;
		}

		if (!IsValidPtr(vector_target)) {
			return;
		}

		__try {
			DataPed data = {};
			if (!refresh_current_aim_target_data(vector_target, &data)) {
				std::lock_guard<std::mutex> lock(target_mutex);
				vector_target = 0;
				vector_aimTargetDist = 99999.f;
				return;
			}

			Vector3 lp_pos = { 0, 0, 0 };
			Vector3 vt_pos = { 0, 0, 0 };
			if (!get_position(local.player, &lp_pos) || !get_position(vector_target, &vt_pos)) return;
			if (get_distance(lp_pos, vt_pos) > config::get("hack", "max_range", 1000.f)) return;

			Vector3 aim3d = get_aimbone_for_vector(data);
			if (!IsBoneDataValid(aim3d)) return;

			ImVec2 aim2d;
			if (!WorldToScreen(aim3d, &aim2d)) return;
			float Distance2d = screen_distance(Game.screen.x / 2, Game.screen.y / 2, aim2d.x, aim2d.y);
			if (!(Distance2d > config::get("aimbot", "aim_stop_radius", 0.f) && vector_radius > Distance2d)) return;

			Vector3 Crosshair3D = cam->m_cam_pos;
			if (!std::isfinite(Crosshair3D.x) || !std::isfinite(Crosshair3D.y) || !std::isfinite(Crosshair3D.z)) return;

			float dist = get_distance(Crosshair3D, aim3d);
			if (!std::isfinite(dist) || dist <= 0.001f || dist > 100000.f) return;

			Vector3 Out((aim3d.x - Crosshair3D.x) / dist, (aim3d.y - Crosshair3D.y) / dist, (aim3d.z - Crosshair3D.z) / dist);
			if (!std::isfinite(Out.x) || !std::isfinite(Out.y) || !std::isfinite(Out.z)) return;
			if (fabsf(Out.x) > 2.f || fabsf(Out.y) > 2.f || fabsf(Out.z) > 2.f) return;

			if (!IsValidPtr(cam->m_cam_data)) return;

			const float smoothness = std::clamp(get_aim_config_float("vector_smoothness", 50.f), 0.f, 100.f);
			const float divisor = 1.f + smoothness;

			cam->m_tps_angles.x += (Out.x - cam->m_tps_angles.x) / divisor;
			cam->m_tps_angles.y += (Out.y - cam->m_tps_angles.y) / divisor;
			cam->m_tps_angles.z += (Out.z - cam->m_tps_angles.z) / divisor;

			cam->m_fps_angles.x += (Out.x - cam->m_fps_angles.x) / divisor;
			cam->m_fps_angles.y += (Out.y - cam->m_fps_angles.y) / divisor;
			cam->m_fps_angles.z += (Out.z - cam->m_fps_angles.z) / divisor;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			std::lock_guard<std::mutex> lock(target_mutex);
			vector_target = 0;
			vector_aimTargetDist = 99999.f;
			return;
		}
		return;
	}

	void tick_vector_aim() {
		__try {
			tick_vector_aim_impl();
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
		}
	}


	void tick() {
		tick_vector_aim();
	}

	void shutdown() {
		// nop
	}

	void init() {
		runtime_debug::last_section = "aimbot_init";
	}
}