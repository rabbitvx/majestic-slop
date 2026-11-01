#pragma once
#include "core/imports.h"
#include "weapon_icons.hpp"
#include "weapons_highlight.hpp"
#include "object_hash_registry.hpp"
#include "config/bind_state.hpp"
#include "network/ws_bridge.hpp"
#include <set>
#include <unordered_map>
#include <TlHelp32.h>
#include <Psapi.h>
#include "esp/weapon_arrow_weapons.hpp"
#include "esp/overlay.hpp"
#pragma comment(lib, "psapi.lib")

struct PedCache {
	float hp; float maxHp; float armor; float alpha;
	Vector3 pos;
	bool valid;
};
inline bool read_ped_cache(CObject* ped, PedCache* out) {
	if (!IsValidPtr(ped)) return false;
	out->hp = ped->HP;
	out->maxHp = ped->MaxHP;
	out->armor = ped->GetArmor();
	out->alpha = ped->GetAlpha();
	out->pos = ped->fPosition;
	out->valid = true;
	return true;
}
inline bool read_entity_pos(CObject* ent, Vector3* outPos) {
	if (!IsValidPtr(ent)) return false;
	*outPos = ent->fPosition;
	return true;
}
inline uint32_t read_dword(uintptr_t addr) {
	return *reinterpret_cast<uint32_t*>(addr);
}
#undef min
#undef max

#define M_PI 3.14159265358979323846
#ifndef PI
#define PI 3.14159265358979323846
#endif
#define M_PI_2 1.57079632679489661923
namespace game {
	extern vector<pair<CObject*, DataPed>> ped_list;
	extern std::mutex ped_list_mutex;
	extern vector<CObject*> object_list;
	extern vector<CVehicle*> vehicle_list;

	extern bool isValidPlayer(DWORD hash, CObject* player);
}

namespace esp {
#include "esp/weapon_icon_map.hpp"
#include "esp/player.hpp"
#include "esp/radar.hpp"
#include "esp/alerts.hpp"
#include "esp/render.hpp"
}
