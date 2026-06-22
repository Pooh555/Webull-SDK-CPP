#pragma once

#include <client/market.hpp>
#include <core/credentials.hpp>
#include <core/curl_pool.hpp>
#include <core/token.hpp>

#include <memory>

class Application {
public:
    Application();
    ~Application() = default;

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    Application(Application&&)            = delete;
    Application& operator=(Application&&) = delete;

    void run();
private:
    // static constexpr std::string_view HOST        = "th-api.uat.webullbroker.com"; // Test endpoint
    static constexpr std::string_view HOST             { "api.webull.co.th" };            // Production endpoint
    static constexpr std::string_view TOKEN_PATH       { "/home/Pooh555/programming/Webull-SDK/examples/res/token.json" };
    static constexpr std::string_view CREDENTIALS_PATH { "/home/Pooh555/programming/Webull-SDK/examples/res/credentials.json" };

    std::unique_ptr<wdk::core::CurlPool>    curl_pool;
    std::unique_ptr<wdk::core::Credentials> credentials;
    std::unique_ptr<wdk::core::Token>       token;

    void demo();
};