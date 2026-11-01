#pragma once

#include <cctype>
#include <iostream>
#include <string_view>

using namespace ImGui;

inline char search_buf[16];
inline bool g_collect_search_index = false;

struct c_child {
    int tab;
    int subtab;
    std::string label;
    std::string desc;
    std::vector< std::string > widgets{ };
};

inline std::vector< c_child > childs;

inline void add_child( int tab, int subtab, const char* label, const char* desc ) {
    if ( ( GImGui->CurrentItemFlags & ImGuiItemFlags_Search ) && !g_collect_search_index ) {
        return;
    }

    const std::string child_label = label ? label : "";
    for ( auto& child : childs ) {
        if ( child.tab == tab && child.subtab == subtab && child.label == child_label ) {
            child.desc = desc ? desc : "";
            return;
        }
    }

    childs.emplace_back( c_child{ tab, subtab, child_label, desc ? desc : "" } );
}

inline std::string to_lower( std::string_view str ) {
    std::string result;
    result.reserve( str.size( ) );

    for ( unsigned char ch : str ) {
        result += static_cast<char>( std::tolower( ch ) );
    }

    return result;
}

inline bool search_match( std::string_view value, const std::string& query ) {
    return to_lower( value ).find( query ) != std::string::npos;
}

inline std::string search_display_label( std::string_view label ) {
    const size_t id_separator = label.find( "##" );
    if ( id_separator == std::string_view::npos ) {
        return std::string( label );
    }

    return std::string( label.substr( 0, id_separator ) );
}

inline void activate_search_result( int tab, int subtab ) {
    ui::cur_page = tab;
    ui::next_tab = tab;
    ui::tabs[tab].cur_subtab = subtab;
    ui::tabs[tab].next_page = subtab;
    ui::content_anim = 1.f;
    ui::content_anim_dest = 1.f;
    ui::content_anim2 = 1.f;
    ui::content_anim2_dest = 1.f;
    search_buf[0] = '\0';
}

inline void render_search_result_row( const c_child& child, const std::string& label, const char* id ) {
    const float row_height = SCALE( 36 );
    const ImVec2 row_pos = GetCursorScreenPos( );
    const ImVec2 row_size = ImVec2( GetContentRegionAvail( ).x, row_height );

    InvisibleButton( id, row_size );
    const bool hovered = IsItemHovered( );
    if ( IsItemClicked( ) ) {
        activate_search_result( child.tab, child.subtab );
    }

    ImDrawList* draw_list = GetWindowDrawList( );
    if ( hovered ) {
        draw_list->AddRectFilled( row_pos, row_pos + row_size, GetColorU32( ImGuiCol_FrameBgHovered ), SCALE( 3 ) );
    }

    const std::string path = std::string( ui::tabs[child.tab].label ).append( " / " ).append( search_display_label( child.label ) );
    draw_list->AddText( row_pos + SCALE( 10, 5 ), GetColorU32( ImGuiCol_Text ), label.c_str( ), label.c_str( ) + label.size( ) );
    draw_list->AddText( row_pos + SCALE( 10, 20 ), GetColorU32( ImGuiCol_TextDisabled ), path.c_str( ), path.c_str( ) + path.size( ) );
}

