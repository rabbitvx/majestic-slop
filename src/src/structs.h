#pragma once

#include <Windows.h>
#include <stdio.h> 
#include <string> 
#include <sstream> 
#include <iostream>
#include <time.h>       

#include <dwmapi.h> 
#include <TlHelp32.h>

#pragma warning ( disable: 4102 ) 
#pragma warning ( disable: 4311 ) 
#pragma warning ( disable: 4312 ) 

#pragma warning ( disable: 4244 ) 
#pragma warning ( disable: 4996 ) 

bool isFiveM = false;

#ifndef IsValidPtr
#define IsValidPtr(x) ((x) != 0 && !IsBadReadPtr((void*)(x), sizeof(void*)))
#endif

#include "simple_math.h"
using namespace DirectX::SimpleMath;
using namespace std;

#pragma endregion

#pragma pack(push, 1)
struct PVector3
{
public:
	PVector3() = default;

	PVector3(float x, float y, float z) :
		x(x), y(y), z(z) {
	}
public:
	float x{};
private:
	char m_padding1[0x04]{};
public:
	float y{};
private:
	char m_padding2[0x04]{};
public:
	float z{};
private:
	char m_padding3[0x04]{};
};

#pragma pack(pop)

enum Bones {
	HEAD = 0x796E,
	NECK = 0x9995,
	RIGHT_HAND = 0xDEAD,
	RIGHT_FOREARM = 0x6E5C,
	RIGHT_UPPER_ARM = 0x9D4D,
	RIGHT_CLAVICLE = 0x29D2,

	LEFT_HAND = 0x49D9,
	LEFT_FOREARM = 0xEEEB,
	LEFT_UPPER_ARM = 0xB1C5,
	LEFT_CLAVICLE = 0xFCD9,

	PELVIS = 0x2E28,
	SPINE_ROOT = 0xE0FD,
	SPINE0 = 0x5C01,
	SPINE1 = 0x60F0,
	SPINE2 = 0x60F1,
	SPINE3 = 0x60F2,

	RIGHT_TOE = 0x512D,
	RIGHT_FOOT = 0xCC4D,
	RIGHT_CALF = 0x9000,
	RIGHT_THIGH = 0xCA72,
	LEFT_TOE = 0x083C,
	LEFT_FOOT = 0x3779,
	LEFT_CALF = 0xF9BB,
	LEFT_THIGH = 0xE39F
};

static std::wstring asciiDecode(const std::string& s) {
	std::wostringstream    w;
	wchar_t                c;

	for (size_t i = 0; i < s.length(); i++) {
		mbtowc(&c, &s[i], 1);
		w << c;
	}
	return w.str();
}

enum class thread_state_t: std::uint32_t
{
	idle,
	running,
	killed,
	unk_3,
	unk_4,
};

class thread_context_t
{
public:
	std::uint32_t m_thread_id;
	std::uint32_t m_script_hash;
	thread_state_t m_state;
	std::uint32_t m_instruction_pointer;
	std::uint32_t m_frame_pointer;
	std::uint32_t m_stack_pointer;
	float m_timer_a;
	float m_timer_b;
	float m_timer_c;
	char m_padding1[0x2C];
	std::uint32_t m_stack_size;
	char m_padding2[0x54];
};

class game_thread
{
public:
	virtual ~game_thread() = default;
	virtual thread_state_t reset(std::uint32_t script_hash, void* args, std::uint32_t arg_count) = 0;
	virtual thread_state_t run() = 0;
	virtual thread_state_t tick() = 0;
	virtual void kill() = 0;

	inline static game_thread*& get() {
		uintptr_t tlsBase = *reinterpret_cast<uintptr_t*>(__readgsqword(0x58));
		if (!tlsBase) {
			static game_thread* nullThread = 0;
			return nullThread;
		}
		
		uintptr_t threadPtrAddr = tlsBase + 0x830;
		
		if (IsBadReadPtr(reinterpret_cast<void*>(threadPtrAddr), sizeof(void*))) {
			static game_thread* nullThread = 0;
			return nullThread;
		}
		
		return *(game_thread**)(threadPtrAddr);
	}
public:
	thread_context_t m_context;
	void* m_stack;
	char m_padding[0x10];
	const char* m_exit_message;
	char m_name[0x40];
	void* m_handler;
	char m_padding2[0x28];
	std::uint8_t m_flag1;
	std::uint8_t m_flag2;
	char m_padding3[0x16];
};

enum game_state_t
{
	playing,
	intro,
	unk,
	license,
	main_menu,
	load_online
};

typedef __int64(__fastcall* gta_event_gun_shot_t)(
	__int64 a1,
	__int64 a2,
	float* a3,
	float* a4,
	unsigned int a5,
	unsigned int a6,
	unsigned int a7,
	int a8,
	char a9,
	int a10,
	char a11,
	char a12
	);
gta_event_gun_shot_t gta_event_gun_shot = 0;
gta_event_gun_shot_t original_gta_event_gun_shot = 0;
uintptr_t gta_get_is_accurate = 0;

typedef __int64(__fastcall* sBulletInstance)(
	__int64* sBulletInstance_1,
	const __int64* pWeaponInfo,
	Vector3* vStart,
	Vector3* vEnd,
	float fVelocity,
	unsigned int weaponHash,
	bool createsTrace,
	bool isAccurate
	);
sBulletInstance gta_bullet_instance = 0;
sBulletInstance original_bullet_instance = 0;

using script_thread_tick_t = uintptr_t(*)(game_thread* thread, int ops_to_execute);
script_thread_tick_t gta_script_thread_tick = 0;
script_thread_tick_t original_native_thread = 0;

using get_native_handler_t = void* (*)(uintptr_t* table, uint64_t hash);
get_native_handler_t gta_get_native_handler;

using ragemp_fetch_handler_t = uintptr_t(*)(uintptr_t hash);
ragemp_fetch_handler_t ragemp_fetch_handler;
ragemp_fetch_handler_t orig_ragemp_fetch_handler;

uintptr_t* gta_native_registration_table = 0;

using fix_context_vector_t = void(*)(void* context);
fix_context_vector_t gta_fix_context_vector;

struct CVector3 {
public:
	union {
		struct {
			float x;
			float y;
			float z;
		};
		float m[3];
	};
};

#pragma pack(push, 1)
struct screenReso {
	uint32_t w, h;
};

class CViewPort {
public:
	char _0x0000[0x24C];
	float fViewMatrix[0x10];
};
#pragma pack(pop)
static CViewPort* m_viewPort;

