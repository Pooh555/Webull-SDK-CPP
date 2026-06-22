#pragma once

#include "core/curl_pool.hpp"
#include "core/thread_pool.hpp"
#include "core/credentials.hpp"
#include "utilities/http.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace wdk::client {

struct OrderRequest {
    std::string           account_id              { "" };
    std::string           combo_type              { "" };
    std::string           client_order_id         { "" };
    std::string           instrument_type         { "" };
    std::string           market                  { "" };
    std::string           symbol                  { "" };
    std::string           order_type              { "" };
    std::string           entrust_type            { "" };
    std::string           trading_session         { "" };
    std::string           time_in_force           { "" };
    std::string           side                    { "" };
    std::optional<double> quantity                { std::nullopt };
    std::optional<double> limit_price             { std::nullopt };
    std::optional<double> stop_price              { std::nullopt };  
};

class TradingClient {
public:
    TradingClient(
              wdk::core::CurlPool&    pool,
              wdk::core::ThreadPool&  thread_pool,
        const wdk::core::Credentials& credentials, 
              std::string_view        host, 
              std::string_view        token);
    ~TradingClient() = default;

    wdk::utilities::Response preview_order(const OrderRequest& request);
    wdk::utilities::Response place_order(const OrderRequest& request);
    wdk::utilities::Response modify_order(const OrderRequest& request);
    wdk::utilities::Response cancel_order(const OrderRequest& request);
    
    std::future<wdk::utilities::Response> preview_order_async(const OrderRequest& request);
    std::future<wdk::utilities::Response> place_order_async(const OrderRequest& request);
    std::future<wdk::utilities::Response> modify_order_async(const OrderRequest& request);
    std::future<wdk::utilities::Response> cancel_order_async(const OrderRequest& request);

    [[nodiscard]] std::string              get_account_id();
    [[nodiscard]] wdk::utilities::Response fetch_account_list();
    [[nodiscard]] wdk::utilities::Response fetch_account_balance(const std::string& account_id);
    [[nodiscard]] wdk::utilities::Response fetch_account_position(const std::string& account_id);

    [[nodiscard]] std::future<wdk::utilities::Response> fetch_account_list_async();
    [[nodiscard]] std::future<wdk::utilities::Response> fetch_account_balance_async(const std::string& account_id);
    [[nodiscard]] std::future<wdk::utilities::Response> fetch_account_position_async(const std::string& account_id);
private:
    static constexpr std::string_view ACCOUNT_LIST_PATH     { "/openapi/account/list" };
    static constexpr std::string_view ACCOUNT_BALANCE_PATH  { "/openapi/assets/balance" };
    static constexpr std::string_view ACCOUNT_POSITION_PATH { "/openapi/assets/positions" };
    static constexpr std::string_view PREVIEW_ORDER_PATH    { "/openapi/trade/order/preview" };
    static constexpr std::string_view PLACE_ORDER_PATH      { "/openapi/trade/order/place" };
    static constexpr std::string_view MODIFY_ORDER_PATH     { "/openapi/trade/order/replace" };
    static constexpr std::string_view CANCEL_ORDER_PATH     { "/openapi/trade/order/cancel" };
    
          std::string             account_id_  { "" };
          wdk::core::CurlPool&    pool_;  
          wdk::core::ThreadPool&  thread_pool_;
    const wdk::core::Credentials& credentials_;
          std::string             host_        { "" };
          std::string             token_       { "" };

    [[nodiscard]] std::future<wdk::utilities::Response> execute_request_async(
        std::string                path,
        wdk::utilities::HttpMethod method,
        std::string                body_str = "");
};

}