#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <iostream>

#include <dwmapi.h> 
#include <TlHelp32.h>
#include <stdio.h>

#include <ctime>
#include <chrono>
#include <thread>
#include <iostream>
#include <cstdint>
#include <string>
#include <Windows.h>
#include <vector>

#include <sstream>
#include <string_view>
#include <algorithm>
#include <xmmintrin.h>
#include <map>
#include <directxmath.h>
#include <mutex>

#include <nlohmann/json.hpp>

#define IM_MAX(_A,_B)       (((_A) >= (_B)) ? (_A) : (_B))

#include "core/memory.hpp"

using json = nlohmann::json;
bool debug = false;

using namespace std;

#include "minhook/include/MinHook.h"

#include "simple_math.h"

using namespace DirectX::SimpleMath;

#include "render/renderer.h"
#include "xor.h"
#include "useful.h"
#include "core/d3d11_image.hpp"

#define add_log(str) logs::add(enc(str))

imgui_render renderer;

void load_image_assets(ID3D11Device* device)
{
	(void)device;
}

#include "structs.h"

typedef void(__fastcall* Game_tick) ();

#define BaseAddShift 0x13000
class Scanner
{
public:
	static Scanner* Get() {
		static Scanner* m_pSingleton = 0;
		if (m_pSingleton == 0)
			m_pSingleton = new Scanner;
		return m_pSingleton;
	}

	uint64_t resolverelativeaddress(uint64_t Address, int InstructionLength) {
		DWORD Offset = *(DWORD*)(Address + (InstructionLength - 4));
		return Address + InstructionLength + Offset;
	}

