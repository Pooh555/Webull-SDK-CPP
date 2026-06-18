#include "utilities.hpp"

#include <spdlog/spdlog.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <cctype>
#include <chrono>
#include <format>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <sstream>

namespace utilities {

void read_json(nlohmann::json* json, const std::filesystem::path& input_path) {
    if (json == nullptr) {
        spdlog::error("[Utilities] Failed to open JSON file: {}", input_path.string());
        return;
    }
    try {
        std::ifstream in(input_path);
        
        if (!in.is_open()) {
            spdlog::error("[Utilities] Failed to open file stream for {}", input_path.string());
            return;
        }

        in >> *json;
    } catch (const std::exception& e) {
        spdlog::error("[Utilities] Exception reading {}: {}", input_path.string(), e.what());
    }
}

void write_json(const nlohmann::json& json, const std::filesystem::path& output_path) {
    try {
        if (output_path.has_parent_path()) {
            std::filesystem::create_directories(output_path.parent_path());
        }

        std::ofstream out(output_path);

        if (!out.is_open()) {
            spdlog::error("[Utilities] Failed to open file stream for {}", output_path.string());
            return;
        }

        out << std::setw(4) << json;
    } catch (const std::exception& e) {
        spdlog::error("[Utilities] Exception writing {}: {}", output_path.string(), e.what());
    }
}

std::string get_utc_timestamp() {
    std::chrono::utc_clock::time_point current_time = std::chrono::utc_clock::now();
    return std::format("{:%Y-%m-%dT%H:%M:%SZ}", std::chrono::floor<std::chrono::seconds>(current_time));
}

std::string generate_nonce(size_t length) {
    static constexpr std::string_view digits = "0123456789";
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, digits.size() - 1);

    std::string nonce;

    nonce.reserve(length);

    for (size_t i { 0uz }; i < length; ++i) {
        nonce += digits[dist(gen)];
    }

    return nonce;
}

std::string compute_hmac_sha1(
    const std::string& key,
    const std::string& message) {
    u_char   hash[EVP_MAX_MD_SIZE];
    uint32_t hash_length = 0;

    HMAC(
        EVP_sha1(),
        key.data(),
        static_cast<int>(key.size()),
        reinterpret_cast<const u_char*>(message.data()),
        message.size(),
        hash,
        &hash_length
    );

    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new(BIO_s_mem());

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    BIO* bio = BIO_push(b64, mem);

    BIO_write(bio, hash, static_cast<int>(hash_length));
    BIO_flush(bio);

    BUF_MEM* buffer = nullptr;
    BIO_get_mem_ptr(bio, &buffer);

    std::string result(buffer->data, buffer->length);

    BIO_free_all(bio);

    return result;
}

std::string compute_hmac_sha256(const std::string& key, const std::string& message) {
    std::string key_with_amp { key + "&" };
    u_char      hash[EVP_MAX_MD_SIZE];
    uint32_t    hash_length { 0u };

    HMAC(EVP_sha256(),
         key_with_amp.data(), static_cast<int>(key_with_amp.size()), 
         reinterpret_cast<const u_char*>(message.data()), message.size(), 
         hash, &hash_length);
         
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new(BIO_s_mem());

    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); 
    BIO_write(bio, hash, static_cast<int>(hash_length));
    
    BIO_flush(bio);

    BUF_MEM* buffer_ptr;
    BIO_get_mem_ptr(bio, &buffer_ptr);

    std::string base64_signature(buffer_ptr->data, buffer_ptr->length)
    ;
    BIO_free_all(bio);

    return base64_signature;
}

std::string compute_md5(const std::string& data) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_MD_fetch(nullptr, "MD5", nullptr);
    
    u_char   hash[EVP_MAX_MD_SIZE];
    uint32_t length { 0u };

    EVP_DigestInit_ex(context, md, nullptr);
    EVP_DigestUpdate(context, data.c_str(), data.size());
    EVP_DigestFinal_ex(context, hash, &length);
    EVP_MD_CTX_free(context);

    std::stringstream ss;
    for (uint32_t i { 0u }; i < length; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    std::string md5_str = ss.str();

    for (char& c : md5_str) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    
    return md5_str;
}

std::string generate_openapi_signature(
          CURL*                                             curl,
    const std::string&                                      app_key,
    const std::string&                                      app_secret,
    const std::string&                                      nonce,
    const std::string&                                      timestamp,
          std::string_view                                  request_path,
    const std::vector<std::pair<std::string, std::string>>& query_params,
    const std::string&                                      request_body) {
    std::vector<std::pair<std::string, std::string>> parameters {
        {"host", "th-api.uat.webullbroker.com"},
        {"x-app-key", app_key},
        {"x-signature-algorithm", "HMAC-SHA1"},
        {"x-signature-nonce", nonce},
        {"x-signature-version", "1.0"},
        {"x-timestamp", timestamp}
    };

    parameters.insert(parameters.end(), query_params.begin(), query_params.end());

    std::sort(parameters.begin(), parameters.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });

    std::string canonical       = "";
    size_t      parameters_size = parameters.size();

    for (size_t i { 0 }; i < parameters_size; ++i) {
        if (i > 0) {
            canonical += "&";
        }

        canonical += parameters[i].first + "=" + parameters[i].second;
    }

    std::string sign_string = std::string(request_path) + "&" + canonical;
    
    if (!request_body.empty()) {
        sign_string += "&" + utilities::compute_md5(request_body);
    }

    char*       escaped = curl_easy_escape(curl, sign_string.c_str(), static_cast<int>(sign_string.size()));
    std::string encoded_sign_string = escaped ? escaped : "";
    
    curl_free(escaped);

    std::string signing_key = app_secret + "&";
   
    return compute_hmac_sha1(signing_key, encoded_sign_string);
}

}