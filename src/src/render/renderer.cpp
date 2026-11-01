#include <fstream>
#include <iostream>
#include <ctime>
#include <cmath>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx11.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_internal.h"
#include "./renderer.h"
#include "core/d3d11_image.hpp"
#include "ui/menu/menu.hpp"
#include "ui/menu/gui.h"
#include "useful.h"
#include "platform/debug.hpp"
#include "platform/crash_logger.hpp"

#define SAFE_RELEASE(pObject) { if(pObject) { (pObject)->Release(); (pObject) = NULL; } }

namespace {
	struct dx11_backend_probe {
		ID3D11Device* device;
		ID3D11DeviceContext* context;
	};

	bool imgui_dx11_backend_ready() {
		if (!ImGui::GetCurrentContext()) return false;
		auto* backend = static_cast<dx11_backend_probe*>(ImGui::GetIO().BackendRendererUserData);
		return backend && backend->device && backend->context;
	}

	int render_draw_data_exception_filter(EXCEPTION_POINTERS* exception) {
		crash_logger::log_exception("imgui_render_draw_data", exception);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	bool render_draw_data_seh(ImDrawData* draw_data) {
		__try {
			ImGui_ImplDX11_RenderDrawData(draw_data);
			return true;
		}
		__except (render_draw_data_exception_filter(GetExceptionInformation())) {
			HELLO_RUNTIME_LOG("SEH: imgui render draw data 0x%08X", GetExceptionCode());
			return false;
		}
	}
}

bool imgui_render::ready() {
	return pDevice && pContext;
}

void imgui_render::SetResourceLoad(vResourceLoadCall funct2) {
	
	loadCall = funct2;
}

static float resolve_font_key(float desiredSize, float minKey = 8.f, float maxKey = 16.f) {
	float key = std::round(desiredSize / dpi_scale);
	if (key < minKey) key = minKey;
	if (key > maxKey) key = maxKey;
	return key;
}
ImFont* imgui_render::EspBaseFont(float desiredSize, float* renderSize) {
	ImFont* selected = fonts[font].get(resolve_font_key(desiredSize));
	if (!selected) {
		selected = fonts[font].get(13.f);
	}
	if (!selected) {
		selected = espFont;
	}
	if (renderSize) {
		*renderSize = selected ? selected->FontSize : desiredSize;
	}
	return selected;
}

ImFont* imgui_render::EspNameFont(float desiredSize, float* renderSize) {
	ImFont* selected = fonts[fontb].get(resolve_font_key(desiredSize, 7.f, 24.f));
	if (!selected) {
		selected = fonts[fontb].get(13.f);
	}
	if (!selected) {
		selected = hudFont;
	}
	if (renderSize) {
		*renderSize = selected ? selected->FontSize : desiredSize;
	}
	return selected;
}

ImFont* imgui_render::EspSmallFont(float desiredSize, float* renderSize) {
	ImFont* selected = fonts[font_small].get(resolve_font_key(desiredSize, 7.f, 24.f));
	if (!selected) {
		selected = fonts[font_small].get(10.f);
	}
	if (!selected) {
		selected = EspNameFont(desiredSize, nullptr);
	}
	if (renderSize) {
		*renderSize = selected ? selected->FontSize : desiredSize;
	}
	return selected;
}

ImFont* imgui_render::EspSmallFont(float* renderSize) {
	return EspSmallFont(10.f * dpi_scale, renderSize);
}

ImFont* imgui_render::EspWeaponIconFont(float desiredSize, float* renderSize) {
	float key = std::round(desiredSize / dpi_scale);
	if (key < 8.f) key = 8.f;
	if (key > 42.f) key = 42.f;
	ImFont* selected = fonts[weapon_icon_font].get(key);
	if (!selected) {
		selected = fonts[weapon_icon_font].get(13.f);
	}
	if (renderSize) {
		*renderSize = selected ? selected->FontSize : desiredSize;
	}
	return selected;
}
void imgui_render::Initialize(HWND targetWindow, IDXGISwapChain* pSwapchain) {
	if (finishedInit) {
		logs::add("Renderer::Initialize SKIPPED - already initialized");
		return;
	}
	logs::add("Renderer::Initialize START");
	logs::add("targetWindow: " + std::to_string((uintptr_t)targetWindow));
	logs::add("pSwapchain: " + std::to_string((uintptr_t)pSwapchain));
	
	if (!targetWindow) {
		logs::add("ERROR: targetWindow is NULL!");
		return;
	}
	if (!pSwapchain) {
		logs::add("ERROR: pSwapchain is NULL!");
		return;
	}
	if (!IsWindow(targetWindow)) {
		logs::add("ERROR: targetWindow is not a valid window!");
		return;
	}
	
	logs::add("Creating ImGui context...");
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	
	if (!ImGui::GetCurrentContext()) {
		logs::add("ERROR: Failed to create ImGui context!");
		return;
	}
	logs::add("ImGui context created successfully");
	
	io = ImGui::GetIO(); (void)io;
	pSwapChain = pSwapchain;

	io.IniFilename = NULL;
	logs::add("IniFilename disabled");

	logs::add("Getting D3D11 device from swapchain...");
	HRESULT hr = pSwapchain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice);
	if (SUCCEEDED(hr)) {
		logs::add("GetDevice succeeded, pDevice: " + std::to_string((uintptr_t)pDevice));
		pDevice->GetImmediateContext(&pContext);
		logs::add("pContext: " + std::to_string((uintptr_t)pContext));
	} else {
		logs::addHResult("GetDevice FAILED", hr);
		return;
	}
	
