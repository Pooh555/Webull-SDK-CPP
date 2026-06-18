#pragma once

#include <filesystem>
#include <string>

class Secret {
public:
    Secret(const std::filesystem::path& secret_path);
    ~Secret() = default;

    Secret(const Secret&)            = delete;
    Secret& operator=(const Secret&) = delete;
    
    [[nodiscard]] std::string get_id()     const { return id; }
    [[nodiscard]] std::string get_key()    const { return key; }
    [[nodiscard]] std::string get_secret() const { return secret; }
private:
    std::string id     = "";
    std::string key    = "";
    std::string secret = "";
};