#include "curl.hpp"

#include <spdlog/spdlog.h>

Curl::Curl() {
    curl = curl_easy_init();

    if (curl != nullptr) {
        spdlog::info("[Curl] Successfully initialized curl");
    } else {
        spdlog::critical("[Curl] Failed to initialize curl");
    }
}

Curl::~Curl() {
    if (curl != nullptr) {
        curl_easy_cleanup(curl); 
    } 
}