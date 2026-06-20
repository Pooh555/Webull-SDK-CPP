#include "openapi.hpp"

#include "cryptography.hpp"

#include <algorithm>

namespace utilities::openapi {
    
std::string generate_signature(
          CURL* curl,
          std::string_view                                  app_key,
          std::string_view                                  app_secret,
          std::string_view                                  nonce,
          std::string_view                                  timestamp,
          std::string_view                                  host,
          std::string_view                                  request_path,
    const std::vector<std::pair<std::string, std::string>>& query_params,
          std::string_view                                  request_body) {
    
    std::vector<std::pair<std::string, std::string>> parameters {
        {"host", std::string(host)},
        {"x-app-key", std::string(app_key)},
        {"x-signature-algorithm", "HMAC-SHA1"},
        {"x-signature-nonce", std::string(nonce)},
        {"x-signature-version", "1.0"},
        {"x-timestamp", std::string(timestamp)}
    };

    parameters.insert(parameters.end(), query_params.begin(), query_params.end());

    std::sort(parameters.begin(), parameters.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });

    std::string canonical       = "";
    size_t      parameters_size = parameters.size();

    for (size_t i { 0uz }; i < parameters_size; ++i) {
        if (i > 0) {
            canonical += "&";
        }

        canonical += parameters[i].first + "=" + parameters[i].second;
    }

    std::string sign_string = std::string(request_path) + "&" + canonical;

    if (!request_body.empty()) {
        sign_string += "&" + utilities::cryptography::compute_md5(request_body);
    }

    char* escaped = curl_easy_escape(curl, sign_string.c_str(), static_cast<int>(sign_string.size()));
    std::string encoded_sign_string = escaped ? escaped : "";
    
    curl_free(escaped);

    std::string signing_key = std::string(app_secret) + "&";
   
    return utilities::cryptography::compute_hmac_sha1(signing_key, encoded_sign_string);
}

}