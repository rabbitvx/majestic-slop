#pragma once
#include "core/imports.h"
#include "kiero.h"
#include "network/ws_bridge.hpp"

IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using D3D11PresentHook = HRESULT(__stdcall*)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
using D3D11ResizeBuffersHook = HRESULT(__stdcall*)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

inline D3D11PresentHook phookD3D11Present = nullptr;
inline D3D11ResizeBuffersHook phookD3D11ResizeBuffers = nullptr;

using namespace std::chrono_literals;
namespace hook {

	std::chrono::time_point<std::chrono::high_resolution_clock> resize_counter = std::chrono::high_resolution_clock::now();
	bool present_setup = false;
	static bool display_transition = false;

	inline void set_render_ready(bool ready) {
		Game.renderReady = ready;
		Game.renderReadySince = ready ? GetTickCount64() : 0;
	}

	void patch_wndproc();

	inline void release_render_target_for_display_change(const char* reason) {
		set_render_ready(false);
		display_transition = true;

		if (renderer.pContext) {
			renderer.pContext->OMSetRenderTargets(0, nullptr, nullptr);
		}

		if (renderer.RenderTargetView) {
			renderer.RenderTargetView->Release();
			renderer.RenderTargetView = nullptr;
		}

		if (ImGui::GetCurrentContext()) {
			ImGui_ImplDX11_InvalidateDeviceObjects();
		}

		if (renderer.pContext) {
			renderer.pContext->Flush();
		}
	}

