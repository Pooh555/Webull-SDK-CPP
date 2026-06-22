#include <utilities/http.hpp>

#include <utilities/cryptography.hpp>
#include <utilities/openapi.hpp>
#include <utilities/time.hpp>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace wdk::utilities {

size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    static_cast<std::string*>(userp)->append(static_cast<const char*>(contents), size * nmemb);
    return size * nmemb;
}

Response execute_request(
          wdk::core::CurlPool&    pool,
    const wdk::core::Credentials& credentials,
          std::string_view        host,
          std::string_view        path,
          HttpMethod              method,
          std::string_view        body_str,
          std::string_view        token) {
    static constexpr size_t MAX_RETRIES = 3uz;

    size_t attempt = 0uz;

    while (attempt <= MAX_RETRIES) {
        wdk::core::CurlPool::CurlHandle curl_guard = pool.acquire();
        CURL* curl       = curl_guard.get();

        if (curl == nullptr) {
            spdlog::error("[Utilities] Failed to acquire a valid CURL handle from pool");
            return { CURLE_FAILED_INIT, R"({"error":"curl is nullptr"})" };
        }

        curl_easy_setopt(curl, CURLOPT_POST, method == HttpMethod::POST ? 1L : 0L);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, method == HttpMethod::POST ? 0L : 1L);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, nullptr);

        const std::string timestamp = get_utc_timestamp();
        const std::string nonce     = generate_nonce();

        std::string                                      request_path { path };
        std::vector<std::pair<std::string, std::string>> query_parameters {};

        if (const size_t question_mark = request_path.find('?');
            question_mark != std::string::npos) {

            std::string query_string = request_path.substr(question_mark + 1);
            request_path.erase(question_mark);

            size_t current_position { 0uz };

            while (current_position < query_string.size()) {
                const size_t ampersand =
                    query_string.find('&', current_position);

                const std::string parameter = query_string.substr(
                    current_position, 
                    ampersand == std::string::npos
                        ? std::string::npos
                        : ampersand - current_position
                    );

                if (const size_t equals = parameter.find('=');
                    equals != std::string::npos) {

                    query_parameters.emplace_back( 
                        parameter.substr(0, equals),
                        parameter.substr(equals + 1)
                    );
                }

                if (ampersand == std::string::npos) break;
                current_position = ampersand + 1;
            }
        }

        const std::string signature = generate_signature(
            curl,
            credentials.get_key(),
            credentials.get_secret(),
            nonce,
            timestamp,
            host,
            request_path,
            query_parameters,
            body_str
        );

        const std::string url = "https://" + std::string(host) + std::string(path);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        std::string body_std_str { body_str };
        if (method == HttpMethod::POST) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_std_str.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

        curl_slist* raw_headers = generate_headers(
                credentials,
                timestamp,
                nonce,
                signature,
                token
            );

        auto header_guard = std::unique_ptr<curl_slist, void(*)(curl_slist*)>(
                raw_headers,
                [](curl_slist* h) {
                    curl_slist_free_all(h);
                }
            );

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_guard.get());

        std::string response_message {};

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_message);

        CURLcode response_code = curl_easy_perform(curl);
        long     http_code { 0L };

        if (response_code == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

            if (http_code == 429L && attempt < MAX_RETRIES) {
                spdlog::warn("[Utilities] Rate limit hit (429) on {}. Initiating backoff attempt {}", path, attempt + 1uz);
                std::this_thread::sleep_for(std::chrono::milliseconds(500uz * (1uz << attempt)));
                
                ++attempt;
                
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, nullptr);
                
                if (method == HttpMethod::POST) {
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, nullptr);
                }
                
                continue; 
            }

            if (http_code == 200L) {
                spdlog::debug("[Utilities] Successfully executed request to {}", path);

                try {
                    auto json_response = nlohmann::json::parse(response_message);
                    spdlog::debug("[Utilities] Response payload:\n{}", json_response.dump(4));
                } catch (const nlohmann::json::parse_error& e) {
                    spdlog::warn("[Utilities] Failed to parse JSON response: {}", e.what());
                }
            } else {
                try {
                    auto json_error = nlohmann::json::parse(response_message);
                    spdlog::error("[Utilities] Request rejected. HTTP {}:\n{}", http_code, json_error.dump(4));
                } catch (const nlohmann::json::parse_error&) {
                    spdlog::error("[Utilities] Request rejected. HTTP {}: {}", http_code, response_message);
                }
            }
        } else {
            spdlog::error("[Utilities] Curl routing execution failed: {}", curl_easy_strerror(response_code));
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, nullptr);

        if (method == HttpMethod::POST) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, nullptr);
        }

        return { http_code, std::move(response_message) };
    }

    return { 429L, R"({"error": "Max retries exceeded due to rate limiting"})" };
}

curl_slist* generate_headers(
    const wdk::core::Credentials& credentials,
          std::string_view        timestamp,
          std::string_view        nonce,
          std::string_view        signature,
          std::string_view        token) {
    curl_slist* raw_headers = nullptr;
    
    raw_headers = curl_slist_append(raw_headers, "Accept: application/json");
    raw_headers = curl_slist_append(raw_headers, "Content-Type: application/json"); 
    raw_headers = curl_slist_append(raw_headers, "User-Agent: WebullBot/1.0 (C++23 Client)");
    raw_headers = curl_slist_append(raw_headers, std::format("x-app-key: {}", credentials.get_key()).c_str());
    raw_headers = curl_slist_append(raw_headers, std::format("x-timestamp: {}", timestamp).c_str());
    raw_headers = curl_slist_append(raw_headers, "x-signature-version: 1.0");
    raw_headers = curl_slist_append(raw_headers, "x-signature-algorithm: HMAC-SHA1");
    raw_headers = curl_slist_append(raw_headers, std::format("x-signature-nonce: {}", nonce).c_str());
    
    if (!token.empty()) {
        raw_headers = curl_slist_append(raw_headers, std::format("x-access-token: {}", token).c_str());
    }
    
    raw_headers = curl_slist_append(raw_headers, "x-version: v2");
    raw_headers = curl_slist_append(raw_headers, std::format("x-signature: {}", signature).c_str());

    if (raw_headers != nullptr) {
        spdlog::debug("[Utilities] Successfully generated HTTP headers");
    }

    return raw_headers;
}

}