#pragma once

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace player_marks {
    enum class relation : int {
        neutral = 0,
        friend_player = 1,
        enemy_player = 2,
        family_player = 3,
        fraction_player = 4
    };

    inline std::mutex g_lock;
    inline std::string g_server_id;
    inline std::map<int, relation> g_marks;
    inline std::map<int, relation> g_local_marks;
    inline std::map<int, relation> g_pending_marks;
    inline void set_server_id(const std::string& value) {
        std::lock_guard<std::mutex> lock(g_lock);
        g_server_id = value;
    }

    inline std::string server_id() {
        std::lock_guard<std::mutex> lock(g_lock);
        return g_server_id;
    }

    inline void replace(const std::string& server_id_value, const std::vector<std::pair<int, relation>>& marks) {
        std::lock_guard<std::mutex> lock(g_lock);
        g_server_id = server_id_value;
        g_marks.clear();
        for (const auto& mark : marks) {
            if (mark.first > 0 && mark.second != relation::neutral) g_marks[mark.first] = mark.second;
        }
        for (const auto& mark : g_pending_marks) {
            if (mark.second == relation::neutral) g_marks.erase(mark.first);
            else g_marks[mark.first] = mark.second;
        }
    }
    inline relation get(int static_id) {
        if (static_id <= 0) return relation::neutral;
        std::lock_guard<std::mutex> lock(g_lock);
        const auto it = g_marks.find(static_id);
        if (it != g_marks.end() && (it->second == relation::friend_player || it->second == relation::enemy_player)) return it->second;
        const auto local_it = g_local_marks.find(static_id);
        if (local_it != g_local_marks.end()) return local_it->second;
        return it == g_marks.end() ? relation::neutral : it->second;
    }
    inline relation persistent_get(int static_id) {
        if (static_id <= 0) return relation::neutral;
        std::lock_guard<std::mutex> lock(g_lock);
        const auto it = g_marks.find(static_id);
        return it == g_marks.end() ? relation::neutral : it->second;
    }

    inline void set(int static_id, relation value) {
        if (static_id <= 0) return;
        std::lock_guard<std::mutex> lock(g_lock);
        if (value == relation::neutral) {
            g_marks.erase(static_id);
        } else {
            g_marks[static_id] = value;
        }
    }

    inline void set_pending(int static_id, relation value) {
        if (static_id <= 0) return;
        std::lock_guard<std::mutex> lock(g_lock);
        g_pending_marks[static_id] = value;
        if (value == relation::neutral) {
            g_marks.erase(static_id);
        } else {
            g_marks[static_id] = value;
        }
    }

    inline void clear_pending_if(int static_id, relation value) {
        if (static_id <= 0) return;
        std::lock_guard<std::mutex> lock(g_lock);
        const auto it = g_pending_marks.find(static_id);
        if (it != g_pending_marks.end() && it->second == value) g_pending_marks.erase(it);
    }

    inline void set_local(int static_id, relation value) {
        if (static_id <= 0) return;
        std::lock_guard<std::mutex> lock(g_lock);
        if (value == relation::neutral) {
            g_local_marks.erase(static_id);
        } else {
            g_local_marks[static_id] = value;
        }
    }

    inline void replace_local(const std::vector<std::pair<int, relation>>& marks) {
        std::lock_guard<std::mutex> lock(g_lock);
        g_local_marks.clear();
        for (const auto& mark : marks) {
            if (mark.first > 0 && (mark.second == relation::family_player || mark.second == relation::fraction_player)) {
                g_local_marks[mark.first] = mark.second;
            }
        }
    }
    inline bool is_friend(int static_id) {
        return get(static_id) == relation::friend_player;
    }

    inline bool is_enemy(int static_id) {
        return get(static_id) == relation::enemy_player;
    }

    inline bool is_family(int static_id) {
        return get(static_id) == relation::family_player;
    }

    inline bool is_fraction(int static_id) {
        return get(static_id) == relation::fraction_player;
    }
}
