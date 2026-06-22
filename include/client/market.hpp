#pragma once

#include "core/curl_pool.hpp"
#include "core/thread_pool.hpp"
#include "core/credentials.hpp"
#include "core/token.hpp"
#include "utilities/http.hpp"

#include <future>
#include <string>
#include <string_view>
#include <vector>

namespace wdk::client {

struct MarketRequest {
    std::string           symbol          { "" };
    std::string           category        { "" };
    std::optional<size_t> count;
    std::string           trading_session { "" };
};

struct TickData {
    std::string symbol          { "" };
    std::string instrument_id   { "" };
    // std::string price           { "" };
    // std::string open            { "" };
    // std::string high            { "" };
    // std::string low             { "" };
    std::string volume          { "" };
    std::string side            { "" };
    // std::string change          { "" };
    // std::string change_ratio    { "" };
    // std::string pre_close       { "" };
    // std::string last_trade_time { "" };
    std::string trading_session { "" };
};

class MarketClient {
public:
    MarketClient(
              wdk::core::CurlPool&    pool,
              wdk::core::ThreadPool&  thread_pool,
        const wdk::core::Credentials& credentials, 
              std::string_view        host  = "", 
              std::string_view        token = "");
    ~MarketClient() = default;

    MarketClient(const MarketClient&)            = delete;
    MarketClient& operator=(const MarketClient&) = delete;

    [[nodiscard]] wdk::utilities::Response fetch_tick_data(const MarketRequest& request);

    [[nodiscard]] std::future<wdk::utilities::Response> fetch_tick_data_async(const MarketRequest& request);
private:
    static constexpr std::string_view TICK_PATH = "/openapi/market-data/stock/tick";

          wdk::core::CurlPool&    pool_;  
          wdk::core::ThreadPool&  thread_pool_;
    const wdk::core::Credentials& credentials_;
          std::string             host_         { "" };
          std::string             token_        { "" };

    [[nodiscard]] std::future<wdk::utilities::Response> execute_request_async(
        std::string                path,
        wdk::utilities::HttpMethod method,
        std::string                body_str = "");
};

}