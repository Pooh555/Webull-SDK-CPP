#pragma once

#include "secret/secret.hpp"

#include <curl/curl.h>

#include <string>
#include <string_view>

class Token {
public:
    Token() = default;
    
    void generate(CURL* curl, const Secret& secret);
    void verify(CURL* curl, const Secret& secret);

    [[nodiscard]] std::string get_handle() const { return token; }
    [[nodiscard]] bool is_valid() const { return !token.empty(); }
private:
    static constexpr std::string_view HOST        = "th-api.uat.webullbroker.com";
    static constexpr std::string_view CREATE_PATH = "/openapi/auth/token/create";
    static constexpr std::string_view VERIFY_PATH = "/openapi/auth/token/check";

    std::string token = "";
};