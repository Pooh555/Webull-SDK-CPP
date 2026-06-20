#include "token.hpp"

#include "utilities/http.hpp"
#include "utilities/json.hpp"

#include <spdlog/spdlog.h>

Token::Token(
    const std::filesystem::path& token_path,
          CURL*                  curl,
    const Secret&                secret,
    const std::string_view&      host) {
    nlohmann::json json_data;
    utilities::json::read(&json_data, token_path);

    try {
        token = json_data.value("token", "");
    } catch (const std::exception& e) {
        spdlog::critical("[Token] Failed map JSON fields to internal registry: {}", e.what());
    }

    verify(curl, secret, host);

    if (!is_valid() || get_status() != "NORMAL") {
        spdlog::info("[Token] The current token is invalid. Generating new token...");

        generate(curl, secret, host);

        while (get_status() == "PENDING") {
            spdlog::info("[Token] Token is PENDING. Please open your Webull Mobile App to approve the login");
            
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            verify(curl, secret, host);
        }
    }

    if (get_status() != "NORMAL") {
        spdlog::error("[Token] Token failed to activate. Status returned: {}", get_status());
        return;
    }   

    spdlog::info("[Token] Saving newly activated token to disk...");
    
    json_data["token"] = this->token; 
    utilities::json::write(json_data, token_path);

    spdlog::info("[Token] Successfully activated token");
}

void Token::generate(CURL* curl, const Secret& secret, const std::string_view& host) {
    std::string response_message = utilities::http::execute_request(curl, secret, host, CREATE_PATH, true);

    if (!response_message.empty()) {
        try {
            auto json_response = nlohmann::json::parse(response_message);

            if (json_response.contains("token") && !json_response["token"].is_null()) {
                this->token  = json_response["token"].get<std::string>();
                this->status = json_response.value("status", "UNKNOWN");
                
                spdlog::debug("[Token] Successfully assigned internal token handle.");
            } else {
                spdlog::warn("[Token] JSON payload was successful but missing 'token' key.");
            }
        } catch (const nlohmann::json::parse_error& e) {
            spdlog::error("[Token] Failed to parse JSON response: {}", e.what());
            spdlog::info("[Token] Raw Response: {}", response_message);
        }
    }
}

void Token::verify(CURL* curl, const Secret& secret, const std::string_view& host) {
    if (!is_valid()) {
        spdlog::warn("[Token] Token is invalid");
        return;
    }

    nlohmann::json json_payload;
    json_payload["token"] = this->token; 
    std::string request_body = json_payload.dump(); 

    std::string response_message = utilities::http::execute_request(curl, secret, host, VERIFY_PATH, true, request_body);

    if (!response_message.empty()) {
        try {
            auto json_response = nlohmann::json::parse(response_message);
            this->status = json_response.value("status", this->status);
            
            spdlog::info("[Token] Verification Response:\n{}", json_response.dump(4));
        } catch (const nlohmann::json::parse_error& e) {
            spdlog::warn("[Token] Failed to parse JSON verification response: {}", e.what());
            spdlog::info("[Token] Raw Response: {}", response_message);
        }
    }
}