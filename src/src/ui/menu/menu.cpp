#include "menu.hpp"
#include "config/interface.hpp"
#include "network/playerlist_snapshot.hpp"
#include "runtime/player_marks.hpp"
#include "network/ws_bridge.hpp"
#define HELLO_CLOUD_CLIENT_NO_GAME
#undef HELLO_CLOUD_CLIENT_NO_GAME
#include "gui.h"
#include "notify_bridge.hpp"
#include "platform/build_profile.hpp"
#include "platform/crash_logger.hpp"
#include "platform/debug.hpp"
#include "platform/hello_paths.hpp"
#include "runtime/session.hpp"
#include "assets/majestic_weapon_icons_font.hpp"

#include "features/visuals/esp/weapon_arrow_weapons.hpp"
#include "features/visuals/config_color.hpp"
#include "features/visuals/object_hash_registry.hpp"
#include "features/visuals/weapons_highlight.hpp"
#include "features/visuals/esp/overlay.hpp"
#include "ui/menu/menu_actions.hpp"
#include "ui/preview/model_preview.h"
#include <imgui_impl_dx11.h>
#include "core/d3d11_image.hpp"

#include <algorithm>
#include <atomic>
#include <array>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace hacks {
    void disable_panic_features();
}

namespace hello_notify {
    void push(const std::string& message, status type, float duration) {
        notify::notify_status mapped_status = notify::notify_info;

        switch ( type ) {
            case status::success:
                mapped_status = notify::notify_success;
                break;
            case status::error:
                mapped_status = notify::notify_error;
                break;
            case status::info:
            default:
                mapped_status = notify::notify_info;
                break;
        }

        notify::add( message, mapped_status, duration );
    }
}

namespace {
    bool g_initialized = false;
    bool g_pages_added = false;
    char g_config_name_buffer[128] = "default";
    bool g_configs_loaded = false;
    DWORD g_next_config_list_attempt_ms = 0;
    std::atomic_bool g_config_list_loading{ false };
    std::atomic_bool g_config_list_done{ false };
    std::atomic_bool g_config_list_ok{ false };
    std::vector<std::string> g_local_config_names;
    std::vector<std::string> g_local_config_paths;
    int g_local_config_selected = 0;

    int menu_exception_filter( const char* tag, EXCEPTION_POINTERS* exception ) {
        crash_logger::log_exception( tag ? tag : "menu", exception );
        return EXCEPTION_EXECUTE_HANDLER;
    }

    bool render_model_preview_safe( float rotation_y, float rotation_x, float view_scale ) {
        __try {
            runtime_debug::last_section = "visuals_preview_model";
            model_preview::render( rotation_y, rotation_x, false, 0, view_scale );
            return true;
        }
        __except ( menu_exception_filter( "visuals_preview_model", GetExceptionInformation( ) ) ) {
            return false;
        }
    }

    bool draw_model_preview_skeleton_safe( ImDrawList* draw_list, ImVec2 model_min, ImVec2 model_max, float rotation_y, float rotation_x, ImU32 color, float thickness, float view_scale ) {
        __try {
            runtime_debug::last_section = "visuals_preview_skeleton";
            model_preview::draw_skeleton_overlay( draw_list, model_min, model_max, rotation_y, rotation_x, color, thickness, view_scale );
            return true;
        }
        __except ( menu_exception_filter( "visuals_preview_skeleton", GetExceptionInformation( ) ) ) {
            return false;
        }
    }

    bool is_window_descendant_of( const ImGuiWindow* window, const ImGuiWindow* parent ) {
        for ( const ImGuiWindow* current = window; current != nullptr; current = current->ParentWindow ) {
            if ( current == parent ) {
                return true;
            }
        }

        return false;
    }

    bool is_window_in_begin_stack_of( const ImGuiWindow* window, const ImGuiWindow* parent ) {
        for ( const ImGuiWindow* current = window; current != nullptr; current = current->ParentWindowInBeginStack ) {
            if ( current == parent ) {
                return true;
            }
        }

        return false;
    }

    bool is_window_part_of_popup_hierarchy( const ImGuiWindow* window, const ImGuiWindow* popup_window ) {
        if ( window == nullptr || popup_window == nullptr ) {
            return false;
        }

        if ( window == popup_window ) {
            return true;
        }

        if ( window->RootWindowPopupTree == popup_window->RootWindowPopupTree ) {
            return true;
        }

        return is_window_descendant_of( window, popup_window ) || is_window_in_begin_stack_of( window, popup_window );
    }

    struct script_hotkey_state {
        bool active = false;
        bool was_down = false;
        bool toggled = false;
    };

    std::map<std::string, script_hotkey_state> g_script_hotkeys;
    std::set<std::string> g_hydrated_menu_items;
    std::string g_listening_bind;
    int g_bind_ignore_frames = 0;
    ID3D11ShaderResourceView* g_playerlist_avatar = nullptr;
    char g_object_hash_search_buffer[64] = {};
    char g_object_hash_display_name_buffer[64] = {};
    DWORD g_object_hash_display_name_hash = 0;
    DWORD g_object_hash_selected = 0;
    bool g_object_hash_rows_were_empty = true;
    bool g_object_hash_esp_was_enabled = false;
    bool g_object_hash_scroll_top = false;
    char g_highlight_weapon_hash_buffer[32] = {};
    char g_highlight_weapon_name_buffer[64] = {};
    float g_highlight_weapon_color[4] { 180.f / 255.f, 167.f / 255.f, 245.f / 255.f, 1.f };
    DWORD g_highlight_weapon_settings_hash = 0;
    char g_ignore_family_id_buffer[32] = {};

    static const std::vector<const char*> k_bone_items {
        "nearest",
        "head",
        "neck",
        "chest",
        "spine",
        "pelvis"
    };

    struct bone_multi_item {
        const char* id;
        const char* label;
    };

    static const std::vector<bone_multi_item> k_bone_multi_items {
        { "head", "head" },
        { "neck", "neck" },
        { "chest", "chest" },
        { "spine", "spine" },
        { "pelvis", "pelvis" }
    };

    static const std::vector<const char*> k_box_style_items {
        "default",
        "corner",
        "filled"
    };

    static const std::vector<const char*> k_health_mode_items {
        "none",
        "hp only",
        "hp and armor",
        "adaptive"
    };

    static const std::vector<const char*> k_esp_element_font_items {
        "mini",
        "bold"
    };

    static const std::vector<const char*> k_esp_element_case_items {
        "default",
        "lowercase",
        "uppercase"
    };

    static const std::vector<const char*> k_weapon_text_language_items {
        "english",
        "russian"
    };

    static const std::vector<const char*> k_fraction_color_mode_items {
        "default",
        "static",
        "custom"
    };

    static const std::vector<const char*> k_relation_color_mode_items {
        "default",
        "custom"
    };


    static const std::vector<const char*> k_bar_position_items {
        "left",
        "right",
        "top",
        "bottom"
    };


    static const std::vector<const char*> k_bind_mode_items {
        "hold",
        "toggle",
        "always on"
    };

    struct fraction_multi_item {
        const char* id;
        const char* label;
    };

    static const std::vector<fraction_multi_item> k_ignore_fraction_items {
        { "5", "GOV" },
        { "12", "Marabunta" },
        { "4", "SANG" },
        { "2", "EMS" },
        { "1", "LSPD" },
        { "7", "FIB" },
        { "6", "WN" },
        { "8", "Ballas" },
        { "9", "Vagos" },
        { "3", "Sheriff" },
        { "10", "Families" },
        { "11", "Bloods" }
    };

    struct bind_config_keys {
        const char* bind_key;
        const char* bind_mode;
        const char* legacy_category;
        const char* legacy_key;
        const char* legacy_mode_key;
    };

    enum class esp_preview_settings_kind {
        none,
        element,
        box,
        skeleton
    };

    struct esp_preview_settings_popup {
        bool open = false;
        bool just_opened = false;
        float anim = 0.f;
        esp_preview_settings_kind kind = esp_preview_settings_kind::none;
        esp::projected_esp_element_id element = esp::projected_esp_element_id::none;
        ImVec2 pos{};
    };

    bool g_preview_skeleton_invisible = false;

    using player_list_entry = game::player_list_ui_entry;

    std::string trim_player_text( std::string value ) {
        value.erase( value.begin( ), std::find_if( value.begin( ), value.end( ), []( unsigned char ch ) {
            return !std::isspace( ch );
        } ) );
        value.erase( std::find_if( value.rbegin( ), value.rend( ), []( unsigned char ch ) {
            return !std::isspace( ch );
        } ).base( ), value.end( ) );
        return value;
    }

    bool parse_object_hash( const char* text, DWORD& out_hash ) {
        std::string value = trim_player_text( text ? text : "" );
        if ( value.empty( ) ) {
            return false;
        }
        if ( value[0] == '-' ) {
            return false;
        }

        const bool has_hex_letters = value.find_first_of( "abcdefABCDEF" ) != std::string::npos;
        char* end = nullptr;
        errno = 0;
        const unsigned long parsed = std::strtoul( value.c_str( ), &end, has_hex_letters ? 16 : 0 );
        if ( end == value.c_str( ) || ( end && *end != '\0' ) || parsed == 0 || errno == ERANGE ) {
            return false;
        }

        out_hash = static_cast< DWORD >( parsed );
        return true;
    }

    bool parse_positive_id( const char* text, int& out_id ) {
        std::string value = trim_player_text( text ? text : "" );
        if ( value.empty( ) || value[0] == '-' ) {
            return false;
        }

        char* end = nullptr;
        errno = 0;
        const unsigned long parsed = std::strtoul( value.c_str( ), &end, 10 );
        if ( end == value.c_str( ) || ( end && *end != '\0' ) || parsed == 0 || parsed > static_cast<unsigned long>( ( std::numeric_limits<int>::max )( ) ) || errno == ERANGE ) {
            return false;
        }

        out_id = static_cast<int>( parsed );
        return true;
    }

    std::string trim_player_text( const char* value ) {
        if ( !value ) {
            return { };
        }

        return trim_player_text( std::string( value ) );
    }

    std::string lowercase_copy( std::string value ) {
        std::transform( value.begin( ), value.end( ), value.begin( ), []( unsigned char ch ) {
            return static_cast<char>( std::tolower( ch ) );
        } );
        return value;
    }

    std::string build_timestamp_string( ) {
        static const char* month_names[] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };

        char month[4] {};
        int day = 0;
        int year = 0;
        int hour = 0;
        int minute = 0;
        if ( std::sscanf( __DATE__, "%3s %d %d", month, &day, &year ) != 3 ||
             std::sscanf( __TIME__, "%d:%d", &hour, &minute ) != 2 ) {
            return std::string( __DATE__ ) + " " + __TIME__;
        }

        int month_index = 0;
        for ( int i = 0; i < 12; ++i ) {
            if ( std::strcmp( month, month_names[i] ) == 0 ) {
                month_index = i;
                break;
            }
        }

