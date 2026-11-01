#pragma once

#include <string>
#include <vector>

namespace game {
    struct player_list_ui_entry {
        std::string friend_key;
        std::string display_name;
        std::string altv_name;
        std::string gta_name;
        std::string faction_name;
        int fraction_id = 0;
        int family_id = 0;
        bool is_family = false;
        bool is_fraction = false;
        int static_id = 0;
        int dynamic_id = 0;
        int index = 0;
        bool has_ws_data = false;
        bool is_admin = false;
        bool is_friend = false;
    };

    std::vector<player_list_ui_entry> get_player_list_snapshot();
}