class GameViewMatrices;
class CPlayerInfo;
class CPlayers;
class CInventory;
class CWeaponManager;
class CObjectWrapper;
class CObject;
class CVehicleManager;
class CAttacker;
class CVehicleHandling;
class CVehicleColorOptions;
class CVehicleColors;
class CBoneManager;

class CObjectNavigation {
public:
	char pad_0[0x20];
	Vector4 Rotation;
	char pad_1[0x20];
	Vector3 Position;

	void setPosition(Vector3 p) {
		*(float*)(((DWORD64)this + 0x50)) = p.x;
		*(float*)(((DWORD64)this + 0x54)) = p.y;
		*(float*)(((DWORD64)this + 0x58)) = p.z;
	}
	void setRotation(Vector3 p) {
		*(float*)(((DWORD64)this + 0x30)) = p.x;
		*(float*)(((DWORD64)this + 0x34)) = p.y;
		*(float*)(((DWORD64)this + 0x38)) = p.z;
	}
};

class CEventGunShot
{
public:
	char pad_0x0000[0x40];
	Vector3 shoot_coord;
	char pad_0x004C[0x4];
	Vector3 bullet_impact;
	char pad_0x005C[0xC];
	__int32 weapon_hash;
	char pad_0x006C[0x8];

};

class CReplayInterfacePed {
private:
	class CPedList {
	private:
		struct Ped {
			CObject* ped;
			__int32 iHandle;
			char _pad0[0x4];
		};

	public:
		Ped peds[256];
	};

public:
	char _pad0[0x100];
	CPedList* ped_list;
	int max_peds;
	char _pad1[0x4];
	int number_of_peds;

	CObject* get_ped(const unsigned int& index) {
		return (index <= static_cast<unsigned int>(max_peds) && IsValidPtr(ped_list->peds[index].ped) ? ped_list->peds[index].ped : 0);
	}
};

class CReplayInterfaceVehicle {
private:
	class VehicleHandle {
	public:
		CVehicleManager* vehicle;
		__int32 handle;
		char _pad0[0x4];
	};

	class CVehicleList {
	public:
		VehicleHandle vehicles[0x100];
	};

public:
	char _0x0000[0x180];
	CVehicleList* vehicle_list;
	__int32 max_vehicles;
	char _0x010C[0x4];
	__int32 number_of_vehicles;
	char _0x0114[0x34];

	CVehicleManager* get_vehicle(const unsigned int& index) {
		return (index <= static_cast<unsigned int>(max_vehicles) && IsValidPtr(vehicle_list->vehicles[index].vehicle) ? vehicle_list->vehicles[index].vehicle : 0);
	}
};

class CPickup {
public:
	char pad_0x0000[0x30];
	CObjectNavigation* pCNavigation;
	char pad_0x0038[0x58];
	Vector3 v3VisualPos;
	char pad_0x009C[0x3F4];
	__int32 iValue;
	char pad_0x0494[0xC4];

};

class CPickupHandle {
public:
	CPickup* pCPickup;
	__int32 iHandle;
	char pad_0x000C[0x4];

};

class CPickupList {
public:
	CPickupHandle pickups[73];

};

class CPickupInterface {
public:
	char pad_0x0000[0x100];
	CPickupList* pCPickupList;
	__int32 iMaxPickups;
	char pad_0x010C[0x4];
	__int32 iCurPickups;

	CPickup* get_pickup(const int& index) {
		if (index < iMaxPickups)
			return pCPickupList->pickups[index].pCPickup;
		return 0;
	}
};

class CObjectInterface {
private:
	class CObjectHandle {
	public:
		CObject* pCObject;
		__int32 iHandle;
		char pad_0x000C[0x4];

	};

	class CObjectList {
	public:
		CObjectHandle ObjectList[2300];

	};

public:
	char pad_0x0000[0x158];
	CObjectList* pCObjectList;
	__int32 iMaxObjects;
	char pad_0x0164[0x4];
	__int32 iCurObjects;
	char pad_0x016C[0x5C];

	CObject* get_object(const unsigned int& index) {
		return (IsValidPtr(pCObjectList) && index < static_cast<unsigned int>(iMaxObjects) && IsValidPtr(pCObjectList->ObjectList[index].pCObject) ? pCObjectList->ObjectList[index].pCObject : 0);
	}
	bool Remove_object(const unsigned int& index) {
		if (IsValidPtr(pCObjectList) && index < static_cast<unsigned int>(iMaxObjects) && IsValidPtr(pCObjectList->ObjectList[index].pCObject) ? pCObjectList->ObjectList[index].pCObject : 0) {
			pCObjectList->ObjectList[index].iHandle = 0;
		}
		return true;
	}
	__int32 getHandle(const unsigned int& index) {
		if (IsValidPtr(pCObjectList) && index < static_cast<unsigned int>(iMaxObjects) && IsValidPtr(pCObjectList->ObjectList[index].pCObject) ? pCObjectList->ObjectList[index].pCObject : 0) {
			return pCObjectList->ObjectList[index].iHandle;
		}
		return NULL;
	}

};

class CReplayInterface {
public:
	void* game_interface;
	void* camera_interface;
	CReplayInterfaceVehicle* vehicle_interface;
	CReplayInterfacePed* ped_interface;
	CPickupInterface* pickup_interface;
	CObjectInterface* object_interface;
};

class CModelInfo {
public:
	char pad_0x0000[0x270];
	BYTE Type;
	char pad_0x0271[0xF];

	DWORD GetHash() {
		return (*(DWORD*)((DWORD64)this + 0x18));
	}

	char* GetName() {
		return (*reinterpret_cast<char**>((DWORD64)this + 0x1C8));
	}

};

class CVehicleManager {
public:
	char pad_0x0000[0x30];
	CObjectNavigation* _ObjectNavigation;
	char pad_0x0038[0x10];
	CVehicleColorOptions* CVehicleColorOptions;
	char pad_0x0050[0x40];
	Vector3 fPosition;
	char pad_0x009C[0xED];
	BYTE btGodMode;
	char pad_0x018A[0xF6];
	float fHealth;
	char pad_0x0284[0x1C];
	float fHealthMax;
	char pad_0x02A4[0x4];
	CAttacker* CAttacker;
	char pad_0x02B0[0x59C];
	float fHealth2;
	char pad_0x0850[0x28];
	CVehicleHandling* CVehicleHandling;
	char pad_0x0880[0x3];
	BYTE btBulletproofTires;
	char pad_0x0884[0x2F8];
	float fGravity;

