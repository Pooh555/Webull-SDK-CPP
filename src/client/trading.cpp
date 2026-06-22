#include <client/trading.hpp>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <format>
#include <future> 

namespace wdk::client {

TradingClient::TradingClient(
          wdk::core::CurlPool&    pool, 
    const wdk::core::Credentials& credentials, 
          std::string_view        host, 
          std::string_view        token)
    : pool_(pool), credentials_(credentials), host_(host), token_(token) {}

wdk::utilities::Response TradingClient::fetch_account_list() {
    return wdk::utilities::execute_request(
        pool_,
        credentials_, 
        host_, 
        ACCOUNT_LIST_PATH, 
        wdk::utilities::HttpMethod::GET, 
        "", 
        token_
    );
}

/*
 * Deprecated function before implementing concurrecy
 * 
 * std::string TradingClient::get_account_id() {
 *     if (!account_id.empty()) return account_id;
 * 
 *     std::string account_list_json    = fetch_account_list().message;
 *     std::string extracted_account_id = "";
 * 
 *     try {
 *         if (!account_list_json.empty()) {
 *             auto parsed_response = nlohmann::json::parse(account_list_json);
 *             
 *             if (parsed_response.is_array() && !parsed_response.empty()) {
 *                 extracted_account_id = parsed_response[0].value("account_id", "");
 *             } else if (parsed_response.contains("data") && parsed_response["data"].is_array() && !parsed_response["data"].empty()) {
 *                 extracted_account_id = parsed_response["data"][0].value("account_id", "");
 *             }
 *         }
 *     } catch (const nlohmann::json::parse_error& e) {
 *         spdlog::error("[TradingClient] Failed to parse account listing JSON metrics: {}", e.what());
 *         return "";
 *     }
 * 
 *     if (extracted_account_id.empty()) {
 *         spdlog::error("[TradingClient] Could not determine a valid account ID target mapping");
 *     }
 * 
 *     spdlog::info("[TradingClient] Successfully retrieved account ID");
 * 
 *     account_id = extracted_account_id;
 * 
 *     return account_id;
 * }
 */

std::string TradingClient::get_account_id() {
    if (!account_id.empty()) return account_id;

    std::string account_list_json = fetch_account_list_async().get().message;
    std::string extracted_account_id = "";

    try {
        if (!account_list_json.empty()) {
            auto parsed_response = nlohmann::json::parse(account_list_json);
            
            if (parsed_response.is_array() && !parsed_response.empty()) {
                extracted_account_id = parsed_response[0].value("account_id", "");
            } else if (parsed_response.contains("data") && parsed_response["data"].is_array() && !parsed_response["data"].empty()) {
                extracted_account_id = parsed_response["data"][0].value("account_id", "");
            }
        }
    } catch (const nlohmann::json::parse_error& e) {
        spdlog::error("[TradingClient] Failed to parse account listing JSON metrics: {}", e.what());
        return "";
    }

    if (extracted_account_id.empty()) {
        spdlog::error("[TradingClient] Could not determine a valid account ID target mapping");
    }

    spdlog::info("[TradingClient] Successfully retrieved account ID");

    account_id = extracted_account_id;

    return account_id;
}

wdk::utilities::Response TradingClient::fetch_account_balance(const std::string& account_id) {
    if (account_id.empty()) {
        spdlog::error("[TradingClient] Failed to fetch account balance: account_id is empty");
        return { 0L, R"({"error": "Empty account ID"})" };
    }

    std::string path = std::format(
        "{}?account_id={}",
        ACCOUNT_BALANCE_PATH,
        account_id
    );

    return wdk::utilities::execute_request(
        pool_,
        credentials_,
        host_,
        path,
        wdk::utilities::HttpMethod::GET,
        "",
        token_
    );
}
    
wdk::utilities::Response TradingClient::fetch_account_position(const std::string& account_id) {
    if (account_id.empty()) {
        spdlog::error("[TradingClient] Failed to fetch account positions: account_id is empty");
        return { 0L, R"({"error": "Empty account ID"})" };
    }

    std::string path = std::format(
        "{}?account_id={}",
        ACCOUNT_POSITION_PATH,
        account_id
    );

    return wdk::utilities::execute_request(
        pool_,
        credentials_,
        host_,
        path,
        wdk::utilities::HttpMethod::GET,
        "",
        token_
    );
}

wdk::utilities::Response TradingClient::preview_order(const OrderRequest& request) {
    nlohmann::json order_item {
        {"combo_type",              request.combo_type},
        {"client_order_id",         request.client_order_id},
        {"instrument_type",         request.instrument_type},
        {"market",                  request.market},
        {"symbol",                  request.symbol},
        {"order_type",              request.order_type},
        {"entrust_type",            request.entrust_type},
        {"support_trading_session", request.support_trading_session},
        {"time_in_force",           request.time_in_force},
        {"side",                    request.side}
    };

    if (request.quantity)    order_item["quantity"]    = std::format("{}", *request.quantity);
    if (request.limit_price) order_item["limit_price"] = std::format("{:.2f}", *request.limit_price);
    if (request.stop_price)  order_item["stop_price"]  = std::format("{:.2f}", *request.stop_price);

    nlohmann::json root_payload {
        {"account_id", request.account_id},
        {"new_orders", nlohmann::json::array({order_item})}
    };

    return wdk::utilities::execute_request(
        pool_, 
        credentials_, 
        host_, 
        PREVIEW_ORDER_PATH, 
        wdk::utilities::HttpMethod::POST, 
        root_payload.dump(), 
        token_);
}

wdk::utilities::Response TradingClient::place_order(const OrderRequest& request) {
    nlohmann::json order_item {
        {"combo_type",              request.combo_type},
        {"client_order_id",         request.client_order_id},
        {"instrument_type",         request.instrument_type},
        {"market",                  request.market},
        {"symbol",                  request.symbol},
        {"order_type",              request.order_type},
        {"entrust_type",            request.entrust_type},
        {"support_trading_session", request.support_trading_session},
        {"time_in_force",           request.time_in_force},
        {"side",                    request.side}
    };

    if (request.quantity)    order_item["quantity"]    = std::format("{}", *request.quantity);
    if (request.limit_price) order_item["limit_price"] = std::format("{:.2f}", *request.limit_price);
    if (request.stop_price)  order_item["stop_price"]  = std::format("{:.2f}", *request.stop_price);

    nlohmann::json root_payload {
        {"account_id", request.account_id},
        {"new_orders", nlohmann::json::array({order_item})}
    };

    return wdk::utilities::execute_request(
        pool_, 
        credentials_, 
        host_, 
        PLACE_ORDER_PATH, 
        wdk::utilities::HttpMethod::POST, 
        root_payload.dump(), 
        token_);
}

wdk::utilities::Response TradingClient::modify_order(const OrderRequest& request) {
    nlohmann::json modify_item {
        {"client_order_id", request.client_order_id}
    };

    if (request.quantity)               modify_item["quantity"]      = std::format("{}", *request.quantity);
    if (request.limit_price)            modify_item["limit_price"]   = std::format("{:.2f}", *request.limit_price);
    if (request.stop_price)             modify_item["stop_price"]    = std::format("{:.2f}", *request.stop_price);
    if (!request.time_in_force.empty()) modify_item["time_in_force"] = request.time_in_force;

    nlohmann::json root_payload {
        {"account_id", request.account_id},
        {"modify_orders", nlohmann::json::array({modify_item})}
    };

    return wdk::utilities::execute_request(
        pool_, 
        credentials_, 
        host_, 
        MODIFY_ORDER_PATH, 
        wdk::utilities::HttpMethod::POST, 
        root_payload.dump(), 
        token_);
}

wdk::utilities::Response TradingClient::cancel_order(const OrderRequest& request) {
    nlohmann::json root_payload {
        {"account_id", request.account_id},
        {"client_order_id", request.client_order_id}
    };

    return wdk::utilities::execute_request(
        pool_, 
        credentials_,
        host_, 
        CANCEL_ORDER_PATH, 
        wdk::utilities::HttpMethod::POST, 
        root_payload.dump(), 
        token_);
}

std::future<wdk::utilities::Response> TradingClient::fetch_account_list_async() {
    return wdk::utilities::execute_request_async(
        pool_,
        credentials_, 
        host_, 
        ACCOUNT_LIST_PATH, 
        wdk::utilities::HttpMethod::GET, 
        "", 
        token_
    );
}

std::future<wdk::utilities::Response> TradingClient::fetch_account_balance_async(const std::string& account_id) {
    if (account_id.empty()) {
        spdlog::error("[TradingClient] Failed to fetch account balance: account_id is empty");

        return std::async(std::launch::deferred, []() {
            return wdk::utilities::Response{ 0L, R"({"error": "Empty account ID"})" };
        });
    }

    std::string path = std::format("{}?account_id={}", ACCOUNT_BALANCE_PATH, account_id);

    return wdk::utilities::execute_request_async(
        pool_,
        credentials_,
        host_,
        path,
        wdk::utilities::HttpMethod::GET,
        "",
        token_
    );
}
    
std::future<wdk::utilities::Response> TradingClient::fetch_account_position_async(const std::string& account_id) {
    if (account_id.empty()) {
        spdlog::error("[TradingClient] Failed to fetch account positions: account_id is empty");
        
        return std::async(std::launch::deferred, []() {
            return wdk::utilities::Response{ 0L, R"({"error": "Empty account ID"})" };
        });
    }

    std::string path = std::format("{}?account_id={}", ACCOUNT_POSITION_PATH, account_id);

    return wdk::utilities::execute_request_async(
        pool_,
        credentials_,
        host_,
        path,
        wdk::utilities::HttpMethod::GET,
        "",
        token_
    );
}

std::future<wdk::utilities::Response> TradingClient::preview_order_async(const OrderRequest& request) {
    nlohmann::json order_item {
        {"combo_type",              request.combo_type},
        {"client_order_id",         request.client_order_id},
        {"instrument_type",         request.instrument_type},
        {"market",                  request.market},
        {"symbol",                  request.symbol},
        {"order_type",              request.order_type},
        {"entrust_type",            request.entrust_type},
        {"support_trading_session", request.support_trading_session},
        {"time_in_force",           request.time_in_force},
        {"side",                    request.side}
    };

    if (request.quantity)    order_item["quantity"]    = std::format("{}", *request.quantity);
    if (request.limit_price) order_item["limit_price"] = std::format("{:.2f}", *request.limit_price);
    if (request.stop_price)  order_item["stop_price"]  = std::format("{:.2f}", *request.stop_price);

    nlohmann::json root_payload {
        {"account_id", request.account_id},
        {"new_orders", nlohmann::json::array({order_item})}
    };

    return wdk::utilities::execute_request_async(
        pool_, 
        credentials_, 
        host_, 
        PREVIEW_ORDER_PATH, 
        wdk::utilities::HttpMethod::POST, 
        root_payload.dump(), 
        token_
    );
}

std::future<wdk::utilities::Response> TradingClient::place_order_async(const OrderRequest& request) {
    nlohmann::json order_item {
        {"combo_type",              request.combo_type},
        {"client_order_id",         request.client_order_id},
        {"instrument_type",         request.instrument_type},
        {"market",                  request.market},
        {"symbol",                  request.symbol},
        {"order_type",              request.order_type},
        {"entrust_type",            request.entrust_type},
        {"support_trading_session", request.support_trading_session},
        {"time_in_force",           request.time_in_force},
        {"side",                    request.side}
    };

    if (request.quantity)    order_item["quantity"]    = std::format("{}", *request.quantity);
    if (request.limit_price) order_item["limit_price"] = std::format("{:.2f}", *request.limit_price);
    if (request.stop_price)  order_item["stop_price"]  = std::format("{:.2f}", *request.stop_price);

    nlohmann::json root_payload {
        {"account_id", request.account_id},
        {"new_orders", nlohmann::json::array({order_item})}
    };

    return wdk::utilities::execute_request_async(
        pool_, 
        credentials_, 
        host_, 
        PLACE_ORDER_PATH, 
        wdk::utilities::HttpMethod::POST, 
        root_payload.dump(), 
        token_
    );
}

std::future<wdk::utilities::Response> TradingClient::modify_order_async(const OrderRequest& request) {
    nlohmann::json modify_item {
        {"client_order_id", request.client_order_id}
    };

    if (request.quantity)               modify_item["quantity"]      = std::format("{}", *request.quantity);
    if (request.limit_price)            modify_item["limit_price"]   = std::format("{:.2f}", *request.limit_price);
    if (request.stop_price)             modify_item["stop_price"]    = std::format("{:.2f}", *request.stop_price);
    if (!request.time_in_force.empty()) modify_item["time_in_force"] = request.time_in_force;

    nlohmann::json root_payload {
        {"account_id", request.account_id},
        {"modify_orders", nlohmann::json::array({modify_item})}
    };

    return wdk::utilities::execute_request_async(
        pool_, 
        credentials_, 
        host_, 
        MODIFY_ORDER_PATH, 
        wdk::utilities::HttpMethod::POST, 
        root_payload.dump(), 
        token_
    );
}

std::future<wdk::utilities::Response> TradingClient::cancel_order_async(const OrderRequest& request) {
    nlohmann::json root_payload {
        {"account_id", request.account_id},
        {"client_order_id", request.client_order_id}
    };

    return wdk::utilities::execute_request_async(
        pool_, 
        credentials_,
        host_, 
        CANCEL_ORDER_PATH, 
        wdk::utilities::HttpMethod::POST, 
        root_payload.dump(), 
        token_
    );
}

}