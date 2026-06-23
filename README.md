<div align="center">

# WDK — Webull Developer Kit

### A Modern C++23 Client for the Webull OpenAPI

**Asynchronous · Thread-Safe · Header-Light · Cryptographically Signed**

Built on `libcurl` · `nlohmann/json` · `OpenSSL` · `spdlog`

[![C++](https://img.shields.io/badge/C%2B%2B-23-00599C?style=flat-square&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/23)
[![Build](https://img.shields.io/badge/build-CMake%20%2B%20Ninja-064F8C?style=flat-square&logo=cmake)](https://cmake.org/)
[![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)](#license)

</div>

---

## Overview
WDK (**Webull Developer Kit**) is a native C++23 client library for the Webull OpenAPI. It provides fully typed, asynchronous access to market data and trading operations through a three-layer architecture that separates transport, signing, and business logic.

The library was designed with the following principles:

- **Zero dynamic dispatch in the hot path.** All client objects are instantiated by the caller; no virtual tables are used in the core I/O path.
- **Cooperative ownership.** All heavyweight resources (`CurlPool`, `ThreadPool`, `Token`) are managed through `std::unique_ptr` by the consuming application. Clients receive non-owning references.
- **Symmetric async/sync surface.** Every client method has both a blocking synchronous variant and a `std::future`-returning asynchronous variant. The caller selects the scheduling model.
- **Request signing encapsulated from business logic.** HMAC-SHA1 signing, nonce generation, and UTC timestamping are performed inside `wdk::utilities::execute_request` and are never visible to the calling code.

## Documentation
Visit the full documentation here: [Documentation](docs/documentation.md)

Visit the documentation for LLMs here: [Documentation for AI Agents](docs/llms.txt)

## Prerequisites

### Compiler

| Requirement | Minimum Version |
|---|---|
| GCC | 13.0 |
| Clang | 17.0 |
| C++ Standard | C++23 (`-std=c++23`) |

### Build Tools

| Tool | Purpose |
|---|---|
| CMake | Build system generator |
| Ninja | Recommended build backend |

### Runtime Dependencies

| Library | Purpose | Notes |
|---|---|---|
| `libcurl` | HTTP transport | With HTTPS / HTTP/2 support |
| `OpenSSL` | HMAC-SHA1, MD5, Base64 | libcrypto is used |
| `spdlog` | Structured logging | Header-only mode supported |
| `nlohmann/json` | JSON serialization | Bundled under `lib/` |

All dependencies except `nlohmann/json` must be installed system-wide and discoverable by CMake's `find_package`.

On a Debian/Ubuntu system:

```sh
sudo apt install libcurl4-openssl-dev libssl-dev libspdlog-dev cmake ninja-build
```

On Arch Linux:

```sh
sudo pacman -S curl openssl spdlog cmake ninja
```


## Installation
### Quick Start
Cloning the main repository.
### Building from Source

Clone the repository and use the provided build script:

```sh
git clone https://github.com/Pooh555/Webull-SDK.git
cd Webull-SDK
./build.sh
```

The build script accepts the following flags:

| Flag | Description | Default |
|---|---|---|
| `-b, --build-type <type>` | CMake build type: `Debug`, `Release`, `RelWithDebInfo` | `Debug` |
| `-c, --clean` | Remove the existing build directory before building | Off |
| `-h, --help` | Print the help menu and exit | — |

**Example: release build**

```sh
./build.sh --build-type Release
```

**Example: clean debug build**

```sh
./build.sh --clean
```

Upon successful completion, the following artifacts are produced:

| Artifact | Location |
|---|---|
| Static library | `out/build/<preset>/libWebull-SDK.a` |
| Demo binary | `examples/bin/Webull-SDK-Demo` |

To execute the bundled demo:

```sh
./run.sh
```

### Integration as a Third-Party Library

WDK exposes a CMake target `Webull::SDK` for downstream consumption.

**Step 1.** Add the repository as a Git submodule:

```sh
git submodule add https://github.com/Pooh555/Webull-SDK.git third-party/Webull-SDK
git submodule update --init --recursive
```

**Step 2.** Configure your project's `CMakeLists.txt`:

```cmake
add_subdirectory(third-party/Webull-SDK)

add_executable(MyApplication
    src/main.cpp
)

target_link_libraries(MyApplication
    PRIVATE
        Webull::SDK
)
```

**Step 3.** Call `cmake --build` as usual. The `Webull::SDK` target propagates all required include directories and compile options transitively.

## Usage Guide

### Initialization

The following objects must be constructed before any client can be used. They are typically held by the application's top-level class.

```cpp
#include <core/credentials.hpp>
#include <core/curl_pool.hpp>
#include <core/thread_pool.hpp>
#include <core/token.hpp>

static constexpr std::string_view HOST             { "api.webull.co.th" };
static constexpr std::string_view TOKEN_PATH       { "examples/res/token.json" };
static constexpr std::string_view CREDENTIALS_PATH { "examples/res/credentials.json" };

// Allocate infrastructure (order matters: pool and credentials before token)
auto curl_pool   = std::make_unique<wdk::core::CurlPool>(10uz);
auto thread_pool = std::make_unique<wdk::core::ThreadPool>();
auto credentials = std::make_unique<wdk::core::Credentials>(CREDENTIALS_PATH);

// Token constructor blocks until the session is active.
// The user may need to approve the login in the Webull mobile app.
auto token = std::make_unique<wdk::core::Token>(
    TOKEN_PATH, *curl_pool, *credentials, HOST
);
```

### Market Data

#### Tick Data

```cpp
wdk::client::MarketClient market_client(
    *curl_pool, *thread_pool, *credentials, HOST, token->get_handle()
);
```cpp
std::future<wdk::utilities::Response> future = market_client.fetch_historical_bars_data_async({
    .symbol               { "AAPL" },
    .category             { "US_STOCK" },
    .timespan             { "M5" },
    .count                { 100uz },
    .real_time_required   { false },
    .trading_sessions     { "CORE" }
});

wdk::utilities::Response response = future.get();

if (response.http_code == 200L) {
    std::vector<wdk::data::HistoricalBarsData> history =
        wdk::data::convert_response_to_historical_bars_vector(response);

    for (const auto& symbol_data : history) {
        for (const auto& bar : symbol_data.bars) {
            // Access bar.time, bar.open, bar.high, bar.low, bar.close, bar.volume
        }
    }
}
```
### Trading Operations

> Before placing orders, always call `preview_order` first to validate the request and review estimated costs.

#### Order Lifecycle Example

```cpp
wdk::client::TradingClient trading_client(
    *curl_pool, *thread_pool, *credentials, HOST, token->get_handle()
);

const std::string account_id      = trading_client.get_account_id();
const std::string client_order_id = wdk::utilities::generate_nonce();

// Step 1: Preview the order
wdk::utilities::Response preview = trading_client.preview_order({
    .account_id      { account_id },
    .combo_type      { "NORMAL" },
    .client_order_id { client_order_id },
    .instrument_type { "EQUITY" },
    .market          { "US" },
    .symbol          { "NVDA" },
    .order_type      { "LIMIT" },
    .entrust_type    { "QTY" },
    .trading_session { "CORE" },
    .time_in_force   { "DAY" },
    .side            { "BUY" },
    .quantity        { 1.0 },
    .limit_price     { 135.00 },
    .stop_price      { std::nullopt }
});

// Step 2: Place the order (only if preview confirms acceptable terms)
if (preview.http_code == 200L) {
    wdk::utilities::Response placed = trading_client.place_order({
        .account_id      { account_id },
        .combo_type      { "NORMAL" },
        .client_order_id { client_order_id },
        .instrument_type { "EQUITY" },
        .market          { "US" },
        .symbol          { "NVDA" },
        .order_type      { "LIMIT" },
        .entrust_type    { "QTY" },
        .trading_session { "CORE" },
        .time_in_force   { "DAY" },
        .side            { "BUY" },
        .quantity        { 1.0 },
        .limit_price     { 135.00 },
        .stop_price      { std::nullopt }
    });
}

// Step 3: Modify the order price
wdk::utilities::Response modified = trading_client.modify_order({
    .account_id      { account_id },
    .client_order_id { client_order_id },
    .time_in_force   { "DAY" },
    .quantity        { 1.0 },
    .limit_price     { 134.50 },
    .stop_price      { std::nullopt }
});

// Step 4: Cancel the order
wdk::utilities::Response cancelled = trading_client.cancel_order({
    .account_id      { account_id },
    .client_order_id { client_order_id }
});
```

---

### Account Management

```cpp
// Fetch account list (and resolve the primary account ID in one call)
const std::string account_id = trading_client.get_account_id();

// Account balance
std::future<wdk::utilities::Response> balance_future =
    trading_client.fetch_account_balance_async(account_id);
wdk::utilities::Response balance = balance_future.get();

// Open positions
std::future<wdk::utilities::Response> position_future =
    trading_client.fetch_account_position_async(account_id);
wdk::utilities::Response positions = position_future.get();

// Order history (paginated)
std::future<wdk::utilities::Response> history_future =
    trading_client.fetch_order_history_async({
        .account_id     { account_id },
        .start_date     { "2026-01-01" },
        .page_size      { 50uz },
        .last_client_id { "" }
    });
wdk::utilities::Response history = history_future.get();
```
---

<div align="center">

_Given Python's dubious appeal and the questionable maintenance of the Java SDK, C++ was an inevitability._
