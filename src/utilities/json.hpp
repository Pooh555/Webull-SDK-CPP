#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>

namespace utilities::json {
    nlohmann::json* read(nlohmann::json* json, const std::filesystem::path& input_path);
    void            write(const nlohmann::json& json, const std::filesystem::path& output_path);
}