        char buffer[32] {};
        std::snprintf( buffer, sizeof( buffer ), "%d %s %d %02d:%02d", day, month_names[month_index], year, hour, minute );
        return buffer;
    }

    void text_unformatted_clipped( const char* text, float clip_max_x ) {
        if ( !text || clip_max_x <= ImGui::GetCursorPosX( ) ) {
            ImGui::Dummy( ImVec2( 0.f, ImGui::GetTextLineHeight( ) ) );
            return;
        }

        ImGuiWindow* window = ImGui::GetCurrentWindow( );
        const char* text_end = text + std::strlen( text );
        const ImVec2 pos = window->DC.CursorPos;
        const float line_height = ImGui::GetTextLineHeight( );
        const float clip_max_screen_x = window->Pos.x + clip_max_x;
        const ImRect clip_rect( pos, ImVec2( clip_max_screen_x, pos.y + line_height ) );
        ImGui::RenderTextClipped( pos, clip_rect.Max, text, text_end, nullptr, ImVec2( 0.f, 0.f ), &clip_rect );

        const float available_width = ( std::max )( 0.f, clip_max_screen_x - pos.x );
        const float text_width = ImGui::CalcTextSize( text, text_end, false ).x;
        ImGui::Dummy( ImVec2( ( std::min )( text_width, available_width ), line_height ) );
    }

    ImU32 read_config_color_u32(
        const char* category,
        const char* r_key,
        const char* g_key,
        const char* b_key,
        const char* a_key,
        float default_r,
        float default_g,
        float default_b,
        float default_a
    ) {
        const float r = std::clamp( config::get( category, r_key, default_r ), 0.f, 1.f );
        const float g = std::clamp( config::get( category, g_key, default_g ), 0.f, 1.f );
        const float b = std::clamp( config::get( category, b_key, default_b ), 0.f, 1.f );
        const float a = std::clamp( config::get( category, a_key, default_a ), 0.f, 1.f );

        return IM_COL32(
            static_cast<int>( r * 255.f ),
            static_cast<int>( g * 255.f ),
            static_cast<int>( b * 255.f ),
            static_cast<int>( a * 255.f )
        );
    }

    enum class player_relation {
        neutral,
        friend_player,
        enemy_player
    };

    struct object_hash_row {
        DWORD hash = 0;
        std::string display_name;
        std::string label;
        bool seen = false;
        bool has_custom_color = false;
        bool line_enabled = false;
        bool box_enabled = false;
    };

    std::string format_object_hash( DWORD hash ) {
        char buffer[16];
        sprintf_s( buffer, "0x%X", hash );
        return buffer;
    }

    std::string object_hash_label( DWORD hash, const std::string& display_name ) {
        std::string label = format_object_hash( hash );
        if ( !display_name.empty( ) ) {
            label += " (" + display_name + ")";
        }

        return label;
    }

    void copy_to_buffer( char* buffer, size_t buffer_size, const std::string& value ) {
        if ( !buffer || buffer_size == 0 ) {
            return;
        }

        std::snprintf( buffer, buffer_size, "%s", value.c_str( ) );
    }

    bool object_hash_line_enabled( const std::map<DWORD, std::string>& hash_lines, DWORD hash ) {
        const auto it = hash_lines.find( hash );
        return it != hash_lines.end( ) && it->second == "1";
    }

    bool object_hash_box_enabled( const std::map<DWORD, std::string>& hash_boxes, DWORD hash ) {
        const auto it = hash_boxes.find( hash );
        return it != hash_boxes.end( ) && it->second == "1";
    }

    bool object_hash_seen( const std::vector<DWORD>& seen_hashes, DWORD hash ) {
        return std::find( seen_hashes.begin( ), seen_hashes.end( ), hash ) != seen_hashes.end( );
    }

    std::vector<object_hash_row> build_object_hash_rows(
        const std::map<DWORD, std::string>& hash_names,
        const std::map<DWORD, std::string>& hash_colors,
        const std::map<DWORD, std::string>& hash_lines,
        const std::map<DWORD, std::string>& hash_boxes,
        const std::vector<DWORD>& seen_hashes
    ) {
        std::set<DWORD> hashes( seen_hashes.begin( ), seen_hashes.end( ) );
        for ( const auto& [hash, name] : hash_names ) {
            ( void ) name;
            hashes.insert( hash );
        }
        for ( const auto& [hash, color] : hash_colors ) {
            ( void ) color;
            hashes.insert( hash );
        }
        for ( const auto& [hash, line] : hash_lines ) {
            ( void ) line;
            hashes.insert( hash );
        }
        for ( const auto& [hash, box] : hash_boxes ) {
            ( void ) box;
            hashes.insert( hash );
        }

        std::vector<object_hash_row> rows;
        rows.reserve( hashes.size( ) );
        for ( DWORD hash : hashes ) {
            const auto name_it = hash_names.find( hash );
            const std::string display_name = name_it != hash_names.end( ) ? name_it->second : std::string{ };
            rows.push_back( {
                hash,
                display_name,
                object_hash_label( hash, display_name ),
                object_hash_seen( seen_hashes, hash ),
                hash_colors.find( hash ) != hash_colors.end( ),
                object_hash_line_enabled( hash_lines, hash ),
                object_hash_box_enabled( hash_boxes, hash )
            } );
        }

        return rows;
    }


    bool matches_search( const std::string& text, const char* search ) {
        if ( !search || search[0] == '\0' ) {
            return true;
        }

        const std::string lowered_text = lowercase_copy( text );
        const std::string lowered_search = lowercase_copy( trim_player_text( search ) );
        return lowered_text.find( lowered_search ) != std::string::npos;
    }

    void set_window_scroll_y( ImGuiWindow* window, float scroll_y ) {
        if ( !window ) {
            return;
        }

        const float value = std::clamp( scroll_y, 0.f, ( std::max )( 0.f, window->ScrollMax.y ) );
        window->Scroll.y = value;
        window->scroll_y = value;
        window->ScrollTarget.y = value;
        window->ScrollTargetCenterRatio.y = 0.f;
        window->ScrollTargetEdgeSnapDist.y = 0.f;
    }

    bool set_current_child_scroll_top( ) {
        bool applied = false;
        for ( ImGuiWindow* window = ImGui::GetCurrentWindow( ); window && ( window->Flags & ImGuiWindowFlags_ChildWindow ); window = window->ParentWindow ) {
            set_window_scroll_y( window, 0.f );
            applied = true;
        }

        return applied;
    }

    void clamp_current_child_scroll_y( ) {
        for ( ImGuiWindow* window = ImGui::GetCurrentWindow( ); window && ( window->Flags & ImGuiWindowFlags_ChildWindow ); window = window->ParentWindow ) {
            const float max_scroll_y = ( std::max )( 0.f, window->ScrollMax.y );
            if ( window->scroll_y > max_scroll_y || window->Scroll.y > max_scroll_y ) {
                set_window_scroll_y( window, max_scroll_y );
            }
        }
    }

    void save_player_relation( const player_list_entry& entry, player_relation relation ) {
        if ( entry.static_id <= 0 ) {
            return;
        }

        const auto mark = relation == player_relation::friend_player
            ? player_marks::relation::friend_player
            : relation == player_relation::enemy_player
                ? player_marks::relation::enemy_player
                : player_marks::relation::neutral;

        if ( player_marks::server_id().empty( ) ) {
            const std::string server_id = ws_server::get_server_id();
            if ( server_id.empty() ) {
                hello_notify::push( "server id is not ready", hello_notify::status::error );
                return;
            }
            player_marks::set_server_id( server_id );
        }

        const player_marks::relation previous_mark = player_marks::persistent_get( entry.static_id );
        player_marks::set_pending( entry.static_id, mark );
        std::string message = "marks cleared: " + entry.display_name;
        hello_notify::status status = hello_notify::status::info;
        if ( relation == player_relation::friend_player ) {
            message = "friend added: " + entry.display_name;
            status = hello_notify::status::success;
        } else if ( relation == player_relation::enemy_player ) {
            message = "enemy added: " + entry.display_name;
            status = hello_notify::status::success;
        }

        player_marks::set( entry.static_id, mark );
        player_marks::clear_pending_if( entry.static_id, mark );
        hello_notify::push( message, status );
        (void)previous_mark;
    }

    bool add_ignored_family_id( int family_id, bool enable_ignore, bool notify ) {
        if ( family_id <= 0 ) {
            return false;
        }

        std::map<std::string, std::string> ignored_families =
            config::get( "aimbot", "ignore_families", std::map<std::string, std::string>{ } );
        const std::string family_key = std::to_string( family_id );
        const auto existing = ignored_families.find( family_key );
        const bool already_ignored = existing != ignored_families.end( ) && existing->second == "1";

        ignored_families[family_key] = "1";
        config::update( ignored_families, "aimbot", "ignore_families", std::map<std::string, std::string>{ } );

        if ( enable_ignore ) {
            config::update( 1, "aimbot", "ignore_other_families", 0 );
        }

        if ( notify ) {
            hello_notify::push(
                std::string( already_ignored ? "family already ignored: " : "family ignored: " ) + family_key,
                already_ignored ? hello_notify::status::info : hello_notify::status::success
            );
        }

        return true;
    }

    static const bind_config_keys k_vector_aim_bind {
        "vector_aim_key",
        "vector_aim_mode",
        "aimbot",
        "aim_key",
        "aim_key_mode"
    };

    static const bind_config_keys k_panic_bind {
        "panic_key",
        "panic_mode",
        "misc",
        "panic_key",
        "panic_mode"
    };




    bool refresh_local_configs( );
    void render_local_configs_panel( );
    void render_aimbot_page( );
    void render_visuals_players_page( );
    void render_visuals_world_page( );
    void render_misc_page( );
    void render_settings_page( );
    void render_hashes_page( );
    void render_playerlist_page( );
    void player_esp_options( );
    void box_options( );
    void health_static_options( );
    void armor_options( );
    void skeleton_options( );
    void rebuild_fonts( );
    void render_esp_preview_inline( );
    void setup_pages( );

    int read_bind_key( const bind_config_keys& bind_keys ) {
        int value = config::get( "binds", bind_keys.bind_key, 0 );
        if ( value == 0 && bind_keys.legacy_category && bind_keys.legacy_key ) {
            value = config::get( bind_keys.legacy_category, bind_keys.legacy_key, 0 );
        }

        return value;
    }

    int read_bind_mode( const bind_config_keys& bind_keys ) {
        int value = config::get( "binds", bind_keys.bind_mode, 0 );
        if ( value == 0 && bind_keys.legacy_category && bind_keys.legacy_mode_key ) {
            value = config::get( bind_keys.legacy_category, bind_keys.legacy_mode_key, 0 );
        }

        if ( value < 0 || static_cast<size_t>( value ) >= k_bind_mode_items.size( ) ) {
            value = 0;
        }

        return value;
    }

    void save_bind_state( const bind_config_keys& bind_keys, int key, int mode ) {
        config::update( key, "binds", bind_keys.bind_key, 0 );
        config::update( mode, "binds", bind_keys.bind_mode, 0 );

        if ( bind_keys.legacy_category && bind_keys.legacy_key ) {
            config::update( key, bind_keys.legacy_category, bind_keys.legacy_key, 0 );
        }

        if ( bind_keys.legacy_category && bind_keys.legacy_mode_key ) {
            config::update( mode, bind_keys.legacy_category, bind_keys.legacy_mode_key, 0 );
        }
    }

    void render_bind_mode_popup( const bind_config_keys& bind_keys ) {
        int mode = read_bind_mode( bind_keys );
        const ImVec2 popup_item_size = ImVec2( SCALE( 230 ), GImGui->FontSize );

        ImGui::TextDisabled( "mode" );

        const std::string hold_id = std::string( "hold##" ) + bind_keys.bind_key;
        if ( ui::selectable( hold_id.c_str( ), mode == 1, popup_item_size ) ) {
            save_bind_state( bind_keys, read_bind_key( bind_keys ), 1 );
        }

        const std::string toggle_id = std::string( "toggle##" ) + bind_keys.bind_key;
        if ( ui::selectable( toggle_id.c_str( ), mode == 0, popup_item_size ) ) {
            save_bind_state( bind_keys, read_bind_key( bind_keys ), 0 );
        }

        const std::string always_on_id = std::string( "always on##" ) + bind_keys.bind_key;
        if ( ui::selectable( always_on_id.c_str( ), mode == 2, popup_item_size ) ) {
            save_bind_state( bind_keys, read_bind_key( bind_keys ), 2 );
        }
    }

    void vector_aim_options( );

    void clamp_index( int& index, size_t size ) {
        if ( size == 0 ) {
            index = 0;
            return;
        }

        if ( index < 0 ) {
            index = 0;
            return;
        }

        if ( static_cast<size_t>( index ) >= size ) {
            index = static_cast<int>( size ) - 1;
        }
    }

    std::string trim_copy( std::string value ) {
        while ( !value.empty( ) && std::isspace( static_cast<unsigned char>( value.front( ) ) ) ) {
            value.erase( value.begin( ) );
        }

        while ( !value.empty( ) && std::isspace( static_cast<unsigned char>( value.back( ) ) ) ) {
            value.pop_back( );
        }

        return value;
    }

    void copy_string( char* buffer, size_t buffer_size, const std::string& value ) {
        if ( !buffer || buffer_size == 0 ) {
            return;
        }

        std::strncpy( buffer, value.c_str( ), buffer_size - 1 );
        buffer[buffer_size - 1] = '\0';
    }

    std::string sanitize_config_name( const char* raw ) {
        std::string result;

        if ( raw ) {
            for ( size_t i = 0; raw[i] != '\0'; ++i ) {
                const unsigned char ch = static_cast<unsigned char>( raw[i] );
                if ( std::isalnum( ch ) || ch == '_' || ch == '-' || ch == ' ' ) {
                    result.push_back( static_cast<char>( ch ) );
                }
            }
        }

        result = trim_copy( result );
        if ( result.empty( ) ) {
            result = "default";
        }

        return result;
    }

    std::string read_text_file( const std::string& path ) {
        try {
            std::ifstream file( path, std::ios::binary );
            if ( !file.is_open( ) ) {
                return { };
            }

            return std::string(
                std::istreambuf_iterator<char>( file ),
                std::istreambuf_iterator<char>( )
            );
        } catch ( ... ) {
            return { };
        }
    }

    std::string key_name_for_vk( int vk ) {
        switch ( vk ) {
            case 0: return "none";
            case VK_LBUTTON: return "m1";
            case VK_RBUTTON: return "m2";
            case VK_MBUTTON: return "m3";
            case VK_XBUTTON1: return "m4";
            case VK_XBUTTON2: return "m5";
            case VK_MENU: return "alt";
            case VK_CONTROL: return "ctrl";
            case VK_SHIFT: return "shift";
            case VK_SPACE: return "space";
            case VK_TAB: return "tab";
            case VK_INSERT: return "ins";
            case VK_DELETE: return "del";
            case VK_HOME: return "home";
            case VK_END: return "end";
            case VK_PRIOR: return "pgup";
            case VK_NEXT: return "pgdn";
            default: break;
        }

        if ( vk >= '0' && vk <= '9' ) {
            return std::string( 1, static_cast<char>( std::tolower( vk ) ) );
        }

        if ( vk >= 'A' && vk <= 'Z' ) {
            return std::string( 1, static_cast<char>( vk ) );
        }

        if ( vk >= VK_F1 && vk <= VK_F24 ) {
            return "f" + std::to_string( vk - VK_F1 + 1 );
        }

        UINT scan_code = MapVirtualKeyA( static_cast<UINT>( vk ), MAPVK_VK_TO_VSC );
        if ( vk == VK_LEFT || vk == VK_UP || vk == VK_RIGHT || vk == VK_DOWN ||
             vk == VK_PRIOR || vk == VK_NEXT || vk == VK_END || vk == VK_HOME ||
             vk == VK_INSERT || vk == VK_DELETE ) {
            scan_code |= 0x100;
        }

        LONG lparam = static_cast<LONG>( scan_code << 16 );
        char buffer[64] { };
        if ( GetKeyNameTextA( lparam, buffer, static_cast<int>( sizeof( buffer ) ) ) > 0 ) {
            std::string name = buffer;
            std::transform( name.begin( ), name.end( ), name.begin( ), []( unsigned char ch ) { return static_cast<char>( std::tolower( ch ) ); } );
            return name;
        }

        return "vk " + std::to_string( vk );
    }

    bool capture_bind_key( const std::string& id, int& key ) {
        if ( g_listening_bind != id ) {
            return false;
        }

        if ( g_bind_ignore_frames > 0 ) {
            --g_bind_ignore_frames;
            return false;
        }

        for ( int vk = 1; vk < 256; ++vk ) {
            if ( ( GetAsyncKeyState( vk ) & 1 ) == 0 ) {
                continue;
            }

            key = vk == VK_ESCAPE ? 0 : vk;
            g_listening_bind.clear( );
            return true;
        }

        return false;
    }


    std::string strip_imgui_label_id( const char* label ) {
        std::string value = label ? label : "";
        const size_t marker = value.find( "##" );
        if ( marker != std::string::npos ) {
            value.resize( marker );
        }
        return value;
    }

    std::string normalize_ui_path_part( const std::string& value ) {
        std::string result;
        result.reserve( value.size( ) );
        for ( char ch : value ) {
            const unsigned char uch = static_cast<unsigned char>( ch );
            if ( std::isalnum( uch ) || ch == '_' ) result.push_back( static_cast<char>( std::tolower( uch ) ) );
            else if ( !result.empty( ) && result.back( ) != '_' ) result.push_back( '_' );
        }
        while ( !result.empty( ) && result.front( ) == '_' ) result.erase( result.begin( ) );
        while ( !result.empty( ) && result.back( ) == '_' ) result.pop_back( );
        return result.empty( ) ? "item" : result;
    }

    std::string current_builtin_tab( ) {
        if ( ui::cur_page >= 0 && ui::cur_page < static_cast<int>( ui::tabs.size( ) ) ) {
            return ui::tabs[ui::cur_page].label ? ui::tabs[ui::cur_page].label : "menu";
        }
        return "menu";
    }

    std::string current_builtin_child( ) {
        ImGuiWindow* window = ImGui::GetCurrentWindow( );
        if ( !window || !window->Name ) return "main";
        std::string name = window->Name;
        const size_t slash = name.find_last_of( '/' );
        if ( slash != std::string::npos && slash + 1 < name.size( ) ) {
            name = name.substr( slash + 1 );
        }
        const size_t hash = name.find( "##" );
        if ( hash != std::string::npos ) {
            name.resize( hash );
        }
        return name.empty( ) ? "main" : name;
    }

    std::string builtin_control_id( const std::string& tab, const std::string& child, const std::string& label ) {
        return normalize_ui_path_part( tab ) + "/" + normalize_ui_path_part( child ) + "/" + normalize_ui_path_part( label );
    }

    json color_json( const float color[4] ) {
        return json{
            { "r", std::clamp( color[0], 0.f, 1.f ) },
            { "g", std::clamp( color[1], 0.f, 1.f ) },
            { "b", std::clamp( color[2], 0.f, 1.f ) },
            { "a", std::clamp( color[3], 0.f, 1.f ) }
        };
    }

    float color_component( const json& value, const char* key, float fallback ) {
        if ( !value.is_object( ) || !value.contains( key ) || !value[key].is_number( ) ) return fallback;
        return std::clamp( value[key].get<float>( ), 0.f, 1.f );
    }

    int hotkey_mode_to_native( const std::string& mode ) {
        if ( mode == "hold" ) return 1;
        if ( mode == "always" ) return 2;
        return 0;
    }

    std::string hotkey_mode_from_native( int mode ) {
        if ( mode == 1 ) return "hold";
        if ( mode == 2 ) return "always";
        return "toggle";
    }

    json hotkey_json( int key, int mode, bool active = false ) {
        return json{ { "key", key }, { "mode", hotkey_mode_from_native( mode ) }, { "active", active } };
    }

    struct builtin_override_state {
        bool visible = true;
        bool disabled = false;
        std::string tooltip;
    };

    struct builtin_binding {
        std::string kind;
        std::string category;
        std::string key;
        int default_int = 0;
        float default_float = 0.f;
        float min_value = 0.f;
        float max_value = 0.f;
        std::array<std::string, 4> color_keys{};
        std::array<float, 4> color_defaults{ 0.f, 0.f, 0.f, 1.f };
        std::string bind_key;
        std::string bind_mode;
        std::vector<std::string> option_ids;
        std::vector<std::string> option_labels;
    };

    std::map<std::string, builtin_binding> g_builtin_bindings;
    std::map<std::string, builtin_override_state> g_builtin_overrides;

    builtin_override_state& builtin_override( const std::string& id ) {
        return g_builtin_overrides[id];
    }

    ws_server::MenuItem make_builtin_item(
        const char* label,
        const char* kind,
        const json& value,
        const builtin_binding& binding,
        const char* tooltip = nullptr
    ) {
        const std::string tab = current_builtin_tab( );
        const std::string child = current_builtin_child( );
        const std::string clean_label = strip_imgui_label_id( label );
        const std::string id = builtin_control_id( tab, child, clean_label );
        builtin_override_state& override_state = builtin_override( id );

        ws_server::MenuItem item;
        item.script = "__builtin";
        item.id = id;
        item.kind = kind;
        item.label = clean_label;
        item.group = child;
        item.path = { tab, child, clean_label };
        item.value = value;
        item.min = binding.min_value;
        item.max = binding.max_value;
        item.option_ids = binding.option_ids;
        item.options = binding.option_labels;
        item.visible = override_state.visible;
        item.disabled = override_state.disabled;
        item.tooltip = !override_state.tooltip.empty( ) ? override_state.tooltip : ( tooltip ? tooltip : "" );
        return item;
    }

    std::string register_builtin_control(
        const char* label,
        const char* kind,
        const json& value,
        builtin_binding binding,
        const char* tooltip = nullptr
    ) {
        ws_server::MenuItem item = make_builtin_item( label, kind, value, binding, tooltip );
        const std::string id = item.id;
        g_builtin_bindings[id] = std::move( binding );
        ws_server::register_builtin_menu_item( std::move( item ) );
        return id;
    }

    bool builtin_begin_visible( const std::string& id ) {
        const auto it = g_builtin_overrides.find( id );
        return it == g_builtin_overrides.end( ) || it->second.visible;
    }

    void builtin_begin_disabled( const std::string& id ) {
        const auto it = g_builtin_overrides.find( id );
        if ( it != g_builtin_overrides.end( ) && it->second.disabled ) {
            ImGui::BeginDisabled( );
        }
    }

    void builtin_end_disabled( const std::string& id ) {
        const auto it = g_builtin_overrides.find( id );
        if ( it != g_builtin_overrides.end( ) && it->second.disabled ) {
            ImGui::EndDisabled( );
        }
    }

    void apply_builtin_value( const std::string& id, const builtin_binding& binding, const json& value ) {
        if ( binding.kind == "checkbox" ) {
            config::update( value == true ? 1 : 0, binding.category, binding.key, binding.default_int );
            ws_server::send_menu_event( "__builtin", id, value == true );
        } else if ( binding.kind == "slider_int" ) {
            const int next = value.is_number( ) ? value.get<int>( ) : binding.default_int;
            config::update( next, binding.category, binding.key, binding.default_int );
            ws_server::send_menu_event( "__builtin", id, next );
        } else if ( binding.kind == "slider_float" ) {
            const float next = value.is_number( ) ? value.get<float>( ) : binding.default_float;
            config::update( next, binding.category, binding.key, binding.default_float );
            ws_server::send_menu_event( "__builtin", id, next );
        } else if ( binding.kind == "combo" ) {
            int next = value.is_number_integer( ) ? value.get<int>( ) : binding.default_int;
            if ( value.is_string( ) ) {
                const std::string selected = value.get<std::string>( );
                const auto it = std::find( binding.option_ids.begin( ), binding.option_ids.end( ), selected );
                if ( it != binding.option_ids.end( ) ) next = static_cast<int>( std::distance( binding.option_ids.begin( ), it ) );
            }
            clamp_index( next, binding.option_ids.size( ) );
            config::update( next, binding.category, binding.key, binding.default_int );
            ws_server::send_menu_event( "__builtin", id, next );
        } else if ( binding.kind == "color_picker" ) {
            config::set( binding.category, binding.color_keys[0], std::to_string( color_component( value, "r", binding.color_defaults[0] ) ) );
            config::set( binding.category, binding.color_keys[1], std::to_string( color_component( value, "g", binding.color_defaults[1] ) ) );
            config::set( binding.category, binding.color_keys[2], std::to_string( color_component( value, "b", binding.color_defaults[2] ) ) );
            if ( !binding.color_keys[3].empty( ) ) {
                config::set( binding.category, binding.color_keys[3], std::to_string( color_component( value, "a", binding.color_defaults[3] ) ) );
            }
            ws_server::send_menu_event( "__builtin", id, value );
        } else if ( binding.kind == "hotkey" ) {
            const int key = value.is_object( ) ? value.value( "key", 0 ) : 0;
            const int mode = value.is_object( ) ? hotkey_mode_to_native( value.value( "mode", std::string( "hold" ) ) ) : 1;
            if ( !binding.category.empty( ) && !binding.key.empty( ) ) {
                config::update( key, binding.category, binding.key, binding.default_int );
            } else {
                config::update( key, "binds", binding.bind_key, 0 );
                config::update( mode, "binds", binding.bind_mode, 0 );
            }
            ws_server::send_menu_event( "__builtin", id, hotkey_json( key, mode ) );
        }
    }

    void apply_builtin_menu_updates( ) {
        const std::vector<ws_server::MenuUpdate> updates = ws_server::consume_builtin_menu_updates( );
        for ( const ws_server::MenuUpdate& update : updates ) {
            const auto binding_it = g_builtin_bindings.find( update.id );
            if ( update.patch.contains( "visible" ) ) builtin_override( update.id ).visible = update.patch.value( "visible", true );
            if ( update.patch.contains( "disabled" ) ) builtin_override( update.id ).disabled = update.patch.value( "disabled", false );
            if ( update.patch.contains( "tooltip" ) ) builtin_override( update.id ).tooltip = update.patch.value( "tooltip", std::string( ) );
            if ( binding_it != g_builtin_bindings.end( ) && update.patch.contains( "value" ) ) {
                apply_builtin_value( update.id, binding_it->second, update.patch["value"] );
            }
        }
    }

    ImFont* navbar_font( ) {
        ImFont* nav_font = fonts[font].get( 13 );
        if ( !nav_font ) {
            nav_font = ImGui::GetFont( );
        }

        return nav_font;
    }



    bool cfg_checkbox_state( const char* label, const char* category, const char* key, bool default_value = false, bool* changed = nullptr, const char* tooltip = nullptr, void( *options )( ) = nullptr ) {
        bool value = config::get( category, key, default_value ? 1 : 0 ) != 0;
        builtin_binding binding;
        binding.kind = "checkbox";
        binding.category = category;
        binding.key = key;
        binding.default_int = default_value ? 1 : 0;
        const std::string builtin_id = register_builtin_control( label, "checkbox", value, binding, tooltip );
        if ( !builtin_begin_visible( builtin_id ) ) {
            if ( changed ) *changed = false;
            return value;
        }
        builtin_begin_disabled( builtin_id );
        const bool changed_now = ui::checkbox( label, &value, nullptr, { }, options, tooltip );
        builtin_end_disabled( builtin_id );
        if ( changed_now ) {
            config::update( value ? 1 : 0, category, key, default_value ? 1 : 0 );
            ws_server::send_menu_event( "__builtin", builtin_id, value );
        }

        if ( changed ) {
            *changed = changed_now;
        }

        return value;
    }

    bool cfg_checkbox_bind_state(
        const char* label,
        const char* category,
        const char* key,
        bool default_value,
        const bind_config_keys& bind_keys,
        void( *options )( ) = nullptr,
        bool* changed = nullptr,
        const char* tooltip = nullptr
    ) {
        bool value = config::get( category, key, default_value ? 1 : 0 ) != 0;
        const int bind_key = read_bind_key( bind_keys );
        const int bind_mode = read_bind_mode( bind_keys );
        c_key bind { bind_key, bind_mode };
        builtin_binding binding;
        binding.kind = "checkbox";
        binding.category = category;
        binding.key = key;
        binding.default_int = default_value ? 1 : 0;
        const std::string builtin_id = register_builtin_control( label, "checkbox", value, binding, tooltip );
        builtin_binding hotkey_binding;
        hotkey_binding.kind = "hotkey";
        hotkey_binding.bind_key = bind_keys.bind_key;
        hotkey_binding.bind_mode = bind_keys.bind_mode;
        const std::string hotkey_label = strip_imgui_label_id( label ) + " hotkey";
        register_builtin_control( hotkey_label.c_str( ), "hotkey", hotkey_json( bind_key, bind_mode ), hotkey_binding );

        if ( !builtin_begin_visible( builtin_id ) ) {
            if ( changed ) *changed = false;
            return value;
        }
        builtin_begin_disabled( builtin_id );
        const bool changed_now = ui::checkbox( label, &value, &bind, { }, options, tooltip );
        builtin_end_disabled( builtin_id );
        if ( changed_now ) {
            config::update( value ? 1 : 0, category, key, default_value ? 1 : 0 );
            ws_server::send_menu_event( "__builtin", builtin_id, value );
        }

        if ( bind.key != bind_key || bind.mode != bind_mode ) {
            save_bind_state( bind_keys, bind.key, bind.mode );
            ws_server::send_menu_event( "__builtin", builtin_control_id( current_builtin_tab( ), current_builtin_child( ), hotkey_label ), hotkey_json( bind.key, bind.mode ) );
        }

        if ( changed ) {
            *changed = changed_now;
        }

        return value;
    }

    int cfg_combo_state( const char* label, const char* category, const char* key, int default_value, const std::vector<const char*>& items, bool* changed = nullptr ) {
        int value = config::get( category, key, default_value );
        clamp_index( value, items.size( ) );
        builtin_binding binding;
        binding.kind = "combo";
        binding.category = category;
        binding.key = key;
        binding.default_int = default_value;
        for ( const char* item : items ) {
            binding.option_ids.push_back( item ? item : "" );
            binding.option_labels.push_back( item ? item : "" );
        }
        const std::string builtin_id = register_builtin_control( label, "combo", value, binding );
        if ( !builtin_begin_visible( builtin_id ) ) {
            if ( changed ) *changed = false;
            return value;
        }

        builtin_begin_disabled( builtin_id );
        const bool changed_now = ui::combo( label, &value, items );
        builtin_end_disabled( builtin_id );
        if ( changed_now ) {
            config::update( value, category, key, default_value );
            ws_server::send_menu_event( "__builtin", builtin_id, value );
        }

        if ( changed ) {
            *changed = changed_now;
        }

        return value;
    }

    bool legacy_bone_selected( int legacy_value, const char* bone_id ) {
        if ( legacy_value <= 0 ) {
            return true;
        }

        if ( legacy_value == 1 ) return std::strcmp( bone_id, "head" ) == 0;
        if ( legacy_value == 2 ) return std::strcmp( bone_id, "neck" ) == 0;
        if ( legacy_value == 3 ) return std::strcmp( bone_id, "chest" ) == 0;
        if ( legacy_value == 4 ) return std::strcmp( bone_id, "spine" ) == 0;
        if ( legacy_value == 5 ) return std::strcmp( bone_id, "pelvis" ) == 0;
        return std::strcmp( bone_id, "head" ) == 0;
    }

    void cfg_multi_fraction_state( const char* label, const char* category, const char* key ) {
        const std::map<std::string, std::string> saved_value =
            config::get( category, key, std::map<std::string, std::string>{ } );

        std::vector<multi_select_item> items;
        std::vector<bool> before_state;
        items.reserve( k_ignore_fraction_items.size( ) );
        before_state.reserve( k_ignore_fraction_items.size( ) );

        for ( const fraction_multi_item& fraction : k_ignore_fraction_items ) {
            multi_select_item item { fraction.label };
            auto it = saved_value.find( fraction.id );
            item.selected = it != saved_value.end( ) && it->second == "1";
            before_state.push_back( item.selected );
            items.push_back( item );
        }

        ui::multi_select( label, items );

        bool changed = false;
        for ( size_t i = 0; i < items.size( ); ++i ) {
            if ( items[i].selected != before_state[i] ) {
                changed = true;
                break;
            }
        }

        if ( !changed ) {
            return;
        }

        std::map<std::string, std::string> updated_value;
        for ( size_t i = 0; i < items.size( ); ++i ) {
            if ( items[i].selected ) {
                updated_value[k_ignore_fraction_items[i].id] = "1";
            }
        }

        config::update( updated_value, category, key, std::map<std::string, std::string>{ } );
    }

    void cfg_multi_bone_state( const char* label, const char* category, const char* key, const char* legacy_key ) {
        const std::map<std::string, std::string> saved_value =
            config::get( category, key, std::map<std::string, std::string>{ } );
        const bool has_saved_value = !saved_value.empty( );
        const int legacy_value = config::get( category, legacy_key, 0 );

        std::vector<multi_select_item> items;
        std::vector<bool> before_state;
        items.reserve( k_bone_multi_items.size( ) );
        before_state.reserve( k_bone_multi_items.size( ) );

        for ( const bone_multi_item& bone : k_bone_multi_items ) {
            multi_select_item item { bone.label };

            if ( has_saved_value ) {
                auto it = saved_value.find( bone.id );
                item.selected = it != saved_value.end( ) && it->second == "1";
            } else {
                item.selected = legacy_bone_selected( legacy_value, bone.id );
            }

            before_state.push_back( item.selected );
            items.push_back( item );
        }

        ui::multi_select( label, items );

        bool changed = false;
        for ( size_t i = 0; i < items.size( ); ++i ) {
            if ( items[i].selected != before_state[i] ) {
                changed = true;
                break;
            }
        }

        if ( !changed ) {
            return;
        }

        std::map<std::string, std::string> updated_value;
        for ( size_t i = 0; i < items.size( ); ++i ) {
            if ( items[i].selected ) {
                updated_value[k_bone_multi_items[i].id] = "1";
            }
        }

        if ( updated_value.empty( ) ) {
            updated_value["head"] = "1";
        }

        config::update( updated_value, category, key, std::map<std::string, std::string>{ } );
    }

    float cfg_slider_float_state( const char* label, const char* category, const char* key, float default_value, float min_value, float max_value, const char* format ) {
        float value = config::get( category, key, default_value );
        builtin_binding binding;
        binding.kind = "slider_float";
        binding.category = category;
        binding.key = key;
        binding.default_float = default_value;
        binding.min_value = min_value;
        binding.max_value = max_value;
        const std::string builtin_id = register_builtin_control( label, "slider", value, binding );
        if ( !builtin_begin_visible( builtin_id ) ) return value;
        builtin_begin_disabled( builtin_id );
        if ( ui::slider_float( label, &value, min_value, max_value, format ) ) {
            config::update( value, category, key, default_value );
            ws_server::send_menu_event( "__builtin", builtin_id, value );
        }
        builtin_end_disabled( builtin_id );

        return value;
    }

    int cfg_slider_int_state( const char* label, const char* category, const char* key, int default_value, int min_value, int max_value, const char* format ) {
        int value = config::get( category, key, default_value );
        builtin_binding binding;
        binding.kind = "slider_int";
        binding.category = category;
        binding.key = key;
        binding.default_int = default_value;
        binding.min_value = static_cast<float>( min_value );
        binding.max_value = static_cast<float>( max_value );
        const std::string builtin_id = register_builtin_control( label, "slider", value, binding );
        if ( !builtin_begin_visible( builtin_id ) ) return value;
        builtin_begin_disabled( builtin_id );
        if ( ui::slider_int( label, &value, min_value, max_value, format ) ) {
            config::update( value, category, key, default_value );
            ws_server::send_menu_event( "__builtin", builtin_id, value );
        }
        builtin_end_disabled( builtin_id );

        return value;
    }

    void cfg_color_state( const char* label, const char* category, const char* r_key, const char* g_key, const char* b_key, const char* a_key, float default_r, float default_g, float default_b, float default_a ) {
        float color[4] {
            config::get( category, r_key, default_r ),
            config::get( category, g_key, default_g ),
            config::get( category, b_key, default_b ),
            config::get( category, a_key, default_a )
        };
        builtin_binding binding;
        binding.kind = "color_picker";
        binding.category = category;
        binding.color_keys = { r_key, g_key, b_key, a_key };
        binding.color_defaults = { default_r, default_g, default_b, default_a };
        const std::string builtin_id = register_builtin_control( label, "color_picker", color_json( color ), binding );
        if ( !builtin_begin_visible( builtin_id ) ) return;
        builtin_begin_disabled( builtin_id );

        if ( ui::color_edit( label, color ) ) {
            config::set( category, r_key, std::to_string( color[0] ) );
            config::set( category, g_key, std::to_string( color[1] ) );
            config::set( category, b_key, std::to_string( color[2] ) );
            config::set( category, a_key, std::to_string( color[3] ) );
            ws_server::send_menu_event( "__builtin", builtin_id, color_json( color ) );
        }
        builtin_end_disabled( builtin_id );
    }

    void cfg_color_rgb_state( const char* label, const char* category, const char* r_key, const char* g_key, const char* b_key, float default_r, float default_g, float default_b ) {
        float color[4] {
            config::get( category, r_key, default_r ),
            config::get( category, g_key, default_g ),
            config::get( category, b_key, default_b ),
            1.0f
        };
        builtin_binding binding;
        binding.kind = "color_picker";
        binding.category = category;
        binding.color_keys = { r_key, g_key, b_key, "" };
        binding.color_defaults = { default_r, default_g, default_b, 1.f };
        const std::string builtin_id = register_builtin_control( label, "color_picker", color_json( color ), binding );
        if ( !builtin_begin_visible( builtin_id ) ) return;
        builtin_begin_disabled( builtin_id );

        if ( ui::color_edit( label, color ) ) {
            config::set( category, r_key, std::to_string( color[0] ) );
            config::set( category, g_key, std::to_string( color[1] ) );
            config::set( category, b_key, std::to_string( color[2] ) );
            ws_server::send_menu_event( "__builtin", builtin_id, color_json( color ) );
        }
        builtin_end_disabled( builtin_id );
    }

    float color_component( ImU32 color, int shift ) {
        return static_cast<float>( ( color >> shift ) & 0xFF ) / 255.f;
    }

    void cfg_projected_esp_font_state( esp::projected_esp_element_id id ) {
        const std::string key = esp::projected_esp_font_key( id );
        const std::string label = std::string( "font##esp_style_" ) + esp::projected_esp_element_key( id );
        cfg_combo_state(
            label.c_str( ),
            "visual",
            key.c_str( ),
            static_cast<int>( esp::default_projected_esp_font_style( id ) ),
            k_esp_element_font_items
        );
    }

    void cfg_projected_esp_case_state( esp::projected_esp_element_id id ) {
        const std::string key = esp::projected_esp_case_key( id );
        const std::string label = std::string( "text style##esp_style_" ) + esp::projected_esp_element_key( id );
        cfg_combo_state(
            label.c_str( ),
            "visual",
            key.c_str( ),
            static_cast<int>( esp::default_projected_esp_text_case( id ) ),
            k_esp_element_case_items
        );
    }

    void cfg_projected_esp_size_state( esp::projected_esp_element_id id ) {
        if ( !esp::is_projected_esp_scalable_element( id ) ) {
            return;
        }

        const std::string key = esp::projected_esp_size_key( id );
        const char* visible_label = "text size";
        if ( id == esp::projected_esp_element_id::weapon_icon ) {
            visible_label = "icon size";
        } else if ( id == esp::projected_esp_element_id::health || id == esp::projected_esp_element_id::armor ) {
            visible_label = "bar size";
        }
        const std::string label = std::string( visible_label ) + "##esp_style_" + esp::projected_esp_element_key( id );
        cfg_slider_float_state(
            label.c_str( ),
            "visual",
            key.c_str( ),
            esp::default_projected_esp_size( id ),
            esp::projected_esp_size_min( id ),
            esp::projected_esp_size_max( id ),
            "%.0f"
        );
    }

    void cfg_projected_esp_show_value_state( esp::projected_esp_element_id id ) {
        if ( id != esp::projected_esp_element_id::health && id != esp::projected_esp_element_id::armor ) {
            return;
        }

        const std::string key = esp::projected_esp_show_value_key( id );
        const std::string label = std::string( "show value##esp_style_" ) + esp::projected_esp_element_key( id );
        cfg_checkbox_state( label.c_str( ), "visual", key.c_str( ), true );
    }

    void cfg_projected_esp_color_state( esp::projected_esp_element_id id, ImU32 fallback ) {
        const std::string key = esp::projected_esp_element_key( id );
        const std::string label = std::string( "color##esp_style_" ) + key;
        const std::string r_key = esp::projected_esp_color_key( id, "r" );
        const std::string g_key = esp::projected_esp_color_key( id, "g" );
        const std::string b_key = esp::projected_esp_color_key( id, "b" );
        const std::string a_key = esp::projected_esp_color_key( id, "a" );
        cfg_color_state(
            label.c_str( ),
            "visual",
            r_key.c_str( ),
            g_key.c_str( ),
            b_key.c_str( ),
            a_key.c_str( ),
            color_component( fallback, IM_COL32_R_SHIFT ),
            color_component( fallback, IM_COL32_G_SHIFT ),
            color_component( fallback, IM_COL32_B_SHIFT ),
            color_component( fallback, IM_COL32_A_SHIFT )
        );
    }

    bool cfg_checkbox_color_state(
        const char* label,
        const char* category,
        const char* key,
        bool default_value,
        const char* r_key,
        const char* g_key,
        const char* b_key,
        const char* a_key,
        float default_r,
        float default_g,
        float default_b,
        float default_a,
        void( *options )( ) = nullptr,
        const char* tooltip = nullptr
    ) {
        bool value = config::get( category, key, default_value ? 1 : 0 ) != 0;
        float color[4] {
            config::get( category, r_key, default_r ),
            config::get( category, g_key, default_g ),
            config::get( category, b_key, default_b ),
            config::get( category, a_key, default_a )
        };
        const float prev_color[4] { color[0], color[1], color[2], color[3] };
        builtin_binding binding;
        binding.kind = "checkbox";
        binding.category = category;
        binding.key = key;
        binding.default_int = default_value ? 1 : 0;
        const std::string builtin_id = register_builtin_control( label, "checkbox", value, binding, tooltip );
        builtin_binding color_binding;
        color_binding.kind = "color_picker";
        color_binding.category = category;
        color_binding.color_keys = { r_key, g_key, b_key, a_key };
        color_binding.color_defaults = { default_r, default_g, default_b, default_a };
        const std::string color_label = strip_imgui_label_id( label ) + " color";
        const std::string color_id = register_builtin_control( color_label.c_str( ), "color_picker", color_json( color ), color_binding );
        if ( !builtin_begin_visible( builtin_id ) ) return value;
        builtin_begin_disabled( builtin_id );

        const bool changed_now = ui::checkbox( label, &value, nullptr, { color }, options, tooltip );
        builtin_end_disabled( builtin_id );
        if ( changed_now ) {
            config::update( value ? 1 : 0, category, key, default_value ? 1 : 0 );
            ws_server::send_menu_event( "__builtin", builtin_id, value );
        }

        if ( prev_color[0] != color[0] || prev_color[1] != color[1] || prev_color[2] != color[2] || prev_color[3] != color[3] ) {
            config::update( color[0], category, r_key, default_r );
            config::update( color[1], category, g_key, default_g );
            config::update( color[2], category, b_key, default_b );
            config::update( color[3], category, a_key, default_a );
            ws_server::send_menu_event( "__builtin", color_id, color_json( color ) );
        }

        return value;
    }

    bool cfg_checkbox_color_split_state(
        const char* label,
        const char* checkbox_category,
        const char* checkbox_key,
        bool default_value,
        const char* color_category,
        const char* r_key,
        const char* g_key,
        const char* b_key,
        const char* a_key,
        float default_r,
        float default_g,
        float default_b,
        float default_a
    ) {
        (void)label; (void)checkbox_category; (void)checkbox_key; (void)default_value;
        (void)color_category; (void)r_key; (void)g_key; (void)b_key; (void)a_key;
        (void)default_r; (void)default_g; (void)default_b; (void)default_a;
        return config::get( checkbox_category, checkbox_key, default_value ? 1 : 0 ) != 0;
    }

    int cfg_health_mode_state( const char* label, void( *options )( ) = nullptr ) {
        int value = config::get( "visual", "health_mode", config::get( "visual", "draw_healthbar", 0 ) != 0 ? 1 : 0 );
        clamp_index( value, k_health_mode_items.size( ) );

        if ( ui::combo( label, &value, k_health_mode_items, options ) ) {
            config::update( value, "visual", "health_mode", 0 );
            config::update( value != 0 ? 1 : 0, "visual", "draw_healthbar", 0 );
        }

        return value;
    }

    void cfg_bind_state( const char* label, const char* bind_key, const char* bind_mode, const char* legacy_category, const char* legacy_key, const char* legacy_mode_key ) {
        int key = config::get( "binds", bind_key, 0 );
        if ( key == 0 && legacy_category && legacy_key ) {
            key = config::get( legacy_category, legacy_key, 0 );
        }

        int mode = config::get( "binds", bind_mode, 0 );
        if ( mode == 0 && legacy_category && legacy_mode_key ) {
            mode = config::get( legacy_category, legacy_mode_key, 0 );
        }

        builtin_binding binding;
        binding.kind = "hotkey";
        binding.bind_key = bind_key;
        binding.bind_mode = bind_mode;
        const std::string builtin_id = register_builtin_control( label, "hotkey", hotkey_json( key, mode ), binding );
        if ( !builtin_begin_visible( builtin_id ) ) {
            return;
        }

        bool changed = false;
        const std::string listen_id = std::string( "bind:" ) + bind_key;

        builtin_begin_disabled( builtin_id );
        ImGui::PushID( listen_id.c_str( ) );
        ImGui::TextDisabled( "%s", label );

        std::string key_caption = g_listening_bind == listen_id ? "press key..." : key_name_for_vk( key );
        if ( ui::button( key_caption.c_str( ), ImVec2( SCALE( 110 ), 0 ) ) ) {
            g_listening_bind = listen_id;
            g_bind_ignore_frames = 2;
        }

        if ( capture_bind_key( listen_id, key ) ) {
            changed = true;
        }

        ImGui::SameLine( 0, SCALE( 10 ) );
        if ( ui::combo( "type", &mode, k_bind_mode_items ) ) {
            changed = true;
        }
        ImGui::PopID( );
        builtin_end_disabled( builtin_id );

        if ( changed ) {
            config::update( key, "binds", bind_key, 0 );
            config::update( mode, "binds", bind_mode, 0 );

            if ( legacy_category && legacy_key ) {
                config::update( key, legacy_category, legacy_key, 0 );
            }

            if ( legacy_category && legacy_mode_key ) {
                config::update( mode, legacy_category, legacy_mode_key, 0 );
            }

            ws_server::send_menu_event( "__builtin", builtin_id, hotkey_json( key, mode ) );
        }
    }

    void cfg_simple_bind_state( const char* label, const char* category, const char* key_name, int default_key ) {
        c_key bind { config::get( category, key_name, default_key ), 0 };
        builtin_binding binding;
        binding.kind = "hotkey";
        binding.category = category;
        binding.key = key_name;
        binding.default_int = default_key;
        binding.bind_key = key_name;
        const std::string builtin_id = register_builtin_control( label, "hotkey", hotkey_json( bind.key, bind.mode ), binding );
        if ( !builtin_begin_visible( builtin_id ) || !add_widget( label ) ) {
            return;
        }

        auto* window = ImGui::GetCurrentWindow( );
        const float width = ImGui::CalcItemWidth( );
        const float height = SCALE( 17 );
        const ImVec2 pos = window->DC.CursorPos;
        const ImVec2 label_size = ImGui::CalcTextSize( label, 0, true );
        const ImRect row_bb( pos, pos + ImVec2( width, height ) );

        ImGui::ItemSize( row_bb );
        ImGui::ItemAdd( row_bb, window->GetID( label ) );
        window->DrawList->AddText( ImVec2( row_bb.Min.x, row_bb.GetCenter( ).y - label_size.y * 0.5f ), ImGui::GetColorU32( ImGuiCol_Text ), label, ImGui::FindRenderedTextEnd( label ) );

        const std::string bind_text = std::string( "key: " ).append( KEY_NAMES[bind.key] );
        const float bind_width = ImMax( SCALE( 52 ), ui::text_size( font, 13, bind_text.c_str( ) ).x + GImGui->Style.FramePadding.x * 2 );
        const ImVec2 old_pos = window->DC.CursorPos;
        window->DC.CursorPos = ImVec2( row_bb.Max.x - bind_width, row_bb.GetCenter( ).y - height * 0.5f );

        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
        builtin_begin_disabled( builtin_id );
        ImGui::BeginChild( std::string( label ).append( "simple_binder" ).c_str( ), ImVec2( bind_width, height ), 0, ImGuiWindowFlags_NoBackground );
        const bool changed = ui::binder( std::string( "##" ).append( label ).append( "binder" ).c_str( ), &bind, false );
        ImGui::EndChild( );
        builtin_end_disabled( builtin_id );
        ImGui::PopStyleVar( );
        window->DC.CursorPos = old_pos;

        if ( changed ) {
            config::update( bind.key, category, key_name, default_key );
            ws_server::send_menu_event( "__builtin", builtin_id, hotkey_json( bind.key, bind.mode ) );
        }
    }

    void player_esp_options( ) {
        cfg_checkbox_state( "altv nickname", "visual", "altv_nickname", false );
        cfg_checkbox_state( "altv static", "visual", "altv_static", false );
        cfg_checkbox_state( "altv faction", "visual", "altv_faction", false );
        cfg_checkbox_state( "altv admin", "visual", "altv_admin", false );
        cfg_checkbox_state( "altv tester", "visual", "altv_tester", false );
        cfg_checkbox_state( "altv level", "visual", "altv_level", false );
        cfg_checkbox_state( "altv dead", "visual", "altv_dead", false );
        cfg_checkbox_state( "altv afk", "visual", "altv_afk", false );
        cfg_checkbox_state( "altv media", "visual", "altv_media", false );
        cfg_checkbox_state( "admins around", "visual", "admins_around_indicator", true );
    }

    void box_options( ) {
        cfg_combo_state( "box style", "visual", "draw_box_style", 0, k_box_style_items );
        cfg_slider_float_state( "radius", "visual", "box_radius", 0.f, 0.f, 12.f, "%.1f" );
        cfg_slider_float_state( "thickness##box_options", "visual", "box_thickness", 1.f, 0.5f, 5.f, "%.1f" );
    }

    void health_static_options( ) {
        cfg_checkbox_color_state( "static health color", "visual", "healthbar_static_color", false, "healthbar_color_r", "healthbar_color_g", "healthbar_color_b", "healthbar_color_a", 0.f, 0.9f, 0.18f, 1.f );
    }

    void armor_options( ) {
        cfg_color_state( "armor color", "visual", "armorbar_color_r", "armorbar_color_g", "armorbar_color_b", "armorbar_color_a", 0.f, 122.f / 255.f, 1.f, 1.f );
    }
    void weapon_options( ) {
        cfg_color_state( "weapon color", "visual", "weap_color_r", "weap_color_g", "weap_color_b", "weap_color_a", 241.f / 255.f, 241.f / 255.f, 241.f / 255.f, 1.f );
    }

    void faction_color_options( ) {
        const int color_mode = cfg_combo_state( "color", "visual", "fraction_color_mode", 0, k_fraction_color_mode_items );
        const int legacy_custom_enabled = color_mode == 2 ? 1 : 0;
        if ( config::get( "visual", "fraction_use_custom_colors", 1 ) != legacy_custom_enabled ) {
            config::update( legacy_custom_enabled, "visual", "fraction_use_custom_colors", 1 );
        }

        if ( color_mode == 1 ) {
            cfg_color_state( "static color", "visual", "altv_faction_color_r", "altv_faction_color_g", "altv_faction_color_b", "altv_faction_color_a", 180.f / 255.f, 1.f, 240.f / 255.f, 1.f );
        } else if ( color_mode == 2 ) {
            cfg_color_state( "GOV", "visual", "fraction_5_color_r", "fraction_5_color_g", "fraction_5_color_b", "fraction_5_color_a", 64.f / 255.f, 112.f / 255.f, 1.f, 1.f );
            cfg_color_state( "Marabunta", "visual", "fraction_12_color_r", "fraction_12_color_g", "fraction_12_color_b", "fraction_12_color_a", 65.f / 255.f, 145.f / 255.f, 1.f, 1.f );
            cfg_color_state( "SANG", "visual", "fraction_4_color_r", "fraction_4_color_g", "fraction_4_color_b", "fraction_4_color_a", 98.f / 255.f, 150.f / 255.f, 78.f / 255.f, 1.f );
            cfg_color_state( "EMS", "visual", "fraction_2_color_r", "fraction_2_color_g", "fraction_2_color_b", "fraction_2_color_a", 1.f, 92.f / 255.f, 92.f / 255.f, 1.f );
            cfg_color_state( "LSPD", "visual", "fraction_1_color_r", "fraction_1_color_g", "fraction_1_color_b", "fraction_1_color_a", 56.f / 255.f, 128.f / 255.f, 1.f, 1.f );
            cfg_color_state( "FIB", "visual", "fraction_7_color_r", "fraction_7_color_g", "fraction_7_color_b", "fraction_7_color_a", 40.f / 255.f, 78.f / 255.f, 120.f / 255.f, 1.f );
            cfg_color_state( "WN", "visual", "fraction_6_color_r", "fraction_6_color_g", "fraction_6_color_b", "fraction_6_color_a", 1.f, 198.f / 255.f, 66.f / 255.f, 1.f );
            cfg_color_state( "Ballas", "visual", "fraction_8_color_r", "fraction_8_color_g", "fraction_8_color_b", "fraction_8_color_a", 152.f / 255.f, 84.f / 255.f, 1.f, 1.f );
            cfg_color_state( "Vagos", "visual", "fraction_9_color_r", "fraction_9_color_g", "fraction_9_color_b", "fraction_9_color_a", 1.f, 214.f / 255.f, 73.f / 255.f, 1.f );
            cfg_color_state( "Sheriff", "visual", "fraction_3_color_r", "fraction_3_color_g", "fraction_3_color_b", "fraction_3_color_a", 194.f / 255.f, 124.f / 255.f, 54.f / 255.f, 1.f );
            cfg_color_state( "Families", "visual", "fraction_10_color_r", "fraction_10_color_g", "fraction_10_color_b", "fraction_10_color_a", 78.f / 255.f, 210.f / 255.f, 104.f / 255.f, 1.f );
            cfg_color_state( "Bloods", "visual", "fraction_11_color_r", "fraction_11_color_g", "fraction_11_color_b", "fraction_11_color_a", 216.f / 255.f, 45.f / 255.f, 45.f / 255.f, 1.f );
        }
    }

    void relation_tags_options( ) {
        cfg_checkbox_state( "auto family", "visual", "relation_auto_family", true );
        cfg_checkbox_state( "auto fraction", "visual", "relation_auto_fraction", true );

        const int color_mode = cfg_combo_state( "color", "visual", "relation_color_mode", 0, k_relation_color_mode_items );
        if ( color_mode != 1 ) {
            return;
        }

        cfg_color_state( "friend", "visual", "friend_color_r", "friend_color_g", "friend_color_b", "friend_color_a", 110.f / 255.f, 1.f, 110.f / 255.f, 1.f );
        cfg_color_state( "enemy", "visual", "enemy_color_r", "enemy_color_g", "enemy_color_b", "enemy_color_a", 1.f, 90.f / 255.f, 90.f / 255.f, 1.f );
        cfg_color_state( "family", "visual", "family_relation_color_r", "family_relation_color_g", "family_relation_color_b", "family_relation_color_a", 1.f, 150.f / 255.f, 35.f / 255.f, 1.f );
        cfg_color_state( "fraction", "visual", "fraction_relation_color_r", "fraction_relation_color_g", "fraction_relation_color_b", "fraction_relation_color_a", 170.f / 255.f, 90.f / 255.f, 1.f, 1.f );
    }

    void skeleton_options( ) {
        cfg_checkbox_color_state( "invisible color", "visual", "skel_invisible_enabled", true, "skel_invisible_r", "skel_invisible_g", "skel_invisible_b", "skel_invisible_a", 1.f, 1.f, 1.f, 1.f );
        cfg_slider_float_state( "thickness##skeleton_options", "visual", "skeleton_thickness", 0.1f, 0.1f, 1.f, "%.1f" );
    }

    ImU32 projected_esp_popup_fallback_color( esp::projected_esp_element_id id, const esp::projected_esp_settings& settings ) {
        switch ( id ) {
            case esp::projected_esp_element_id::relation:
                return settings.enemy_color;
            case esp::projected_esp_element_id::name:
                return esp::read_visual_color_u32( "altv_nickname_color_r", "altv_nickname_color_g", "altv_nickname_color_b", "altv_nickname_color_a", 1.f, 1.f, 1.f, 1.f );
            case esp::projected_esp_element_id::static_id:
                return settings.static_color;
            case esp::projected_esp_element_id::faction:
                return settings.faction_color;
            case esp::projected_esp_element_id::admin:
                return settings.admin_color;
            case esp::projected_esp_element_id::tester:
                return esp::get_altv_tester_color_u32( );
            case esp::projected_esp_element_id::media:
                return settings.media_color;
            case esp::projected_esp_element_id::level:
                return settings.level_color;
            case esp::projected_esp_element_id::afk:
                return settings.afk_color;
            case esp::projected_esp_element_id::dead:
                return settings.dead_color;
            case esp::projected_esp_element_id::cuffed:
                return settings.cuffed_color;
            case esp::projected_esp_element_id::wanted:
                return settings.wanted_color;
            case esp::projected_esp_element_id::armor_tag:
                return settings.armor_tag_color;
            case esp::projected_esp_element_id::social:
                return settings.social_color;
            case esp::projected_esp_element_id::visible:
                return settings.visible_flag_color;
            case esp::projected_esp_element_id::armor:
                return esp::get_armor_status_color( );
            case esp::projected_esp_element_id::weapon_icon:
            case esp::projected_esp_element_id::weapon_text:
                return settings.weapon_color;
            case esp::projected_esp_element_id::distance:
                return settings.distance_color;
            default:
                return IM_COL32( 255, 255, 255, 255 );
        }
    }

    const char* esp_preview_settings_title( const esp_preview_settings_popup& popup, const esp::projected_esp_settings& settings ) {
        switch ( popup.kind ) {
            case esp_preview_settings_kind::box:
                return "box";
            case esp_preview_settings_kind::skeleton:
                return "skeleton";
            case esp_preview_settings_kind::element:
                if ( popup.element == esp::projected_esp_element_id::health && settings.health_mode == 3 ) {
                    return "adaptive";
                }
                return esp::projected_esp_element_label( popup.element );
            default:
                return "settings";
        }
    }

    void render_esp_preview_settings_content( const esp_preview_settings_popup& popup, const esp::projected_esp_settings& settings ) {
        if ( popup.kind == esp_preview_settings_kind::box ) {
            cfg_color_state( "color##esp_preview_box", "visual", "box_color_r", "box_color_g", "box_color_b", "box_color_a", 1.f, 1.f, 1.f, 1.f );
            cfg_color_state( "invisible color##esp_preview_box", "visual", "invisible_box_color_r", "invisible_box_color_g", "invisible_box_color_b", "invisible_box_color_a", 1.f, 0.f, 0.f, 1.f );
            cfg_color_state( "fill color##esp_preview_box", "visual", "draw_box_fill_color_r", "draw_box_fill_color_g", "draw_box_fill_color_b", "draw_box_fill_color_a", 0.2f, 0.2f, 0.2f, 0.2f );
            box_options( );
            return;
        }

        if ( popup.kind == esp_preview_settings_kind::skeleton ) {
            cfg_color_state( "visible color##esp_preview_skeleton", "visual", "skel_visible_r", "skel_visible_g", "skel_visible_b", "skel_visible_a", 1.f, 1.f, 1.f, 1.f );
            skeleton_options( );
            return;
        }

        if ( popup.kind != esp_preview_settings_kind::element || !esp::is_projected_esp_element( popup.element ) ) {
            return;
        }

        if ( popup.element == esp::projected_esp_element_id::health ) {
            cfg_projected_esp_size_state( popup.element );
            cfg_projected_esp_show_value_state( popup.element );
            health_static_options( );
            if ( settings.health_mode == 3 ) {
                armor_options( );
            }
            return;
        }

        if ( popup.element == esp::projected_esp_element_id::armor ) {
            cfg_projected_esp_size_state( popup.element );
            cfg_projected_esp_show_value_state( popup.element );
            armor_options( );
            return;
        }

        cfg_projected_esp_size_state( popup.element );

        if ( popup.element == esp::projected_esp_element_id::weapon_text ) {
            cfg_combo_state( "language##esp_weapon_text", "visual", "esp_weapon_text_language", 1, k_weapon_text_language_items );
        }

        if ( esp::is_projected_esp_text_element( popup.element ) && popup.element != esp::projected_esp_element_id::weapon_text ) {
            cfg_projected_esp_font_state( popup.element );
            cfg_projected_esp_case_state( popup.element );
        }

        if ( popup.element == esp::projected_esp_element_id::faction ) {
            faction_color_options( );
            return;
        }

        if ( popup.element == esp::projected_esp_element_id::relation ) {
            relation_tags_options( );
            return;
        }

        cfg_projected_esp_color_state( popup.element, projected_esp_popup_fallback_color( popup.element, settings ) );
    }

    void render_esp_preview_settings_popup( esp_preview_settings_popup& popup, const esp::projected_esp_settings& settings ) {
        popup.anim = ImLerp( popup.anim, popup.open ? 1.f : 0.f, std::clamp( ImGui::GetIO( ).DeltaTime * 14.f, 0.f, 1.f ) );
        if ( popup.anim <= 0.01f ) {
            popup.just_opened = false;
            return;
        }

        const char* title = esp_preview_settings_title( popup, settings );
        const ImVec2 popup_padding = SCALE( 14, 14 );
        const float min_content_width = SCALE( 230 );
        const float min_popup_width = min_content_width + popup_padding.x * 2.f;
        const float display_width = ImGui::GetIO( ).DisplaySize.x;
        const float display_height = ImGui::GetIO( ).DisplaySize.y;
        const float popup_margin = SCALE( 6 );

        ImGui::PushStyleVar( ImGuiStyleVar_Alpha, popup.anim * GImGui->Style.Alpha );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, GImGui->Style.FrameRounding );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, SCALE( 14, 14 ) );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, popup_padding );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1 );
        ImGui::PushStyleColor( ImGuiCol_WindowBg, ImGui::GetColorU32( ImGuiCol_FrameBg ) );
        ImGui::PushFont( fonts[font].get( 13 ) );

        const std::string window_name = std::string( "esp preview settings##" ) + title;
        ImGui::SetNextWindowSizeConstraints( ImVec2( min_popup_width, 0.f ), ImVec2( ( std::min )( SCALE( 520 ), display_width - popup_margin * 2.f ), FLT_MAX ) );
        ImGui::Begin( window_name.c_str( ), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings );
        {
            const ImGuiWindow* popup_window = ImGui::GetCurrentWindow( );
            const float popup_width = ImGui::GetWindowWidth( );
            const float popup_height = ImGui::GetWindowHeight( );
            const float popup_x = std::clamp( popup.pos.x + SCALE( 10 ), popup_margin, ( std::max )( popup_margin, display_width - popup_width - popup_margin ) );
            const float popup_y = std::clamp( popup.pos.y + SCALE( 10 ), popup_margin, ( std::max )( popup_margin, display_height - popup_height - popup_margin ) );
            ImGui::SetWindowPos( ImVec2( popup_x, popup_y ) );

            const bool clicked_outside =
                ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ||
                ImGui::IsMouseClicked( ImGuiMouseButton_Right );
            if ( !popup.just_opened && clicked_outside && !is_window_part_of_popup_hierarchy( GImGui->HoveredWindow, popup_window ) ) {
                popup.open = false;
            }
            if ( ImGui::IsKeyPressed( ImGuiKey_Escape ) ) {
                popup.open = false;
            }

            ImGui::BringWindowToFocusFront( ImGui::GetCurrentWindow( ) );
            ImGui::BringWindowToDisplayFront( ImGui::GetCurrentWindow( ) );
            ImGui::GetWindowDrawList( )->AddText( ImGui::GetCurrentWindow( )->DC.CursorPos, ImGui::GetColorU32( ImGuiCol_Text ), title, ImGui::FindRenderedTextEnd( title ) );
            ImGui::GetWindowDrawList( )->AddRectFilled(
                ImGui::GetWindowPos( ) + ImVec2{ 0, GImGui->Style.WindowPadding.y * 2 + GImGui->FontSize },
                ImGui::GetWindowPos( ) + ImVec2{ ImGui::GetWindowWidth( ), GImGui->Style.WindowPadding.y * 2 + GImGui->FontSize + 1 },
                ImGui::GetColorU32( ImGuiCol_Border )
            );
            ImGui::Dummy( ImGui::CalcTextSize( title, 0, 1 ) );

            ImGui::PushItemFlag( ImGuiItemFlags_Search, true );
            ImGui::PushItemWidth( min_content_width );
            ImGui::SetCursorPosY( GImGui->FontSize + GImGui->Style.WindowPadding.y * 3 );
            render_esp_preview_settings_content( popup, settings );
            ImGui::PopItemWidth( );
            ImGui::PopItemFlag( );
        }
        ImGui::End( );

        ImGui::PopFont( );
        ImGui::PopStyleColor( );
        ImGui::PopStyleVar( 5 );
        popup.just_opened = false;
    }

    void ignore_fraction_options( ) {
        cfg_checkbox_state( "ignore other fractions", "aimbot", "ignore_other_fractions", false );
        cfg_multi_fraction_state( "ignore fractions", "aimbot", "ignore_fractions" );
    }

    void ignore_family_options( ) {
        cfg_checkbox_state( "ignore other families", "aimbot", "ignore_other_families", false );

        std::map<std::string, std::string> ignored_families =
            config::get( "aimbot", "ignore_families", std::map<std::string, std::string>{ } );

        ImGui::InputTextWithHint( "family id##ignore_family_add", "id", g_ignore_family_id_buffer, sizeof( g_ignore_family_id_buffer ) );

        if ( ui::button( "add##ignore_family", ImVec2( -1, 0 ) ) ) {
            int family_id = 0;
            if ( parse_positive_id( g_ignore_family_id_buffer, family_id ) ) {
                ignored_families[std::to_string( family_id )] = "1";
                config::update( ignored_families, "aimbot", "ignore_families", std::map<std::string, std::string>{ } );
                g_ignore_family_id_buffer[0] = '\0';
            } else {
                hello_notify::push( "invalid family id", hello_notify::status::error );
            }
        }

        bool has_ignored_families = false;
        for ( const auto& [family_id, enabled] : ignored_families ) {
            if ( enabled == "1" ) {
                has_ignored_families = true;
                break;
            }
        }

        if ( !has_ignored_families ) {
            return;
        }

        ImGui::Separator( );
        std::string delete_family_id;
        for ( const auto& [family_id, enabled] : ignored_families ) {
            if ( enabled != "1" ) {
                continue;
            }

            ImGui::PushID( family_id.c_str( ) );
            ImGui::Text( "family id: %s", family_id.c_str( ) );
            ImGui::SameLine( );
            if ( ui::button( "delete", ImVec2( SCALE( 58 ), 0 ) ) ) {
                delete_family_id = family_id;
            }
            ImGui::PopID( );
        }

        if ( !delete_family_id.empty( ) ) {
            ignored_families.erase( delete_family_id );
            config::update( ignored_families, "aimbot", "ignore_families", std::map<std::string, std::string>{ } );
        }
    }

    void weapon_arrow_options( ) {
        cfg_slider_float_state( "arrow size##weapon_arrows", "visual", "weapon_arrow_size", 14.f, 8.f, 40.f, "%.0f" );
        cfg_slider_float_state( "icon size##weapon_arrows", "visual", "weapon_arrow_icon_size", 42.f, 10.f, 42.f, "%.0f" );
        cfg_slider_float_state( "distance##weapon_arrows", "visual", "weapon_arrow_distance", 42.f, 1.f, 300.f, "%.0f%%" );

        struct weapon_arrow_option {
            DWORD hash;
            std::string label;
        };

        std::vector<weapon_arrow_option> weapon_options;
        const std::map<DWORD, std::string> custom_weapons = weapons_highlight::custom_entries( );
        std::map<DWORD, std::string> selected_weapons = config::get( "visual", "weapon_arrow_weapons", weapon_arrows::default_selected( custom_weapons ) );
        weapon_options.reserve( std::size( weapon_arrows::k_guns ) + custom_weapons.size( ) );
        for ( const auto& weapon : weapon_arrows::k_guns ) {
            weapon_options.push_back( { weapon.hash, weapon.name } );
        }
        for ( const auto& [hash, name] : custom_weapons ) {
            if ( name.empty( ) || weapon_arrows::is_builtin_allowed( hash ) ) {
                continue;
            }
            weapon_options.push_back( { hash, name } );
        }

        if ( ui::button( "select all##weapon_arrows", ImVec2( SCALE( 108 ), 0 ) ) ) {
            selected_weapons.clear( );
            for ( const auto& weapon : weapon_options ) {
                selected_weapons[weapon.hash] = "1";
            }
            config::update( selected_weapons, "visual", "weapon_arrow_weapons", weapon_arrows::default_selected( custom_weapons ) );
        }
        ImGui::SameLine( );
        if ( ui::button( "clear##weapon_arrows", ImVec2( -1, 0 ) ) ) {
            selected_weapons.clear( );
            for ( const auto& weapon : weapon_options ) {
                selected_weapons[weapon.hash] = "0";
            }
            config::update( selected_weapons, "visual", "weapon_arrow_weapons", weapon_arrows::default_selected( custom_weapons ) );
        }

        std::vector<multi_select_item> weapon_items;
        std::vector<bool> before_state;
        weapon_items.reserve( weapon_options.size( ) );
        before_state.reserve( weapon_options.size( ) );

        for ( const auto& weapon : weapon_options ) {
            multi_select_item item { weapon.label.c_str( ) };
            item.selected = weapon_arrows::selected( selected_weapons, weapon.hash );
            before_state.push_back( item.selected );
            weapon_items.push_back( item );
        }

        ui::multi_select( "weapons##weapon_arrows", weapon_items );

        bool changed = false;
        for ( size_t i = 0; i < weapon_items.size( ); ++i ) {
            if ( weapon_items[i].selected != before_state[i] ) {
                changed = true;
                break;
            }
        }

        if ( changed ) {
            selected_weapons.clear( );
            for ( size_t i = 0; i < weapon_items.size( ); ++i ) {
                selected_weapons[weapon_options[i].hash] = weapon_items[i].selected ? "1" : "0";
            }
            config::update( selected_weapons, "visual", "weapon_arrow_weapons", weapon_arrows::default_selected( custom_weapons ) );
        }
    }


    void world_weapon_options( ) {
        std::vector<multi_select_item> element_items {
            { "text" },
            { "icon" },
            { "distance" }
        };
        element_items[0].selected = config::get( "visual", "world_esp_text", 1 ) != 0;
        element_items[1].selected = config::get( "visual", "world_esp_icon", 1 ) != 0;
        element_items[2].selected = config::get( "visual", "world_esp_show_dist", 1 ) != 0;
        ui::multi_select( "elements##world_esp", element_items );
        config::update( element_items[0].selected ? 1 : 0, "visual", "world_esp_text", 1 );
        config::update( element_items[1].selected ? 1 : 0, "visual", "world_esp_icon", 1 );
        config::update( element_items[2].selected ? 1 : 0, "visual", "world_esp_show_dist", 1 );

        cfg_slider_float_state( "distance##world_esp", "visual", "world_esp_dist", 300.f, 1.f, 300.f, "%.0f m" );
        cfg_slider_float_state( "icon size##world_esp", "visual", "world_esp_icon_size", 28.f, 10.f, 42.f, "%.0f" );
    }

    void cfg_weapons_highlight_state( const char* label, void( *options )( ) = nullptr ) {
        std::map<DWORD, std::string> selected_weapons =
            config::get( "visual", "weapons_highlighted", weapons_highlight::default_selected( ) );
        struct highlight_option {
            DWORD hash;
            std::string label;
        };

        const std::map<DWORD, std::string> custom_weapons = weapons_highlight::custom_entries( );
        std::vector<highlight_option> highlight_options;
        std::vector<multi_select_item> weapon_items;
        std::vector<std::string> weapon_labels;
        std::vector<bool> before_state;
        highlight_options.reserve( std::size( weapons_highlight::k_entries ) + custom_weapons.size( ) );

        for ( const auto& weapon : weapons_highlight::k_entries ) {
            highlight_options.push_back( { weapon.hash, lowercase_copy( weapons_highlight::label( weapon.hash ) ) } );
        }
        for ( const auto& [hash, name] : custom_weapons ) {
            if ( name.empty( ) || weapons_highlight::find( hash ) ) {
                continue;
            }
            highlight_options.push_back( { hash, lowercase_copy( weapons_highlight::label( hash ) ) } );
        }

        weapon_items.reserve( highlight_options.size( ) );
        weapon_labels.reserve( highlight_options.size( ) );
        before_state.reserve( highlight_options.size( ) );

        for ( const auto& weapon : highlight_options ) {
            auto it = selected_weapons.find( weapon.hash );
            weapon_labels.push_back( weapon.label );
            multi_select_item item { weapon_labels.back( ).c_str( ) };
            item.selected = it != selected_weapons.end( ) && it->second == "1";
            before_state.push_back( item.selected );
            weapon_items.push_back( item );
        }

        ui::multi_select( label, weapon_items, options );

        bool changed = false;
        for ( size_t i = 0; i < weapon_items.size( ); ++i ) {
            if ( weapon_items[i].selected != before_state[i] ) {
                changed = true;
                break;
            }
        }

        if ( !changed ) {
            return;
        }

        selected_weapons.clear( );
        for ( size_t i = 0; i < weapon_items.size( ); ++i ) {
            if ( weapon_items[i].selected ) {
                selected_weapons[highlight_options[i].hash] = "1";
            }
        }

        if ( selected_weapons.empty( ) ) {
            selected_weapons[weapons_highlight::k_none_hash] = "1";
        }

        config::update( selected_weapons, "visual", "weapons_highlighted", weapons_highlight::default_selected( ) );
    }

    void highlight_weapon_settings_popup( ) {
        const DWORD hash = g_highlight_weapon_settings_hash;
        if ( hash == 0 ) {
            return;
        }

        const std::string hash_label = format_object_hash( hash );
        const std::string weapon_label = weapons_highlight::label( hash );
        if ( !weapon_label.empty( ) ) {
            ImGui::Text( "%s", weapon_label.c_str( ) );
            ImGui::TextDisabled( "%s", hash_label.c_str( ) );
        } else {
            ImGui::Text( "%s", hash_label.c_str( ) );
        }

        std::map<DWORD, std::string> custom_colors = weapons_highlight::custom_colors( );
        std::map<DWORD, std::string> custom_names = weapons_highlight::custom_names( );
        std::map<DWORD, std::string> highlight_lines = weapons_highlight::line_settings( );
        char name_buffer[64] {};
        strncpy_s( name_buffer, weapon_label.c_str( ), _TRUNCATE );
        if ( ImGui::InputTextWithHint( "name##highlight_weapon_settings", weapons_highlight::base_label( hash ).c_str( ), name_buffer, sizeof( name_buffer ) ) ) {
            const std::string name = trim_player_text( name_buffer );
            if ( name.empty( ) ) {
                custom_names.erase( hash );
            } else {
                custom_names[hash] = name;
            }
            config::update( custom_names, "visual", "weapons_highlight_custom_names", std::map<DWORD, std::string>{ } );
        }

        const visual_config::rgba fallback_color {
            config::get( "hud", "accent_r", 180.f / 255.f ),
            config::get( "hud", "accent_g", 167.f / 255.f ),
            config::get( "hud", "accent_b", 245.f / 255.f ),
            1.f
        };
        const ImU32 default_color = weapons_highlight::default_color( hash, visual_config::to_u32( fallback_color ), weapons_highlight::accent_color( ) );
        visual_config::rgba row_color = visual_config::map_color( custom_colors, hash, visual_config::from_u32( default_color ) );
        float color[4];
        visual_config::to_float4( row_color, color );

        if ( ui::color_edit( "color##highlight_weapon_settings", color ) ) {
            custom_colors[hash] = visual_config::format_rgba( visual_config::from_float4( color ) );
            config::update( custom_colors, "visual", "weapons_highlight_custom_colors", std::map<DWORD, std::string>{ } );
        }

        bool line_enabled = weapons_highlight::line_enabled( hash );
        if ( ui::checkbox( "line##highlight_weapon_settings", &line_enabled ) ) {
            highlight_lines[hash] = line_enabled ? "1" : "0";
            config::update( highlight_lines, "visual", "weapons_highlight_lines", std::map<DWORD, std::string>{} );
        }
    }

    void weapons_highlighted_options( ) {
        cfg_checkbox_state( "play sound##weapons", "visual", "weapons_sound", false );

        ImGui::Separator( );
        ImGui::InputTextWithHint( "hash##highlight_weapon_add", "0x...", g_highlight_weapon_hash_buffer, sizeof( g_highlight_weapon_hash_buffer ) );
        ImGui::InputTextWithHint( "name##highlight_weapon_add", "name", g_highlight_weapon_name_buffer, sizeof( g_highlight_weapon_name_buffer ) );
        ui::color_edit( "color##highlight_weapon_add", g_highlight_weapon_color );

        std::map<DWORD, std::string> custom_weapons = weapons_highlight::custom_entries( );
        std::map<DWORD, std::string> custom_colors = weapons_highlight::custom_colors( );
        std::map<DWORD, std::string> selected_weapons =
            config::get( "visual", "weapons_highlighted", weapons_highlight::default_selected( ) );

        if ( ui::button( "add##highlight_weapon", ImVec2( -1, 0 ) ) ) {
            DWORD hash = 0;
            const std::string name = trim_player_text( g_highlight_weapon_name_buffer );
            if ( parse_object_hash( g_highlight_weapon_hash_buffer, hash ) && !name.empty( ) && !weapons_highlight::find( hash ) ) {
                custom_weapons[hash] = name;
                custom_colors[hash] = visual_config::format_rgba( visual_config::from_float4( g_highlight_weapon_color ) );
                selected_weapons[hash] = "1";
                config::update( custom_weapons, "visual", "weapons_highlight_custom", std::map<DWORD, std::string>{ } );
                config::update( custom_colors, "visual", "weapons_highlight_custom_colors", std::map<DWORD, std::string>{ } );
                config::update( selected_weapons, "visual", "weapons_highlighted", weapons_highlight::default_selected( ) );
                g_highlight_weapon_hash_buffer[0] = '\0';
                g_highlight_weapon_name_buffer[0] = '\0';
            } else {
                hello_notify::push( "invalid or existing weapon hash", hello_notify::status::error );
            }
        }

        ImGui::Separator( );
        DWORD delete_hash = 0;

        struct highlight_weapon_option {
            DWORD hash;
            std::string name;
            bool removable;
        };

        std::vector<highlight_weapon_option> weapon_options;
        weapon_options.reserve( std::size( weapons_highlight::k_entries ) + custom_weapons.size( ) );

        for ( const auto& weapon : weapons_highlight::k_entries ) {
            weapon_options.push_back( { weapon.hash, weapons_highlight::label( weapon.hash ), false } );
        }

        for ( const auto& [hash, name] : custom_weapons ) {
            if ( name.empty( ) || weapons_highlight::find( hash ) ) {
                continue;
            }
            weapon_options.push_back( { hash, weapons_highlight::label( hash ), true } );
        }

        bool has_delete_column = false;
        for ( const highlight_weapon_option& option : weapon_options ) {
            if ( option.removable ) {
                has_delete_column = true;
                break;
            }
        }

        for ( const highlight_weapon_option& option : weapon_options ) {
            char hash_buf[16];
            sprintf_s( hash_buf, "0x%X", option.hash );

            ImGui::PushID( hash_buf );
            const float row_right = ImGui::GetCursorPosX( ) + ImGui::CalcItemWidth( );
            const float delete_width = SCALE( 58 );
            const float settings_size = SCALE( 14 );
            const float control_gap = SCALE( 8 );
            const float delete_x = row_right - delete_width;
            const float settings_x = has_delete_column ? delete_x - control_gap - settings_size : row_right - settings_size;

            ImGui::Text( "%s", hash_buf );
            ImGui::SameLine( 0, SCALE( 8 ) );
            text_unformatted_clipped( option.name.c_str( ), settings_x - control_gap );
            ImGui::SameLine( );
            ImGui::SetCursorPosX( settings_x );
            g_highlight_weapon_settings_hash = option.hash;
            char settings_id[64];
            sprintf_s( settings_id, "weapon##highlight_weapon_settings_%X", option.hash );
            ui::settings_btn( settings_id, highlight_weapon_settings_popup );
            if ( option.removable ) {
                ImGui::SameLine( );
                ImGui::SetCursorPosX( delete_x );
                if ( ui::button( "delete", ImVec2( SCALE( 58 ), 0 ) ) ) {
                    delete_hash = option.hash;
                }
            }
            ImGui::PopID( );
        }

        if ( delete_hash != 0 ) {
            custom_weapons.erase( delete_hash );
            custom_colors.erase( delete_hash );
            selected_weapons.erase( delete_hash );
            std::map<DWORD, std::string> custom_names = weapons_highlight::custom_names( );
            custom_names.erase( delete_hash );
            if ( g_highlight_weapon_settings_hash == delete_hash ) {
                g_highlight_weapon_settings_hash = 0;
            }
            if ( selected_weapons.empty( ) ) {
                selected_weapons[weapons_highlight::k_none_hash] = "1";
            }
            config::update( custom_weapons, "visual", "weapons_highlight_custom", std::map<DWORD, std::string>{ } );
            config::update( custom_colors, "visual", "weapons_highlight_custom_colors", std::map<DWORD, std::string>{ } );
            config::update( custom_names, "visual", "weapons_highlight_custom_names", std::map<DWORD, std::string>{ } );
            std::map<DWORD, std::string> highlight_lines = weapons_highlight::line_settings( );
            highlight_lines.erase( delete_hash );
            config::update( highlight_lines, "visual", "weapons_highlight_lines", std::map<DWORD, std::string>{} );
            config::update( selected_weapons, "visual", "weapons_highlighted", weapons_highlight::default_selected( ) );
        }
    }

    void vector_aim_options( ) {
        bool fov_changed = false;
        const bool show_fov = cfg_checkbox_state( "show fov", "aimbot", "vector_show_radius", false, &fov_changed, "draw the vector aim radius on screen" );
        if ( fov_changed && show_fov ) {
            config::update( 0, "aimbot", "silent_show_radius", 0 );
        }

        cfg_color_state( "fov color", "aimbot", "vector_fov_r", "vector_fov_g", "vector_fov_b", "vector_fov_a", 1.f, 1.f, 1.f, 0.5f );
    }



    void copy_config_list_to_menu( ) {
        g_local_config_names.clear( );
        g_local_config_paths.clear( );
        const config::snapshot config_state = config::current_snapshot( );

        for ( const auto& entry : config_state.configs ) {
            g_local_config_names.push_back( entry.first );
            g_local_config_paths.push_back( entry.second );
        }

        clamp_index( g_local_config_selected, g_local_config_names.size( ) );

        if ( !config_state.active_config_name.empty( ) ) {
            for ( size_t i = 0; i < g_local_config_names.size( ); ++i ) {
                if ( g_local_config_names[i] == config_state.active_config_name ) {
                    g_local_config_selected = static_cast<int>( i );
                    break;
                }
            }
        }

        if ( !g_local_config_names.empty( ) ) {
            copy_string( g_config_name_buffer, sizeof( g_config_name_buffer ), g_local_config_names[g_local_config_selected] );
        }
    }

    bool refresh_local_configs( ) {
        if constexpr ( build_profile::production ) {
            if ( g_config_list_loading.load( std::memory_order_acquire ) ) return false;
            g_config_list_loading.store( true, std::memory_order_release );
            g_config_list_done.store( false, std::memory_order_release );
            g_config_list_ok.store( false, std::memory_order_release );
            config::set_status_message( "loading configs" );
            std::thread( []( ) {
                const bool ok = config::list_configs( );
                g_config_list_ok.store( ok, std::memory_order_release );
                g_config_list_done.store( true, std::memory_order_release );
                g_config_list_loading.store( false, std::memory_order_release );
            } ).detach( );
            return true;
        }

        if ( !config::list_configs( ) ) {
            g_configs_loaded = false;
            g_next_config_list_attempt_ms = GetTickCount( ) + 1500;
            return false;
        }
        copy_config_list_to_menu( );
        g_configs_loaded = true;
        return true;
    }

    void apply_config_list_refresh( ) {
        if constexpr ( build_profile::production ) {
            if ( !g_config_list_done.exchange( false, std::memory_order_acq_rel ) ) return;
            if ( !g_config_list_ok.load( std::memory_order_acquire ) ) {
                g_configs_loaded = false;
                g_next_config_list_attempt_ms = GetTickCount( ) + 1500;
                return;
            }
            copy_config_list_to_menu( );
            g_configs_loaded = true;
        }
    }

    void render_local_configs_panel( ) {
        apply_config_list_refresh( );
        if ( !g_configs_loaded && GetTickCount( ) >= g_next_config_list_attempt_ms ) {
            refresh_local_configs( );
        }
        float avail = ImGui::GetContentRegionAvail( ).x;
        if ( ImGui::BeginListBox( "##local_configs", ImVec2( avail, SCALE( 138 ) ) ) ) {
            for ( int i = 0; i < static_cast<int>( g_local_config_names.size( ) ); ++i ) {
                const bool selected = g_local_config_selected == i;
                if ( ImGui::Selectable( g_local_config_names[i].c_str( ), selected ) ) {
                    g_local_config_selected = i;
                    copy_string( g_config_name_buffer, sizeof( g_config_name_buffer ), g_local_config_names[i] );
                }
            }
            ImGui::EndListBox( );
        }

        ImGui::InputText( "config name", g_config_name_buffer, sizeof( g_config_name_buffer ) );

        const float spacing = ImGui::GetStyle( ).ItemSpacing.x;
        const float button_width = ( avail - spacing ) * 0.5f;

        if ( ui::button( "load##local_cfg", ImVec2( button_width, 0 ) ) && !g_local_config_names.empty( ) ) {
            config::load_from_file( g_local_config_names[g_local_config_selected] );
        }
        ImGui::SameLine( 0, spacing );
        if ( ui::button( "save##local_cfg", ImVec2( -1, 0 ) ) ) {
            const std::string config_name = sanitize_config_name( g_config_name_buffer );
            config::save_to_file( config_name );
            if ( std::find( g_local_config_names.begin( ), g_local_config_names.end( ), config_name ) == g_local_config_names.end( ) ) {
                g_local_config_names.push_back( config_name );
                g_local_config_paths.push_back( config_name );
            }
            g_local_config_selected = static_cast<int>( std::find( g_local_config_names.begin( ), g_local_config_names.end( ), config_name ) - g_local_config_names.begin( ) );
            copy_string( g_config_name_buffer, sizeof( g_config_name_buffer ), config_name );
        }

        if ( ui::button( "delete##local_cfg", ImVec2( button_width, 0 ) ) && !g_local_config_names.empty( ) ) {
            const std::string config_name = g_local_config_names[g_local_config_selected];
            if constexpr ( build_profile::production ) {
                config::delete_config( config_name );
                const auto it = std::find( g_local_config_names.begin( ), g_local_config_names.end( ), config_name );
                if ( it != g_local_config_names.end( ) ) {
                    const size_t index = static_cast<size_t>( it - g_local_config_names.begin( ) );
                    g_local_config_names.erase( g_local_config_names.begin( ) + index );
                    if ( index < g_local_config_paths.size( ) ) {
                        g_local_config_paths.erase( g_local_config_paths.begin( ) + index );
                    }
                    clamp_index( g_local_config_selected, g_local_config_names.size( ) );
                    if ( !g_local_config_names.empty( ) ) {
                        copy_string( g_config_name_buffer, sizeof( g_config_name_buffer ), g_local_config_names[g_local_config_selected] );
                    }
                }
            } else {
                config::delete_config( config_name );
                refresh_local_configs( );
            }
        }
        ImGui::SameLine( 0, spacing );
        if ( ui::button( "refresh##local_cfg", ImVec2( -1, 0 ) ) ) {
            refresh_local_configs( );
        }

        const config::snapshot config_state = config::current_snapshot( );
        if ( !config_state.active_config_name.empty( ) ) {
            ImGui::TextDisabled( "active: %s", config_state.active_config_name.c_str( ) );
        }

        if ( !config_state.status_message.empty( ) ) {
            ImGui::TextDisabled( "%s", config_state.status_message.c_str( ) );
        }
    }

    void render_aimbot_page( ) {
        ImGui::BeginGroup( );
        {
            ui::child( "aimbot", []( ) {
                bool changed = false;
                const bool enabled = cfg_checkbox_bind_state(
                    "enabled##vector_aim",
                    "aimbot",
                    "vector_enable",
                    false,
                    k_vector_aim_bind,
                    nullptr,
                    &changed,
                    nullptr
                );

                cfg_multi_bone_state( "bone##vector_aim", "aimbot", "vector_aim_bones", "vector_aim_bone" );
                cfg_slider_float_state( "fov##vector_aim", "aimbot", "vector_radius", 50.f, 0.f, 500.f, "%.0f" );
                static const std::vector<const char*> k_aim_priority_items { "screen distance", "3d distance" };
                cfg_combo_state( "priority##vector_aim", "aimbot", "vector_priority", 0, k_aim_priority_items );
                cfg_slider_float_state( "switch delay##vector_aim", "aimbot", "target_switch_delay", 0.f, 0.f, 2000.f, "%.0f ms" );
                cfg_checkbox_color_state( "draw fov##vector_aim", "aimbot", "vector_show_radius", false, "vector_fov_r", "vector_fov_g", "vector_fov_b", "vector_fov_a", 1.f, 1.f, 1.f, 0.5f );
                cfg_slider_float_state( "smoothness##vector_aim", "aimbot", "vector_smoothness", 50.f, 0.f, 100.f, "%.0f%%" );
            } );

        }
        ImGui::EndGroup( );

        ImGui::SameLine( );

        ImGui::BeginGroup( );
        {


            ui::child( "settings", []( ) {
                cfg_checkbox_state( "force shot", "aimbot", "force_shot", false, nullptr, "fires as soon as a valid target is ready" );

                cfg_checkbox_state( "ignore friends", "aimbot", "ignore_friends", true );
                cfg_checkbox_state( "visible only", "aimbot", "visible_only", false, nullptr, "only aim at players with a clear line of sight" );
                cfg_checkbox_state( "ignore my family", "aimbot", "ignore_family", false, nullptr, nullptr, ignore_family_options );
                cfg_checkbox_state( "ignore my fraction", "aimbot", "ignore_fraction", false, nullptr, nullptr, ignore_fraction_options );
                cfg_slider_float_state( "target timeout", "aimbot", "target_timeout", 0.f, 0.f, 2000.f, "%.0f" );
            } );
        }
        ImGui::EndGroup( );
    }

    void render_visuals_players_page( ) {
        const ImVec2 esp_child_start = ImGui::GetCursorScreenPos( );

        ImGui::BeginGroup( );
        {
            ui::child( "esp", []( ) {
                cfg_checkbox_state( "enabled##esp", "visual", "enable", false );
                cfg_checkbox_state( "box##esp", "visual", "draw_box", false );
                cfg_checkbox_state( "name##esp", "visual", "altv_nickname", false );
                cfg_checkbox_state( "relation##esp", "visual", "altv_relation", true, nullptr, nullptr, relation_tags_options );
                cfg_checkbox_state( "fraction##esp", "visual", "altv_faction", false );
                cfg_checkbox_state( "skeleton##esp", "visual", "draw_skeleton", false );
                cfg_checkbox_state( "preview invisible##esp", "visual", "skel_preview_invisible", false );
                cfg_health_mode_state( "health##esp" );
                std::vector<multi_select_item> weapon_items {
                    { "icon" },
                    { "text" }
                };
                const bool legacy_weapon_enabled = config::get( "visual", "draw_weapons", 0 ) != 0;
                const bool weapon_text_enabled = config::get( "visual", "draw_weapon_text", 0 ) != 0;
                const bool weapon_icon_enabled = config::get( "visual", "draw_weapon_icon", 0 ) != 0;
                weapon_items[0].selected = weapon_icon_enabled;
                weapon_items[1].selected = weapon_text_enabled || ( legacy_weapon_enabled && !weapon_icon_enabled );
                ui::multi_select( "weapon", weapon_items );
                config::update( weapon_items[1].selected ? 1 : 0, "visual", "draw_weapon_text", 0 );
                config::update( weapon_items[0].selected ? 1 : 0, "visual", "draw_weapon_icon", 0 );
                config::update( weapon_items[0].selected || weapon_items[1].selected ? 1 : 0, "visual", "draw_weapons", 0 );

                std::vector<multi_select_item> flag_items {
                    { "distance" },
                    { "static" },
                    { "admin" },
                    { "tester" },
                    { "level" },
                    { "dead" },
                    { "afk" },
                    { "media" },
                    { "cuffed" },
                    { "wanted" },
                    { "armor" },
                    { "social" },
                    { "visible" }
                };
                flag_items[0].selected = config::get( "visual", "draw_distance", 0 ) != 0;
                flag_items[1].selected = config::get( "visual", "altv_static", 0 ) != 0;
                flag_items[2].selected = config::get( "visual", "altv_admin", 0 ) != 0;
                flag_items[3].selected = config::get( "visual", "altv_tester", 0 ) != 0;
                flag_items[4].selected = config::get( "visual", "altv_level", 0 ) != 0;
                flag_items[5].selected = config::get( "visual", "altv_dead", 0 ) != 0;
                flag_items[6].selected = config::get( "visual", "altv_afk", 0 ) != 0;
                flag_items[7].selected = config::get( "visual", "altv_media", 0 ) != 0;
                flag_items[8].selected = config::get( "visual", "altv_cuffed", 0 ) != 0;
                flag_items[9].selected = config::get( "visual", "altv_wanted", 0 ) != 0;
                flag_items[10].selected = config::get( "visual", "altv_armor_flag", 0 ) != 0;
                flag_items[11].selected = config::get( "visual", "altv_social", 0 ) != 0;
                flag_items[12].selected = config::get( "visual", "altv_visible", 0 ) != 0;

                ui::multi_select( "flags", flag_items );
                config::update( flag_items[0].selected ? 1 : 0, "visual", "draw_distance", 0 );
                config::update( flag_items[1].selected ? 1 : 0, "visual", "altv_static", 0 );
                config::update( flag_items[2].selected ? 1 : 0, "visual", "altv_admin", 0 );
                config::update( flag_items[3].selected ? 1 : 0, "visual", "altv_tester", 0 );
                config::update( flag_items[4].selected ? 1 : 0, "visual", "altv_level", 0 );
                config::update( flag_items[5].selected ? 1 : 0, "visual", "altv_dead", 0 );
                config::update( flag_items[6].selected ? 1 : 0, "visual", "altv_afk", 0 );
                config::update( flag_items[7].selected ? 1 : 0, "visual", "altv_media", 0 );
                config::update( flag_items[8].selected ? 1 : 0, "visual", "altv_cuffed", 0 );
                config::update( flag_items[9].selected ? 1 : 0, "visual", "altv_wanted", 0 );
                config::update( flag_items[10].selected ? 1 : 0, "visual", "altv_armor_flag", 0 );
                config::update( flag_items[11].selected ? 1 : 0, "visual", "altv_social", 0 );
                config::update( flag_items[12].selected ? 1 : 0, "visual", "altv_visible", 0 );

                cfg_slider_float_state( "render distance##esp", "hack", "max_range", 300.f, 1.f, 300.f, "%.0f m" );
            } );

            ui::child( "other", []( ) {
                cfg_checkbox_state( "admins around", "visual", "admins_around_indicator", true );
                cfg_checkbox_state( "dim dead players", "visual", "dim_dead_players", true );
                cfg_checkbox_state( "cheap render text", "visual", "cheap_render_text", false );
            } );
        }
        ImGui::EndGroup( );

        if ( !g_collect_search_index ) {
            ImGui::SameLine( );
            ImGui::SetCursorScreenPos( { ImGui::GetCursorScreenPos( ).x, esp_child_start.y } );

            ImGui::BeginGroup( );
            render_esp_preview_inline( );
            ImGui::EndGroup( );
        }
    }

    void render_visuals_world_page( ) {
        ImGui::BeginGroup( );
        {
            ui::child( "weapon detection", []( ) {
                cfg_checkbox_state( "enabled##weapons", "visual", "weapons_enabled", false );
                cfg_checkbox_state( "widget##weapons", "visual", "weapons_widget", false );
                cfg_checkbox_state( "offscreen arrows##weapon_arrows", "visual", "weapon_arrows", false, nullptr, nullptr, weapon_arrow_options );
                cfg_checkbox_state( "highlight player##weapons", "visual", "weapons_highlight", false, nullptr, "highlights selected weapons on player esp" );
                cfg_weapons_highlight_state( "highlighted weapons##weapons", weapons_highlighted_options );
            } );
        }
        ImGui::EndGroup( );

        ImGui::SameLine( );

        ImGui::BeginGroup( );
        {
            ui::child( "world", []( ) {
                cfg_checkbox_color_state(
                    "weapons##world_esp",
                    "visual",
                    "world_esp",
                    false,
                    "world_esp_color_r",
                    "world_esp_color_g",
                    "world_esp_color_b",
                    "world_esp_color_a",
                    1.f,
                    1.f,
                    1.f,
                    1.f,
                    world_weapon_options
                );
            } );
        }
        ImGui::EndGroup( );
    }

    void render_misc_page( ) {

        ImGui::BeginGroup( );
        {
            ui::child( "camera", []( ) {
                cfg_checkbox_state( "custom fov", "hacks", "custom_fov", false );
                cfg_slider_float_state( "fov value", "hacks", "custom_fov_value", 60.f, 30.f, 120.f, "%.0f" );
                cfg_checkbox_state( "aspect ratio", "hacks", "custom_aspect", false );
                cfg_slider_float_state( "aspect value", "hacks", "custom_aspect_value", 1.7777778f, 0.5f, 3.f, "%.2f" );
            } );
        }
        ImGui::EndGroup( );
    }
    void render_settings_page( ) {
        ImGui::BeginGroup( );
        {
            ui::child( "general", []( ) {
                cfg_checkbox_state( "watermark", "hack", "watermark", true );
                cfg_checkbox_state( "keybinds", "hud", "keybinds", true );
                cfg_checkbox_state( "indicators", "hud", "indicators", true );
                cfg_color_rgb_state( "accent color", "hud", "accent_r", "accent_g", "accent_b", 180.f / 255.f, 167.f / 255.f, 245.f / 255.f );
                cfg_simple_bind_state( "menu key", "menu", "menu_key", VK_END );
                cfg_checkbox_bind_state( "panic key", "misc", "panic_enabled", false, k_panic_bind, nullptr, nullptr, "hold / toggle / always-on: disables all ESP while active, restores on release" );
                if ( ui::button( "unload", ImVec2( -1, 0 ) ) ) {
                    hacks::disable_panic_features();
                    runtime_session::request_unload();
                }
            } );

            ui::child( "misc", []( ) {
                cfg_checkbox_state( "require teleport approval", "misc", "require_teleport_approval", true, nullptr, "teleport requires RMB confirmation while the preview key is held" );
            } );

        }
        ImGui::EndGroup( );

        ImGui::SameLine( );

        ImGui::BeginGroup( );
        {
            ui::child( "configs", []( ) {
                render_local_configs_panel( );
            } );
        }
        ImGui::EndGroup( );
    }

    std::string menu_value_to_string( const json& value ) {
        if ( value.is_boolean( ) ) return value.get<bool>( ) ? "1" : "0";
        if ( value.is_number_integer( ) ) return std::to_string( value.get<int>( ) );
        if ( value.is_number_float( ) ) return std::to_string( value.get<float>( ) );
        if ( value.is_string( ) ) return value.get<std::string>( );
        return value.dump( );
    }

    json menu_value_from_string( const ws_server::MenuItem& item, const std::string& saved ) {
        if ( item.kind == "checkbox" ) return saved == "1" || saved == "true";
        if ( item.kind == "slider" ) return static_cast<float>( std::atof( saved.c_str( ) ) );
        if ( item.kind == "combo" ) {
            const int index = std::atoi( saved.c_str( ) );
            if ( index >= 0 && index < static_cast<int>( item.options.size( ) ) ) return index;
            return saved;
        }
        if ( item.kind == "color_picker" || item.kind == "hotkey" || item.kind == "multi_select" ) {
            try {
                return json::parse( saved );
            } catch ( ... ) {
                if ( item.kind == "hotkey" ) return hotkey_json( std::atoi( saved.c_str( ) ), 1 );
                if ( item.kind == "multi_select" ) return json::array( );
            }
        }
        return saved;
    }

    void hydrate_menu_item( const ws_server::MenuItem& item ) {
        const std::string key = item.script + "\x1f" + item.id;
        if ( g_hydrated_menu_items.find( key ) != g_hydrated_menu_items.end( ) ) return;
        g_hydrated_menu_items.insert( key );

        const std::string saved = config::get( "workshop." + item.script, item.id, std::string( ) );
        if ( saved.empty( ) ) return;
        const json value = menu_value_from_string( item, saved );
        ws_server::set_menu_item_value( item.script, item.id, value );
        ws_server::send_menu_event( item.script, item.id, value );
    }

    void hydrate_script_menu_items( ) {
        const std::vector<ws_server::MenuItem> items = ws_server::copy_menu_items( );
        for ( const ws_server::MenuItem& item : items ) {
            hydrate_menu_item( item );
        }
    }

    void persist_menu_item( const ws_server::MenuItem& item, const json& value ) {
        json persisted = value;
        if ( item.kind == "hotkey" && persisted.is_object( ) ) {
            persisted.erase( "active" );
        }
        config::set( "workshop." + item.script, item.id, menu_value_to_string( persisted ) );
        config::flush_to_disk( );
    }

    void update_menu_item_value( const ws_server::MenuItem& item, const json& value ) {
        ws_server::set_menu_item_value( item.script, item.id, value );
        persist_menu_item( item, value );
        ws_server::send_menu_event( item.script, item.id, value );
    }

    void clear_hydrated_menu_items( const std::string& script ) {
        const std::string prefix = script + "\x1f";
        for ( auto it = g_hydrated_menu_items.begin( ); it != g_hydrated_menu_items.end( ); ) {
            if ( it->rfind( prefix, 0 ) == 0 ) it = g_hydrated_menu_items.erase( it );
            else ++it;
        }
        for ( auto it = g_script_hotkeys.begin( ); it != g_script_hotkeys.end( ); ) {
            if ( it->first.rfind( prefix, 0 ) == 0 ) it = g_script_hotkeys.erase( it );
            else ++it;
        }
    }


    struct script_ui_group {
        std::string name;
        std::vector<ws_server::MenuItem> items;
    };

    float script_ui_item_height( const ws_server::MenuItem& item ) {
        if ( item.kind == "slider" ) return SCALE( 28 );
        if ( item.kind == "combo" ) return SCALE( 46 );
        if ( item.kind == "input" ) return SCALE( 46 );
        if ( item.kind == "button" ) return SCALE( 24 );
        if ( item.kind == "hotkey" ) return SCALE( 17 );
        if ( item.kind == "color_picker" ) return SCALE( 13 );
        return SCALE( 13 );
    }

    float script_ui_content_height( const script_ui_group& group ) {
        float height = 0.f;
        int visible_items = 0;
        for ( const ws_server::MenuItem& item : group.items ) {
            if ( !item.visible ) continue;
            if ( visible_items > 0 ) height += SCALE( 10 );
            height += script_ui_item_height( item );
            ++visible_items;
        }
        return visible_items > 0 ? height : SCALE( 13 );
    }

    ImVec2 script_ui_child_size( const script_ui_group& group ) {
        const float padding = SCALE( 28 );
        const float estimated_height = padding + script_ui_content_height( group );
        const float available_height = ImGui::GetContentRegionAvail( ).y;
        const float max_height = available_height > SCALE( 120 ) ? available_height : SCALE( 430 );
        return ImVec2{ 0, estimated_height < max_height ? estimated_height : max_height };
    }

    bool script_slider_is_integer( const ws_server::MenuItem& item ) {
        if ( item.value.is_number_float( ) ) return false;
        return std::floor( item.min ) == item.min && std::floor( item.max ) == item.max;
    }

    std::string script_item_label( const ws_server::MenuItem& item ) {
        return item.label + "##" + item.script + "_" + item.id;
    }

    std::string script_item_key( const ws_server::MenuItem& item ) {
        return item.script + "\x1f" + item.id;
    }

    int script_option_index( const ws_server::MenuItem& item, const json& value ) {
        if ( value.is_number_integer( ) ) {
            int index = value.get<int>( );
            clamp_index( index, item.options.size( ) );
            return index;
        }
        if ( value.is_string( ) ) {
            const std::string selected = value.get<std::string>( );
            const auto id_it = std::find( item.option_ids.begin( ), item.option_ids.end( ), selected );
            if ( id_it != item.option_ids.end( ) ) return static_cast<int>( std::distance( item.option_ids.begin( ), id_it ) );
            const auto label_it = std::find( item.options.begin( ), item.options.end( ), selected );
            if ( label_it != item.options.end( ) ) return static_cast<int>( std::distance( item.options.begin( ), label_it ) );
        }
        return 0;
    }

    std::string script_option_id( const ws_server::MenuItem& item, size_t index ) {
        if ( index < item.option_ids.size( ) ) return item.option_ids[index];
        if ( index < item.options.size( ) ) return item.options[index];
        return "";
    }

    bool json_array_contains( const json& value, const std::string& id ) {
        if ( !value.is_array( ) ) return false;
        return std::find_if( value.begin( ), value.end( ), [&]( const json& entry ) {
            return entry.is_string( ) && entry.get<std::string>( ) == id;
        } ) != value.end( );
    }

    json color_value_from_item( const json& value ) {
        return json{
            { "r", color_component( value, "r", 1.f ) },
            { "g", color_component( value, "g", 1.f ) },
            { "b", color_component( value, "b", 1.f ) },
            { "a", color_component( value, "a", 1.f ) }
        };
    }

    void item_color_to_float( const json& value, float color[4] ) {
        color[0] = color_component( value, "r", 1.f );
        color[1] = color_component( value, "g", 1.f );
        color[2] = color_component( value, "b", 1.f );
        color[3] = color_component( value, "a", 1.f );
    }

    json hotkey_value_from_item( const json& value ) {
        if ( !value.is_object( ) ) return hotkey_json( value.is_number_integer( ) ? value.get<int>( ) : 0, 1 );
        return hotkey_json(
            value.value( "key", 0 ),
            hotkey_mode_to_native( value.value( "mode", std::string( "hold" ) ) ),
            value.value( "active", false )
        );
    }

    bool script_hotkey_active( const ws_server::MenuItem& item, int key, int mode ) {
        script_hotkey_state& state = g_script_hotkeys[script_item_key( item )];
        const bool down = key > 0 && ( GetAsyncKeyState( key ) & 0x8000 ) != 0;
        bool active = false;
        if ( mode == 2 ) {
            active = true;
        } else if ( key > 0 && mode == 1 ) {
            active = down;
        } else if ( key > 0 ) {
            if ( down && !state.was_down ) {
                state.toggled = !state.toggled;
            }
            active = state.toggled;
        }
        state.was_down = down;
        return active;
    }

    void update_script_hotkey_state( const ws_server::MenuItem& item ) {
        if ( item.disabled || !item.visible ) return;

        const json hotkey = hotkey_value_from_item( item.value );
        const int key = hotkey.value( "key", 0 );
        const int mode = hotkey_mode_to_native( hotkey.value( "mode", std::string( "hold" ) ) );
        const bool active = script_hotkey_active( item, key, mode );
        script_hotkey_state& state = g_script_hotkeys[script_item_key( item )];
        if ( active != state.active ) {
            state.active = active;
            const json next = hotkey_json( key, mode, active );
            ws_server::set_menu_item_value( item.script, item.id, next );
            ws_server::send_menu_event( item.script, item.id, next );
        }
    }

    void poll_script_hotkeys( ) {
        const std::vector<ws_server::MenuItem> items = ws_server::copy_menu_items( );
        for ( const ws_server::MenuItem& item : items ) {
            if ( item.kind != "hotkey" ) continue;
            hydrate_menu_item( item );
            update_script_hotkey_state( item );
        }
    }

    void show_script_tooltip( const ws_server::MenuItem& item ) {
        if ( !item.tooltip.empty( ) && ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled ) ) {
            ImGui::SetTooltip( "%s", item.tooltip.c_str( ) );
        }
    }

    void render_script_ui_item( const ws_server::MenuItem& item ) {
        hydrate_menu_item( item );
        if ( !item.visible ) return;

        if ( item.disabled ) ImGui::BeginDisabled( );

        const std::string id_label = script_item_label( item );
        if ( item.kind == "checkbox" ) {
            bool value = item.value.is_boolean( ) ? item.value.get<bool>( ) : false;
            if ( ui::checkbox( id_label.c_str( ), &value, nullptr, { }, nullptr, item.tooltip.empty( ) ? nullptr : item.tooltip.c_str( ) ) ) {
                update_menu_item_value( item, value );
            }
        } else if ( item.kind == "slider" ) {
            if ( script_slider_is_integer( item ) ) {
                int value = item.value.is_number( ) ? item.value.get<int>( ) : static_cast<int>( item.min );
                if ( ui::slider_int( id_label.c_str( ), &value, static_cast<int>( item.min ), static_cast<int>( item.max ), "%d" ) ) {
                    update_menu_item_value( item, value );
                }
                show_script_tooltip( item );
            } else {
                float value = item.value.is_number( ) ? item.value.get<float>( ) : static_cast<float>( item.min );
                if ( ui::slider_float( id_label.c_str( ), &value, static_cast<float>( item.min ), static_cast<float>( item.max ), "%.1f" ) ) {
                    update_menu_item_value( item, value );
                }
                show_script_tooltip( item );
            }
        } else if ( item.kind == "combo" ) {
            int value = script_option_index( item, item.value );
            std::vector<const char*> options;
            options.reserve( item.options.size( ) );
            for ( const std::string& option : item.options ) {
                options.push_back( option.c_str( ) );
            }
            if ( !options.empty( ) && ui::combo( id_label.c_str( ), &value, options ) ) {
                update_menu_item_value( item, value );
            }
            show_script_tooltip( item );
        } else if ( item.kind == "input" ) {
            char buffer[256] { };
            const std::string value = item.value.is_string( ) ? item.value.get<std::string>( ) : "";
            std::strncpy( buffer, value.c_str( ), sizeof( buffer ) - 1 );
            ImGui::TextDisabled( "%s", item.label.c_str( ) );
            if ( ImGui::InputTextWithHint( ( "##input_" + item.script + "_" + item.id ).c_str( ), item.label.c_str( ), buffer, sizeof( buffer ) ) ) {
                update_menu_item_value( item, std::string( buffer ) );
            }
            show_script_tooltip( item );
        } else if ( item.kind == "color_picker" ) {
            float color[4];
            item_color_to_float( color_value_from_item( item.value ), color );
            if ( ui::color_edit( id_label.c_str( ), color ) ) {
                update_menu_item_value( item, color_json( color ) );
            }
            show_script_tooltip( item );
        } else if ( item.kind == "multi_select" ) {
            std::vector<multi_select_item> options;
            options.reserve( item.options.size( ) );
            for ( size_t i = 0; i < item.options.size( ); ++i ) {
                multi_select_item option { item.options[i].c_str( ) };
                option.selected = json_array_contains( item.value, script_option_id( item, i ) );
                options.push_back( option );
            }
            std::vector<bool> before;
            before.reserve( options.size( ) );
            for ( const multi_select_item& option : options ) before.push_back( option.selected );
            ui::multi_select( id_label.c_str( ), options );
            show_script_tooltip( item );
            bool changed = false;
            json selected = json::array( );
            for ( size_t i = 0; i < options.size( ); ++i ) {
                if ( options[i].selected != before[i] ) changed = true;
                if ( options[i].selected ) selected.push_back( script_option_id( item, i ) );
            }
            if ( changed ) update_menu_item_value( item, selected );
        } else if ( item.kind == "button" ) {
            if ( ui::button( id_label.c_str( ), ImVec2( -1, 0 ) ) ) {
                static uint64_t click_counter = 0;
                ws_server::send_menu_event( item.script, item.id, ++click_counter );
            }
            show_script_tooltip( item );
        } else if ( item.kind == "hotkey" ) {
            const json hotkey = hotkey_value_from_item( item.value );
            c_key bind { hotkey.value( "key", 0 ), hotkey_mode_to_native( hotkey.value( "mode", std::string( "hold" ) ) ) };
            if ( ui::binder( id_label.c_str( ), &bind, true ) ) {
                update_menu_item_value( item, hotkey_json( bind.key, bind.mode ) );
            }
            update_script_hotkey_state( item );
            show_script_tooltip( item );
        } else {
            ImGui::Text( "%s", item.label.c_str( ) );
            show_script_tooltip( item );
        }

        if ( item.disabled ) ImGui::EndDisabled( );
    }

    void render_script_ui_items( ) {
        std::vector<ws_server::MenuItem> items = ws_server::copy_menu_items( );
        if ( items.empty( ) ) {
            ui::child( "script ui", []( ) {
                ImGui::TextDisabled( "no script ui" );
            } );
            return;
        }

        std::vector<script_ui_group> groups;
        for ( const auto& item : items ) {
            const std::string group_name = item.group.empty( ) ? "script ui" : item.group;
            auto group_it = std::find_if( groups.begin( ), groups.end( ), [&]( const script_ui_group& group ) {
                return group.name == group_name;
            } );
            if ( group_it == groups.end( ) ) {
                groups.push_back( { group_name, { } } );
                group_it = std::prev( groups.end( ) );
            }
            group_it->items.push_back( item );
        }

        for ( const script_ui_group& group : groups ) {
            ui::child( group.name.c_str( ), [&group]( ) {
                for ( const ws_server::MenuItem& item : group.items ) {
                    if ( !item.parent.empty( ) ) continue;
                    render_script_ui_item( item );
                    for ( const ws_server::MenuItem& child : group.items ) {
                        if ( child.parent == item.id ) render_script_ui_item( child );
                    }
                }
            }, script_ui_child_size( group ) );
        }
    }


    void render_hashes_page( ) {
        std::map<DWORD, std::string> hash_names = config::get( "visual", "object_hash_names", std::map<DWORD, std::string>{ } );
        std::map<DWORD, std::string> hash_colors = config::get( "visual", "object_hash_colors", std::map<DWORD, std::string>{ } );
        std::map<DWORD, std::string> hash_lines = config::get( "visual", "object_hash_lines", std::map<DWORD, std::string>{ } );
        std::map<DWORD, std::string> hash_boxes = config::get( "visual", "object_hash_boxes", std::map<DWORD, std::string>{ } );
        const bool show_all_hashes_enabled = config::get( "visual", "pickup_show_all", 0 ) != 0;
        const std::vector<DWORD> seen_hashes = show_all_hashes_enabled ? object_hash_registry::snapshot( ) : std::vector<DWORD>{ };
        const std::vector<object_hash_row> rows = build_object_hash_rows( hash_names, hash_colors, hash_lines, hash_boxes, seen_hashes );
        const bool object_hash_esp_enabled = config::get( "visual", "pickup_esp", 0 ) != 0;
        const bool rows_empty = rows.empty( );
        const visual_config::rgba fallback_color {
            config::get( "visual", "pickup_color_r", 1.f ),
            config::get( "visual", "pickup_color_g", 1.f ),
            config::get( "visual", "pickup_color_b", 1.f ),
            1.f
        };

        if ( rows.empty( ) ) {
            g_object_hash_selected = 0;
        } else if ( g_object_hash_selected == 0 || std::find_if( rows.begin( ), rows.end( ), []( const object_hash_row& row ) {
            return row.hash == g_object_hash_selected;
        } ) == rows.end( ) ) {
            g_object_hash_selected = rows.front( ).hash;
        }

        if ( ( !rows_empty && g_object_hash_rows_were_empty ) || object_hash_esp_enabled != g_object_hash_esp_was_enabled ) {
            g_object_hash_scroll_top = true;
        }
        g_object_hash_rows_were_empty = rows_empty;
        g_object_hash_esp_was_enabled = object_hash_esp_enabled;

        const float page_height = ImGui::GetWindowHeight( ) - GImGui->Style.WindowPadding.y * 2 - SCALE( 7 );
        ui::child( "hashes", [&]( ) {
            if ( g_object_hash_scroll_top ) {
                g_object_hash_scroll_top = !set_current_child_scroll_top( );
            }

            ImGui::PushFont( fonts[font].get( 13 ) );
            ImGui::InputTextWithHint( "search##object_hashes", "type", g_object_hash_search_buffer, sizeof( g_object_hash_search_buffer ) );
            ImGui::PopFont( );

            if ( rows.empty( ) ) {
                ImGui::TextDisabled( "no hashes found" );
                clamp_current_child_scroll_y( );
                return;
            }

            bool any_visible = false;
            for ( const object_hash_row& row : rows ) {
                if ( !matches_search( row.label, g_object_hash_search_buffer ) && !matches_search( format_object_hash( row.hash ), g_object_hash_search_buffer ) ) {
                    continue;
                }

                any_visible = true;
                const std::string row_id = std::string( "hash_row##" ) + std::to_string( row.hash );
                const ImVec2 row_pos = ImGui::GetCursorScreenPos( );
                const ImVec2 row_size( ( std::max )( SCALE( 1 ), ImGui::GetContentRegionAvail( ).x ), SCALE( 17 ) );
                if ( ImGui::InvisibleButton( row_id.c_str( ), row_size ) ) {
                    g_object_hash_selected = row.hash;
                }

                ImU32 text_color = ImGui::GetColorU32( row.hash == g_object_hash_selected ? ImGuiCol_Scheme : ImGuiCol_TextDisabled );
                if ( row.has_custom_color && row.hash != g_object_hash_selected ) {
                    text_color = visual_config::to_u32( visual_config::map_color( hash_colors, row.hash, fallback_color ) );
                }

                const ImVec2 text_pos = row_pos + ImVec2{ 0, row_size.y / 2 - GImGui->FontSize / 2 };
                const float clip_right = ( std::max )( row_pos.x, row_pos.x + row_size.x - SCALE( 8 ) );
                const ImVec2 clip_max( clip_right, row_pos.y + row_size.y );
                ImGui::GetWindowDrawList( )->PushClipRect( row_pos, clip_max, true );
                ImGui::GetWindowDrawList( )->AddText(
                    text_pos,
                    text_color,
                    row.label.c_str( ),
                    row.label.c_str( ) + row.label.size( )
                );
                ImGui::GetWindowDrawList( )->PopClipRect( );
            }

            if ( !any_visible ) {
                ImGui::TextDisabled( "no hashes match search" );
            }
            clamp_current_child_scroll_y( );
        }, { 0, page_height } );

        ImGui::SameLine( );

        ui::child( "advanced", [&]( ) {
            bool object_hash_esp = object_hash_esp_enabled;
            float default_hash_color[4] {
                config::get( "visual", "pickup_color_r", 1.f ),
                config::get( "visual", "pickup_color_g", 1.f ),
                config::get( "visual", "pickup_color_b", 1.f ),
                1.f
            };
            const float prev_default_hash_color[4] {
                default_hash_color[0],
                default_hash_color[1],
                default_hash_color[2],
                default_hash_color[3]
            };

            if ( ui::checkbox( "object hash esp", &object_hash_esp ) ) {
                config::update( object_hash_esp ? 1 : 0, "visual", "pickup_esp", 0 );
                g_object_hash_scroll_top = true;
            }

            bool show_all_hashes = show_all_hashes_enabled;
            if ( ui::checkbox( "show all hashes", &show_all_hashes, nullptr, { default_hash_color } ) ) {
                config::update( show_all_hashes ? 1 : 0, "visual", "pickup_show_all", 0 );
                g_object_hash_scroll_top = true;
            }

            bool ignore_local_held_objects = config::get( "visual", "pickup_ignore_local_held_objects", 0 ) != 0;
            if ( ui::checkbox( "ignore local held objects", &ignore_local_held_objects ) ) {
                config::update( ignore_local_held_objects ? 1 : 0, "visual", "pickup_ignore_local_held_objects", 0 );
            }

            if ( prev_default_hash_color[0] != default_hash_color[0] || prev_default_hash_color[1] != default_hash_color[1] || prev_default_hash_color[2] != default_hash_color[2] ) {
                config::update( default_hash_color[0], "visual", "pickup_color_r", 1.f );
                config::update( default_hash_color[1], "visual", "pickup_color_g", 1.f );
                config::update( default_hash_color[2], "visual", "pickup_color_b", 1.f );
            }

            ImGui::Separator( );

            if ( rows.empty( ) || g_object_hash_selected == 0 ) {
                ImGui::TextDisabled( "hash settings will appear here" );
                return;
            }

            const auto selected_it = std::find_if( rows.begin( ), rows.end( ), []( const object_hash_row& row ) {
                return row.hash == g_object_hash_selected;
            } );
            if ( selected_it == rows.end( ) ) {
                ImGui::TextDisabled( "hash settings will appear here" );
                return;
            }

            const object_hash_row& row = *selected_it;
            const std::string hash_label = format_object_hash( row.hash );
            ImGui::Text( "hash: %s", hash_label.c_str( ) );

            if ( g_object_hash_display_name_hash != row.hash ) {
                copy_to_buffer( g_object_hash_display_name_buffer, sizeof( g_object_hash_display_name_buffer ), row.display_name );
                g_object_hash_display_name_hash = row.hash;
            }

            ImGui::TextUnformatted( "display name" );
            ImGui::PushFont( fonts[font].get( 13 ) );
            if ( ImGui::InputTextWithHint( "##object_hash_display_name", "name", g_object_hash_display_name_buffer, sizeof( g_object_hash_display_name_buffer ) ) ) {
                const std::string display_name = trim_player_text( g_object_hash_display_name_buffer );
                if ( display_name.empty( ) ) {
                    hash_names.erase( row.hash );
                } else {
                    hash_names[row.hash] = display_name;
                }
                config::update( hash_names, "visual", "object_hash_names", std::map<DWORD, std::string>{ } );
            }
            ImGui::PopFont( );

            const visual_config::rgba default_custom_color { 1.f, 1.f, 1.f, 1.f };
            float color[4];
            visual_config::to_float4( visual_config::map_color( hash_colors, row.hash, default_custom_color ), color );
            const float prev_color[4] { color[0], color[1], color[2], color[3] };
            bool custom_color = row.has_custom_color;
            if ( ui::checkbox( "custom color##object_hash_selected", &custom_color, nullptr, { color } ) ) {
                if ( custom_color ) {
                    hash_colors[row.hash] = visual_config::format_rgba( visual_config::from_float4( color ) );
                } else {
                    hash_colors.erase( row.hash );
                }
                config::update( hash_colors, "visual", "object_hash_colors", std::map<DWORD, std::string>{ } );
            }
            if ( prev_color[0] != color[0] || prev_color[1] != color[1] || prev_color[2] != color[2] || prev_color[3] != color[3] ) {
                hash_colors[row.hash] = visual_config::format_rgba( visual_config::from_float4( color ) );
                config::update( hash_colors, "visual", "object_hash_colors", std::map<DWORD, std::string>{ } );
            }

            bool line_enabled = row.line_enabled;
            if ( ui::checkbox( "line##object_hash_selected", &line_enabled ) ) {
                if ( line_enabled ) {
                    hash_lines[row.hash] = "1";
                } else {
                    hash_lines.erase( row.hash );
                }
                config::update( hash_lines, "visual", "object_hash_lines", std::map<DWORD, std::string>{ } );
            }

            bool box_enabled = row.box_enabled;
            if ( ui::checkbox( "box##object_hash_selected", &box_enabled ) ) {
                if ( box_enabled ) {
                    hash_boxes[row.hash] = "1";
                } else {
                    hash_boxes.erase( row.hash );
                }
                config::update( hash_boxes, "visual", "object_hash_boxes", std::map<DWORD, std::string>{ } );
            }

            if ( ui::button( "reset##object_hash_selected", ImVec2( -1, 0 ) ) ) {
                hash_names.erase( row.hash );
                hash_colors.erase( row.hash );
                hash_lines.erase( row.hash );
                hash_boxes.erase( row.hash );
                config::update( hash_names, "visual", "object_hash_names", std::map<DWORD, std::string>{ } );
                config::update( hash_colors, "visual", "object_hash_colors", std::map<DWORD, std::string>{ } );
                config::update( hash_lines, "visual", "object_hash_lines", std::map<DWORD, std::string>{ } );
                config::update( hash_boxes, "visual", "object_hash_boxes", std::map<DWORD, std::string>{ } );
                g_object_hash_display_name_hash = 0;
                if ( !row.seen ) {
                    g_object_hash_selected = 0;
                }
            }
        }, { 0, page_height } );
    }

    void render_playerlist_page( ) {
        static int selected_player = 0;
        static char search[32] = { };

        const std::vector<player_list_entry> players = game::get_player_list_snapshot( );
        const auto is_player_friend = [&]( const player_list_entry& player ) {
            return player.is_friend || player_marks::is_friend( player.static_id );
        };
        const auto is_player_enemy = [&]( const player_list_entry& player ) {
            return player_marks::is_enemy( player.static_id );
        };
        const auto persistent_relation = [&]( const player_list_entry& player ) {
            return player_marks::persistent_get( player.static_id );
        };
        const auto is_player_family = [&]( const player_list_entry& player ) {
            return persistent_relation( player ) == player_marks::relation::family_player || ( config::get( "visual", "relation_auto_family", 1 ) != 0 && player.is_family );
        };
        const auto is_player_fraction = [&]( const player_list_entry& player ) {
            return persistent_relation( player ) == player_marks::relation::fraction_player || ( config::get( "visual", "relation_auto_fraction", 1 ) != 0 && player.is_fraction );
        };

        static std::string selected_player_key;
        auto player_identity = []( const player_list_entry& player ) {
            if ( player.static_id > 0 ) return std::string( "static:" ) + std::to_string( player.static_id );
            if ( player.dynamic_id > 0 ) return std::string( "dynamic:" ) + std::to_string( player.dynamic_id );
            return std::string( "index:" ) + std::to_string( player.index );
        };

        if ( players.empty( ) ) {
            selected_player = 0;
            selected_player_key.clear( );
        } else {
            bool restored_selection = false;
            if ( !selected_player_key.empty( ) ) {
                for ( int i = 0; i < static_cast<int>( players.size( ) ); ++i ) {
                    if ( player_identity( players[i] ) == selected_player_key ) {
                        selected_player = i;
                        restored_selection = true;
                        break;
                    }
                }
            }
            if ( !restored_selection && ( selected_player < 0 || selected_player >= static_cast<int>( players.size( ) ) ) ) {
                selected_player = 0;
            }
            selected_player_key = player_identity( players[selected_player] );
        }

        ui::child( "players", [&]( ) {
            ImGui::PushFont( fonts[font].get( 13 ) );
            ImGui::InputTextWithHint( "search", "type", search, sizeof( search ) );
            ImGui::PopFont( );

            if ( players.empty( ) ) {
                ImGui::TextDisabled( "no players found" );
                return;
            }

            for ( int i = 0; i < static_cast<int>( players.size( ) ); ++i ) {
                const player_list_entry& player = players[i];
                if ( !matches_search( player.display_name, search ) ) {
                    continue;
                }

                const std::string row_id = std::string( "player_row##" ) + std::to_string( i );
                ImGui::BeginChild( row_id.c_str( ), { 0, SCALE( 17 ) }, 0, ImGuiWindowFlags_NoBackground );
                {
                    const ImVec2 avatar_min = ImGui::GetCurrentWindow( )->DC.CursorPos;
                    const ImVec2 avatar_max = avatar_min + SCALE( 17, 17 );

                    if ( g_playerlist_avatar ) {
                        ImGui::GetWindowDrawList( )->AddImageRounded(
                            g_playerlist_avatar,
                            avatar_min,
                            avatar_max,
                            { 0, 0 },
                            { 1, 1 },
                            col( 255, 255, 255, 1.f ),
                            SCALE( 3 )
                        );
                    } else {
                        ImGui::GetWindowDrawList( )->AddRectFilled(
                            avatar_min,
                            avatar_max,
                            ImGui::GetColorU32( i == selected_player ? ImGuiCol_Scheme : ImGuiCol_FrameBgHovered ),
                            SCALE( 3 )
                        );
                    }

                    ImGui::Dummy( SCALE( 17, 17 ) );
                    ImGui::SameLine( 0, SCALE( 8 ) );
                    const float player_text_alpha = GImGui->Style.Alpha;
                    ImU32 text_color = ImGui::GetColorU32( i == selected_player ? ImGuiCol_Scheme : ImGuiCol_TextDisabled );
                    if ( is_player_enemy( player ) ) {
                        text_color = esp::esp_apply_opacity( esp::get_enemy_color_u32( ), player_text_alpha );
                    } else if ( is_player_friend( player ) ) {
                        text_color = esp::esp_apply_opacity( esp::get_friend_color_u32( ), player_text_alpha );
                    } else if ( is_player_family( player ) ) {
                        text_color = esp::esp_apply_opacity( esp::get_family_relation_color_u32( ), player_text_alpha );
                    } else if ( is_player_fraction( player ) ) {
                        text_color = esp::esp_apply_opacity( esp::get_fraction_relation_color_u32( ), player_text_alpha );
                    }
                    ImGui::GetWindowDrawList( )->AddText(
                        ImGui::GetCurrentWindow( )->DC.CursorPos + ImVec2{ 0, SCALE( 17 ) / 2 - GImGui->FontSize / 2 },
                        text_color,
                        player.display_name.c_str( )
                    );

                    if ( ImGui::IsWindowHovered( ) && ImGui::IsMouseClicked( 0 ) ) {
                        selected_player = i;
                        selected_player_key = player_identity( player );
                    }
                }
                ImGui::EndChild( );
            }
        }, { 0, ImGui::GetWindowHeight( ) - GImGui->Style.WindowPadding.y * 2 - SCALE( 7 ) } );

        ImGui::SameLine( );

        ui::child( "advanced", [&]( ) {
            if ( players.empty( ) ) {
                ImGui::TextDisabled( "player info will appear here" );
                return;
            }

            const player_list_entry& player = players[selected_player];
            ImGui::Text( "altv name: %s", player.altv_name.empty( ) ? "-" : player.altv_name.c_str( ) );
            ImGui::Text( "family id: %d%s", player.family_id, is_player_family( player ) ? " (family)" : "" );
            ImGui::Text( "fraction: %s%s", player.faction_name.empty( ) ? "-" : player.faction_name.c_str( ), is_player_fraction( player ) ? " (fraction)" : "" );
            ImGui::Text( "fraction id: %d", player.fraction_id );
            if ( player.static_id > 0 ) {
                ImGui::Text( "static id: %d", player.static_id );
            } else {
                ImGui::Text( "static id: -" );
            }
            if ( player.dynamic_id > 0 ) {
                ImGui::Text( "dynamic id: %d", player.dynamic_id );
            } else {
                ImGui::Text( "dynamic id: -" );
            }
            ImGui::Text( "index: %d", player.index );
            ImGui::Text( "admin: %s", player.is_admin ? "yes" : "no" );
            if ( player.family_id <= 0 ) {
                ImGui::BeginDisabled( );
            }
            if ( ui::button( "ignore this family", ImVec2( -1, 0 ) ) ) {
                add_ignored_family_id( player.family_id, true, true );
            }
            if ( player.family_id <= 0 ) {
                ImGui::EndDisabled( );
            }
            bool friend_player = is_player_friend( player );
            bool enemy_player = is_player_enemy( player );
            if ( player.static_id <= 0 ) {
                ImGui::BeginDisabled( );
            }

            ImGui::SetNextItemWidth( SCALE( 13 ) + GImGui->Style.ItemInnerSpacing.x + ImGui::CalcTextSize( "mark as friend" ).x );
            if ( ui::checkbox( "mark as friend", &friend_player, 0, { }, 0, "ignore in aimbot and mark with FRIEND in ESP" ) ) {
                if ( friend_player ) {
                    enemy_player = false;
                    save_player_relation( player, player_relation::friend_player );
                } else {
                    save_player_relation( player, player_relation::neutral );
                }
            }

            ImGui::SetNextItemWidth( SCALE( 13 ) + GImGui->Style.ItemInnerSpacing.x + ImGui::CalcTextSize( "mark as enemy" ).x );
            if ( ui::checkbox( "mark as enemy", &enemy_player, 0, { }, 0, "mark with ENEMY in ESP" ) ) {
                if ( enemy_player ) {
                    friend_player = false;
                    save_player_relation( player, player_relation::enemy_player );
                } else {
                    save_player_relation( player, player_relation::neutral );
                }
            }

            if ( player.static_id <= 0 ) {
                ImGui::EndDisabled( );
                ImGui::TextDisabled( "friend/enemy toggle needs static id" );
            }

        }, { 0, ImGui::GetWindowHeight( ) - GImGui->Style.WindowPadding.y * 2 - SCALE( 7 ) } );
    }

    void rebuild_fonts( ) {
        ImGuiIO& io = ImGui::GetIO( );

        const char* const tahoma_font_path = "C:\\Windows\\Fonts\\tahoma.ttf";
        const char* const tahoma_bold_font_path = "C:\\Windows\\Fonts\\tahomabd.ttf";
        const char* const verdana_bold_font_path = "C:\\Windows\\Fonts\\verdanab.ttf";
        const char* const calibri_font_path = "C:\\Windows\\Fonts\\calibri.ttf";
        const char* const calibri_bold_font_path = "C:\\Windows\\Fonts\\calibrib.ttf";
        const ImWchar* const default_ranges = io.Fonts->GetGlyphRangesDefault( );
        const ImWchar* const cyrillic_ranges = io.Fonts->GetGlyphRangesCyrillic( );
        fonts[font].set_data( b_font, sizeof( b_font ) );
        fonts[font].set_ranges( default_ranges );
        fonts[font].merge_file( calibri_font_path, cyrillic_ranges );

        fonts[fontb].set_file( tahoma_bold_font_path );
        fonts[font_small].set_file( tahoma_bold_font_path );
        fonts[font_tahoma_bold].set_file( tahoma_bold_font_path );
        fonts[font_verdana_bold].set_file( verdana_bold_font_path );
        fonts[font_calibri].set_file( calibri_font_path );
        fonts[font_calibri_bold].set_file( calibri_bold_font_path );
        fonts[icons].set_data( glyphter, sizeof( glyphter ) );
        fonts[weapon_icon_font].set_data( embedded_fonts::majestic_weapon_icons_ttf, embedded_fonts::majestic_weapon_icons_ttf_size );
        fonts[fontb].set_ranges( cyrillic_ranges );
        fonts[font_small].set_ranges( cyrillic_ranges );
        fonts[font_tahoma_bold].set_ranges( cyrillic_ranges );
        fonts[font_verdana_bold].set_ranges( default_ranges );
        fonts[font_calibri].set_ranges( cyrillic_ranges );
        fonts[font_calibri_bold].set_ranges( default_ranges );
        const static ImWchar icons_ranges[] = { 0x1 + 59647, 0x1 + 62748, 0 };
        const static ImWchar weapon_icon_ranges[] = { 0xE000, 0xE100, 0 };
        fonts[icons].set_ranges( icons_ranges );
        fonts[weapon_icon_font].set_ranges( weapon_icon_ranges );

        fonts[font].init( { 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f } );
        fonts[fontb].init( { 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f, 18.f, 20.f, 22.f, 24.f } );
        fonts[font_small].init( { 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f, 18.f, 20.f, 22.f, 24.f } );
        fonts[font_tahoma_bold].init( { 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f } );
        fonts[font_verdana_bold].init( { 11.f } );
        fonts[font_calibri].init( { 12.f } );
        fonts[font_calibri_bold].init( { 22.f } );
        fonts[icons].init( { 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f } );
        fonts[weapon_icon_font].init( { 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f, 18.f, 20.f, 22.f, 24.f, 26.f, 28.f, 30.f, 32.f, 34.f, 36.f, 38.f, 40.f, 42.f } );
        io.FontDefault = fonts[font].get( 13.f );
    }

    void render_esp_preview_inline( ) {
        static float preview_rot_y = 0.f;
        static float preview_rot_x = 0.f;
        static esp::projected_esp_element_id dragging_element = esp::projected_esp_element_id::none;
        static ImVec2 dragging_element_offset{};
        static std::array<float, esp::k_projected_esp_element_count> element_interaction_anim{};
        static std::array<ImVec2, esp::k_projected_esp_element_count> element_position_anim{};
        static std::array<bool, esp::k_projected_esp_element_count> element_position_anim_valid{};
        static esp::projected_esp_overlay_rects last_rects{};
        static bool g_visual_preview_failed = false;
        static esp_preview_settings_popup settings_popup{};
        static ImVec2 last_preview_origin{};
        static bool last_preview_origin_valid = false;

        {
            const ImVec2 cursor_min = ImGui::GetCursorScreenPos( );
            ImGuiWindow* window = ImGui::GetCurrentWindow( );
            const ImVec2 area_min = window ? window->WorkRect.Min : cursor_min;
            const ImVec2 area_max = window ? window->WorkRect.Max : cursor_min + ImGui::GetContentRegionAvail( );
            const ImVec2 img_min( cursor_min.x, area_min.y );
            ImVec2 view_size(
                ( std::max )( area_max.x - cursor_min.x, 1.f ),
                ( std::max )( area_max.y - area_min.y, 1.f )
            );
            const ImVec2 img_max = img_min + view_size;
            ImDrawList* draw_list = ImGui::GetWindowDrawList( );
            draw_list->PushClipRect( img_min, img_max, true );
            const float preview_ui_alpha = std::clamp( ImGui::GetStyle( ).Alpha, 0.f, 1.f );

            preview_rot_x = 0.f;

            esp::projected_esp_settings preview_settings = esp::read_projected_esp_settings( );
            esp::projected_esp_player preview_player{};
            const int preview_weapon_language = std::clamp( config::get( "visual", "esp_weapon_text_language", 1 ), 0, 1 );
            preview_player.name = "Hello Watson";
            preview_player.social = "hello";
            preview_player.faction = "EMS";
            preview_player.weapon_label = preview_weapon_language == 0 ? "Heavy Rifle" : "\xD0\xA2\xD1\x8F\xD0\xB6\xD1\x91\xD0\xBB\xD0\xB0\xD1\x8F\x20\xD0\xB2\xD0\xB8\xD0\xBD\xD1\x82\xD0\xBE\xD0\xB2\xD0\xBA\xD0\xB0";
            preview_player.static_id = 1234;
            preview_player.dynamic_id = 56;
            preview_player.fraction_id = 2;
            preview_player.level = 10;
            preview_player.admin_level = 3;
            preview_player.health = 78.f;
            preview_player.armor = 42.f;
            preview_player.weapon_hash = 0xC78D71B4u;
            preview_player.is_admin = true;
            preview_player.is_tester = true;
            preview_player.is_media = true;
            preview_player.is_afk = true;
            preview_player.is_cuffed = true;
            preview_player.has_wear_armor = true;
            preview_player.wanted_level = 3;
            g_preview_skeleton_invisible = config::get( "visual", "skel_preview_invisible", 0 ) != 0;
            preview_player.visible = !g_preview_skeleton_invisible;
            preview_player.is_fraction = true;
            preview_settings.faction_color = IM_COL32(
                (int)( config::get( "hud", "accent_r", 180.f / 255.f ) * 255.f ),
                (int)( config::get( "hud", "accent_g", 167.f / 255.f ) * 255.f ),
                (int)( config::get( "hud", "accent_b", 245.f / 255.f ) * 255.f ),
                255
            );
            preview_settings.force_dead_label = preview_settings.show_dead;

            constexpr float preview_model_scale = 0.80f;
            constexpr float preview_aspect = 1.18f / 2.38f;

            ImVec2 preview_min{};
            ImVec2 preview_size{};
            ImVec2 model_min{};
            ImVec2 model_max{};
            ImVec2 box_center{};
            ImVec2 top2d{};
            ImVec2 bottom2d{};
            ImVec2 box_min{};
            ImVec2 box_max{};

            auto update_preview_geometry = [ & ]( ) {
                auto fit_preview_size = [ & ]( float height ) {
                    ImVec2 size( height * preview_aspect, height );
                    if ( size.x > view_size.x ) {
                        size.x = view_size.x;
                        size.y = view_size.x / preview_aspect;
                    }
                    return size;
                };

                const float top_stack_shift = ( std::min )( SCALE( 84.f ), view_size.y * 0.18f );
                const float bottom_reserved = SCALE( 28.f );
                preview_size = fit_preview_size( ( std::max )( 1.f, view_size.y - top_stack_shift - bottom_reserved ) );
                if ( preview_size.x > view_size.x ) {
                    preview_size.x = view_size.x;
                    preview_size.y = view_size.x / preview_aspect;
                }

                preview_min = ImVec2(
                    img_min.x + ( view_size.x - preview_size.x ) * 0.5f,
                    img_min.y + top_stack_shift
                );

                const float model_y_adjust = preview_size.y * 0.035f;
                model_min = ImVec2( preview_min.x, preview_min.y - model_y_adjust );
                model_max = ImVec2( preview_min.x + preview_size.x, preview_min.y + preview_size.y - model_y_adjust );

                const float box_top_offset = preview_size.y * 0.06f;
                const float box_height = preview_size.y * 0.80f;
                top2d = ImVec2( preview_min.x + preview_size.x * 0.5f, preview_min.y + box_top_offset );
                bottom2d = ImVec2( top2d.x, top2d.y + box_height );
                box_center = ImVec2( top2d.x, top2d.y + box_height * 0.5f );
                const float box_width = box_height * 0.5f;
                box_min = ImVec2( top2d.x - box_width * 0.5f, top2d.y );
                box_max = ImVec2( top2d.x + box_width * 0.5f, bottom2d.y );
            };

            update_preview_geometry( );

            ImGui::SetCursorScreenPos( img_min );
            ImGui::InvisibleButton( "##preview_drag", view_size );
            const bool hovered = ImGui::IsItemHovered( );
            const ImVec2 mouse_pos = ImGui::GetMousePos( );

            esp::projected_esp_element_id hovered_element = esp::projected_esp_element_id::none;
            if ( hovered ) {
                hovered_element = esp::hit_test_projected_esp_overlay( last_rects, mouse_pos, ImVec2( SCALE( 4.f ), SCALE( 3.f ) ) );
            }

            auto apply_hovered_size_delta = [ & ]( esp::projected_esp_element_id id, float wheel_delta ) {
                if ( !esp::is_projected_esp_scalable_element( id ) || std::fabs( wheel_delta ) <= 0.001f ) {
                    return false;
                }

                const std::string key = esp::projected_esp_size_key( id );
                const float current = esp::clamp_projected_esp_size( id, config::get( "visual", key.c_str( ), esp::default_projected_esp_size( id ) ) );
                const float next = esp::clamp_projected_esp_size( id, current + wheel_delta * esp::k_projected_esp_size_step );
                if ( std::fabs( next - current ) <= 0.001f ) {
                    return false;
                }

                config::update( next, "visual", key.c_str( ), esp::default_projected_esp_size( id ) );
                const int index = esp::projected_esp_element_index( id );
                if ( index >= 0 ) {
                    preview_settings.element_styles[index].size = next;
                    element_position_anim_valid[index] = false;
                }
                return true;
            };

            if ( hovered && hovered_element != esp::projected_esp_element_id::none ) {
                apply_hovered_size_delta( hovered_element, ImGui::GetIO( ).MouseWheel );
            }

            const ImRect box_rect( box_min, box_max );
            auto box_edge_contains = [ & ]( const ImVec2& pos ) {
                const float hit_pad = SCALE( 5.f );
                const ImRect outer( box_rect.Min - ImVec2( hit_pad, hit_pad ), box_rect.Max + ImVec2( hit_pad, hit_pad ) );
                const ImRect inner( box_rect.Min + ImVec2( hit_pad, hit_pad ), box_rect.Max - ImVec2( hit_pad, hit_pad ) );
                return outer.Contains( pos ) && !inner.Contains( pos );
            };

            if ( hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Right ) ) {
                if ( hovered_element != esp::projected_esp_element_id::none ) {
                    settings_popup.open = true;
                    settings_popup.just_opened = true;
                    settings_popup.kind = esp_preview_settings_kind::element;
                    settings_popup.element = hovered_element;
                    settings_popup.pos = mouse_pos;
                }
                else if ( preview_settings.draw_box && box_edge_contains( mouse_pos ) ) {
                    settings_popup.open = true;
                    settings_popup.just_opened = true;
                    settings_popup.kind = esp_preview_settings_kind::box;
                    settings_popup.element = esp::projected_esp_element_id::none;
                    settings_popup.pos = mouse_pos;
                }
                else if ( preview_settings.draw_skeleton && box_rect.Contains( mouse_pos ) ) {
                    settings_popup.open = true;
                    settings_popup.just_opened = true;
                    settings_popup.kind = esp_preview_settings_kind::skeleton;
                    settings_popup.element = esp::projected_esp_element_id::none;
                    settings_popup.pos = mouse_pos;
                }
                else if ( !settings_popup.open ) {
                    preview_rot_y = 0.f;
                    preview_rot_x = 0.f;
                }
            }

            if ( hovered && dragging_element == esp::projected_esp_element_id::none && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) && hovered_element != esp::projected_esp_element_id::none ) {
                dragging_element = hovered_element;
                const int hovered_index = esp::projected_esp_element_index( hovered_element );
                if ( hovered_index >= 0 && last_rects.elements[hovered_index].visible ) {
                    dragging_element_offset = last_rects.elements[hovered_index].rect.Min - mouse_pos;
                } else {
                    dragging_element_offset = ImVec2( 0.f, 0.f );
                }
            }

            if ( dragging_element == esp::projected_esp_element_id::none && ImGui::IsItemActive( ) && ImGui::IsMouseDragging( ImGuiMouseButton_Left, 0.f ) ) {
                preview_rot_y -= ImGui::GetIO( ).MouseDelta.x * 0.01f;
            }

            auto resolve_side = [ & ]( const ImVec2& pos ) {
                if ( pos.x < box_min.x ) {
                    return esp::projected_esp_side::left;
                }
                if ( pos.x > box_max.x ) {
                    return esp::projected_esp_side::right;
                }
                return pos.y < box_center.y ? esp::projected_esp_side::top : esp::projected_esp_side::bottom;
            };

            auto resolve_order = [ & ]( esp::projected_esp_side side, const ImVec2& pos, esp::projected_esp_element_id moving ) {
                std::vector<float> centers;
                for ( int i = 0; i < esp::k_projected_esp_element_count; ++i ) {
                    const auto& element = last_rects.elements[i];
                    if ( !element.visible || element.side != side || static_cast<esp::projected_esp_element_id>( i ) == moving ) {
                        continue;
                    }
                    const auto id = static_cast<esp::projected_esp_element_id>( i );
                    if ( ( side == esp::projected_esp_side::left || side == esp::projected_esp_side::right ) &&
                         ( id == esp::projected_esp_element_id::health || id == esp::projected_esp_element_id::armor ) ) {
                        continue;
                    }
                    centers.push_back( element.rect.GetCenter( ).y );
                }
                std::sort( centers.begin( ), centers.end( ) );
                int order = 0;
                for ( const float center_y : centers ) {
                    if ( pos.y > center_y ) {
                        ++order;
                    }
                }
                return order;
            };

            if ( dragging_element != esp::projected_esp_element_id::none ) {
                const esp::projected_esp_side side = resolve_side( mouse_pos );
                preview_settings.layout_override_active = true;
                preview_settings.layout_override_element = dragging_element;
                preview_settings.layout_override.side = side;
                preview_settings.layout_override.order = resolve_order( side, mouse_pos, dragging_element );
                preview_settings.drag_visual_active = true;
                preview_settings.drag_visual_element = dragging_element;
                preview_settings.drag_visual_pos = mouse_pos + dragging_element_offset;
                update_preview_geometry( );
            }

            const bool preview_origin_changed = last_preview_origin_valid &&
                ( fabsf( last_preview_origin.x - preview_min.x ) > 0.5f || fabsf( last_preview_origin.y - preview_min.y ) > 0.5f );
            if ( preview_origin_changed && dragging_element == esp::projected_esp_element_id::none ) {
                element_position_anim_valid.fill( false );
            }
            last_preview_origin = preview_min;
            last_preview_origin_valid = true;

            const float interaction_lerp = std::clamp( ImGui::GetIO( ).DeltaTime * 14.f, 0.f, 1.f );
            for ( int i = 0; i < esp::k_projected_esp_element_count; ++i ) {
                const auto id = static_cast<esp::projected_esp_element_id>( i );
                const bool settings_active = settings_popup.open && settings_popup.kind == esp_preview_settings_kind::element && id == settings_popup.element;
                const bool active = id == hovered_element || id == dragging_element || settings_active;
                element_interaction_anim[i] = ImLerp( element_interaction_anim[i], active ? 1.f : 0.f, interaction_lerp );
            }
            preview_settings.draw_interaction_backgrounds = true;
            preview_settings.interaction_alpha = element_interaction_anim;
            preview_settings.animated_positions = &element_position_anim;
            preview_settings.animated_position_valid = &element_position_anim_valid;
            preview_settings.layout_lerp = std::clamp( ImGui::GetIO( ).DeltaTime * 14.f, 0.f, 1.f );
            preview_settings.clamp_top_stack_y = true;
            preview_settings.top_stack_min_y = img_min.y + SCALE( 2.f );

            float preview_opacity = 1.f;
            if ( preview_player.is_dead && preview_settings.dim_dead_players ) {
                preview_opacity *= 0.7f;
            }
            preview_opacity *= preview_ui_alpha;

            if ( !g_visual_preview_failed ) {
                if ( !render_model_preview_safe( preview_rot_y, preview_rot_x, preview_model_scale ) ) {
                    g_visual_preview_failed = true;
                }
            }
            if ( !g_visual_preview_failed && model_preview::is_loaded( ) ) {
                ImTextureID preview_texture = model_preview::get_texture( );
                if ( preview_texture ) {
                    draw_list->AddImage( preview_texture, model_min, model_max, ImVec2( 0.f, 0.f ), ImVec2( 1.f, 1.f ), IM_COL32( 255, 255, 255, (int)( 255.f * preview_ui_alpha ) ) );
                }
            }

            if ( !g_visual_preview_failed && config::get( "visual", "draw_skeleton", 0 ) != 0 && model_preview::get_bone_count( ) > 0 ) {
                const ImU32 skeleton_color = esp::esp_apply_opacity( esp::read_visual_color_u32(
                    g_preview_skeleton_invisible ? "skel_invisible_r" : "skel_visible_r",
                    g_preview_skeleton_invisible ? "skel_invisible_g" : "skel_visible_g",
                    g_preview_skeleton_invisible ? "skel_invisible_b" : "skel_visible_b",
                    g_preview_skeleton_invisible ? "skel_invisible_a" : "skel_visible_a",
                    1.f, 1.f, 1.f, 1.f ), preview_ui_alpha );
                const float skeleton_thickness = std::clamp( config::get( "visual", "skeleton_thickness", 0.1f ), 0.1f, 1.f );
                if ( !draw_model_preview_skeleton_safe( draw_list, model_min, model_max, preview_rot_y, preview_rot_x, skeleton_color, skeleton_thickness, preview_model_scale ) ) {
                    g_visual_preview_failed = true;
                }
            }

            runtime_debug::last_section = "visuals_preview_overlay";
            last_rects = esp::draw_projected_player_overlay( draw_list, preview_settings, preview_player, top2d, bottom2d, 12.f, preview_opacity );
            if ( settings_popup.open && settings_popup.kind == esp_preview_settings_kind::box && preview_settings.draw_box ) {
                draw_list->AddRect(
                    box_rect.Min - ImVec2( SCALE( 3.f ), SCALE( 3.f ) ),
                    box_rect.Max + ImVec2( SCALE( 3.f ), SCALE( 3.f ) ),
                    IM_COL32( 255, 255, 255, (int)( 45.f * preview_ui_alpha ) ),
                    preview_settings.box_radius,
                    0,
                    SCALE( 1.f )
                );
            }
            else if ( settings_popup.open && settings_popup.kind == esp_preview_settings_kind::skeleton && preview_settings.draw_skeleton ) {
                draw_list->AddRectFilled( box_rect.Min, box_rect.Max, IM_COL32( 255, 255, 255, (int)( 12.f * preview_ui_alpha ) ), 0.f );
            }

            if ( dragging_element != esp::projected_esp_element_id::none && !ImGui::IsMouseDown( ImGuiMouseButton_Left ) ) {
                for ( int i = 0; i < esp::k_projected_esp_element_count; ++i ) {
                    const auto& element = last_rects.elements[i];
                    if ( element.visible ) {
                        esp::save_projected_esp_layout( static_cast<esp::projected_esp_element_id>( i ), element.side, element.order );
                    }
                }
                dragging_element = esp::projected_esp_element_id::none;
            }
            draw_list->PopClipRect( );
            render_esp_preview_settings_popup( settings_popup, preview_settings );
        }
    }

    void setup_pages( ) {
        if ( g_pages_added ) {
            return;
        }

        ui::tabs = {
            { "aimbot" },
            { "visuals", { "players", "other" } },
            { "misc" },
            { "settings" },
            { "playerlist" },
            { "hashes" }
        };

        childs.clear( );
        search_buf[0] = '\0';
        ui::cur_page = 0;
        ui::next_tab = 0;

        ui::add_page( 0, render_aimbot_page );
        ui::add_page( 1, render_visuals_players_page );
        ui::add_page( 1, render_visuals_world_page );
        ui::add_page( 2, render_misc_page );
        ui::add_page( 3, render_settings_page );
        ui::add_page( 4, render_playerlist_page );
        ui::add_page( 5, render_hashes_page );

        g_pages_added = true;
    }
}

