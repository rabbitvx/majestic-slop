#pragma once

#pragma once
#include "core/imports.h"
#include <iostream>
#include <fstream>
#include <Psapi.h>

#pragma warning(push)
#pragma warning(disable: 4090)

extern bool isFiveM;

#define INRANGE(x,a,b)    (x >= a && x <= b) 
#define getBits( x )    (INRANGE((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xa) : (INRANGE(x,'0','9') ? x - '0' : 0))
#define getByte( x )    (getBits(x[0]) << 4 | getBits(x[1]))
#define FIND_NT_HEADER(x) reinterpret_cast<PIMAGE_NT_HEADERS>( uint64_t(x) + ((PIMAGE_DOS_HEADER)(x))->e_lfanew )
namespace pointers {
	inline bool is_writable_memory(const void* ptr, size_t size) {
		MEMORY_BASIC_INFORMATION mbi = {};
		if (!ptr || !VirtualQuery(ptr, &mbi, sizeof(mbi))) {
			return false;
		}

		const uintptr_t start = reinterpret_cast<uintptr_t>(ptr);
		const uintptr_t end = start + size;
		const uintptr_t region_end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
		if (mbi.State != MEM_COMMIT || end > region_end || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
			return false;
		}

		const DWORD writable =
			PAGE_READWRITE |
			PAGE_WRITECOPY |
			PAGE_EXECUTE_READWRITE |
			PAGE_EXECUTE_WRITECOPY;
		return (mbi.Protect & writable) != 0;
	}

	inline bool is_valid_aspect_ratio(float value) {
		return value > 0.5f && value < 4.f;
	}

	inline void resolve_aspect_ratio_address(const string& game_executable) {
		Game.AspectRatioAddr = nullptr;
		Game.AspectRatioDefault = 0.f;

		HMODULE gta_module = GetModuleHandleA(game_executable.c_str());
		if (!gta_module) {
			return;
		}

		MODULEINFO module_info = {};
		if (!GetModuleInformation(GetCurrentProcess(), gta_module, &module_info, sizeof(module_info))) {
			return;
		}

		constexpr uintptr_t aspect_offset = 0x1E9406C;
		if (aspect_offset + sizeof(float) > module_info.SizeOfImage) {
			return;
		}

		float* candidate = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(gta_module) + aspect_offset);
		if (!is_writable_memory(candidate, sizeof(float))) {
			return;
		}