	if (!pDevice || !pContext) {
		logs::add("ERROR: pDevice or pContext is NULL after GetDevice!");
		return;
	}


	logs::add("Calling loadCall callback...");
	if (loadCall) {
		loadCall(pDevice);
		logs::add("loadCall completed");
	} else {
		logs::add("WARNING: loadCall is NULL!");
	}

	logs::add("Initializing ImGui Win32...");
	if (!ImGui_ImplWin32_Init(targetWindow)) {
		logs::add("ERROR: ImGui_ImplWin32_Init FAILED!");
		return;
	}
	logs::add("ImGui_ImplWin32_Init succeeded");

	logs::add("Initializing ImGui DX11...");
	if (!ImGui_ImplDX11_Init(pDevice, pContext)) {
		logs::add("ERROR: ImGui_ImplDX11_Init FAILED!");
		return;
	}
	logs::add("ImGui_ImplDX11_Init succeeded");

	logs::add("Loading default fonts...");
	imFont = io.Fonts->AddFontDefault();
	espFont = nullptr;
	smallFont = nullptr;
	tahomaBoldFont = nullptr;
	verdanaBoldFont = nullptr;
	calibriFont = nullptr;
	calibriIndicatorFont = nullptr;
	hudFont = nullptr;
	logs::add("Default fonts loaded");
	menu::initialize(pDevice);

	hudFont = fonts[fontb].get(13.f);
	if (!hudFont) {
		hudFont = fonts[font].get(13.f);
	}
	if (!hudFont) {
		hudFont = ImGui::GetFont();
	}

	espFont = fonts[font_tahoma_bold].get(13.f);
	if (!espFont) {
		espFont = hudFont;
	}

	smallFont = fonts[font_small].get(10.f);
	if (!smallFont) {
		smallFont = espFont;
	}

	tahomaBoldFont = fonts[font_tahoma_bold].get(10.f);
	if (!tahomaBoldFont) {
		tahomaBoldFont = espFont;
	}

	verdanaBoldFont = fonts[font_verdana_bold].get(11.f);
	calibriFont = fonts[font_calibri].get(12.f);
	calibriIndicatorFont = fonts[font_calibri_bold].get(22.f);

	imFont = hudFont;
	if (!imFont) {
		imFont = espFont;
	}
	if (!imFont && io.Fonts && io.Fonts->Fonts.Size > 0) {
		imFont = io.Fonts->Fonts[0];
	}

	finishedInit = true;
	logs::add("Renderer::Initialize COMPLETE");
	return;
}

