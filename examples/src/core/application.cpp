#include "application.hpp"

#include <client/trading.hpp>
#include <utilities/cryptography.hpp>
#include <utilities/http.hpp>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <future> 

Application::Application() {
    spdlog::set_level(spdlog::level::info);

    static constexpr size_t connections { 10uz };

    curl_pool   = std::make_unique<wdk::core::CurlPool>(connections);
    credentials = std::make_unique<wdk::core::Credentials>(CREDENTIALS_PATH);
    token       = std::make_unique<wdk::core::Token>(TOKEN_PATH, *curl_pool.get(), *credentials.get(), HOST);
}

void Application::run() {
    demo();
}

void Application::demo() {
    wdk::client::TradingClient client(
        *curl_pool, 
        *credentials.get(), 
        HOST, 
        token->get_handle()
    );

    const std::string extracted_account_id = client.get_account_id();

    spdlog::info("[Application] Dispatching balance request concurrently...");

    std::future<wdk::utilities::Response> balance_future  = client.fetch_account_balance_async(extracted_account_id);
    
    wdk::utilities::Response account_balance = balance_future.get();

    if (account_balance.http_code == 200L) {
        spdlog::info("[Application] Successfully fetched account balance:\n {}", nlohmann::json::parse(account_balance.message).dump(4));
    } else {
        spdlog::error("[Application] Failed to fetch account balance:\n {}", nlohmann::json::parse(account_balance.message).dump(4));
    }
    
    spdlog::info("[Application] Dispatching position request concurrently...");

    std::future<wdk::utilities::Response> position_future = client.fetch_account_position_async(extracted_account_id);
    
    wdk::utilities::Response account_position = position_future.get();
    
    if (account_position.http_code == 200L) {
        spdlog::info("[Application] Successfully fetched account positions:\n {}", nlohmann::json::parse(account_position.message).dump(4));
    } else {
        spdlog::error("[Application] Failed to fetch account positions:\n {}", nlohmann::json::parse(account_position.message).dump(4));
    }

    std::string client_order_id = wdk::utilities::generate_nonce(26uz);

    spdlog::info("[Application] Dispatching order placement...");

    std::future<wdk::utilities::Response> place_order_future = client.place_order_async({
        .account_id              { extracted_account_id },          
        .combo_type              { "NORMAL" },                      
        .client_order_id         { client_order_id },               
        .instrument_type         { "EQUITY" },                      
        .market                  { "US" },
        .symbol                  { "SSG" },
        .order_type              { "LIMIT" },
        .entrust_type            { "QTY" },
        .support_trading_session { "CORE" },                        
        .time_in_force           { "DAY" },
        .side                    { "BUY" },
        .quantity                { 1.0 },
        .limit_price             { 11.20 },
        .stop_price              { std::nullopt }
    });

    wdk::utilities::Response place_order = place_order_future.get();
    
    if (place_order.http_code == 200L) {
        spdlog::info("[Application] Successfully placed order:\n {}", nlohmann::json::parse(place_order.message).dump(4));
    } else {
        spdlog::error("[Application] Failed to place order:\n {}", nlohmann::json::parse(place_order.message).dump(4));
    }

    spdlog::info("[Application] Modifying placed order reference: {}", client_order_id);
    
    std::future<wdk::utilities::Response> modify_order_future = client.modify_order_async({
        .account_id      { extracted_account_id },
        .client_order_id { client_order_id },
        .time_in_force   { "DAY" }, 
        .quantity        { 1.0 },
        .limit_price     { 11.30 },
        .stop_price      { std::nullopt }
    });

    wdk::utilities::Response modify_order = modify_order_future.get();

    if (modify_order.http_code == 200L) {
        spdlog::info("[Application] Successfully modified order:\n {}", nlohmann::json::parse(modify_order.message).dump(4));
    } else {
        spdlog::error("[Application] Failed to modify order:\n {}", nlohmann::json::parse(modify_order.message).dump(4));
    }

    spdlog::info("[Application] Cancelling order reference: {}", client_order_id);
    
    std::future<wdk::utilities::Response> cancel_order_future = client.cancel_order_async({
        .account_id      = extracted_account_id,
        .client_order_id = client_order_id,
    });

    wdk::utilities::Response cancel_order = cancel_order_future.get();
    
    if (cancel_order.http_code == 200L) {
        spdlog::info("[Application] Successfully canceled order:\n {}", nlohmann::json::parse(cancel_order.message).dump(4));
    } else {
        spdlog::error("[Application] Failed to cancel order:\n {}", nlohmann::json::parse(cancel_order.message).dump(4));
    }

    wdk::client::MarketClient market_client(
        *curl_pool,
        *credentials,
        HOST,
        token->get_handle()
    );

    spdlog::info("[Application] Fetching tick data (asynchronous)...");

    std::future<wdk::utilities::Response> tick_future = market_client.fetch_tick_data_async({ 
        .symbol          { "AAPL" },
        .category        { "US_STOCK" },
        .count           { 2uz  },
        .trading_session { "PRE" }
    });

    wdk::utilities::Response async_tick_response = tick_future.get();

    if (async_tick_response.http_code == 200L) {
        spdlog::info("[Application] Successfully fetched async tick data:\n{}", nlohmann::json::parse(async_tick_response.message).dump(4));
    } else {
        spdlog::error("[Application] Failed to fetch async tick data:\n{}", nlohmann::json::parse(async_tick_response.message).dump(4));
    }
}