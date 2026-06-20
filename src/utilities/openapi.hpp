#pragma once

#include <curl/curl.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace utilities::openapi {

[[nodiscard]] std::string generate_signature(
          CURL*                                             curl,
          std::string_view                                  app_key,
          std::string_view                                  app_secret,
          std::string_view                                  nonce,
          std::string_view                                  timestamp,
          std::string_view                                  host,
          std::string_view                                  request_path,
    const std::vector<std::pair<std::string, std::string>>& query_params,
          std::string_view                                  request_body);
}