void imgui_render::release() {
	logs::add("Renderer::release called");
	
	if (pContext) {
		pContext->OMSetRenderTargets(0, 0, 0);
	}
	
	if (RenderTargetView) {
		RenderTargetView->Release();
		RenderTargetView = 0;
	}

	ImGui_ImplDX11_InvalidateDeviceObjects();
	ImGui_ImplWin32_Shutdown();
	ImGui_ImplDX11_Shutdown();
	menu::shutdown();

	pDevice = 0;
	pContext = 0;
	pSwapChain = 0;

	finishedInit = false;
	logs::add("Renderer::release completed");
}
void imgui_render::reset(UINT Width, UINT Height) {
	if (!pSwapChain || !pDevice || !pContext) return;
	if (RenderTargetView) {
		RenderTargetView->Release();
		RenderTargetView = 0;
	}

	ID3D11Texture2D* pBuffer;
	hres = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
		(void**)&pBuffer);
	if (FAILED(hres) || !pBuffer) return;

	D3D11_TEXTURE2D_DESC backbuffer_desc{};
	pBuffer->GetDesc(&backbuffer_desc);
	if (Width == 0) {
		Width = backbuffer_desc.Width;
	}
	if (Height == 0) {
		Height = backbuffer_desc.Height;
	}

	hres = pDevice->CreateRenderTargetView(pBuffer, NULL,
		&RenderTargetView);
	pBuffer->Release();
	if (FAILED(hres)) return;
}
bool imgui_render::BeginScene() {
	if (!pSwapChain || !pContext || !pDevice) {
		return false;
	}
	if (!ImGui::GetCurrentContext() || !imgui_dx11_backend_ready()) {
		return false;
	}

	ImGuiIO& io = ImGui::GetIO();

	if (RenderTargetView == 0) {
		ID3D11Texture2D* backbuffer = NULL;

		hres = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backbuffer);

		if (FAILED(hres) || !backbuffer) {
			return false;
		}

		hres = pDevice->CreateRenderTargetView(backbuffer, NULL, &RenderTargetView);
		backbuffer->Release();
		if (FAILED(hres)) {
			return false;
		}
	}
	pContext->OMSetRenderTargets(1, &RenderTargetView, NULL);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
	ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.0f, 0.0f, 0.0f, 0.0f });
	ImGui::Begin("##Backbuffer", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings);
	ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);

	return true;
}

bool imgui_render::Render() {
	if (!pDevice || !pContext || !ImGui::GetCurrentContext() || !imgui_dx11_backend_ready()) {
		return false;
	}
	runtime_debug::last_section = "imgui_render";

	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();
	if (!draw_data) {
		return false;
	}

	runtime_debug::last_section = "imgui_render_draw_data";
	if (!render_draw_data_seh(draw_data)) return false;

	if (init_fonts) {
		if (!imgui_dx11_backend_ready()) return false;
		auto& io = ImGui::GetIO();
		io.Fonts->Clear();

		for (auto& font_entry : fonts) {
			font_entry.get_fonts().clear();
			font_entry.init(font_entry.should_init, false);
		}

		hudFont = fonts[fontb].get(13.f);
		if (!hudFont) {
			hudFont = fonts[font].get(13.f);
		}

		espFont = fonts[font_tahoma_bold].get(13.f);
		if (!espFont) {
			espFont = hudFont;
		}

		smallFont = fonts[font_small].get(10.f);
		if (!smallFont) {
			smallFont = espFont;
		}

		tahomaBoldFont = fonts[font_tahoma_bold].get(10.f);
		if (!tahomaBoldFont) {
			tahomaBoldFont = espFont;
		}

		verdanaBoldFont = fonts[font_verdana_bold].get(11.f);
		calibriFont = fonts[font_calibri].get(12.f);
		calibriIndicatorFont = fonts[font_calibri_bold].get(22.f);

		imFont = hudFont ? hudFont : espFont;
		io.FontDefault = espFont;
		ImGui_ImplDX11_CreateDeviceObjects();
		init_fonts = false;
	}

	runtime_debug::last_section = "imgui_render_done";
	return true;
}
void imgui_render::EndScene() {
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window) {
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(3);
	}

	if (ImGui::GetCurrentContext() && ImGui::GetCurrentContext()->WithinFrameScope) {
		ImGui::EndFrame();
	}
}

void imgui_render::AbortScene() {
	__try {
		ImGuiContext* ctx = ImGui::GetCurrentContext();
		if (!ctx || !ctx->WithinFrameScope) return;
		if (ctx->CurrentWindow && ctx->CurrentWindowStack.Size > 1) {
			ImGui::End();
		}
		while (ctx->ColorStack.Size > 0) ImGui::PopStyleColor();
		while (ctx->StyleVarStack.Size > 0) ImGui::PopStyleVar();
		ImGui::EndFrame();
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}
}
bool inScreen(ImVec2 pos) {
	const ImVec2 display_size = ImGui::GetIO().DisplaySize;
	if (pos.x < 0 || pos.y < 0 || pos.x > display_size.x || pos.y > display_size.y) {
		return false;
	}

	return true;
}

