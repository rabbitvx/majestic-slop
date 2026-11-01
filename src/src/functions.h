#pragma once
#include "core/imports.h"

typedef DWORD(__cdecl* start_shape_test_capsule_t)(Vector3 From, Vector3 To, float radius, IntersectOptions flags, DWORD entity, int p9);
start_shape_test_capsule_t start_shape_test_capsule = 0;

typedef int(__cdecl* get_raycast_result_t)(DWORD Handle, bool* hit, Vector3* endCoords, Vector3* surfaceNormal, DWORD* entityHit);
get_raycast_result_t get_raycast_result = 0;

typedef void(__cdecl* give_weapon_delayed_t)(int32_t ped, uintptr_t hash, int ammo, bool equip_now);
give_weapon_delayed_t give_weapon_delayed = 0;
typedef int32_t(__cdecl* pointer_to_handle_t)(intptr_t pointer);
pointer_to_handle_t pointer_to_handle = 0;

typedef void(__cdecl* disable_all_controlls_t)(unsigned int index);
disable_all_controlls_t disable_all_controlls;

typedef bool(__cdecl* clear_ped_task_t) (int32_t ped);
clear_ped_task_t clear_ped_task;
typedef bool(__fastcall* clear_ped_task_immediatly_t) (int32_t ped);
clear_ped_task_immediatly_t clear_ped_task_immediatly;

typedef void(__fastcall* set_ped_infinite_ammo_clip_t)(int32_t ped, BOOL toggle);
set_ped_infinite_ammo_clip_t set_ped_infinite_ammo_clip = 0;

float SquareRootFloat(float number) {
	long i;
	float x, y;
	const float f = 1.5F;

	x = number * 0.5F;
	y = number;
	i = *(long*)&y;
	i = 0x5f3759df - (i >> 1);
	y = *(float*)&i;
	y = y * (f - (x * y * y));
	y = y * (f - (x * y * y));
	return number * y;
}

float get_distance(Vector3 to, Vector3 from) {
	return (SquareRootFloat(
		((to.x - from.x) * (to.x - from.x)) +
		((to.y - from.y) * (to.y - from.y)) +
		((to.z - from.z) * (to.z - from.z))
	));
}

float screen_distance(float Xx, float Yy, float xX, float yY) {
	return SquareRootFloat((yY - Yy) * (yY - Yy) + (xX - Xx) * (xX - Xx));
}

