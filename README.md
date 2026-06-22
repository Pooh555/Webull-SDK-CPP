<div align="center">

# WDK

### A modern C++23 client for the Webull OpenAPI

Asynchronous · Thread-Safe · Header-Light · Built on `libcurl` & `nlohmann/json`

[![C++](https://img.shields.io/badge/C%2B%2B-23-00599C?style=flat-square&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/23)
[![Build System](https://img.shields.io/badge/build-CMake%20%2B%20Ninja-064F8C?style=flat-square&logo=cmake)](https://cmake.org/)
[![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)](#license)

[Overview](#overview) • [Quick Start](#quick-start) • [Installation](#installation) • [Architecture](#architecture) • [Project Structure](#project-structure)

</div>

---

## Overview
I detest Python, and, coincidentally, the official Java SDK is unreliably maintained, thus let's go with C++. :P

## Documentation
This documentation is AI-generated, and may not be up-to-date cuz I'm too lazy to write. :3

**Visit the full documentation here: [documenetation](docs/documentation.md)**

## Installation
### Quick Start
Cloning the main repository.
```
git clone https://github.com/Pooh555/Webull-SDK/
```
Compiling the library.
```
cd Webull-SDK
./build.sh
```
Executing the example.
```
./run.sh
```
### Integrating as a thrid-party library
Add submodule.
```
git submodule add https://github.com/Pooh555/Webull-SDK.git thrid-party/Webull-SDK
git submodule update --init --recursive
```
Modify CMakeLists.txt.
```cmake
add_subdirectory(external/Webull-SDK)

add_executable(MyApplication
    src/main.cpp
)

target_link_libraries(MyApplication
    PRIVATE
        Webull::SDK
)
```
## Usage
### Market API
Fetch market data. 
Example: Fetching tick data.
```cpp
// Initialize a market client for fetching market data
wdk::client::MarketClient market_client(
    *curl_pool,
    *thread_pool,
    *credentials,
    HOST,
    token->get_handle()
);

// Fetch tick data
spdlog::info("[Application] Fetching tick data ...");

std::future<wdk::utilities::Response> tick_future = market_client.fetch_tick_data_async({ 
    .symbol           { "AAPL" },
    .category         { "US_STOCK" },
    .count            { 2uz  },
    .trading_sessions { "PRE" }
});
wdk::utilities::Response tick_data = tick_future.get();

// Display the response
if (tick_data.http_code == 200L) {
    spdlog::info("[Application] Successfully fetched tick data:\n{}", nlohmann::json::parse(tick_data.message).dump(4));
} else {
    spdlog::error("[Application] Failed to fetch tick data:\n{}", nlohmann::json::parse(tick_data.message).dump(4));
}
```
### Trading API
Place, modify, and cancel orders.
Example: Placing an order.
```cpp
// Initialize a trading client for handling order operations
wdk::client::TradingClient client(
    *curl_pool, 
    *thread_pool,
    *credentials.get(), 
    HOST, 
    token->get_handle()
);

// Extract account id
const std::string extracted_account_id = client.get_account_id();

// Retrive order id (nonce)
std::string client_order_id = wdk::utilities::generate_nonce();

// Place an order
spdlog::info("[Application] Dispatching order placement...");

std::future<wdk::utilities::Response> place_order_future = client.place_order_async({
    .account_id              { extracted_account_id },          
    .combo_type              { "NORMAL" },                      
    .client_order_id         { client_order_id },               
    .instrument_type         { "EQUITY" },                      
    .market                  { "US" },
    .symbol                  { "NVDA" },
    .order_type              { "LIMIT" },
    .entrust_type            { "QTY" },
    .trading_session         { "ALL_DAY" },                        
    .time_in_force           { "DAY" },
    .side                    { "BUY" },
    .quantity                { 1.0 },
    .limit_price             { 200.05 },
    .stop_price              { std::nullopt }
});
wdk::utilities::Response place_order = place_order_future.get();

// Display the response
if (place_order.http_code == 200L) {
    spdlog::info("[Application] Successfully placed order:\n {}", nlohmann::json::parse(place_order.message).dump(4));
} else {
    spdlog::error("[Application] Failed to place order:\n {}", nlohmann::json::parse(place_order.message).dump(4));
}
```
### Architecture

The codebase is split into three layers, each with a single responsibility:

```
┌──────────────────────────────────────────────────────────────┐
│                         wdk::client                          │
│      TradingClient            │            MarketClient      │
│  (accounts · orders)          │        (tick data)           │
└───────────────┬─────────────────────────────┬────────────────┘
                │                             │
┌───────────────▼─────────────────────────────▼───────────────────┐
│                        wdk::utilities                           │
│   http (execute_request)  │  openapi (signing)  │  cryptography │
│   json (read/write)       │  time (UTC stamps)                  │
└───────────────┬─────────────────────────────────────────────────┘
                │
┌───────────────▼───────────────────────────────────────────────┐
│                          wdk::core                            │
│      Secret (credentials)  │  Token (session)  │  CurlPool    │
└───────────────────────────────────────────────────────────────┘
```
### Project Structure
This project is build based on CMake build system.
```
.
├── assets
├── docs
├── examples
│   ├── bin
│   ├── res
│   └── src
│       └── core
├── include
│   ├── client
│   ├── core
│   └── utilities
├── lib
│   └── nlohmann
│       ├── detail
│       │   ├── conversions
│       │   ├── input
│       │   ├── iterators
│       │   ├── meta
│       │   │   └── call_std
│       │   └── output
│       └── thirdparty
│           └── hedley
├── out
│   └── build
│       └── debug-ninja
│           ├── CMakeFiles
│           │   ├── 4.3.4
│           │   │   ├── CompilerIdC
│           │   │   │   └── tmp
│           │   │   └── CompilerIdCXX
│           │   │       └── tmp
│           │   ├── pkgRedirects
│           │   └── Webull-SDK.dir
│           │       └── src
│           │           ├── client
│           │           ├── core
│           │           └── utilities
│           └── examples
│               └── CMakeFiles
│                   └── Webull-SDK-Demo.dir
│                       └── src
│                           └── core
└── src
    ├── client
    ├── core
    └── utilities
```
### License
**Visit the license here: [documenetation](https://github.com/Pooh555/Webull-SDK/blob/main/LICENSE)**
