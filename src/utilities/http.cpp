#include "http.hpp"

#include "cryptography.hpp"
#include "openapi.hpp"
#include "time.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace utilities::http {

size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string execute_request(
          CURL* curl, 
    const Secret&          secret, 
          std::string_view host, 
          std::string_view path, 
          bool             is_post, 
          std::string_view body_str,
          std::string_view token) {
    if (curl == nullptr) {
        spdlog::error("[Utilities] Passed a null curl pointer to execute_request");
        return "";
    }
    
    curl_easy_setopt(curl, CURLOPT_POST, is_post ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, is_post ? 0L : 1L);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, nullptr);

    std::string timestamp = utilities::time::get_utc_timestamp();
    std::string nonce     = utilities::cryptography::generate_nonce(26uz);
    std::vector<std::pair<std::string, std::string>> query_parameters {};

    std::string signature = utilities::openapi::generate_signature(
        curl, 
        secret.get_key(), 
        secret.get_secret(), 
        nonce, 
        timestamp, 
        host, 
        path, 
        query_parameters, 
        body_str
    );

    std::string url = "https://" + std::string(host) + std::string(path);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
\
    std::string body_std_str{body_str};
    
    if (is_post) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_std_str.c_str());
    }
    
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

    curl_slist* raw_headers = generate_headers(
        nullptr, 
        secret, 
        timestamp, 
        nonce, 
        signature, 
        token
    );
        
    auto header_guard = std::unique_ptr<curl_slist, void(*)(curl_slist*)>(
        raw_headers, [](curl_slist* h) { curl_slist_free_all(h); }
    );

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_guard.get());
    
    std::string response_message { "" };

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_message);

    CURLcode response_code = curl_easy_perform(curl);

    if (response_code == CURLE_OK) {
        long int http_code { 0L };

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (http_code == 200L) {
            spdlog::info("[Utilities] Successfully executed request to {}", path);
            try {
                auto json_response = nlohmann::json::parse(response_message);
                spdlog::info("[Utilities] Response payload:\n{}", json_response.dump(4));
            } catch (const nlohmann::json::parse_error& e) {
                spdlog::warn("[Utilities] Failed to parse JSON response: {}", e.what());
            }
        } else {
            spdlog::error("[Utilities] Request rejected. HTTP {}: {}", http_code, response_message);
        }
    } else {
        spdlog::error("[Utilities] Curl routing execution failed: {}", curl_easy_strerror(response_code));
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, nullptr);

    if (is_post) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, nullptr);
    }

    return response_message;
}

curl_slist* generate_headers(
          curl_slist* raw_headers,
    const Secret&          secret,
          std::string_view timestamp,
          std::string_view nonce,
          std::string_view signature,
          std::string_view token) {
    raw_headers = curl_slist_append(raw_headers, "Accept: application/json");
    raw_headers = curl_slist_append(raw_headers, "Content-Type: application/json"); 
    raw_headers = curl_slist_append(raw_headers, "User-Agent: WebullBot/1.0 (C++23 Client)");
    raw_headers = curl_slist_append(raw_headers, ("x-app-key: " + secret.get_key()).c_str());
    raw_headers = curl_slist_append(raw_headers, ("x-timestamp: " + std::string(timestamp)).c_str());
    raw_headers = curl_slist_append(raw_headers, "x-signature-version: 1.0");
    raw_headers = curl_slist_append(raw_headers, "x-signature-algorithm: HMAC-SHA1");
    raw_headers = curl_slist_append(raw_headers, ("x-signature-nonce: " + std::string(nonce)).c_str());
    
    if (!token.empty()) raw_headers = curl_slist_append(raw_headers, ("x-access-token: " + std::string(token)).c_str());
    raw_headers = curl_slist_append(raw_headers, "x-version: v2");
    raw_headers = curl_slist_append(raw_headers, ("x-signature: " + std::string(signature)).c_str());

    if (raw_headers != nullptr) spdlog::info("[Utilities] Successfully generated HTTP headers");

    return raw_headers;
}

}