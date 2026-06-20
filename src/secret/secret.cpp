#include "secret.hpp"

#include "utilities/json.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

Secret::Secret(const std::filesystem::path& secret_path) {
    nlohmann::json json_data;
    utilities::json::read(&json_data, secret_path);

    try {
        id     = json_data.value("id", "");
        key    = json_data.value("key", "");
        secret = json_data.value("secret", ""); 
    } catch (const std::exception& e) {
        spdlog::critical("[Secret] Failed map JSON fields to internal registry: {}", e.what());
    }
}