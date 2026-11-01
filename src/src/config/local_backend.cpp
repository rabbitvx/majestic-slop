#include "config/local_backend.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

#include "platform/hello_paths.hpp"

namespace config::local_backend {
    namespace {
        std::string config_path(const std::string& name) {
            return (hello_paths::local_root() / (name + ".json")).string();
        }
    }

    list_result list_configs() {
        list_result result;
        try {
            hello_paths::migrate_legacy_root();
            const auto root = hello_paths::local_root();
            if (!std::filesystem::exists(root)) {
                result.ok = true;
                result.status_message = "0 configs";
                return result;
            }
            std::vector<std::string> names;
            for (const auto& entry : std::filesystem::directory_iterator(root)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    names.push_back(entry.path().stem().string());
                }
            }
            std::sort(names.begin(), names.end());
            for (const auto& name : names) {
                result.configs[name] = config_path(name);
            }
            result.ok = true;
            result.status_message = std::to_string(names.size()) + " configs";
        }
        catch (...) {
            result.status_message = "failed to list configs";
        }
        return result;
    }

    op_result save_config(const std::string& name, const std::string& data) {
        op_result result;
        try {
            hello_paths::migrate_legacy_root();
            if (!std::filesystem::exists(hello_paths::local_root())) {
                result.status_message = "create " + hello_paths::local_root_string() + " first";
                return result;
            }
            std::ofstream file(config_path(name));
            if (!file.is_open()) {
                result.status_message = "failed to open file";
                return result;
            }
            file << data;
            result.ok = true;
            result.status_message = "saved: " + name;
        }
        catch (...) {
            result.status_message = "save error";
        }
        return result;
    }

    data_result load_config(const std::string& name) {
        data_result result;
        try {
            hello_paths::migrate_legacy_root();
            std::ifstream file(config_path(name));
            if (!file.is_open()) {
                result.status_message = "file not found";
                return result;
            }
            result.data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
            result.ok = true;
            result.status_message = "loaded: " + name;
        }
        catch (...) {
            result.status_message = "load error";
        }
        return result;
    }

    op_result delete_config(const std::string& name) {
        op_result result;
        try {
            hello_paths::migrate_legacy_root();
            std::filesystem::remove(config_path(name));
            result.ok = true;
            result.status_message = "deleted: " + name;
        }
        catch (...) {
            result.status_message = "delete error";
        }
        return result;
    }
}
