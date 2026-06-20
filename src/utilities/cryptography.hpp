#pragma once

#include <string>
#include <string_view>

namespace utilities::cryptography {

[[nodiscard]] std::string compute_hmac_sha1(std::string_view key, std::string_view message);
[[nodiscard]] std::string compute_hmac_sha256(std::string_view key, std::string_view message);
[[nodiscard]] std::string compute_md5(std::string_view data);
[[nodiscard]] std::string generate_nonce(size_t length = 26uz);
 
}