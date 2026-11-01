#pragma once

#include <map>
#include <string>

namespace config::local_backend {
    struct list_result {
        bool ok = false;
        std::map<std::string, std::string> configs;
        std::string status_message;
    };

    struct data_result {
        bool ok = false;
        std::string data;
        std::string status_message;
    };

    struct op_result {
        bool ok = false;
        std::string status_message;
    };

    list_result list_configs();
    op_result save_config(const std::string& name, const std::string& data);
    data_result load_config(const std::string& name);
    op_result delete_config(const std::string& name);
}
