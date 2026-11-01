#pragma once
#include "core/imports.h"
#include "config/bind_state.hpp"
#include <cmath>
#include <algorithm>
#include <chrono>
#include "imgui.h"

#undef min
#undef max

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef PI
#define PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif







static const bind_state::bind_config k_veh_fast_stop_bind {
    "hacks",
    "veh_fast_stop_enabled",
    "veh_fast_stop_key",
    "veh_fast_stop_key",
    "veh_fast_stop_key_mode",
    "veh_fast_stop_mode"
};



using Hash = uint32_t;
namespace game {
    extern vector<pair<CObject*, DataPed>> ped_list;
    extern vector<CObject*> object_list;
    extern vector<CVehicle*> vehicle_list;
    extern bool isValidPlayer(DWORD hash, CObject* player);
    extern DWORD freemode_f;
    extern DWORD freemode_m;
    extern vector<CVehicle*> nearby_vehicle_list;
}
Vector3 lerp(const Vector3& a, const Vector3& b, float t) {
    return Vector3{
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}