namespace menu {
    void initialize( ID3D11Device* device ) {
        if ( g_initialized ) {
            return;
        }

        hello_paths::migrate_legacy_root( );

        ImGuiIO& io = ImGui::GetIO( );
        io.Fonts->Clear( );
        for ( auto& font_entry : fonts ) {
            font_entry.get_fonts( ).clear( );
            font_entry.should_init.clear( );
        }

        rebuild_fonts( );
        ImGui_ImplDX11_CreateDeviceObjects( );

        if ( !g_playerlist_avatar && device ) {
            CreateSRVFromMemory( device, avatarb, sizeof( avatarb ), &g_playerlist_avatar );
        }

        if ( device ) {
            ID3D11DeviceContext* context = nullptr;
            device->GetImmediateContext( &context );
            if ( context ) {
                if ( model_preview::init( device, context ) ) {
                    model_preview::load_happ_embedded( );
                    model_preview::load_bones_embedded( );
                }
                context->Release( );
            }
        }

        g_configs_loaded = false;
        refresh_local_configs( );
        setup_pages( );

        g_initialized = true;
    }

    void shutdown( ) {
        if ( g_playerlist_avatar ) {
            g_playerlist_avatar->Release( );
            g_playerlist_avatar = nullptr;
        }

        model_preview::shutdown( );

        g_initialized = false;
        g_pages_added = false;
        g_configs_loaded = false;
        g_hydrated_menu_items.clear( );
        g_listening_bind.clear( );
    }

