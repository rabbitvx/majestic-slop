#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <imgui.h>

namespace model_preview
{
	bool init(ID3D11Device* device, ID3D11DeviceContext* ctx);
	bool load_obj(const std::string& path);
	bool load_glb(const std::string& path);
	bool load_glb_from_memory(const void* data, size_t size);
	bool load_test_cube(); // diagnostic: known-good cube, bypasses the OBJ loader

	// Loads the embedded model / skeleton assets.
	bool load_happ_embedded();
	bool load_bones_embedded();

	void render(float rotation_y = 0.f, float rotation_x = 0.f,
	            bool silhouette = false, int debug_mode = 0,
	            float view_scale = 1.f);
	void shutdown();

	ImTextureID get_texture();
	bool is_loaded();
	int  get_triangle_count();

	// ----- Skeleton overlay (loaded from a separate .glb that contains an
	// armature). Drawn as 2D lines through ImGui's draw list — independent
	// from the 3D model render pipeline.
	bool load_bones_glb(const std::string& path);
	bool load_bones_glb_from_memory(const void* data, size_t size);
	int  get_bone_count();
	void draw_skeleton_overlay(ImDrawList* dl,
	                           ImVec2 img_min, ImVec2 img_max,
	                           float  rot_y,   float  rot_x,
	                           ImU32  color,   float  thickness,
	                           float  view_scale = 1.f);
}