	CModelInfo* ModelInfo() {
		return (*(CModelInfo**)((DWORD64)this + 0x20));
	}

	bool isGod() {
		return (btGodMode & 0x01) ? true : false;
	}

	bool hasBulletproofTires() {
		return (btBulletproofTires & 0x20) ? true : false;
	}

	float getGravity() {
		return *(float*)this + 0xC5C;
	}
	void setGravity(float v) {
		*(float*)(this + 0xC5C) = v;
	}

};

class CVehicleModelInfoArray {};
class CNavigation {};
class CVehicleDrawHandler {};
class CMatrix {};
class CHandlingData {};
class CPed1 {};
class CAttacker1 {};
class CVehicle_1032 {
public:
	char pad_0x0000[0x20];
	CVehicleModelInfoArray* VehicleModelInfoArrayContainer;
	unsigned char typeByte;
	char pad_0x0029[0x7];
	CNavigation* CNavigation;
	char pad_0x0038[0x10];
	CVehicleDrawHandler* CVehicleColorOptions;
	char pad_0x0050[0x10];
	CMatrix N000072BA;
	char pad_0x00A0[0x20];
	DWORD dwBitmapc0;
	char pad_0x00C4[0x14];
	__int8 N000072C5;
	char pad_0x00D9[0xB0];
	unsigned char btGodMode;
	char pad_0x018A[0xF6];
	float fHealth;
	char pad_0x0284[0x1C];
	float fHealthMax;
	char pad_0x02A4[0x4];
	CAttacker1* CAttacker;
	char pad_0x02B0[0x68];
	__int32 bRocketBoostEnabled;
	float fRocketBoostCharge;
	char pad_0x0320[0x440];
	Vector3 v3Velocity;
	float N00002422;
	float fRotationSpeedRoll;
	float fRotationSpeedPitch;
	float fRotationSpeedYaw;
	float N00006E17;
	char pad_0x0780[0x30];
	float fUnknownHealth;
	float fFuelTankHealth;
	float fFuelLevel;
	float fFireTimer;
	char pad_0x07C0[0x30];
	__int8 iGearNext;
	char pad_0x07F1[0x1];
	__int8 iGearCurrent;
	char pad_0x07F3[0x3];
	__int8 iGearTopGear;
	char pad_0x07F7[0x2D];
	float fCurrentRPM;
	float fCurrentRPM2;
	char pad_0x082C[0x4];
	float fClutch;
	float fThrottle;
	char pad_0x0838[0x8];
	__int32 UnknownFC40CBF7B90CA77C;
	char pad_0x0844[0x4];
	float fTurbo;
	float fSteeringAngleZERO;
	char pad_0x0850[0x8];
	float fVehicleCrashDowndown;
	float fHealth2;
	char pad_0x0860[0x28];
	CHandlingData* vehicleHandling;
	unsigned char vehicleType;
	unsigned char dw4or5;
	unsigned char bitmapCarInfo1;
	unsigned char bitmapBulletProofTires;
	char pad_0x0894[0x2];
	unsigned char bitmapUnknown2;
	char pad_0x0897[0x3];
	unsigned char bitmapJumpRelated;
	char pad_0x089B[0x1];
	__int8 bitmapOwnedByPlayer;
	char pad_0x089D[0xE];
	unsigned char bitmapUnknownVehicleType1;
	char pad_0x08AC[0x4C];
	float N00002483;
	float fWheelBias2;
	char pad_0x0900[0x4];
	float fWheelBias;
	char pad_0x0908[0x4];
	float fThrottlePosition;
	float fBreakPosition;
	unsigned char bHandbrake;
	char pad_0x0915[0x33];
	float fDirtLevel;
	char pad_0x094C[0x70];
	float fEngineTemp;
	char pad_0x09C0[0x34];
	DWORD dwCarAlarmLength;
	char pad_0x09F8[0x8];
	float fDashSpeed;
	char pad_0x0A04[0x6];
	unsigned char bUsedInB2E0C0D6922D31F2;
	char pad_0x0A0B[0x45];
	Vector3 vTravel;
	char pad_0x0A5C[0x44];
	float fVelocityX;
	float fVelocityY;
	float fVelocityZ;
	char pad_0x0AAC[0x6C];
	__int32 iVehicleType;
	char pad_0x0B1C[0x24];
	unsigned char btOpenableDoors;
	char pad_0x0B41[0x4B];
	float fGravity;
	char pad_0x0B90[0x8];
	CPed1 PedsInSeats[15];
	char pad_0x0C10[0x798];
	unsigned char ReducedGrip;
	unsigned char CreatesMoneyWhenExploded;
	char pad_0x13AA[0x3F6];
	float fSudoJumpValue;
	char pad_0x17A4[0x130];

};

class CVehicle {
public:
	char pad_0x0000[0x30];
	CObjectNavigation* ObjectNavigation;
	char pad_0x0038[0x10];
	void* pCVehicleMods;
	char pad_0x0050[0x40];
	Vector3 fPosition;
	char pad_0x009C[0x3C];
	BYTE btState;
	char pad_0x00D9[0xB0];
	BYTE btGodMode;
	char pad_0x018A[0xF6];
	float fHealth;
	float fHealthMax;
	char pad_0x02A4[0x4];
	CAttacker* pCAttacker;
	char pad_0x02B0[0x6A];
	BYTE btVolticRocketState;
	char pad_0x031B[0x1];
	float fVolticRocketEnergy;
	char pad_0x0320[0x430];
	Vector3 v3Velocity;
	char pad_0x075C[0xF0];
	float fHealth2;
	char pad_0x0850[0x28];
	CVehicleHandling* pVehicleHandling;
	char pad_0x0880[0x3];
	BYTE btBulletproofTires;
	char pad_0x0884[0x4];
	BYTE btStolen;
	char pad_0x0889[0x11];
	BYTE N00000954;
	char pad_0x089B[0x41];
	float N0000081E;
	char pad_0x08E0[0x58];
	float fDirtLevel;
	char pad_0x093C[0xA8];
	DWORD dwCarAlarmLength;
	char pad_0x09E8[0x148];
	BYTE btOpenableDoors;
	char pad_0x0B31[0x4B];
	float fGravity;
	BYTE btMaxPassengers;
	char pad_0x0B81[0x1];
	BYTE btNumOfPassengers;
	char pad_0x0B83[0x3D];

	bool isGod() {
		return (btGodMode & 0x01) ? true : false;
	}

