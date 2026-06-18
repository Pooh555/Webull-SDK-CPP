#include "token.hpp"

#include "utilities/utilities.hpp"

#include <spdlog/spdlog.h>

#include <memory>

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void Token::generate(CURL* curl, const Secret& secret) {
    if (curl == nullptr) {
        spdlog::error("[Token] Passed a null curl pointer to generate()");
        return;
    }

    std::string timestamp = utilities::get_utc_timestamp();
    std::string nonce     = utilities::generate_nonce(26uz);
    std::string signature = utilities::generate_openapi_signature(
        curl, 
        secret.get_key(), 
        secret.get_secret(), 
        nonce, 
        timestamp, 
        CREATE_PATH, 
        {}, 
        {});

    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_URL, (static_cast<std::string>(HOST) + static_cast<std::string>(CREATE_PATH)).c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

    curl_slist* raw_headers = nullptr;
    raw_headers = curl_slist_append(raw_headers, "Accept: application/json");
    raw_headers = curl_slist_append(raw_headers, ("x-app-key: " + secret.get_key()).c_str());
    raw_headers = curl_slist_append(raw_headers, ("x-timestamp: " + timestamp).c_str());
    raw_headers = curl_slist_append(raw_headers, "x-signature-version: 1.0");
    raw_headers = curl_slist_append(raw_headers, "x-signature-algorithm: HMAC-SHA1");
    raw_headers = curl_slist_append(raw_headers, ("x-signature-nonce: " + nonce).c_str());
    raw_headers = curl_slist_append(raw_headers, "x-version: v2");
    raw_headers = curl_slist_append(raw_headers, ("x-signature: " + signature).c_str());

    spdlog::debug("[Token] timestamp: {}", timestamp);
    spdlog::debug("[Token] nonce: {}", nonce);
    spdlog::debug("[Token] signature: {}", signature);

    auto header_guard = std::unique_ptr<curl_slist, void(*)(curl_slist*)>(
        raw_headers, 
        [](curl_slist* h) { curl_slist_free_all(h); }
    );

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_guard.get());

    std::string response_message;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_message);

    CURLcode response_code = curl_easy_perform(curl);

    if (response_code == CURLE_OK) {
        long int http_code { 0L };

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
       if (http_code == 200L) {
            spdlog::info("[Token] Successfully generated a token");
            
            try {
                auto json_response = nlohmann::json::parse(response_message);

                spdlog::debug("[Token] Response:\n{}", json_response.dump(4));

                if (json_response.contains("token") && !json_response["token"].is_null()) {
                    this->token = json_response["token"].get<std::string>();
                    spdlog::debug("[Token] Successfully assigned internal token handle.");
                } else {
                    spdlog::warn("[Token] JSON payload was successful but missing 'token' key.");
                }
            } catch (const nlohmann::json::parse_error& e) {
                spdlog::error("[Token] Failed to parse JSON response: {}", e.what());
                spdlog::info("[Token] Raw Response: {}", response_message);
            }
        } else {
            spdlog::error("[Token] API rejected request. HTTP {}: {}", http_code, response_message);
        }
    } else {
        spdlog::error("[Token] Curl request failed: {}", curl_easy_strerror(response_code));
    }
}

void Token::verify(CURL* curl, const Secret& secret) {
    if (token == "") {
        spdlog::warn("[Token] Token is null");
        return;
    }
    if (curl == nullptr) {
        spdlog::error("[Token] Passed a null curl pointer to verify()");
        return;
    }

    std::string timestamp = utilities::get_utc_timestamp();
    std::string nonce     = utilities::generate_nonce(26uz);
   
    nlohmann::json json_payload;
    json_payload["token"] = this->token; 

    std::string request_body = json_payload.dump(); 
    std::string signature = utilities::generate_openapi_signature(
        curl, 
        secret.get_key(), 
        secret.get_secret(), 
        nonce, 
        timestamp, 
        VERIFY_PATH, 
        {}, 
        request_body);

    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_URL, (static_cast<std::string>(HOST) + static_cast<std::string>(VERIFY_PATH)).c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

    curl_slist* raw_headers = nullptr;
    raw_headers = curl_slist_append(raw_headers, "Content-Type: application/json");
    raw_headers = curl_slist_append(raw_headers, "Accept: application/json");
    raw_headers = curl_slist_append(raw_headers, ("x-app-key: " + secret.get_key()).c_str());
    raw_headers = curl_slist_append(raw_headers, ("x-timestamp: " + timestamp).c_str());
    raw_headers = curl_slist_append(raw_headers, "x-signature-version: 1.0");
    raw_headers = curl_slist_append(raw_headers, "x-signature-algorithm: HMAC-SHA1");
    raw_headers = curl_slist_append(raw_headers, ("x-signature-nonce: " + nonce).c_str());
    raw_headers = curl_slist_append(raw_headers, "x-version: v2");
    raw_headers = curl_slist_append(raw_headers, ("x-signature: " + signature).c_str());

    spdlog::debug("[Token] timestamp: {}", timestamp);
    spdlog::debug("[Token] nonce: {}", nonce);
    spdlog::debug("[Token] signature: {}", signature);

    auto header_guard = std::unique_ptr<curl_slist, void(*)(curl_slist*)>(
        raw_headers, 
        [](curl_slist* h) { curl_slist_free_all(h); }
    );

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_guard.get());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());

    std::string response_message = "";

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_message);

    CURLcode response_code = curl_easy_perform(curl);

    if (response_code == CURLE_OK) {
        long int http_code { 0L };

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (http_code == 200L) {
            spdlog::info("[Token] Successfully verified token");
            
            try {
                auto json_response = nlohmann::json::parse(response_message);
                spdlog::info("[Token] Verification Response:\n{}", json_response.dump(4));
            } catch (const nlohmann::json::parse_error& e) {
                spdlog::warn("[Token] Failed to parse JSON verification response: {}", e.what());
                spdlog::info("[Token] Raw Response: {}", response_message);
            }
        } else {
            spdlog::error("[Token] API rejected verification request. HTTP {}: {}", http_code, response_message);
        }
    } else {
        spdlog::error("[Token] Curl verification request failed: {}", curl_easy_strerror(response_code));
    }
}