	inline int d3d_exception_filter(const char* tag, EXCEPTION_POINTERS* exception) {
		DWORD code = exception && exception->ExceptionRecord ? exception->ExceptionRecord->ExceptionCode : 0;
		HELLO_RUNTIME_LOG("SEH: %s 0x%08X", tag, code);
		crash_logger::log_exception(tag, exception);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	typedef LRESULT(__stdcall* WINPROC) (_In_ HWND hWnd,
		_In_ UINT Msg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam);

	LRESULT CALLBACK hook_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	inline LRESULT wndproc_inner(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	inline LRESULT call_original_wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		WNDPROC original = Game.originalWndProc;
		if (!original || original == hook_WndProc) {
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
		return CallWindowProc(original, hWnd, uMsg, wParam, lParam);
	}

	inline bool submit_keyboard_layout_char(HWND hWnd, WPARAM wParam) {
		if (IsWindowUnicode(hWnd)) return false;
		if (wParam < 0x80 || wParam > 0xFF) return false;

		UINT code_page = GetACP();
		char code_page_buf[16]{};
		const LANGID lang_id = LOWORD(reinterpret_cast<uintptr_t>(GetKeyboardLayout(0)));
		if (GetLocaleInfoA(MAKELCID(lang_id, SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, code_page_buf, sizeof(code_page_buf)) > 0) {
			const int parsed_code_page = atoi(code_page_buf);
			if (parsed_code_page > 0) code_page = static_cast<UINT>(parsed_code_page);
		}

		const char input = static_cast<char>(wParam & 0xFF);
		wchar_t wide_char = 0;
		if (MultiByteToWideChar(code_page, MB_PRECOMPOSED, &input, 1, &wide_char, 1) <= 0 || wide_char == 0) {
			return false;
		}

		ImGui::GetIO().AddInputCharacter(static_cast<unsigned int>(wide_char));
		return true;
	}

	LRESULT CALLBACK hook_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		static thread_local bool in_wndproc = false;
		if (in_wndproc) {
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		in_wndproc = true;
		LRESULT result = 0;
		__try {
			result = wndproc_inner(hWnd, uMsg, wParam, lParam);
		}
		__except(d3d_exception_filter("wndproc", GetExceptionInformation())) {
			in_wndproc = false;
			return call_original_wndproc(hWnd, uMsg, wParam, lParam);
		}
		in_wndproc = false;
		return result;
	}

	inline LRESULT wndproc_inner(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		static bool callOnce = false;
		if (!callOnce) {
			callOnce = true;
		}

		if (!Game.originalWndProc) {
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		// Track window focus: minimized / alt-tab / focus loss. The cheat must
		// stop its heavy per-tick work (ped scans, bone natives, ws log reads)
		// while unfocused — it shares the game/client thread with the majestic
		// voice manager, and background activity starves voice (users report
		// mic stops working after minimizing with the cheat injected).
		if (uMsg == WM_ACTIVATEAPP) {
			Game.windowFocused = (wParam == TRUE);
			ws_server::set_window_focused(Game.windowFocused);
		}
		else if (uMsg == WM_SIZE && wParam == SIZE_MINIMIZED) {
			Game.windowFocused = false;
			ws_server::set_window_focused(false);
		}
		else if (uMsg == WM_SIZE && wParam == SIZE_RESTORED) {
			Game.windowFocused = true;
			ws_server::set_window_focused(true);
		}

		if (uMsg == WM_SYSKEYDOWN && wParam == VK_RETURN && (HIWORD(lParam) & KF_ALTDOWN)) {
			if (renderer.ready()) {
				release_render_target_for_display_change("alt_enter");
			}
			display_transition = true;
			return call_original_wndproc(hWnd, uMsg, wParam, lParam);
		}

		if (!Game.renderReady) {
			return call_original_wndproc(hWnd, uMsg, wParam, lParam);
		}

		if (uMsg == WM_KEYUP || uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONUP || uMsg == WM_XBUTTONUP || uMsg == WM_MBUTTONUP) {
			int param = 0;
			switch (uMsg)
			{
			case WM_KEYUP:
				param = static_cast<int>(wParam);
				break;
			case WM_LBUTTONUP:
				param = 1;
				break;
			case WM_RBUTTONUP:
				param = 2;
				break;
			case WM_MBUTTONUP:
				param = 4;
				break;
			case WM_XBUTTONUP:
				param = 4 + GET_XBUTTON_WPARAM(wParam);
				break;
			}
			if (param == config::get("menu", "menu_key", 35)) {
				Game.menuOpen = !Game.menuOpen;
			}
		}

		if (present_setup && Game.window && Game.renderReady && renderer.ready()) {
			ImGuiContext* ctx = ImGui::GetCurrentContext();
			if (ctx != 0) {
				POINT mPos;
				if (GetCursorPos(&mPos)) {
					ScreenToClient(Game.window, &mPos);
					ImGui::GetIO().MousePos.x = static_cast<float>(mPos.x);
					ImGui::GetIO().MousePos.y = static_cast<float>(mPos.y);
				}

				if (uMsg == WM_KEYUP) {
					if (wParam < 256) {
						Game.keyStates[wParam] = false;
					}
				}
				else if (uMsg == WM_KEYDOWN) {
					if (wParam < 256) {
						Game.keyStates[wParam] = true;
					}
				}
				else if (uMsg == WM_LBUTTONUP)   { Game.keyStates[1] = false; }
				else if (uMsg == WM_LBUTTONDOWN) { Game.keyStates[1] = true; }
				else if (uMsg == WM_RBUTTONUP)   { Game.keyStates[2] = false; }
				else if (uMsg == WM_RBUTTONDOWN) { Game.keyStates[2] = true; }
				else if (uMsg == WM_MBUTTONUP)   { Game.keyStates[4] = false; }
				else if (uMsg == WM_MBUTTONDOWN) { Game.keyStates[4] = true; }
				else if (uMsg == WM_XBUTTONUP) {
					int xb = GET_XBUTTON_WPARAM(wParam);
					Game.keyStates[4 + xb] = false;
				}
				else if (uMsg == WM_XBUTTONDOWN) {
					int xb = GET_XBUTTON_WPARAM(wParam);
					Game.keyStates[4 + xb] = true;
				}

				if (Game.menuOpen) {
					if (uMsg == WM_CHAR && submit_keyboard_layout_char(hWnd, wParam)) {
						return TRUE;
					}

					if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
						return TRUE;
					}
					
					switch (uMsg) {
					case WM_SYSCOMMAND:
					case WM_NCCREATE:
					case WM_NCDESTROY:
					case WM_DESTROY:
					case WM_CLOSE:
					case WM_QUIT:
					case WM_PAINT:
					case WM_ERASEBKGND:
					case WM_GETMINMAXINFO:
					case WM_WINDOWPOSCHANGING:
					case WM_WINDOWPOSCHANGED:
					case WM_DISPLAYCHANGE:
					case WM_DPICHANGED:
					case WM_ACTIVATEAPP:
					case WM_ACTIVATE:
					case WM_SETFOCUS:
					case WM_KILLFOCUS:
					case WM_ENABLE:
					case WM_SHOWWINDOW:
					case WM_SIZE:
					case WM_MOVE:
					case WM_MOVING:
					case WM_SIZING:
					case WM_DEVICECHANGE:
					case WM_POWERBROADCAST:
					case WM_TIMECHANGE:
					case WM_TIMER:
					case WM_NCHITTEST:
					case WM_SETCURSOR:
					case WM_GETTEXT:
					case WM_GETTEXTLENGTH:
						return call_original_wndproc(hWnd, uMsg, wParam, lParam);
					}
					
					return 0;
				}
			}
		}

		return call_original_wndproc(hWnd, uMsg, wParam, lParam);
	}

	HRESULT __stdcall hookedD3D11ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
		if (!renderer.ready()) {
			return phookD3D11ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
		}

		release_render_target_for_display_change("resize_buffers");
		add_log("d3d11 resize: invalidate device objects");

		const HRESULT hr = phookD3D11ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
		if (FAILED(hr)) {
			add_log("d3d11 resize: ResizeBuffers failed");

			renderer.pSwapChain = pSwapChain;
			renderer.reset(0, 0);
			if (ImGui::GetCurrentContext()) {
				ImGui_ImplDX11_CreateDeviceObjects();
			}
			set_render_ready(renderer.ready() && renderer.RenderTargetView != 0);
			display_transition = false;
			present_setup = Game.renderReady;

			if (hr == DXGI_ERROR_INVALID_CALL && Game.renderReady) {
				return S_OK;
			}

			return hr;
		}

		renderer.pSwapChain = pSwapChain;
		renderer.reset(Width, Height);

		DXGI_SWAP_CHAIN_DESC desc{};
		if (SUCCEEDED(pSwapChain->GetDesc(&desc))) {
			Game.window = desc.OutputWindow;
			ws_server::set_window(Game.window);
		}

		if (ImGui::GetCurrentContext()) {
			ImGui_ImplDX11_CreateDeviceObjects();
		}

		set_render_ready(renderer.ready() && renderer.RenderTargetView != 0);
		display_transition = false;
		if (Game.renderReady) {
			add_log("d3d11 resize: device objects restored");
		} else {
			add_log("d3d11 resize: render target restore failed");
		}
		return hr;
	}
	int64_t reload_time = 2000;

	static ImGuiContext* g_main_ctx = nullptr;

	inline void feed_manual_input() {
		// No WndProc hook (it broke majestic voice), so feed ImGui input manually
		// every frame: mouse position, buttons, and keys via async state.
		ImGuiIO& io = ImGui::GetIO();
		if (Game.window) {
			POINT p{};
			if (GetCursorPos(&p)) {
				ScreenToClient(Game.window, &p);
				io.AddMousePosEvent(static_cast<float>(p.x), static_cast<float>(p.y));
			}
		}
		const bool lb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
		const bool rb = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
		const bool mb = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
		io.AddMouseButtonEvent(0, lb);
		io.AddMouseButtonEvent(1, rb);
		io.AddMouseButtonEvent(2, mb);
		if (Game.menuOpen) {
			// Poll navigation keys for the menu (ImGuiKey codes).
			struct KeyPair { ImGuiKey key; int vk; };
			static const KeyPair kKeys[] = {
				{ ImGuiKey_UpArrow, VK_UP }, { ImGuiKey_DownArrow, VK_DOWN },
				{ ImGuiKey_LeftArrow, VK_LEFT }, { ImGuiKey_RightArrow, VK_RIGHT },
				{ ImGuiKey_Enter, VK_RETURN }, { ImGuiKey_Backspace, VK_BACK },
				{ ImGuiKey_Escape, VK_ESCAPE }, { ImGuiKey_Tab, VK_TAB },
				{ ImGuiKey_A, 'A' }, { ImGuiKey_C, 'C' }, { ImGuiKey_V, 'V' },
			};
			for (const auto& kp : kKeys) {
				io.AddKeyEvent(kp.key, (GetAsyncKeyState(kp.vk) & 0x8000) != 0);
			}
		}
	}

	inline void render_frame() {
		bool frame_started = false;
		__try {
			if (!ImGui::GetCurrentContext()) return;
			feed_manual_input();
			if (Game.menuOpen) {
				ImGui::GetIO().WantCaptureMouse = true;
				ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
			} else {
				ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
			}
			ImGui::GetIO().MouseDrawCursor = Game.menuOpen;

			runtime_debug::last_section = "render_begin_scene";
			if (!renderer.BeginScene()) {
				set_render_ready(false);
				return;
			}
			frame_started = true;
			runtime_debug::last_section = "game_render";
			game::game_render();
			runtime_debug::last_section = "render_end_scene";
			renderer.EndScene();
			frame_started = false;
			runtime_debug::last_section = "render_draw_data";
			if (!renderer.Render()) {
				set_render_ready(false);
				return;
			}
			runtime_debug::last_section = "render_frame_done";
		}
		__except(d3d_exception_filter("render_frame", GetExceptionInformation())) {
			if (frame_started) renderer.AbortScene();
			set_render_ready(false);
		}
	}

	HRESULT __stdcall hookedD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
		// The new majestic client creates its OWN overlay window + swapchain.
		// Rendering our ImGui into it breaks its render/input loop (symptom:
		// no render + can't move after majestic update). Only render into the
		// GTA5 game window (class "grcWindow"); skip everything else.
		static thread_local bool sc_skip = false;
		{
			DXGI_SWAP_CHAIN_DESC d{};
			if (SUCCEEDED(pSwapChain->GetDesc(&d)) && d.OutputWindow) {
				char cls[64] = {};
				GetClassNameA(d.OutputWindow, cls, sizeof(cls));
				if (cls[0] != '\0') {
					// GTA5 uses "grcWindow". Other classes = overlay windows
					// (e.g. the majestic client UI) — skip them so we never
					// render our ImGui into the client's own surface.
					sc_skip = (strcmp(cls, "grcWindow") != 0);
				} else {
					sc_skip = false;
				}
			} else {
				sc_skip = false;
			}
			if (sc_skip) {
				return phookD3D11Present(pSwapChain, SyncInterval, Flags);
			}
		}

		if (!present_setup) {

			auto now = std::chrono::high_resolution_clock::now();

			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - resize_counter).count();
			if (dur > reload_time) {
				present_setup = true;

				DXGI_SWAP_CHAIN_DESC desc;

				if (SUCCEEDED(pSwapChain->GetDesc(&desc))) {
					Game.window = desc.OutputWindow;
					ws_server::set_window(Game.window);

					// NOTE: no WndProc hook. Even a single install of our WndProc
					// breaks the majestic client's own WndProc chain (voice dies
					// after minimizing). Menu key is handled via GetAsyncKeyState
					// in the tick loop instead; ImGui input is fed manually.

					try {
						renderer.SetResourceLoad(load_image_assets);
						renderer.Initialize(Game.window, pSwapChain);
						add_log("renderer initialized");

						{
							ID3D11Device* _hud_dev = nullptr;
							pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&_hud_dev);
							if (_hud_dev) {
								hud::s_device = _hud_dev;
								_hud_dev->Release();
							}
						}

						g_main_ctx = ImGui::GetCurrentContext();
						set_render_ready(true);
						display_transition = false;
					}
					catch (const std::exception&) {
					}
					catch (...) {
						add_log("Unknown exception in renderer init");
					}
				}

				resize_counter = std::chrono::high_resolution_clock::now();
			}
		}
		if (display_transition) {
			return phookD3D11Present(pSwapChain, SyncInterval, Flags);
		}