	bool hasBulletproofTires() {
		return (btBulletproofTires & 0x20) ? true : false;
	}

	bool disabledDoors() {
		return (btOpenableDoors == 0) ? true : false;
	}

	void openDoors() {
		btOpenableDoors = 1;
	}

	void giveHealth() {
		if (fHealth < fHealthMax)
			fHealth = fHealthMax;
		if (fHealth2 < fHealthMax)
			fHealth2 = fHealthMax;
	}

	bool isVehicleDestroyed() {
		return (btState & 7) == 3;
	}

	void set_stolen(bool toggle) {
		this->btStolen &= 0xFEu;
		this->btStolen |= toggle & 1;
	}

	CModelInfo* ModelInfo() {
		return (*(CModelInfo**)((DWORD64)this + 0x20));
	}

	void setUnlocked(BYTE n) {
		*(BYTE*)((DWORD64)this + 0x1370) = n;
	}

	void setEngine(BYTE n) {
		*(BYTE*)((DWORD64)this + 0x92A) = n;
	}

	float getGravity() {
		return *(float*)((DWORD64)this + 0x0C3C);

	}
	void setGravity(float v) {
		*(float*)((DWORD64)this + 0x0C3C) = v;
	}

	CVehicleHandling* handling() {
		return *(CVehicleHandling**)((uintptr_t)this + 0x0918);
	}

	void setCollision(float v) {
		uintptr_t* value = *(uintptr_t**)((uintptr_t)this + 0x30);
		if (!IsValidPtr(value)) return;
		uintptr_t* value1 = *(uintptr_t**)((uintptr_t)value + 0x10);
		if (!IsValidPtr(value1)) return;
		uintptr_t* value2 = *(uintptr_t**)((uintptr_t)value1 + 0x20);
		if (!IsValidPtr(value2)) return;
		uintptr_t* value3 = *(uintptr_t**)((uintptr_t)value2 + 0x70);
		if (!IsValidPtr(value3)) return;
		uintptr_t* value4 = *(uintptr_t**)((uintptr_t)value3 + 0x0);
		if (!IsValidPtr(value4)) return;
		
		*(float*)((uintptr_t)value4 + 0x2C) = v;
	}
	
	float getCollision() {
		uintptr_t* value = *(uintptr_t**)((uintptr_t)this + 0x30);
		if (!IsValidPtr(value)) return 0.25f;
		uintptr_t* value1 = *(uintptr_t**)((uintptr_t)value + 0x10);
		if (!IsValidPtr(value1)) return 0.25f;
		uintptr_t* value2 = *(uintptr_t**)((uintptr_t)value1 + 0x20);
		if (!IsValidPtr(value2)) return 0.25f;
		uintptr_t* value3 = *(uintptr_t**)((uintptr_t)value2 + 0x70);
		if (!IsValidPtr(value3)) return 0.25f;
		uintptr_t* value4 = *(uintptr_t**)((uintptr_t)value3 + 0x0);
		if (!IsValidPtr(value4)) return 0.25f;
		
		return *(float*)((uintptr_t)value4 + 0x2C);
	}

};

class CWeapon {
public:
	char pad_0x0000[0x10];
	DWORD dwNameHash;
	DWORD dwModelHash;
	char pad_0x0018[0x24];
	DWORD dwAmmoType;
	DWORD dwWeaponWheelSlot;
	char pad_0x0044[0x4];
	void* CAmmoInfo;
	char pad_0x0050[0xC];
	float fSpread;
	char pad_0x0060[0x38];
	float fBulletDamage;
	char pad_0x009C[0x1C];
	float fForce;
	float fForcePed;
	float fForceVehicle;
	float fForceFlying;
	char pad_0x00C8[0x34];
	float fMuzzleVelocity;
	DWORD dwBulletInBatch;
	float fBatchSpread;
};

class CWeaponInfo {
public:
	char pad_0000[96];
	void* _AmmoInfo;
	char pad_0068[12];
	float Spread;
	char pad_0078[56];
	float Damage;
	char pad_0x00B4[36];
	float MinImpulse;
	float MaxImpulse;
	char pad_00E0[52];
	float Velocity;
	int32_t BulitsPerShoot;
	float MultiBulitSpread;
	char pad_0120[12];
	float ReloadTimeMultiplier;
	char pad_0130[4];
	float TimeToShoot;
	char pad_0138[404];
	char pad_02D0[8];
	float RecoilThirdPerson;
	char pad_02DC[772];
	char* szWeaponName;
	char pad_05E8[40];

	void Range(float value) {
		*(float*)((DWORD64)this + 0x28C) = value;
		*(float*)((DWORD64)this + 0x29C) = value;
	}
	float getRange() {
		return *(float*)((DWORD64)this + 0x29C);
	}
	void setRecoil(float value) {
		if (isFiveM) {
			*(float*)((DWORD64)this + 0x2E8) = value;
			return;
		}
		*(float*)((DWORD64)this + 0x2F4) = value;
	}
	float getRecoil() {
		return *(float*)((DWORD64)this + 0x2F4);
	}
	void setSpread(float value) {
		*(float*)((DWORD64)this + 0x84) = value;
	}
	float getSpread() {
		return *(float*)((DWORD64)this + 0x84);
	}
	void setTimeToShoot(float value) {
		*(float*)((DWORD64)this + 0x13C) = value;
	}
	float getTimeToShoot() {
		return *(float*)((DWORD64)this + 0x13C);
	}
	void SuperImpulse() {
		*(float*)((DWORD64)this + 0xD8) = 9999.0f;
		*(float*)((DWORD64)this + 0xDC) = 9999.0f;
	}
	void setDamage(float dmg) {
		*(float*)((DWORD64)this + 0xB0) = dmg;
	}

	void setReloadSpeed(float speed) {
		*(float*)((DWORD64)this + 0x134) = speed;
		*(float*)((DWORD64)this + 0x130) = speed;
	}

	char* GetSzWeaponName() {
		return (*reinterpret_cast<char**>((DWORD64)this + 0x5F0));
	}

};
static CWeaponInfo* m_pCWeaponInfo = 0;
static CWeaponInfo m_CWeapon;

class CWeaponManager {
public:
	char pad_0000[32];
	CWeaponInfo* _WeaponInfo;
	char pad_0028[96];
	void* _CurrentWeapon;
	char pad_0090[272];
	Vector3 Aiming_at_Cord;
	char pad_01AC[4];
	Vector3 Last_Impact_Cord;
	char pad_01BC[44];
};

