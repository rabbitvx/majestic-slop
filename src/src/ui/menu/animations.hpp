#pragma once

#include <unordered_map>

namespace ui_anim_storage {
    template < typename T >
    inline std::unordered_map< ImGuiID, T > values;

    template < typename T >
    inline std::unordered_map< ImGuiID, size_t > step_indices;
}

template < typename T >
inline T& anim_obj( const char* id, int seed, T arg ) {
	ImGuiID im_id = ImHashStr( id, 0, seed );

	auto& map = ui_anim_storage::values< T >;
	auto result = map.find( im_id );

	if ( result == map.end( ) ) {
		result = map.insert( { im_id, arg } ).first;
	}

	return result->second;
}

template < typename T >
inline T& anim_obj( ImGuiID id, int seed, T arg ) {
	ImGuiID im_id = ImHashData( &id, sizeof( id ), seed );

	auto& map = ui_anim_storage::values< T >;
	auto result = map.find( im_id );

	if ( result == map.end( ) ) {
		result = map.insert( { im_id, arg } ).first;
	}

	return result->second;
}

template < typename T >
inline T anim( T v, T min, T max, bool state, float speed = 14.f ) {
	return ImLerp( v, state ? max : min, speed * ImGui::GetIO( ).DeltaTime );
}

template < typename T >
inline T anim2( const char* id, int seed, T v, std::vector< T > steps, bool state, float speed = 19.f ) {
	ImGuiID im_id = ImHashStr( id, 0, seed );

	auto& map = ui_anim_storage::step_indices< T >;
	auto step = map.find( im_id );

	if ( step == map.end( ) ) {
		step = map.insert( { im_id, 0 } ).first;
	}

	if ( abs( steps[step->second] - v ) < 0.01f ) {
        if ( step->second < steps.size( ) - 1 )
            step->second++;
    }

	if ( !state ) step->second = 0;

	return ImLerp( v, steps[step->second], speed * ImGui::GetIO( ).DeltaTime );
}

inline ImColor col_anim( ImColor inactive, ImColor active, float anim ) {
	return ImGui::ColorConvertFloat4ToU32( ImLerp( inactive.Value, active.Value, anim ) );
}