static auto rage_joaat = [](const char* str) -> std::int32_t {
	static auto to_lowercase = [](char c) -> char {
		return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
	};

	std::uint32_t hash = 0;
	while (*str) {
		hash += to_lowercase(*str++);
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return (signed)hash;
};


typedef BOOLEAN(__cdecl* WorldToScreen_t)(Vector3* WorldPos, float* x, float* y);
WorldToScreen_t WorldToScreenAddr = 0;

bool WorldToScreen(Vector3 WorldPos, ImVec2* screen) {
	if (IsValidPtr(WorldToScreenAddr)) {
		ImVec2 pos;
		if (static_cast<BOOLEAN>(WorldToScreenAddr(&WorldPos, &pos.x, &pos.y))) {
			screen->x = pos.x * ImGui::GetIO().DisplaySize.x;
			screen->y = pos.y * ImGui::GetIO().DisplaySize.y;

			return true;
		}
	}
	return false;
}

struct WorldToScreenSnapshot {
	float matrix[16];
	ImVec2 display_size;
	bool valid;
};

bool CaptureWorldToScreenSnapshot(WorldToScreenSnapshot* out) {
	if (!out || !IsValidPtr(Game.viewPort)) return false;

	const ImVec2 display_size = ImGui::GetIO().DisplaySize;
	if (display_size.x <= 0.f || display_size.y <= 0.f) return false;

	const float* source = Game.viewPort->fViewMatrix;
	for (int i = 0; i < 16; ++i) {
		if (!std::isfinite(source[i])) return false;
		out->matrix[i] = source[i];
	}

	out->display_size = display_size;
	out->valid = true;
	return true;
}

bool WorldToScreenMatrix(const WorldToScreenSnapshot& view, Vector3 pos, ImVec2* out) {
	if (!out || !view.valid) return false;

	const float* m = view.matrix;
	const float w = (m[3] * pos.x) + (m[7] * pos.y) + (m[11] * pos.z) + m[15];
	if (w < 0.001f) return false;

	const float screen_x = ((m[1] * pos.x) + (m[5] * pos.y) + (m[9] * pos.z) + m[13]) / w;
	const float screen_y = ((m[2] * pos.x) + (m[6] * pos.y) + (m[10] * pos.z) + m[14]) / w;

	const ImVec2 display_size = view.display_size;
	out->x = (display_size.x * 0.5f) + ((screen_x * display_size.x) * 0.5f);
	out->y = (display_size.y * 0.5f) - ((screen_y * display_size.y) * 0.5f);
	return true;
}

bool WorldToScreenMatrix(Vector3 pos, ImVec2* out) {
	WorldToScreenSnapshot view{};
	if (!CaptureWorldToScreenSnapshot(&view)) return false;
	return WorldToScreenMatrix(view, pos, out);
}

bool isW2SValid(ImVec2 coords) {
	return (coords.x > 1.0f && coords.y > 1.0f);
}

bool WorldToScreen2(Vector3 pos, ImVec2* out) {
	if (!IsValidPtr(Game.viewPort)) return false;
	Vector3	tmp;

	tmp.x = (Game.viewPort->fViewMatrix[1] * pos.x) + (Game.viewPort->fViewMatrix[5] * pos.y) + (Game.viewPort->fViewMatrix[9] * pos.z) + Game.viewPort->fViewMatrix[13];
	tmp.y = (Game.viewPort->fViewMatrix[2] * pos.x) + (Game.viewPort->fViewMatrix[6] * pos.y) + (Game.viewPort->fViewMatrix[10] * pos.z) + Game.viewPort->fViewMatrix[14];
	tmp.z = (Game.viewPort->fViewMatrix[3] * pos.x) + (Game.viewPort->fViewMatrix[7] * pos.y) + (Game.viewPort->fViewMatrix[11] * pos.z) + Game.viewPort->fViewMatrix[15];

	if (tmp.z < 0.001f)
		return false;

	tmp.z = 1.0f / tmp.z;

	tmp.x *= tmp.z;
	tmp.y *= tmp.z;

	int w = ImGui::GetIO().DisplaySize.x;
	int h = ImGui::GetIO().DisplaySize.y;

	out->x = ((w / 2.f) + (.5f * tmp.x * w + 1.f));
	out->y = ((h / 2.f) - (.5f * tmp.y * h + 1.f));

	return true;
}


typedef struct D3DXVECTOR4 {
	FLOAT x;
	FLOAT y;
	FLOAT z;
	FLOAT w;
} D3DXVECTOR4;

typedef void* (__fastcall* GetBoneFromMask2)(CObject* pThis, D3DXVECTOR4& vBonePos, WORD dwMask);
GetBoneFromMask2 GetBoneFunc;

inline bool get_bone_position_internal(CObject* pThis, const int32_t wMask, D3DXVECTOR4* outVec) {
	if (!GetBoneFunc) return false;
	if (!pThis) return false;
	if (!outVec) return false;
	
	if (IsBadReadPtr(pThis, sizeof(void*))) return false;
	
	__try {
		outVec->x = 0.0f;
		outVec->y = 0.0f;
		outVec->z = 0.0f;
		outVec->w = 0.0f;
		
		GetBoneFunc(pThis, *outVec, static_cast<WORD>(wMask));
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

Vector3 GetBonePosition(CObject* pThis, const int32_t wMask) {
	if (!pThis) return Vector3();
	if (IsBadReadPtr(pThis, sizeof(void*))) return Vector3();
	if (!GetBoneFunc) return Vector3();
	
	D3DXVECTOR4 tempVec4 = { 0, 0, 0, 0 };
	
	if (get_bone_position_internal(pThis, wMask, &tempVec4)) {
		if (std::isfinite(tempVec4.x) && std::isfinite(tempVec4.y) && std::isfinite(tempVec4.z)) {
			return Vector3(tempVec4.x, tempVec4.y, tempVec4.z);
		}
	}
	
	return Vector3();
}

Vector3 GetBonePosition(CObject* pThis, Bones bone) {
	return GetBonePosition(pThis, (WORD)bone);
}


CPedStyle* GetPedStyle(CObject* ped) {
	__try {
		if (!IsValidPtr(ped)) return nullptr;
		CPedStyle* style = ped->pCPedStyle;
		return IsValidPtr(style) ? style : nullptr;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return nullptr;
	}
}

int GetPedDrawableVariation(CObject* ped, int group) {
	if (group < 0 || group >= 12) return -1;
	__try {
		CPedStyle* style = GetPedStyle(ped);
		if (!style) return -1;
		const BYTE value = style->propIndex[group];
		return value != 255 ? static_cast<int>(value) : -1;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return -1;
	}
}

int GetPedTextureVariation(CObject* ped, int group) {
	if (group < 0 || group >= 12) return -1;
	__try {
		CPedStyle* style = GetPedStyle(ped);
		if (!style) return -1;
		const BYTE value = style->textureIndex[group];
		return value != 255 ? static_cast<int>(value) : -1;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return -1;
	}
}

int GetPedPaletteVariation(CObject* ped, int group) {
	if (group < 0 || group >= 12) return -1;
	__try {
		CPedStyle* style = GetPedStyle(ped);
		if (!style) return -1;
		const BYTE value = style->paletteIndex[group];
		return value != 255 ? static_cast<int>(value) : -1;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return -1;
	}
}

string GetPedComponentHash(CObject* ped) {
	string r;
	if (!GetPedStyle(ped)) return r;
	for (int i = 0; i < 12; i++) {
		int c = GetPedTextureVariation(ped, i);
		if (c > -1) r += to_string(c);
	}
	for (int i = 0; i < 12; i++) {
		int c = GetPedPaletteVariation(ped, i);
		if (c > -1) r += to_string(c);
	}
	return r;
}

typedef bool(__stdcall* Is_Dlc_Present_t) (std::uint64_t hash, bool a2);
Is_Dlc_Present_t pIs_Dlc_Present = NULL;
Is_Dlc_Present_t Is_Dlc_Present;