class CPedStyle {
public:
	char _0x0000[48];
	void* N4100AE4A;
	void* pColors;
	char _0x0040[168];
	BYTE propIndex[12];
	char _0x00F4[12];
	BYTE textureIndex[12];
	BYTE paletteIndex[12];
	char _0x010C[9];
	BYTE N4100F70F;
	BYTE N4134BE09;
	BYTE N4134C676;
	BYTE btEars;
	char _0x0125[3];
	BYTE btEarsTexture;
	char _0x0129[72];
	BYTE N4100F719;
	BYTE N413811BF;
	BYTE N41381E8E;
	BYTE btHats;
	char _0x0175[3];
	BYTE btHatsTexture;
	BYTE N4100F71A;
	BYTE N4461C523;
	BYTE N4461D118;
	BYTE N4461C524;
	BYTE N445D0F2B;
	BYTE N445D279D;
	BYTE N445D3102;
	BYTE N445D279E;
	BYTE N4100F71B;
	BYTE N445D522D;
	BYTE N445D5D09;
	char _0x0184[61];
	BYTE N4100F723;
	BYTE N4132EABE;
	BYTE N4132F80B;
	BYTE btGlasses;
	char _0x01C5[3];
	BYTE btGlassesTexture;
};

Vector3 LastPosition[1028];

struct Groups {
	bool inGroup = false;
	bool ally = false;
	string name = "\0";

	RGBA color = RGBA(0, 0, 0, 255);
};

struct PlayerBones {
	Vector3 HEAD;
	Vector3 NECK;

	Vector3 RIGHT_HAND;
	Vector3 RIGHT_FOREARM;
	Vector3 RIGHT_UPPER_ARM;
	Vector3 RIGHT_CLAVICLE;

	Vector3 LEFT_HAND;
	Vector3 LEFT_FOREARM;
	Vector3 LEFT_UPPER_ARM;
	Vector3 LEFT_CLAVICLE;

	Vector3 PELVIS;
	Vector3 SPINE_ROOT;
	Vector3 SPINE0;
	Vector3 SPINE1;
	Vector3 SPINE2;
	Vector3 SPINE3;

	Vector3 RIGHT_TOE;
	Vector3	RIGHT_FOOT;
	Vector3	RIGHT_CALF;
	Vector3	RIGHT_THIGH;

	Vector3	LEFT_TOE;
	Vector3	LEFT_FOOT;
	Vector3	LEFT_CALF;
	Vector3	LEFT_THIGH;
};

struct DataPed {
	int visible;
	int index;

	PlayerBones bones;
	Groups group;
	char name[32];
	char altv_nick[64];
	char altv_social[64];
	int altv_static_id;
	int altv_dynamic_id;
	char altv_fraction[32];
	int altv_fraction_id;
	int altv_family_id;
	int altv_leader_id;
	int altv_level;
	int altv_admin_level;
	float altv_hp;
	float altv_armor;
	DWORD altv_weapon_hash;
	bool altv_is_admin;
	bool altv_is_dead;
	bool altv_is_media;
	bool altv_is_tester;
	bool altv_is_afk;
	bool altv_is_cuffed;
	bool altv_has_wear_armor;
	int altv_wanted_level;
	bool has_ws_data;
	bool has_current_ws_data;
	bool is_friend = false;
};

class CObject {
public:
	char pad_0x0000[0x2C];
	BYTE btInvisibleSP;
	char pad_0x002D[0x1];
	BYTE btFreezeMomentum;
	char pad_0x002F[0x1];

	CObjectNavigation* ObjectNavigation;
	char pax_0x0038[0x10];
	CPedStyle* pCPedStyle;
	char pad_0x0038[0x40];
	Vector3 fPosition;

	char pad_0x009C[0xED];
	BYTE godmode;
	char pad_0x018A[0xF6];
	float HP;
	float MaxHP;
	char pad_0x02A4[0x4];
	CAttacker* CAttacker;
	char pad_0x02B0[0x70];
	Vector3 v3Velocity;
	char pad_0x032C[0x9FC];
	CVehicleManager* VehicleManager;
	char pad_0x0D30[0x378];
	BYTE btNoRagdoll;
	char pad_0x10A9[0xF];
	CPlayerInfo* PlayerInfo;
	char pad_0x10C0[0x18];
	CWeaponManager* WeaponManager;
	char pad_0x10D0[0x31C];
	BYTE btSeatBelt;
	char pad_0x13ED[0xB];
	BYTE btSeatbeltWindshield;
	char pad_0x13F9[0x72];
	BYTE btIsInVehicle;
	char pad_0x146C[0x44];
	float Armor;
	char pad_0x14B4[0x3C];
	CVehicleManager* VehicleManager2;

	void setRagdoll(BYTE val) {
		*(BYTE*)((uintptr_t)this + 0x1098) = val;
	}
	void setCollision(float v) {
		uintptr_t* value = *(uintptr_t**)((uintptr_t)this + 0x30);
		if (!IsValidPtr(value)) return;
		uintptr_t* value1 = *(uintptr_t**)((uintptr_t)value + 0x10);
		if (!IsValidPtr(value1)) return;
		uintptr_t* value2 = *(uintptr_t**)((uintptr_t)value1 + 0x20);
		if (!IsValidPtr(value2)) return;
		uintptr_t* value3 = *(uintptr_t**)((uintptr_t)value2 + 0x70);
		if (!IsValidPtr(value3)) return;
		uintptr_t* value4 = *(uintptr_t**)((uintptr_t)value3 + 0x0);
		if (!IsValidPtr(value4)) return;

		*(float*)((uintptr_t)value4 + 0x2C) = v;
	}
	float getCollision() {
		uintptr_t* value = *(uintptr_t**)((uintptr_t)this + 0x30);
		if (!IsValidPtr(value)) return 0.25f;
		uintptr_t* value1 = *(uintptr_t**)((uintptr_t)value + 0x10);
		if (!IsValidPtr(value1)) return 0.25f;
		uintptr_t* value2 = *(uintptr_t**)((uintptr_t)value1 + 0x20);
		if (!IsValidPtr(value2)) return 0.25f;
		uintptr_t* value3 = *(uintptr_t**)((uintptr_t)value2 + 0x70);
		if (!IsValidPtr(value3)) return 0.25f;
		uintptr_t* value4 = *(uintptr_t**)((uintptr_t)value3 + 0x0);
		if (!IsValidPtr(value4)) return 0.25f;
		
		float value5 = *(float*)((uintptr_t)value4 + 0x2C);
		return value5;
	}
	CPlayerInfo* player_info() {

		return *(CPlayerInfo**)((uintptr_t)this + 0x10A8);
	}

