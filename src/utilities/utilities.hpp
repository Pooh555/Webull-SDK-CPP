#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>

namespace utilities {
    
void read_json(nlohmann::json* json, const std::filesystem::path& input_path);
void write_json(const nlohmann::json& json, const std::filesystem::path& output_path);

[[nodiscard]] std::string get_utc_timestamp();
[[nodiscard]] std::string generate_nonce(size_t length = 0uz);
[[nodiscard]] std::string compute_hmac_sha1(const std::string& key, const std::string& message);
[[nodiscard]] std::string compute_hmac_sha256(const std::string& key, const std::string& message);
[[nodiscard]] std::string compute_md5(const std::string& data);
}