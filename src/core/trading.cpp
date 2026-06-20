#include "trading.hpp"

#include "utilities/http.hpp"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <format>

TradingClient::TradingClient(CURL* curl, const Secret& secret, std::string_view host, std::string_view token)
    : curl_(curl), secret_(secret), host_(host), token_(token) {}

std::string TradingClient::get_account_list() {
    return utilities::http::execute_request(curl_, secret_, host_, ACCOUNT_LIST_PATH, false, "", token_);
}

std::string TradingClient::preview_order(const OrderRequest& request) {
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

    return utilities::http::execute_request(curl_, secret_, host_, PREVIEW_ORDER_PATH, true, root_payload.dump(), token_);
}

std::string TradingClient::place_order(const OrderRequest& request) {
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

    return utilities::http::execute_request(curl_, secret_, host_, PLACE_ORDER_PATH, true, root_payload.dump(), token_);
}

std::string TradingClient::modify_order(const OrderRequest& request) {
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

    return utilities::http::execute_request(curl_, secret_, host_, MODIFY_ORDER_PATH, true, root_payload.dump(), token_);
}

std::string TradingClient::cancel_order(const OrderRequest& request) {
    nlohmann::json root_payload {
        {"account_id", request.account_id},
        {"client_order_id", request.client_order_id}
    };

    return utilities::http::execute_request(curl_, secret_, host_, CANCEL_ORDER_PATH, true, root_payload.dump(), token_);
}