	CWeaponManager* weapon() {
		return *(CWeaponManager**)((uintptr_t)this + 0x10B8);
	}

	CVehicle* vehicle() {
		return *(CVehicle**)((uintptr_t)this + 0xD10);

	}

	Vector3 get_bone(int32_t bone_id) {
		Vector3 ret;
		auto boneManager = *reinterpret_cast<__m128**>(reinterpret_cast<int64_t>(this) + 0x180);
		if (!boneManager) return ret;
		auto boneVector = *reinterpret_cast<__m128i*>(reinterpret_cast<int64_t>(this) + 0x430 + bone_id * 0x10);
		auto v1 = _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(boneVector, 0)), *boneManager);
		auto v2 = _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(boneVector, 0x55)), boneManager[1]);
		auto v3 = _mm_mul_ps(_mm_castsi128_ps(_mm_shuffle_epi32(boneVector, 0xAA)), boneManager[2]);
		auto v4 = _mm_add_ps(_mm_add_ps(v1, boneManager[3]), v2);

		ret = Vector3(_mm_add_ps(v3, v4));

		return ret;
	}

	bool IsVisible() {
		return (*(BYTE*)((DWORD64)this + 0xAC) > 0 ? true : false);
	}
	void SetAlpha(float alpha) {
		(*(BYTE*)((DWORD64)this + 0xAC)) = alpha;
	}
	BYTE GetAlpha() {
		return (*(BYTE*)((DWORD64)this + 0xAC));
	}

	void freeze() {
		this->btFreezeMomentum = 1;

	}
	void ResetHealth() {
		this->HP = this->MaxHP;
		*(float*)((DWORD64)this + 0x14B8) = 100.0f;
	}

	float GetArmor() {
		uintptr_t offset = 0x150C;
		return (*(float*)((DWORD64)this + offset));
	}
	void SetArmor(float v) {
		uintptr_t offset = 0x150C;
		*(float*)((DWORD64)this + offset) = v;
	}

	bool is_god_active() {
		return (godmode & 0x01) ? true : false;
	}
	void set_godmode(bool toggle) {
		this->godmode = toggle ? 0x1 : 0x0;
	}

	bool isInvisSP() {
		return (btInvisibleSP & 0x01) ? true : false;
	}

	bool hasFrozenMomentum() {
		return (btFreezeMomentum & 0x02) ? true : false;
	}

	bool canBeRagdolled() {
		return (btNoRagdoll & 0x20) ? false : true;
	}

	WORD PedType() {
		return (*(WORD*)((DWORD64)this + 0x10A8) << 11 >> 25);
	}

	void SetPedType(int Type) {
		*(WORD*)(((DWORD64)this + 0x10A8) << 11 >> 25) = Type;
	}

	bool IsInVehicle() {
		return (*(int*)((DWORD64)this + (0x1477 + 0x30))) == 0 ? true : false;
	}
	void setSeatbelt() {

		*(int*)((DWORD64)this + (0x140C + 0x50)) = 0xC9;
	}

	CModelInfo* ModelInfo() {
		return (*(CModelInfo**)((DWORD64)this + 0x20));
	}

	

};

class CPed {
public:
	char pad_0x0000[0x28];
	BYTE btEntityType;
	char pad_0x0029[0x3];
	BYTE btInvisible;
	char pad_0x002D[0x1];
	BYTE btFreezeMomentum;
	char pad_0x002F[0x1];
	void* pCNavigation;
	char pax_0x0038[0x10];
	CPedStyle* pCPedStyle;
	char pad_0x0038[0x40];
	Vector3 v3VisualPos;
	char pad_0x009C[0xED];
	BYTE btGodMode;
	char pad_0x018A[0xF6];
	float fHealth;
	char pad_0x0284[0x1C];
	float fHealthMax;
	char pad_0x02A4[0x4];
	CAttacker* pCAttacker;
	char pad_0x02B0[0x70];
	Vector3 v3Velocity;
	char pad_0x032C[0x9FC];
	CVehicle* pCVehicleLast;
	char pad_0x0D30[0x378];
	BYTE btNoRagdoll;
	char pad_0x10A9[0xF];
	CPlayerInfo* pCPlayerInfo;
	char pad_0x10C0[0x8];
	CWeaponManager* pCWeaponManager;
	char pad_0x10D0[0x31C];
	BYTE btSeatBelt;
	char pad_0x13ED[0xB];
	BYTE btSeatbeltWindshield;
	char pad_0x13F9[0x1];
	BYTE btCanSwitchWeapons;
	char pad_0x13FB[0x5];
	BYTE btForcedAim;
	BYTE N00000936;
	BYTE N00000939;
	BYTE N00000937;
	char pad_0x1404[0x67];
	BYTE btIsInVehicle;
	char pad_0x146C[0x44];
	float fArmor;
	char pad_0x14B4[0x20];
	float fFatiguedHealthThreshold;
	float fInjuredHealthThreshold;
	float fDyingHealthThreshold;
	float fHurtHealthThreshold;
	char pad_0x14E4[0xC];
	CVehicle* pCVehicleLast2;
	char pad_0x14F8[0xDC];
	__int32 iCash;

	bool isInvisible() {
		return (btInvisible & 0x01) ? true : false;
	}

	bool hasFrozenMomentum() {
		return (btFreezeMomentum & 0x02) ? true : false;
	}

	bool canBeRagdolled() {
		return (btNoRagdoll & 0x20) ? false : true;
	}

	bool hasSeatbelt() {
		return (btSeatBelt & 0x01) ? true : false;
	}

	bool hasSeatbeltWindshield() {
		return (btSeatbeltWindshield & 0x10) ? true : false;
	}

	bool isInVehicle() {
		return (btIsInVehicle & 0x10) ? false : true;
	}

	void setForcedAim(bool toggle) {
		btForcedAim ^= (btForcedAim ^ -(char)toggle) & 0x20;
	}
};

class CAttacker {
public:
	CObject* CPed0;
	char pad_0x0008[0x10];
	CObject* CPed1;
	char pad_0x0020[0x10];
	CObject* CPed2;

};

