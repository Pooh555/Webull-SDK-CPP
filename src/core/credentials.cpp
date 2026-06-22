#include <core/credentials.hpp>

#include <utilities/json.hpp>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <stdexcept>

namespace wdk::core {

Credentials::Credentials(const std::filesystem::path& credentials_path) {
    nlohmann::json json_data = wdk::utilities::read(credentials_path);

    try {
        id_     = json_data.value("id", "");
        key_    = json_data.value("key", "");
        secret_ = json_data.value("secret", ""); 
    } catch (const std::exception& e) {
        spdlog::critical("[Credentials] Failed map JSON fields to internal registry: {}", e.what());
        throw std::runtime_error("[Credentials] Failed to initialize credentials");
    }
    
    spdlog::info("[Credentials] Successfully initialized app credentials");
}

}