		const float value = *candidate;
		if (is_valid_aspect_ratio(value)) {
			Game.AspectRatioAddr = candidate;
			Game.AspectRatioDefault = value;
			add_log("aspect ratio pointer resolved");
		}
	}

	vector<string> listIAT(string moduleName) {
		vector<string> t;
		HMODULE mp = GetModuleHandleA(moduleName.c_str());
		PIMAGE_DOS_HEADER img_dos_headers = (PIMAGE_DOS_HEADER)mp;
		PIMAGE_NT_HEADERS img_nt_headers = (PIMAGE_NT_HEADERS)((BYTE*)img_dos_headers + img_dos_headers->e_lfanew);
		PIMAGE_IMPORT_DESCRIPTOR img_import_desc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)img_dos_headers + img_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		if (img_dos_headers->e_magic != IMAGE_DOS_SIGNATURE)
			add_log("ERROR: e_magic is no valid DOS signature\n");

		for (IMAGE_IMPORT_DESCRIPTOR* iid = img_import_desc; iid->Name != 0; iid++) {
			for (int func_idx = 0; *(func_idx + (void**)(iid->FirstThunk + (size_t)mp)) != 0; func_idx++) {
				char* mod_func_name = (char*)(*(func_idx + (size_t*)(iid->OriginalFirstThunk + (size_t)mp)) + (size_t)mp + 2);
				const intptr_t nmod_func_name = (intptr_t)mod_func_name;
				if (nmod_func_name >= 0) {
					t.push_back((string)moduleName + " -> " + (string)mod_func_name);
				}
			}
		}
		return t;
	}

	void** find_iat(string moduleName, string targetFunc) {
		HMODULE mp = GetModuleHandleA(moduleName.c_str());
		PIMAGE_DOS_HEADER img_dos_headers = (PIMAGE_DOS_HEADER)mp;
		PIMAGE_NT_HEADERS img_nt_headers = (PIMAGE_NT_HEADERS)((BYTE*)img_dos_headers + img_dos_headers->e_lfanew);
		PIMAGE_IMPORT_DESCRIPTOR img_import_desc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)img_dos_headers + img_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		if (img_dos_headers->e_magic != IMAGE_DOS_SIGNATURE)
			add_log("ERROR: e_magic is no valid DOS signature\n");

		for (IMAGE_IMPORT_DESCRIPTOR* iid = img_import_desc; iid->Name != 0; iid++) {
			for (int func_idx = 0; *(func_idx + (void**)(iid->FirstThunk + (size_t)mp)) != 0; func_idx++) {
				char* mod_func_name = (char*)(*(func_idx + (size_t*)(iid->OriginalFirstThunk + (size_t)mp)) + (size_t)mp + 2);
				const intptr_t nmod_func_name = (intptr_t)mod_func_name;
				if (nmod_func_name >= 0) {
					if (!strcmp(targetFunc.c_str(), mod_func_name)) {
						return func_idx + (void**)(iid->FirstThunk + (size_t)mp);
					}

				}
			}
		}
		return 0;
	}

	uintptr_t hook_iat_ptr(string moduleName, string targetFunc, void* newfunction) {
		auto&& func_ptr = find_iat(moduleName, targetFunc);
		if (*func_ptr == newfunction || *func_ptr == 0)
			return 0;

		DWORD old_rights, new_rights = PAGE_READWRITE;
		VirtualProtect(func_ptr, sizeof(uintptr_t), new_rights, &old_rights);
		uintptr_t ret = (uintptr_t)*func_ptr;
		*func_ptr = newfunction;
		VirtualProtect(func_ptr, sizeof(uintptr_t), old_rights, &new_rights);
		return ret;
	}

	template<typename T = uint8_t*>
	inline T fix_mov(uint8_t* patternMatch) {
		return reinterpret_cast<T>(patternMatch + *reinterpret_cast<int32_t*>(patternMatch + 3) + 7);
	}
	template <typename T = uint8_t*>
	inline T fix_call(uint8_t* address) {
		return reinterpret_cast<T>(address + *reinterpret_cast<int32_t*>(address + 1) + 5);
	}

	template <typename T = uint8_t*>
	inline T fix_custom(uint8_t* address, int offset) {
		return reinterpret_cast<T>(address) - offset;
	}
	template <typename T = uint8_t*>
	inline T add(uint8_t* address, int offset) {
		return reinterpret_cast<T>(address) + offset;
	}
	template <typename T = uint8_t*>
	inline T sub(uint8_t* address, int offset) {
		return reinterpret_cast<T>(address) - offset;
	}

	template <typename type>
	type as_relative(type address, int offset = 3) {
		return reinterpret_cast<type>(reinterpret_cast<uintptr_t>(address) + *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(address) + offset) + (offset + 4i32));
	}

	std::uint8_t* findPattern2(const char* module_name, const char* signature) {
		const auto module_handle = GetModuleHandleA(module_name);

		if (!module_handle)
			return 0;

		static auto pattern_to_byte = [](const char* pattern) {
			auto bytes = std::vector<int>{};
			auto start = const_cast<char*>(pattern);
			auto end = const_cast<char*>(pattern) + std::strlen(pattern);

			for (auto current = start; current < end; ++current) {
				if (*current == '?') {
					++current;

					if (*current == '?')
						++current;

					bytes.push_back(-1);
				} else {
					bytes.push_back(std::strtoul(current, &current, 16));
				}
			}
			return bytes;
		};

		auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);
		auto nt_headers =
			reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module_handle) + dos_header->e_lfanew);

		auto size_of_image = nt_headers->OptionalHeader.SizeOfImage;
		auto pattern_bytes = pattern_to_byte(signature);
		auto scan_bytes = reinterpret_cast<std::uint8_t*>(module_handle);

		auto s = pattern_bytes.size();
		auto d = pattern_bytes.data();

		for (auto i = 0ul; i < size_of_image - s; ++i) {
			bool found = true;

			for (auto j = 0ul; j < s; ++j) {
				if (scan_bytes[i + j] != d[j] && d[j] != -1) {
					found = false;
					break;
				}
			}
			if (found)
				return &scan_bytes[i];
		}
		logs::add(enc("failed find pattern again in ") + string(module_name));

		return 0;
	}

	uint8_t* findPattern(const std::string_view module, const std::string_view signature) {
		std::vector<uint8_t> signature_bytes(signature.size() + 1);

		{
			std::vector<std::string> signature_chunks{ };
			std::string current_chunk{ };

			std::istringstream string_stream{signature.data()};

			while (std::getline(string_stream, current_chunk, ' '))
			signature_chunks.push_back(current_chunk);

			std::transform(signature_chunks.cbegin(), signature_chunks.cend(), signature_bytes.begin(), [](const std::string& val) -> uint8_t {
				return val.find('?') != std::string::npos ? 0ui8 : static_cast<uint8_t>(std::stoi(val, 0, 16));
				});
		}

		uint8_t* found_bytes = 0;

		{
			const auto image_start = reinterpret_cast<uint8_t*>(GetModuleHandleA(module.data()));
			const auto image_end = image_start + FIND_NT_HEADER(image_start)->OptionalHeader.SizeOfImage;

			const auto result = std::search(image_start, image_end, signature_bytes.cbegin(), signature_bytes.cend(), [](uint8_t left, uint8_t right) -> bool {
				return right == 0ui8 || left == right;
				});

			found_bytes = (result != image_end) ? result : 0;
		}

		if (found_bytes == 0) {
			logs::add(enc("failed find pattern, fallback ") + string(module.data()));
			return findPattern2(module.data(), signature.data());
		}
		return found_bytes;
	}

	bool findAllPatterns() {
		if (debug) return true;
		add_log("find signatures");

		string game_executable = "GTA5.exe";

		logs::add(enc("game process:") + (string)game_executable);

		auto replayPattern = findPattern(game_executable, enc("48 8D 0D ? ? ? ? 48 8B D7 E8 ? ? ? ? 48 8D 0D ? ? ? ? 8A D8 E8 ? ? ? ? 84 DB 75 13 48 8D 0D"));
		if (!replayPattern) {
			add_log("cant find ReplayInterface pattern");
			return false;
		}
		Game.ReplayInterface = *(CReplayInterface**)fix_mov(replayPattern);
		if (!IsValidPtr(Game.ReplayInterface)) {
			add_log("cant find ReplayInterface");
			return false;
		}

		auto worldPattern = findPattern(game_executable, enc("48 8B 05 ? ? ? ? 48 8B 58 08 48 85 DB 74 32"));
		if (!worldPattern) {
			add_log("cant find World pattern");
			return false;
		}
		Game.World = *(CWorld**)fix_mov(worldPattern);
		if (!IsValidPtr(Game.World)) {
			add_log("cant find World");
			return false;
		}

		auto viewPortPattern = findPattern(game_executable, enc("48 8B 15 ? ? ? ? 48 8D 2D ? ? ? ? 48 8B CD"));
		if (!viewPortPattern) {
			add_log("cant find viewport pattern");
			return false;
		}
		Game.viewPort = *(CViewPort**)fix_mov(viewPortPattern);

		auto camPattern = findPattern(game_executable, enc("48 8B 05 ? ? ? ? 48 8B 98 ? ? ? ? EB"));
		if (!camPattern) {
			add_log("cant find cam pattern");
			return false;
		}
		Game.CamAddr = (uintptr_t)fix_mov(camPattern);

		if (!IsValidPtr(Game.viewPort)) {
			add_log("cant find viewport");
			return false;
		}
		if (!Game.CamAddr) {
			add_log("cant find cam");
			return false;
		}

		WorldToScreenAddr = reinterpret_cast<WorldToScreen_t>(findPattern(game_executable, "48 89 5C 24 ?? 55 56 57 48 83 EC 70 65 4C 8B 0C 25 ?? 00 00 00 8B"));
		if (!IsValidPtr(WorldToScreenAddr)) {
			add_log("cant find w2s");
			return false;
		}
	
		
		GetBoneFunc = reinterpret_cast<GetBoneFromMask2>(findPattern(game_executable, enc("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 60 48 8B 01 41 8B E8 48 8B F2 48 8B F9 33 DB")));
		if (!IsValidPtr(GetBoneFunc)) {
			add_log("cant find bone1");
			return false;
		}

		DWORD64 FrameCount_pattern = (DWORD64)findPattern(game_executable, enc("8B 15 ? ? ? ? 41 FF CF"));
		if (!FrameCount_pattern) {
			add_log("cant find FrameCount pattern");
			return false;
		}
		FrameCount = (FrameCount_pattern + *(DWORD*)(FrameCount_pattern + 0x2) + 0x6);
		if (!FrameCount) {
			add_log("cant find FrameCount");
			return false;
		}

		auto pattern = findPattern(game_executable, enc("83 3D ? ? ? ? ? 75 17 8B 43 20 25"));
		if (!pattern) {
			add_log("cant find game_state pattern");
			return false;
		}
		auto offset1 = add(pattern, 2);
		auto offset2 = as_relative(offset1, 0);
		auto offset3 = add(offset2, 1);
		Game.gta_game_state = reinterpret_cast<game_state_t*>(offset3);
		if (!IsValidPtr(Game.gta_game_state)) {
			add_log("cant find game_state");

			return false;
		}

		auto dlcPattern = findPattern(game_executable, enc("57 48 83 EC 20 40 8A FA 8B 15 ? ? ? ? 48 8B F1"));
		if (!dlcPattern) {
			add_log("cant find Dlc_Present pattern");
			return false;
		}
		Is_Dlc_Present = reinterpret_cast<Is_Dlc_Present_t>(sub(dlcPattern, 0xA));

		if (!IsValidPtr(Is_Dlc_Present)) {
			add_log("cant find Dlc_Present");
			return false;
		}

		pointer_to_handle = reinterpret_cast<pointer_to_handle_t>(findPattern(game_executable, enc("48 89 5c 24 ? 48 89 74 24 ? 57 48 83 ec 20 8b 15 ? ? ? ? 48 8b f9 48 83 c1 10 33 db")));

		if (!IsValidPtr(pointer_to_handle)) {
			add_log("cant find pointer_to_handle");
			return false;
		}

		give_weapon_delayed = reinterpret_cast<give_weapon_delayed_t>(findPattern(game_executable, enc("48 89 5c 24 ? 48 89 6c 24 ? 48 89 74 24 ? 57 48 83 ec ? 41 8a e9 41 8b f0 8b fa e8 ? ? ? ? 48 8b d8 48 85 c0 74")));
		if (!IsValidPtr(give_weapon_delayed)) {
			add_log("cant find gwd");

			return false;
		}

		auto accuratePattern = findPattern(game_executable, enc("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 48 89 78 ? 41 56 48 83 EC 70 0F 29 70 ? 48 8B F1"));
		if (accuratePattern) {
			gta_get_is_accurate = reinterpret_cast<uintptr_t>(accuratePattern);
			add_log("gta_get_is_accurate ok");
		}
		else {
			add_log("cant find gta_get_is_accurate pattern");
		}

		auto bulletInstancePattern = findPattern(game_executable, enc("48 89 5C 24 08 57 48 83 EC 30 41 8B 00 F3"));
		if (bulletInstancePattern) {
			gta_bullet_instance = reinterpret_cast<sBulletInstance>(bulletInstancePattern);
			add_log("gta_bullet_instance ok");
		}
		else {
			add_log("cant find gta_bullet_instance pattern");
		}

		auto gunShotPattern = findPattern(game_executable, enc("E8 ? ? ? ? 8B 75 D4"));
		if (!gunShotPattern) {
			add_log("cant find gta_weapon_shot pattern");
			return false;
		}
		gta_event_gun_shot = fix_call<gta_event_gun_shot_t>(gunShotPattern);
		if (!IsValidPtr(gta_event_gun_shot)) {
			add_log("cant find gta_weapon_shot");

			return false;
		}

		auto scriptTickPattern = findPattern(game_executable, enc("48 83 EC 20 80 B9 4E 01 00 00 00"));
		if (!scriptTickPattern) {
			add_log("cant find gta_script_thread_tick pattern");
			return false;
		}
		gta_script_thread_tick = reinterpret_cast<script_thread_tick_t>(sub(scriptTickPattern, 0xB));
		if (!IsValidPtr(gta_script_thread_tick)) {
			add_log("cant find gta_script_thread_tick");

			return false;
		}

		auto nativeHandlerPattern = findPattern(game_executable, enc("48 8D 0D ? ? ? ? 48 8B 14 FA E8 ? ? ? ? 48 85 C0 75 0A"));
		if (!nativeHandlerPattern) {
			add_log("cant find gta_get_native_handler pattern");
			return false;
		}
		gta_get_native_handler = reinterpret_cast<get_native_handler_t>(as_relative(nativeHandlerPattern, 12));
		if (!IsValidPtr(gta_get_native_handler)) {
			add_log("cant find gta_get_native_handler");
			return false;
		}

		gta_native_registration_table = reinterpret_cast<uintptr_t*>(as_relative(nativeHandlerPattern, 3));
		if (!IsValidPtr(gta_native_registration_table)) {
			add_log("cant find gta_native_registration_table");
			return false;
		}

		gta_fix_context_vector = reinterpret_cast<fix_context_vector_t>(findPattern(game_executable, enc("83 79 18 00 48 8B D1 74 4A FF 4A 18 48 63 4A 18 48 8D 41 04 48 8B 4C CA")));
		if (!IsValidPtr(gta_fix_context_vector)) {
			add_log("cant find gta_fix_context_vector");
			return false;
		}

		resolve_aspect_ratio_address(game_executable);

		if (GetModuleHandleA("multiplayerL.dll") || GetModuleHandleA("multiplayer.dll")) {
			add_log("detected ragemp");
			Game.game_mod_base = 1;
		}
		if (GetModuleHandleA("altv-client.dll")) {
			add_log("detected altv");
			Game.game_mod_base = 2;
		}

		add_log("found signatures");
		return true;

	}
}

#pragma warning(pop)