    void render( bool menu_open ) {
        if ( !g_initialized || !ImGui::GetCurrentContext( ) ) {
            return;
        }

        ui::styles( );

        ui::menu_col[0] = config::get( "hud", "accent_r", 180.f / 255.f );
        ui::menu_col[1] = config::get( "hud", "accent_g", 167.f / 255.f );
        ui::menu_col[2] = config::get( "hud", "accent_b", 245.f / 255.f );
        ui::menu_col[3] = 1.f;
        ui::colors( );
        apply_builtin_menu_updates( );
        hydrate_script_menu_items( );
        poll_script_hotkeys( );

        static float menu_alpha = 0.f;
        const float target_alpha = menu_open ? 1.f : 0.f;
        const float alpha_lerp = std::clamp( ImGui::GetIO( ).DeltaTime * 12.f, 0.f, 1.f );
        menu_alpha = ImLerp( menu_alpha, target_alpha, alpha_lerp );

        if ( menu_alpha < 0.001f ) {
            notify::draw( );
            return;
        }

        init_search( );


        ImGui::SetNextWindowSize( SCALE( ui::size.x, ui::size.y ), ImGuiCond_Once );
        ImGui::SetNextWindowPos(
            ImVec2( ImGui::GetIO( ).DisplaySize.x * 0.5f, ImGui::GetIO( ).DisplaySize.y * 0.5f ),
            ImGuiCond_Once,
            ImVec2( 0.5f, 0.5f )
        );

        ImGui::PushStyleVar( ImGuiStyleVar_Alpha, menu_alpha );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowMinSize, SCALE( ui::size.x, ui::size.y ) );
        if ( ImGui::Begin( "ui", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings ) ) {
            ImGui::PopStyleVar( );

            ImGui::BeginChild( "navbar", SCALE( 0, 56 ), 0, ImGuiWindowFlags_NoBackground );
            {
                ImGui::SetCursorPos( SCALE( 20, 56 / 2 - 11 / 2 ) );
                ImGui::BeginGroup( );
                {
                    ImGui::PushFont( fonts[font].get( 13 ) );
                    ImGui::Text( "nigga" );
                    ImGui::SameLine( 0, 0 );
                    ImGui::TextColored( ImGui::GetStyleColorVec4( ImGuiCol_Scheme ), "hook" );
                    ImGui::SameLine( 0, SCALE( 21 ) );
                    ui::tabs_manager::render( SCALE( 10 ), true );
                    ImGui::PopFont( );
                }
                ImGui::EndGroup( );
                const float tabs_end_x = ImGui::GetItemRectMax( ).x - ImGui::GetWindowPos( ).x;

                static bool search_active = false;
                static float search_anim = 0.f;
                search_anim = ImLerp( search_anim, search_active ? 1.f : 0.f, ImGui::GetIO( ).DeltaTime * 15.f );

                const float search_x = ImGui::GetWindowWidth( ) - SCALE( 36 ) - SCALE( 100 ) * search_anim;

                ImGui::SetCursorPos( { search_x, ImGui::GetWindowHeight( ) / 2 - SCALE( 8 ) } );
                ImGui::BeginChild( "search_field", SCALE( 250, 16 ), 0, ImGuiWindowFlags_NoBackground );
                {
                    const bool hovered = ImGui::IsWindowHovered( ImGuiHoveredFlags_AllowWhenBlockedByActiveItem );
                    ui::add_text( icons, 16, ImGui::GetWindowPos( ), ImGui::GetColorU32( ImGuiCol_Scheme ), search_2_line );

                    ImGui::SetCursorPos( { SCALE( 26 ) + SCALE( 20 ) * ( 1.f - search_anim ), ImGui::GetWindowHeight( ) / 2 - SCALE( 13 ) / 2 + 1 } );
                    ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 0, 0 ) );
                    ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 0 );
                    ImGui::PushStyleColor( ImGuiCol_FrameBg, ImGui::GetColorU32( ImGuiCol_FrameBg, 0.f ) );
                    ImGui::InputTextEx( "##search", "type...", search_buf, sizeof( search_buf ), SCALE( 200, 13 ), 0 );
                    ImGui::PopStyleColor( );
                    ImGui::PopStyleVar( 2 );