inline void init_search( ) {
    static bool init = false;

    if ( init && !childs.empty( ) ) {
        return;
    }

    childs.clear( );

    const int old_page = ui::cur_page;
    const int old_next_tab = ui::next_tab;
    std::vector< int > old_cur_subtabs;
    std::vector< int > old_next_pages;
    old_cur_subtabs.reserve( ui::tabs.size( ) );
    old_next_pages.reserve( ui::tabs.size( ) );

    for ( const auto& tab : ui::tabs ) {
        old_cur_subtabs.emplace_back( tab.cur_subtab );
        old_next_pages.emplace_back( tab.next_page );
    }

    SetNextWindowPos( ImVec2( -10000.f, -10000.f ), ImGuiCond_Always );
    SetNextWindowSize( ImVec2( 1.f, 1.f ), ImGuiCond_Always );
    PushStyleVar( ImGuiStyleVar_Alpha, 0.f );
    Begin( "##search_index", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground );
    g_collect_search_index = true;

    for ( int i = 0; i < static_cast<int>( ui::tabs.size( ) ); ++i ) {
        if ( ui::tabs[i].pages.empty( ) ) continue;

        ui::cur_page = i;
        ui::next_tab = i;
        ui::tabs[i].cur_subtab = 0;
        ui::tabs[i].next_page = 0;

        for ( int v = 0; v < static_cast<int>( ui::tabs[i].pages.size( ) ); ++v ) {
            ui::tabs[i].cur_subtab = v;
            ui::tabs[i].next_page = v;
            ui::tabs[i].pages[v]( );
        }
    }

    g_collect_search_index = false;
    End( );
    PopStyleVar( );

    ui::cur_page = old_page;
    ui::next_tab = old_next_tab;

    for ( int i = 0; i < static_cast<int>( ui::tabs.size( ) ); ++i ) {
        ui::tabs[i].cur_subtab = old_cur_subtabs[i];
        ui::tabs[i].next_page = old_next_pages[i];
    }

    init = !childs.empty( );
}

inline bool add_widget( const char* label ) {
    if ( g_collect_search_index ) {
        if ( !childs.empty( ) ) {
            auto& widgets = childs.back( ).widgets;
            const std::string widget_label = label ? label : "";
            for ( const auto& existing_label : widgets ) {
                if ( existing_label == widget_label ) {
                    return false;
                }
            }

            widgets.emplace_back( widget_label );
        }

        return false;
    }

    if ( childs.empty( ) ) {
        return !( GImGui->CurrentItemFlags & ImGuiItemFlags_Search );
    }

    struct s {
        bool init = false;
    }; auto& obj = anim_obj( label, 433232, s{ } );

    if ( !obj.init && !( GImGui->CurrentItemFlags & ImGuiItemFlags_Search ) ) {
        childs[childs.size( ) - 1].widgets.emplace_back( label );
        obj.init = true;
    }

    int cur_child = 0;

    while ( cur_child < childs.size( ) - 1 ) {
        bool should_break = false;
        for ( auto& l : childs[cur_child].widgets ) {
            if ( l == label ) {
                should_break = true;
                break;
            }
        }

        if ( should_break ) break;
        cur_child++;
    }

    const std::string query = to_lower( search_buf );
    return search_match( childs[cur_child].label, query ) || search_match( label, query ) || !( GImGui->CurrentItemFlags & ImGuiItemFlags_Search );
}

inline void render_window( ) {
    if ( strlen( search_buf ) == 0 )
        return;

    const std::string query = to_lower( search_buf );
    bool has_results = false;

    BeginChild( "search_results", { 0, 0 }, 0, ImGuiWindowFlags_NoBackground );
    for ( auto& child : childs ) {
        const std::string child_label = search_display_label( child.label );

        if ( search_match( child_label, query ) ) {
            has_results = true;
            const std::string row_id = "child_result##" + std::to_string( child.tab ) + ":" + std::to_string( child.subtab ) + ":" + child.label;
            render_search_result_row( child, child_label, row_id.c_str( ) );
        }

        for ( int i = 0; i < static_cast<int>( child.widgets.size( ) ); ++i ) {
            const std::string widget_label = search_display_label( child.widgets[i] );
            if ( widget_label.empty( ) || !search_match( widget_label, query ) ) {
                continue;
            }

            has_results = true;
            const std::string row_id = "widget_result##" + std::to_string( child.tab ) + ":" + std::to_string( child.subtab ) + ":" + std::to_string( i );
            render_search_result_row( child, widget_label, row_id.c_str( ) );
        }
    }

    if ( !has_results ) {
        TextDisabled( "no results." );
    }
    EndChild( );
}