	uint64_t ReturnScan(uint64_t pModuleBaseAddress, const char* sSignature, size_t nSelectResultIndex)
	{
		static auto patternToByte = [](const char* pattern) {
			auto bytes = std::vector<int>{};
			const auto start = const_cast<char*>(pattern);
			const auto end = const_cast<char*>(pattern) + strlen(pattern);

			for (auto current = start; current < end; ++current) {
				if (*current == '?') {
					++current;
					if (*current == '?')
						++current;
					bytes.push_back(-1);
				}
				else
					bytes.push_back(strtoul((const char*)current, &current, 16));
			}
			return bytes;
			};

		const auto dosHeader = (PIMAGE_DOS_HEADER)pModuleBaseAddress;
		const auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)pModuleBaseAddress + dosHeader->e_lfanew);

		const auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
		auto patternBytes = patternToByte(sSignature);
		const auto scanBytes = (uint8_t*)(pModuleBaseAddress);

		const auto size = patternBytes.size();
		const auto data = patternBytes.data();

		MEMORY_BASIC_INFORMATION mbi;
		size_t nFoundResults = 0;
		uint8_t* currentAddr = (uint8_t*)(pModuleBaseAddress);
		uint8_t* endAddr = currentAddr + ((PIMAGE_NT_HEADERS)((std::uint8_t*)pModuleBaseAddress + ((PIMAGE_DOS_HEADER)pModuleBaseAddress)->e_lfanew))->OptionalHeader.SizeOfImage;

		while (currentAddr < endAddr) {
			if (!VirtualQueryEx((HANDLE)-1, currentAddr, &mbi, sizeof(mbi))) {
				break;
			}

			if (mbi.Protect != PAGE_NOACCESS && !(mbi.Protect & PAGE_GUARD)) {
				const auto scanBytes = (uint8_t*)(mbi.BaseAddress);
				const auto regionSize = mbi.RegionSize;

				for (size_t i = 0; i < regionSize - size; ++i) {
					bool found = true;

					for (size_t j = 0; j < size; ++j) {
						if (scanBytes[i + j] != data[j] && data[j] != -1) {
							found = false;
							break;
						}
					}

					if (found) {
						if (nSelectResultIndex != 0) {
							if (nFoundResults < nSelectResultIndex) {
								nFoundResults++;
								found = false;
							}
							else {
								DWORD oldProtection;
								VirtualProtectEx((HANDLE)-1, (LPVOID)&currentAddr[i], size, PAGE_EXECUTE_READWRITE, &oldProtection);
								uint64_t result = (uint64_t)(&currentAddr[i]);
								VirtualProtectEx((HANDLE)-1, (LPVOID)&currentAddr[i], size, oldProtection, &oldProtection);
								return result;
							}
						}
						else {
							DWORD oldProtection;
							VirtualProtectEx((HANDLE)-1, (LPVOID)&currentAddr[i], size, PAGE_EXECUTE_READWRITE, &oldProtection);
							uint64_t result = (uint64_t)(&currentAddr[i]);
							VirtualProtectEx((HANDLE)-1, (LPVOID)&currentAddr[i], size, oldProtection, &oldProtection);
							return result;
						}
					}
				}
			}

			currentAddr = (uint8_t*)(mbi.BaseAddress) + mbi.RegionSize;
		}

		return NULL;
	}

	uint64_t Scan(const char* szModuleName, const char* sSignature, size_t nSelectResultIndex, int InstructionLength) {
		auto ret = ReturnScan((uint64_t)GetModuleHandleA(szModuleName), sSignature, nSelectResultIndex);
		if (InstructionLength != 0) {
			ret = resolverelativeaddress(ret, InstructionLength);
		}
		return ret;
	}

	std::vector<int> PatternToByte(const char* Pattern) {
		std::vector<int> Bytes;
		const auto Start = const_cast<char*>(Pattern);
		const auto End = Start + strlen(Pattern);

		for (auto Current = Start; Current < End; ++Current) {
			if (*Current == '?') {
				++Current;
				if (*Current == '?')
					++Current;
				Bytes.push_back(-1);
			}
			else {
				Bytes.push_back(strtoul(reinterpret_cast<const char*>(Current), &Current, 16));
			}
		}

		return Bytes;
	}

	uintptr_t FindPatternEx(uintptr_t Base, const char* Pattern, size_t Index, bool Module) {
		const auto Header = (PIMAGE_DOS_HEADER)Base;
		const auto Headers = (PIMAGE_NT_HEADERS)((uint8_t*)Base + Header->e_lfanew);

		const auto SizeOfImage = Headers->OptionalHeader.SizeOfImage;
		auto PatternBytes = PatternToByte(Pattern);

		if (!Module) {
			Base += 0x13000;
		}

		const auto ScanBytes = (uint8_t*)(Base);

		const auto PatternBytesSize = PatternBytes.size();
		const auto PatternBytesData = PatternBytes.data();

		size_t ResultSize = 0;
		for (auto i = 0ul; i < SizeOfImage - PatternBytesSize; ++i) {
			bool Found = true;

			for (auto j = 0ul; j < PatternBytesSize; ++j) {
				if (ScanBytes[i + j] != PatternBytesData[j] && PatternBytesData[j] != -1) {
					Found = false;
					break;
				}
			}

			if (Found) {
				if (Index != 0) {
					if (ResultSize < Index) {
						ResultSize++;
						Found = false;
					}
					else
						return (uint64_t)(&ScanBytes[i]);
				}
				else
					return (uint64_t)(&ScanBytes[i]);
			}
		}
		return NULL;
	}

	uintptr_t FindPattern(uintptr_t Base, const char* Pattern, size_t Index, int Length, bool Module) {
		if (!Base) return 0;
		auto Result = FindPatternEx(Base, Pattern, Index, Module);

		if (Length != 0) {
			DWORD Offset = *(DWORD*)(Result + (Length - 4));
			Result = Result + Length + Offset;
		}

		if (!Result) {
			MessageBoxA(0, "Memory Error #3", "Ryokai", MB_OK);
		}

		return Result;
	}

	uintptr_t resolvePtr(const char* szModuleName, int64_t targetValue, BYTE compareByte) {
		uintptr_t baseaddy = (uintptr_t)GetModuleHandleA(szModuleName);
		const auto dosHeader = (PIMAGE_DOS_HEADER)baseaddy;
		const auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)baseaddy + dosHeader->e_lfanew);
		const auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;

		int64_t currentAddress = baseaddy + BaseAddShift;

		while (currentAddress < static_cast<int64_t>(baseaddy + sizeOfImage)) {
			auto cp = *(int64_t*)currentAddress;
			if (cp == targetValue) {
				if (*(BYTE*)cp == compareByte)
					return currentAddress;
			}
			currentAddress++;
		}

		return NULL;
	}
};

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
			}
			else {
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

#define FIND_NT_HEADER(x) reinterpret_cast<PIMAGE_NT_HEADERS>( uint64_t(x) + ((PIMAGE_DOS_HEADER)(x))->e_lfanew )

uint8_t* findPattern(const std::string_view module, const std::string_view signature) {
	std::vector<uint8_t> signature_bytes(signature.size() + 1);

	{
		std::vector<std::string> signature_chunks{ };
		std::string current_chunk{ };

		std::istringstream string_stream{ signature.data() };

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

template<typename T = uint8_t*>
inline T fix_mov(uint8_t* patternMatch) {
	return reinterpret_cast<T>(patternMatch + *reinterpret_cast<int32_t*>(patternMatch + 3) + 7);
}
namespace fiber
{
	using script_func_t = void(*)();

	class fiber_task
	{
	public:
		explicit fiber_task(HMODULE hmod, script_func_t func) :
			m_hmodule(hmod),
			m_function(func)
		{ }

		~fiber_task() noexcept
		{
			if (m_script_fiber)
			{
				DeleteFiber(m_script_fiber);
			}
		}

		HMODULE get_hmodule()
		{
			return m_hmodule;
		}

		script_func_t get_function()
		{
			return m_function;
		}

		void on_tick()
		{
			if (GetTickCount64() < m_wake_at)
				return;

			if (!m_main_fiber)
			{
				m_main_fiber = IsThreadAFiber() ? GetCurrentFiber() : ConvertThreadToFiber(0);
			}

			if (m_script_fiber)
			{
				current_fiber_script = this;
				SwitchToFiber(m_script_fiber);
				current_fiber_script = 0;
			}
			else
			{

				m_script_fiber = CreateFiber(0, [](PVOID param)
					{
						auto this_script = static_cast<fiber_task*>(param);
						while (true)
						{
							this_script->m_function();
						}

					}, this);
			}
		}
		void wait(std::uint32_t time)
		{
			m_wake_at = GetTickCount64() + time;
			SwitchToFiber(m_main_fiber);
		}

		inline static fiber_task* get_current_script()
		{
			return current_fiber_script;
		}
	private:
		HMODULE m_hmodule{};
		script_func_t m_function{};

		std::uint32_t m_wake_at{};
		void* m_script_fiber{};
		void* m_main_fiber{};

		inline static fiber_task* current_fiber_script{};
	};
}
std::unique_ptr<fiber::fiber_task> game_fiber;

struct HackBase {
	map<int, bool> keyStates;
	int renderMethod = -1;
	bool renderReady = false;
	DWORD64 renderReadySince = 0;
	bool running = false;
	bool gameDataReady = false;

	bool menuOpen = true;
	bool windowFocused = true;

	ImVec2 screen = ImVec2(0, 0);

	HWND window = 0;
	HMODULE hModule = 0;
	WNDPROC originalWndProc = 0;

	uintptr_t base = 0;
	CReplayInterface* ReplayInterface = 0;
	CWorld* World = 0;

	CViewPort* viewPort = 0;
	game_state_t* gta_game_state = 0;

	uintptr_t CamAddr = 0;
	float* AspectRatioAddr = 0;
	float AspectRatioDefault = 0.f;

	Game_tick tick;

	int game_mod_base = 0;

	vector<string> messages;

	CPlayerAngles* getCam() {
		if (CamAddr){
			return *(CPlayerAngles**)(CamAddr + 0x0);
		}
		return 0;
	}
	
	CGameCameraAngles* CGameCamera;

	bool allowClearTasks = false;
} Game;

struct LocalData {
	CObject* player = 0;
} local;

#include "functions.h"

#include "native/database.hpp"

#include "config/config.hpp"
#include "platform/tick.hpp"
#include "platform/debug.hpp"
#include "platform/crash_logger.hpp"
#include "runtime/player_marks.hpp"

#include "executor/executor.hpp"
#include "ui/ui.hpp"
#include "features/visuals/esp.hpp"
#include "game.hpp"
#include "features/misc/binds.hpp"
#include "features/misc/core.hpp"
#include "features/misc/game_tick.hpp"
#include "features/aimbot/aim.hpp"

#include "hooks.hpp"

#include "native/pointers.hpp"