float imgui_render::RenderText(const std::string& text, const ImVec2& position, float size, RGBA color, bool center, bool outine) {
	ImFont* activeFont = espFont ? espFont : imFont;
	if (!activeFont) {
		activeFont = hudFont;
	}
	if (!activeFont && io.Fonts && io.Fonts->Fonts.Size > 0) {
		activeFont = io.Fonts->Fonts[0];
	}
	if (!activeFont) {
		activeFont = ImGui::GetFont();
	}
	if (!activeFont) return position.y;

	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	if (!draw_list) return position.y;

	float y = 0.0f;
	int i = 0;
	const char* line_begin = text.c_str();
	const char* text_end = line_begin + text.size();
	const ImU32 text_col = IM_COL32(color.r, color.g, color.b, color.a);
	const ImU32 outline_col = IM_COL32(0, 0, 0, color.a);

	while (line_begin < text_end) {
		const char* line_end = line_begin;
		while (line_end < text_end && *line_end != '\n') {
			++line_end;
		}

		ImVec2 textSize = activeFont->CalcTextSizeA(size, FLT_MAX, 0.0f, line_begin, line_end);
		const float line_y = position.y + textSize.y * i;
		const float text_x = center ? position.x - textSize.x / 2.0f : position.x;

		if (outine) {
			draw_list->AddText(activeFont, size, { text_x + 1.0f, line_y + 1.0f }, outline_col, line_begin, line_end);
			draw_list->AddText(activeFont, size, { text_x - 1.0f, line_y - 1.0f }, outline_col, line_begin, line_end);
			draw_list->AddText(activeFont, size, { text_x + 1.0f, line_y - 1.0f }, outline_col, line_begin, line_end);
			draw_list->AddText(activeFont, size, { text_x - 1.0f, line_y + 1.0f }, outline_col, line_begin, line_end);
		}
		draw_list->AddText(activeFont, size, { text_x, line_y }, text_col, line_begin, line_end);

		y = position.y + textSize.y * (i + 1);
		i++;

		line_begin = line_end;
		if (line_begin < text_end && *line_begin == '\n') {
			++line_begin;
		}
	}

	return y;
}

void imgui_render::RenderDot(const ImVec2& from, const ImVec2& to, RGBA color, float thickness) {

	if (inScreen(from) && inScreen(to))
		imgui_render::RenderRect(from, to, color, 5.0f, 0, thickness);
}

void imgui_render::RenderLine(const ImVec2& from, const ImVec2& to, RGBA color, float thickness) {
	if (inScreen(from) && inScreen(to))
		ImGui::GetBackgroundDrawList()->AddLine(from, to, IM_COL32(color.r, color.g, color.b, color.a), thickness);
}

void imgui_render::RenderCircle(const ImVec2& position, float radius, RGBA color, float thickness, uint32_t segments) {
	if (inScreen(position))
		ImGui::GetBackgroundDrawList()->AddCircle(position, radius, IM_COL32(color.r, color.g, color.b, color.a), segments, thickness);
}

void imgui_render::RenderCircleFilled(const ImVec2& position, float radius, RGBA color, uint32_t segments) {
	if (inScreen(position))
		ImGui::GetBackgroundDrawList()->AddCircleFilled(position, radius, IM_COL32(color.r, color.g, color.b, color.a), segments);
}

void imgui_render::RenderRect(const ImVec2& from, const ImVec2& to, RGBA color, float rounding, uint32_t roundingCornersFlags, float thickness) {
	if (inScreen(from) && inScreen(to))
		ImGui::GetBackgroundDrawList()->AddRect(from, to, IM_COL32(color.r, color.g, color.b, color.a), rounding, roundingCornersFlags, thickness);
}

void imgui_render::RenderRectFilled(const ImVec2& from, const ImVec2& to, RGBA color, float rounding, uint32_t roundingCornersFlags) {
	if (inScreen(from) && inScreen(to))
		ImGui::GetBackgroundDrawList()->AddRectFilled(from, to, IM_COL32(color.r, color.g, color.b, color.a), rounding, roundingCornersFlags);
}