class CWantedData {
public:
	char pad_0x0000[0x1C];
	float fWantedCanChange;
	char pad_0x0020[0x10];
	Vector3 v3WantedCenterPos;
	char pad_0x003C[0x4];
	Vector3 v3WantedCenterPos2;
	char pad_0x004C[0x38];
	BYTE btFlag0;
	BYTE btFlag1;
	BYTE btFlag2;
	BYTE btFlag3;
	char pad_0x0088[0xC];
	DWORD dwWantedLevelFake;
	DWORD dwWantedLevel;

};

class CPlayerInfo {
public:
	char pad_0x0000[0x34];
	int32_t iInternalIP;
	int16_t iInternalPort;
	char pad_0x003A[0x2];
	int32_t iRelayIP;
	int16_t iRelayPort;
	char pad_0x0042[0x2];
	int32_t iExternalIP;
	int16_t iExternalPort;
	char pad_0x004A[0x32];
	char sName[20];
	char pad_0x0090[0x4];
	int32_t iTeam;
	char pad_0x0098[0xB0];
	float fSwimSpeed;
	float fRunSpeed;
	char pad_0x0150[0x68];
	uint32_t ulState;
	char pad_0x01BC[0xC];
	CObject* pCPed;
	char pad_0x01D0[0x29];
	BYTE btFrameFlags;
	char pad_0x01FA[0x566];
	CWantedData CWantedData;
	char pad_0x07FC[0x464];
	float fStamina;
	float fStaminaMax;
	char pad_0x0C68[0x10];
	float fDamageMod;
	char pad_0x0C7C[0x4];
	float fMeleeDamageMod;
	char pad_0x0C84[0x4];
	float fVehicleDamageMod;
	char pad_0x0C8C[0x1B4];
	BYTE btAiming;
	char pad_0x0E41[0x7];

	void SetWanted(int lvl) {
		*(__int32*)((DWORD64)this + 0x814) = lvl,
			* (__int32*)((DWORD64)this + 0x818) = lvl;
	}
	__int32 GetWanted() {
		return *(__int32*)((DWORD64)this + 0x814);
	}
	char GetName() {
		return *(char*)((DWORD64)this + 0x84);
	}
	int RockstarID() {
		return *(int*)((DWORD64)this + 0x70);
	}

	int getSprintSpoeed() {
		return *(int*)((DWORD64)this + 0x70);
	}

	void setSwimSpeed(float v) {
		*(float*)((uintptr_t)this + 0x150) = v;
	}
	void setWalkSpeed(float v) {
		*(float*)((uintptr_t)this + 0x16C) = v;
	}
	void setRunSpeed(float v) {
		if (isFiveM) {
			*(float*)((uintptr_t)this + 0x14C) = v;
			return;
		}
		*(float*)((uintptr_t)this + 0xCD0) = v;
	}
	void setFrameflags(BYTE flags) {
		*(BYTE*)((uintptr_t)this + 0x1F9) = flags;
	}

};

class CPrimaryAmmoCount {
public:
	char pad_0x0000[0x18];
	__int32 AmmoCount;

	void InfGunAmmo() {
		AmmoCount = 999;
	}

};

class CAmmoCount {
public:
	CPrimaryAmmoCount* _PrimaryAmmoCount;
	char pad_0x0008[0x38];

};

class CAmmoInfo {
public:
	char pad_0x0000[0x8];
	CAmmoCount* _AmmoCount;
	char pad_0x0010[0x18];
	__int32 AmmoMax;
	char pad_0x002C[0x40C];

};

class CWeaponObject {
public:
	char _0x0000[8];
	CAmmoInfo* m_pAmmoInfo;
	char _0x0010[16];
	__int32 m_iAmmo;
	char _0x0024[20];

};

class CInventory {
public:
	char _0x0000[72];
	CWeaponObject** WeaponList;

	CWeaponObject* getWeapon(int index) {
		return (CWeaponObject*)WeaponList[index];
	}
};

class CObjectWrapper {
public:
	char _0x0000[60];
	__int16 TeamId;
	char _0x0040[104];
	CPlayerInfo* playerInfo;
	char _0x00B0[64];

	__int16 GetTeamId() {
		return ((TeamId) << 0 >> 0 << 0);
	}

};

class CPlayers {
public:
	char _0x0000[376];
	__int32 numPlayersOnline;
	char _0x017C[4];
	CObjectWrapper* ObjectWrapperList[256];

	CObject* getPlayer(int index) {
		if (IsValidPtr(ObjectWrapperList[index]) && IsValidPtr(ObjectWrapperList[index]->playerInfo) && IsValidPtr(ObjectWrapperList[index]->playerInfo->pCPed))
			return (CObject*)ObjectWrapperList[index]->playerInfo->pCPed;
		return NULL;
	}

	CPlayerInfo* getPlayerInfo(int index) {
		if (IsValidPtr(ObjectWrapperList[index]) && IsValidPtr(ObjectWrapperList[index]->playerInfo))
			return (CPlayerInfo*)ObjectWrapperList[index]->playerInfo;
		return NULL;
	}

	char* getPlayerName(int index) {
		if (IsValidPtr(ObjectWrapperList[index]) && IsValidPtr(ObjectWrapperList[index]->playerInfo))
			return (char*)ObjectWrapperList[index]->playerInfo->sName;
		return NULL;
	}

	int GetNumPlayersOnline() {
		if (numPlayersOnline > 1 && numPlayersOnline <= 0x20)
			return numPlayersOnline;
		else
			return 0;
	}
};

DWORD64 FrameCount = 0x0;
static uint64_t GetFrameCount() {
	uint64_t        cur = *(uint64_t*)(FrameCount);
	return cur;
}

class CWorld {
public:
	char _0x0000[8];
	CObject* pLocalPlayer;

	CObject* getLocalPlayer() {
		if (IsValidPtr(pLocalPlayer)) {
			return pLocalPlayer;
		} else {
			return 0;
		}
	}
};

class CGameCameraAngles;
class CCameraManagerAngles;
class CCameraAngles;
class CPlayerAngles;

class CGameCameraAngles {
public:
	CCameraManagerAngles* CameraManagerAngles;
	char _0x0008[56];

};

class CCameraManagerAngles {
public:
	CCameraAngles* MyCameraAngles;

};

class CCameraAngles
{
public:
	char pad_0x0000[0x2C0];
	CPlayerAngles* VehiclePointer1;
	CPlayerAngles* VehiclePointer2;
	char pad_0x02D0[0xF0];
	CPlayerAngles* pMyFPSAngles;
	char pad_0x03C8[0x10];
	__int64 pTPSCamEDX;
	char pad_0x03E0[0x60];

};

