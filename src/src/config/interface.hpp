#pragma once

#include <functional>
#include <map>
#include <string>

namespace config {
    struct snapshot {
        std::map<std::string, std::string> configs;
        std::string active_config_name;
        std::string status_message;
    };

    float get(std::string category, std::string key, float defaultVal);
    int get(std::string category, std::string key, int defaultVal);
    std::string get(std::string category, std::string key, std::string defaultVal);
    std::map<std::string, std::string> get(std::string category, std::string key, std::map<std::string, std::string> defaultValue);
    std::map<unsigned long, std::string> get(std::string category, std::string key, std::map<unsigned long, std::string> defaultValue);

    bool update(float newValue, std::string category, std::string key, float defaultVal);
    bool update(int newValue, std::string category, std::string key, int defaultVal);
    bool update(std::string newValue, std::string category, std::string key, std::string defaultVal);
    bool update(std::map<std::string, std::string> newValue, std::string category, std::string key, std::map<std::string, std::string> defaultValue);
    bool update(std::map<unsigned long, std::string> newValue, std::string category, std::string key, std::map<unsigned long, std::string> defaultValue);
    bool set(std::string category, std::string name, std::string setting);

    bool list_configs();
    bool load_from_file(const std::string& name);
    bool save_to_file(const std::string& name);
    bool delete_config(const std::string& name);
    void new_config(std::string config_name);
    void flush_to_disk();
    void set_runtime_hooks(std::function<void()> before_serialize, std::function<void()> after_load);

    snapshot current_snapshot();
    void set_status_message(std::string message);
}