		if (!Game.renderReady && present_setup) {
			resize_counter = std::chrono::high_resolution_clock::now();
			reload_time = 5000;
			add_log("waiting for init");
			present_setup = false;
		}
		if (!Game.renderReady) return phookD3D11Present(pSwapChain, SyncInterval, Flags);

		WINDOWINFO info;
		info.cbSize = sizeof(WINDOWINFO);
		if (!GetWindowInfo(Game.window, &info)) {
			return phookD3D11Present(pSwapChain, SyncInterval, Flags);
		}
		Game.screen.x = ((info.rcClient.right) - (info.rcClient.left));
		Game.screen.y = ((info.rcClient.bottom) - (info.rcClient.top));

		render_frame();
		return phookD3D11Present(pSwapChain, SyncInterval, Flags);
	}

	void call_thread(game_thread* this_ptr, int ops_to_execute) {
		if ((
			(this_ptr->m_context.m_script_hash == 0x26FB4AB9 || this_ptr->m_context.m_script_hash == 1104607124 || this_ptr->m_context.m_script_hash == 3381724246) ||
			(this_ptr->m_context.m_script_hash == 2003913879 || this_ptr->m_context.m_script_hash == 3522812357)
			)) {
			auto& thread = game_thread::get();
			auto orig_thread = thread;
			thread = this_ptr;
			game_fiber->on_tick();
			thread = orig_thread;
		}
	}

	void on_native_thread() {

		static uint64_t    last = 0;
		uint64_t        cur = *(uint64_t*)(FrameCount);
		if (last != cur) {
			last = cur;

			game::game_tick();

			tick::pnative.on_tick();
		}

		game_fiber->wait(0);
	}

	void call_fiber() {
		add_log("init fiber");
		game_fiber = std::make_unique<fiber::fiber_task>(static_cast<HMODULE>(nullptr), on_native_thread);
	}

	uintptr_t hooked_native_thread(game_thread* this_ptr, int ops_to_execute) {
		call_thread(this_ptr, ops_to_execute);
		return original_native_thread(this_ptr, ops_to_execute);
	}

	uintptr_t original_set_shoot_at_coords = 0;
	void on_shoot_at_coords(native::nsdk::NativeContext& context) {
		add_log("on_shoot_at_coords");
		reinterpret_cast<decltype(&on_shoot_at_coords)>(original_set_shoot_at_coords)(context);
	}
	uintptr_t original_task_shoot_at_coord = 0;
	void on_task_shoot_at_coord(native::nsdk::NativeContext& context) {
		add_log("task_shoot_at_coord");
		reinterpret_cast<decltype(&on_task_shoot_at_coord)>(original_task_shoot_at_coord)(context);
	}

	uintptr_t original_get_is_task_active = 0;
	void on_get_is_task_active(native::nsdk::NativeContext& context) {
		int32_t task = context.GetArgument<int32_t>(1);
		reinterpret_cast<decltype(&on_get_is_task_active)>(original_get_is_task_active)(context);
	}

	uintptr_t original_set_vehicle_engine_on = 0;
	void on_set_vehicle_engine_on(native::nsdk::NativeContext& context) {
		reinterpret_cast<decltype(&on_set_vehicle_engine_on)>(original_set_vehicle_engine_on)(context);
	}
	uintptr_t original_task_leave_vehicle = 0;
	void on_task_leave_vehicle(native::nsdk::NativeContext& context) {
		if (config::get("misc", "no_force_exit", 0)) {
			auto ped = hacks::local_ped_handle();
			if (context.GetArgument<int32_t>(0) == ped)
				return;
		}
		reinterpret_cast<decltype(&on_task_leave_vehicle)>(original_task_leave_vehicle)(context);
	}

	uintptr_t original_get_player_invincible = 0;
	void on_get_player_invincible(native::nsdk::NativeContext& context) {
		if (config::get("misc", "spoof_godmode_checks", 0)) {
			auto ped = hacks::local_ped_handle();
			if (context.GetArgument<int32_t>(0) == ped) {
				context.SetResult<bool>(0, false);
			}
		}
		reinterpret_cast<decltype(&on_get_player_invincible)>(original_get_player_invincible)(context);
	}

	uintptr_t original_create_ped = 0;
	void on_create_ped(native::nsdk::NativeContext& context) {
		if (config::get("misc", "hide_anticheat_npcs", 0)) {
			context.SetArgument<bool>(2, 0.f);
			context.SetArgument<bool>(3, 0.f);
			context.SetArgument<bool>(4, 0.f);
		}
		reinterpret_cast<decltype(&on_create_ped)>(original_create_ped)(context);
	}

	uintptr_t hooked_gta_gun_shot(
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
		char a12) {
		;
		return original_gta_event_gun_shot(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
	}

	uintptr_t original_clear_ped_tasks_immediately = 0;
	void on_clear_ped_tasks_immediately(native::nsdk::NativeContext& context) {
		if (config::get("misc", "spoof_clear_ped_tasks", 0)) {
			auto ped = hacks::local_ped_handle();
			if (context.GetArgument<int32_t>(0) == ped) {
				if (Game.allowClearTasks) {
					Game.allowClearTasks = false;
				}
				else {
					return;
				}
			}
		}
		reinterpret_cast<decltype(&on_clear_ped_tasks_immediately)>(original_clear_ped_tasks_immediately)(context);
	}

	bool hook_game() {
		

		call_fiber();
		// The gta_script_thread_tick hook drives game::game_tick (ESP/aimbot).
		// A/B proved it is NOT the cause of the voice drop; keep it enabled.
		if (config::get("misc", "native_thread_hook", 1) != 0) {
			MH_CreateHook(gta_script_thread_tick, hooked_native_thread, reinterpret_cast<void**>(&original_native_thread));
			if (gta_script_thread_tick) {
				if (MH_EnableHook(gta_script_thread_tick) == MH_OK) {
				}
			}
		}
		auto set_vehicle_engine_on = native::invoker::find_native_handler(0x2497c4717c8b881e);
		if (set_vehicle_engine_on) {
			MH_CreateHook(set_vehicle_engine_on, on_set_vehicle_engine_on, reinterpret_cast<void**>(&original_set_vehicle_engine_on));
			if (MH_EnableHook(set_vehicle_engine_on) == MH_OK) {
			}
		}

		auto task_leave_vehicle = native::invoker::find_native_handler(0xd3dbce61a490be02);
		if (task_leave_vehicle) {
			MH_CreateHook(task_leave_vehicle, on_task_leave_vehicle, reinterpret_cast<void**>(&original_task_leave_vehicle));
			if (MH_EnableHook(task_leave_vehicle) == MH_OK) {
				add_log("task_leave_vehicle ok");
			}
		}

		auto get_player_invincible = native::invoker::find_native_handler(0xb721981b2b939e07);
		if (get_player_invincible) {
			MH_CreateHook(get_player_invincible, on_get_player_invincible, reinterpret_cast<void**>(&original_get_player_invincible));
			if (MH_EnableHook(get_player_invincible) == MH_OK) {
				add_log("get_player_invincible ok");
			}
		}

		auto create_ped = native::invoker::find_native_handler(0xd49f9b0955c367de);
		if (create_ped) {
			MH_CreateHook(create_ped, on_create_ped, reinterpret_cast<void**>(&original_create_ped));
			if (MH_EnableHook(create_ped) == MH_OK) {
				add_log("create_ped ok");
			}
		}

		add_log("game hooks done");
		return true;
	}
	
	static bool wndproc_hooked = false;
	
	void patch_wndproc() {
		if (!Game.window) return;
		// Called exactly once from the first Present, when the window is known.
		// Must NOT be re-applied periodically (that broke majestic voice).
		
		if (!IsWindow(Game.window)) {
			add_log("window is no longer valid");
			return;
		}
		
		if (wndproc_hooked && Game.originalWndProc != 0) {
			LONG_PTR currentWndProc = GetWindowLongPtr(Game.window, GWLP_WNDPROC);
			if (currentWndProc == (LONG_PTR)hook_WndProc) {
				return;
			}
		}
		
		LONG_PTR currentWndProc = GetWindowLongPtr(Game.window, GWLP_WNDPROC);
		if (currentWndProc == 0) {
			add_log("failed to get current wndproc");
			return;
		}
		
		if (currentWndProc != (LONG_PTR)hook_WndProc) {
			add_log("wndproc needs rehook");
			
			SetLastError(0);
			
			WNDPROC original = (WNDPROC)currentWndProc;
			if (original != 0 && original != hook_WndProc) {
				Game.originalWndProc = original;
			}
			
			WNDPROC newOriginal = (WNDPROC)SetWindowLongPtr(Game.window, GWLP_WNDPROC, (LONG_PTR)hook_WndProc);
			
			DWORD err = GetLastError();
			if (err != 0 && newOriginal == 0) {
				Game.originalWndProc = 0;
				return;
			}
			
			wndproc_hooked = true;
			add_log("wndproc hook installed successfully");
		}

		return;
	}

	bool hook_renderer() {
		add_log("hook renderer");
		logs::logSystemInfo();

		// A/B: the new majestic-client may hook Present itself; our kiero hook on
		// top can break its render/input loop (symptom: no render + can't move).
		// Re-enabled for swapchain diagnostics.
		if (config::get("misc", "render_hook_enable", 1) == 0) {
			add_log("render hook disabled");
			return true;
		}

		if (kiero::init(kiero::RenderType::D3D11) != kiero::Status::Success) {
			add_log("d3d11 init failed");
			return false;
		}

		if (kiero::bind(8, reinterpret_cast<void**>(&phookD3D11Present), reinterpret_cast<void*>(hookedD3D11Present)) != kiero::Status::Success) {
			add_log("present hook failed");
			return false;
		}

		if (kiero::bind(13, reinterpret_cast<void**>(&phookD3D11ResizeBuffers), reinterpret_cast<void*>(hookedD3D11ResizeBuffers)) != kiero::Status::Success) {
			add_log("resize hook failed");
			return false;
		}

		Game.renderMethod = 2;
		add_log("d3d11 hooked");
		return true;
	}

	bool disable() {
		add_log("Unloading");

		if (Game.window && Game.originalWndProc && IsWindow(Game.window)) {
			SetWindowLongPtr(Game.window, GWLP_WNDPROC, (LONG_PTR)Game.originalWndProc);
			Game.originalWndProc = 0;
			wndproc_hooked = false;
		}

		Game.tick = nullptr;
		kiero::shutdown();
		MH_DisableHook(MH_ALL_HOOKS);
		MH_RemoveHook(MH_ALL_HOOKS);
		MH_Uninitialize();

		return true;
	}
	bool Ready() {
		if ((GetModuleHandleA("d3d11.dll") != NULL) || (GetModuleHandleA("dxgi.dll") != NULL)) return true;
		return false;
	}

	bool enable() {
		add_log("hook enable");
		while (Game.running == false) {
			if (Ready()) {
				add_log("d3d detected");
				if (MH_Initialize() != MH_OK) {
					add_log("minhook init failed");
					return false;
				}
				add_log("minhook ok");
				do {
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					add_log("waiting for game");
				} while (*Game.gta_game_state != game_state_t::playing);
				add_log("game playing");

				bool rendererResult = hook_renderer();

				bool gameResult = hook_game();

				// WndProc hook is installed once on the first Present
				// (see hookedD3D11Present) — not here, where the window may not
				// be known yet.

				Game.tick = game::game_tick;

				Game.menuOpen = config::get("menu", "openOnLoad", 0);

				return rendererResult && gameResult;
			}
			std::this_thread::sleep_for(1000ms);
		}
		return false;
	}
}
