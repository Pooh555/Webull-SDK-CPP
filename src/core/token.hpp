#pragma once

#include "secret/secret.hpp"

#include <curl/curl.h>

#include <vector>
#include <utility>

class Token {
public:
    Token() = default;
    ~Token() = default;

    Token(const Token&)             = delete;
    Token& operator=(const Token&) = delete;

    void generate(CURL* curl, const Secret& secret);
    void verify(CURL* curl, const Secret& secret);

    [[nodiscard]] std::string get_handle() const { return token; }
private:
    static constexpr std::string_view HOST                = "th-api.uat.webullbroker.com";
    static constexpr std::string_view CREATE_PATH         = "/openapi/auth/token/create";
    static constexpr std::string_view VERIFY_PATH         = "/openapi/auth/token/check";
    static constexpr std::string_view SIGNATURE_ALGORITHM = "HMAC-SHA1";
    static constexpr std::string_view SIGNATURE_VERSION   = "1.0";

    std::string token = "";

void generate_signature(
          CURL*            curl,
    const Secret&          secret,
    const std::string&     nonce,
    const std::string&     timestamp,
          std::string_view request_path,
    const std::string&     request_body, 
            std::string&   signature);
};