class CPlayerCameraData {
public:
	char _0x0000[48];
	float Fov_Zoom;
	char _0x0034[36];
	__int32 m_ZoomState;

	float getMass() {
		return *(float*)((uintptr_t)this + 0xC);
	}

};

class CPlayerAngles {
public:
	char pad_0000[16];
	CPlayerCameraData* m_cam_data;
	char pad_0018[40];
	Vector3 m_fps_angles;
	char pad_004C[20];
	Vector3 m_cam_pos;
	char pad_006C[212];
	Vector3 m_cam_pos2;
	char pad_014C[644];
	Vector3 m_tps_angles;
	char pad_03DC[20];
	Vector3 m_fps_cam_pos;
	char pad_03FC[88];
};

class CVehicleColors {
public:
	char pad_0x0000[0xA4];
	BYTE btPrimaryBlue;
	BYTE btPrimaryGreen;
	BYTE btPrimaryRed;
	BYTE btPrimaryAlpha;
	BYTE btSecondaryBlue;
	BYTE btSecondaryGreen;
	BYTE btSecondaryRed;
	BYTE btSecondaryAlpha;

	void SetColor(float Primary[2], float Secondary[2]) {
		btPrimaryRed = BYTE(Primary[0] * 255);
		btPrimaryGreen = BYTE(Primary[1] * 255);
		btPrimaryBlue = BYTE(Primary[2] * 255);

		btSecondaryRed = BYTE(Secondary[0] * 255);
		btSecondaryGreen = BYTE(Secondary[1] * 255);
		btSecondaryBlue = BYTE(Secondary[2] * 255);
	}

};

class CVehicleColorOptions {
public:
	char pad_0x0000[0x20];
	CVehicleColors* CVehicleColor;

};

class CVehicleHandling {
public:
	char pad_0x0000[0xC];
	float fMass;
	char pad_0x0010[0x10];
	Vector3 v3CentreOfMassOffset;
	char pad_0x002C[0x4];
	Vector3 v3InertiaMult;
	char pad_0x003C[0x4];
	float fPercentSubmerged;
	char pad_0x0044[0x8];
	float fAcceleration;
	BYTE btInitialDriveGears;
	char pad_0x0051[0x3];
	float fDriveInertia;
	float fClutchChangeRateScaleUpShift;
	float fClutchChangeRateScaleDownShift;
	float fInitialDriveForce;
	char pad_0x0064[0x8];
	float fBrakeForce;
	char pad_0x0070[0x4];
	float fBrakeBiasFront;
	char pad_0x0078[0x4];
	float fHandBrakeForce;
	char pad_0x0080[0x8];
	float fTractionCurveMax;
	char pad_0x008C[0x4];
	float fTractionCurveMin;
	char pad_0x0094[0xC];
	float fTractionSpringDeltaMax;
	char pad_0x00A4[0x4];
	float fLowSpeedTractionLossMult;
	float fCamberStiffnesss;
	float fTractionBiasFront;
	float fTwoMinus_fTractionBiasFront;
	float fTractionLossMult;
	float fSuspensionForce;
	float fSuspensionCompDamp;
	float fSuspensionReboundDamp;
	float fSuspensionUpperLimit;
	float fSuspensionLowerLimit;
	char pad_0x00D0[0xC];
	float fAntiRollBarForce;
	char pad_0x00E0[0x8];
	float fRollCentreHeightFront;
	float fRollCentreHeightRear;
	float fCollisionDamageMult;
	float fWeaponDamageMult;
	float fDeformationDamageMult;
	float fEngineDamageMult;
	float fPetrolTankVolume;
	float fOilVolume;

	float getMass() {
		return *(float*)((uintptr_t)this + 0xC);
	}
	void setMass(float v) {
		*(float*)((uintptr_t)this + 0xC) = v;
	}
	float getAcceleration() {
		return *(float*)((uintptr_t)this + 0x4C);
	}
	void setAcceleration(float v) {
		*(float*)((uintptr_t)this + 0x4C) = v;
	}
	float getInitialDriveForce() {
		return *(float*)((uintptr_t)this + 0x60);
	}
	void setInitialDriveForce(float v) {
		*(float*)((uintptr_t)this + 0x60) = v;
	}

	void setEngineMult(float v) {
		*(float*)((uintptr_t)this + 0xFC) = v;
	}
	void setCollisionMult(float v) {
		*(float*)((uintptr_t)this + 0xF0) = v;
	}
	void setDeformationMult(float v) {
		*(float*)((uintptr_t)this + 0xF8) = v;
	}
	void setThrust(float v) {
		*(float*)((uintptr_t)this + 0x338) = v;
	}

};

enum DangerousWeaponHashes : DWORD {
	DANGEROUS_SNIPERRIFLE = 0x05FC3C11,
	DANGEROUS_HEAVYSNIPER = 0x0C472FE2,
	DANGEROUS_MARKSMANRIFLE = 0xC734385A,
	DANGEROUS_REMOTESNIPER = 0x33058E22,
	DANGEROUS_REVOLVER = 0xC1B3C3D1,
	DANGEROUS_MARKSMANPISTOL = 0xDC4DB296,
	DANGEROUS_MG = 0x9D07F764,
	DANGEROUS_COMBATMG = 0x7FD62962,
	DANGEROUS_MARKSMANRIFLE_MK2 = 0x6A6C02E0,
	DANGEROUS_HEAVYSNIPER_MK2 = 0x0A914799,
	DANGEROUS_PRECISIONRIFLE = 0x6E7DDDEC,
	DANGEROUS_MUSKET = 0xA89CB99E
};

enum IntersectOptions {
	IntersectWorld = 1,
	IntersectVehicles = 2,
	IntersectPedsSimpleCollision = 4,
	IntersectPeds = 8,
	IntersectObjects = 16,
	IntersectWater = 32,
	IntersectVegetation = 64,
	IntersectFoliage = 256,
	IntersectMap = 1 | 16 | 256,
	IntersectEverything = -1
};

struct RaycastResult {
	bool DidHitAnything = false;
	PVector3 HitCoords = { 0.0f, 0.0f, 0.0f };
	PVector3 HitNormal = { 0.0f, 0.0f, 0.0f };
	int HitEntity = 0;
};

struct RawRaycastResult {
	int Result = 0;
	bool DidHitAnything = false;
	bool DidHitEntity = false;
	DWORD HitEntity = (DWORD)0;
	Vector3 HitCoords;
};
