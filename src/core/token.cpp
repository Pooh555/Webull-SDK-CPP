#include <core/token.hpp>

#include <utilities/http.hpp>
#include <utilities/json.hpp>

#include <spdlog/spdlog.h>

#include <chrono>
#include <exception>
#include <thread>

namespace wdk::core {

Token::Token(
    const std::filesystem::path& token_path,
          CurlPool&              pool,       
    const Credentials&           credentials,
    const std::string_view&      host) {
    nlohmann::json json_data = wdk::utilities::read(token_path);

    try {
        token_ = json_data.value("token", "");
    } catch (const nlohmann::json::exception& e) {
        spdlog::critical("[Token] Failed to map JSON fields to internal registry: {}", e.what());
    }

    verify(pool, credentials, host);

    if (is_valid() && get_status() == "NORMAL") {
        spdlog::info("[Token] Successfully activated token");
        return;
    } else {
        spdlog::info("[Token] The current token is invalid. Generating new token...");

        generate(pool, credentials, host); 

        while (get_status() == "PENDING") {
            spdlog::info("[Token] Token is PENDING. Please open your Webull Mobile App to approve the login");
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            verify(pool, credentials, host);
        }
    }

    if (get_status() != "NORMAL") {
        spdlog::critical("[Token] Token failed to activate. Status returned: {}", get_status());
        throw std::runtime_error("[Token] Failed to activate token");
    }   

    spdlog::debug("[Token] Saving newly activated token to disk...");

    json_data["token"] = this->token_; 
    
    wdk::utilities::write(json_data, token_path);
    
    spdlog::info("[Token] Successfully activated token");
}

void Token::generate(CurlPool& pool, const Credentials& credentials, const std::string_view& host) {
    std::string response_message = wdk::utilities::execute_request(
        pool, 
        credentials, 
        host, 
        CREATE_PATH, 
        wdk::utilities::HttpMethod::POST).message;
    
    if (!response_message.empty()) {
        try {
            auto json_response = nlohmann::json::parse(response_message);

            if (json_response.contains("token") && !json_response["token"].is_null()) {
                this->token_  = json_response["token"].get<std::string>();
                this->status_ = json_response.value("status", "UNKNOWN");

                spdlog::debug("[Token] Successfully assigned internal token handle.");
            } else {
                spdlog::warn("[Token] JSON payload was successful but missing 'token' key.");
            }
        } catch (const nlohmann::json::exception& e) {
            spdlog::error("[Token] Failed to process JSON response: {}", e.what());
        }
    }
}

void Token::verify(CurlPool& pool, const Credentials& credentials, const std::string_view& host) {
    if (!is_valid()) {
        spdlog::warn("[Token] Token is invalid");
        return;
    }

    nlohmann::json json_payload;
    json_payload["token"] = this->token_; 

    std::string request_body     = json_payload.dump();
    std::string response_message = wdk::utilities::execute_request(
        pool, 
        credentials, 
        host, 
        VERIFY_PATH, 
        wdk::utilities::HttpMethod::POST, 
        request_body).message;

    if (!response_message.empty()) {
        try {
            auto json_response = nlohmann::json::parse(response_message);

            this->status_ = json_response.value("status", this->status_);
            
            spdlog::info("[Token] Verification Response:\n{}", json_response.dump(4));
        } catch (const nlohmann::json::exception& e) {
            spdlog::warn("[Token] Failed to process JSON verification response: {}", e.what());
        }
    }
}

}