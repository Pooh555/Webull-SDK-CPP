#pragma once

#include "secret/secret.hpp"

#include <curl/curl.h>

#include <string>
#include <string_view>

namespace utilities::http {

size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);

[[nodiscard]] std::string execute_request(
          CURL*            curl, 
    const Secret&          secret, 
          std::string_view host, 
          std::string_view path, 
          bool             is_post, 
          std::string_view body_str = "",
          std::string_view token    = "");

curl_slist* generate_headers(
          curl_slist*      raw_headers,
    const Secret&          secret,
          std::string_view timestamp,
          std::string_view nonce,
          std::string_view signature,
          std::string_view token);

}