                    search_active = ImGui::IsItemActive( ) || std::strlen( search_buf ) > 0 || hovered;
                }
                ImGui::EndChild( );
            }
            ImGui::EndChild( );

            ImGui::SetCursorPos( SCALE( 20, 56 ) );

            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, SCALE( 14, 14 ) );
            ImGui::PushStyleColor( ImGuiCol_ChildBg, ImGui::GetColorU32( ImGuiCol_FrameBg ) );
            ImGui::PushStyleColor( ImGuiCol_Border, col( 36, 36, 36, 0.8f ).Value );
            ImGui::BeginChild( "main", { -SCALE( 20 ), -SCALE( 41 ) }, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysUseWindowPadding );
            {
                ImGui::PopStyleColor( 2 );
                if ( std::strlen( search_buf ) == 0 ) {
                    const bool has_subtabs = ui::cur_page >= 0 &&
                        ui::cur_page < static_cast<int>( ui::tabs.size( ) ) &&
                        !ui::tabs[ui::cur_page].subtabs.empty( );
                    if ( has_subtabs ) {
                        ui::subtabs_manager::render( SCALE( 10 ), true );
                    }
                    const float subtab_alpha = has_subtabs ? ui::content_anim2 : 1.f;
                    ImGui::PushStyleVar( ImGuiStyleVar_Alpha, ui::content_anim * subtab_alpha * GImGui->Style.Alpha );
                    ui::render_page( );
                    ImGui::PopStyleVar( );
                } else {
                    render_window( );
                }
            }
            ImGui::EndChild( );
            ImGui::PopStyleVar( );

            std::string footer_label = "build date: ";
            std::string footer_value = build_timestamp_string( );
            std::string footer_user;
            if constexpr ( build_profile::production ) {
                footer_user = runtime_session::user_name( );
                if ( footer_user.empty( ) ) footer_user = "pasted + detected";
            }
            ui::add_text( font, 13, ImGui::GetWindowPos( ) + ImVec2{ SCALE( 20 ), ImGui::GetWindowHeight( ) - SCALE( 27 ) }, ImGui::GetColorU32( ImGuiCol_TextDisabled ), footer_label.c_str( ) );
            ui::add_text( font, 13, ImGui::GetWindowPos( ) + ImVec2{ SCALE( 20 ) + ui::text_size( font, 13, footer_label.c_str( ) ).x, ImGui::GetWindowHeight( ) - SCALE( 27 ) }, ImGui::GetColorU32( ImGuiCol_Scheme ), footer_value.c_str( ) );
            ui::add_text( font, 13, ImGui::GetWindowPos( ) + ImVec2{ ImGui::GetWindowWidth( ) - SCALE( 20 ) - ui::text_size( font, 13, footer_user.c_str( ) ).x, ImGui::GetWindowHeight( ) - SCALE( 27 ) }, ImGui::GetColorU32( ImGuiCol_Text ), footer_user.c_str( ) );
        } else {
            ImGui::PopStyleVar( );
        }
        ImGui::End( );
        ImGui::PopStyleVar( );

        ws_server::flush_builtin_menu_snapshot( );

        ui::handle_alpha_anim( );
        notify::draw( );
    }
}
