#include "json.hpp"

#include <spdlog/spdlog.h>

#include <fstream>

namespace utilities::json {

nlohmann::json* read(nlohmann::json* json, const std::filesystem::path& input_path) {
    if (json == nullptr) {
        spdlog::error("[Utilities] Failed to open JSON file: {}", input_path.string());
        return nullptr;
    }

    try {
        std::ifstream in(input_path);
        
        if (!in.is_open()) {
            spdlog::error("[Utilities] Failed to open file stream for {}", input_path.string());
            return nullptr;
        }

        in >> *json;
    } catch (const std::exception& e) {
        spdlog::error("[Utilities] Exception reading {}: {}", input_path.string(), e.what());
    }

    return json;
}

void write(const nlohmann::json& json, const std::filesystem::path& output_path) {
    try {
        if (output_path.has_parent_path()) {
            std::filesystem::create_directories(output_path.parent_path());
        }

        std::ofstream out(output_path);

        if (!out.is_open()) {
            spdlog::error("[Utilities] Failed to open file stream for {}", output_path.string());
            return;
        }

        out << std::setw(4) << json;
    } catch (const std::exception& e) {
        spdlog::error("[Utilities] Exception writing {}: {}", output_path.string(), e.what());
    }
}

}