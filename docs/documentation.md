<div align="center">

# Webull SDK

### A modern C++23 client for the Webull OpenAPI

Asynchronous · Thread-Safe · Header-Light · Built on `libcurl` & `nlohmann/json`

[![C++](https://img.shields.io/badge/C%2B%2B-23-00599C?style=flat-square&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/23)
[![Build System](https://img.shields.io/badge/build-CMake%20%2B%20Ninja-064F8C?style=flat-square&logo=cmake)](https://cmake.org/)
[![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)](#license)

[Overview](#overview) • [Architecture](#architecture) • [Installation](#installation) • [Quick Start](#quick-start) • [API Reference](#api-reference) • [Security](#security-model)

</div>

---

## Overview

**Webull SDK** (`wdk`) is a native C++23 library for interacting with the [Webull OpenAPI](https://www.webull.com), covering authentication, market data, and order execution. It is designed around three principles:

| Principle | Description |
|---|---|
| 🔁 **Concurrency by default** | Every network-facing call ships with a synchronous *and* an `std::future`-based asynchronous variant. |
| 🔒 **Safety by construction** | RAII wraps every CURL handle, token lifecycle, and credential — there is no manual cleanup for consumers of the SDK. |
| 🧩 **Composable modules** | `core`, `client`, and `utilities` are independently includable namespaces, so you only pay for what you use. |

The SDK currently exposes two client surfaces:

- **`wdk::client::TradingClient`** — account discovery, balances, positions, and full order lifecycle (preview → place → modify → cancel).
- **`wdk::client::MarketClient`** — real-time and historical tick data retrieval.

---

## Architecture

The codebase is split into three layers, each with a single responsibility:

```
┌──────────────────────────────────────────────────────────────┐
│                         wdk::client                           │
│      TradingClient            │            MarketClient       │
│  (accounts · orders)          │        (tick data)            │
└───────────────┬────────────────────────────┬──────────────────┘
                │                             │
┌───────────────▼─────────────────────────────▼──────────────────┐
│                        wdk::utilities                          │
│   http (execute_request)  │  openapi (signing)  │  cryptography │
│   json (read/write)       │  time (UTC stamps)                  │
└───────────────┬──────────────────────────────────────────────────┘
                │
┌───────────────▼──────────────────────────────────────────────┐
│                          wdk::core                            │
│      Secret (credentials)  │  Token (session)  │  CurlPool    │
└────────────────────────────────────────────────────────────────┘
```

### `wdk::core`

| Component | Responsibility |
|---|---|
| `Secret` | Loads `id`, `key`, and `secret` from a local JSON file. Immutable after construction. |
| `Token` | Manages the OpenAPI session token: validates an existing token on disk, transparently regenerates it via mobile-app approval if expired, and persists the refreshed token back to disk. |
| `CurlPool` | A fixed-size, mutex-guarded pool of reusable `CURL*` handles. Handles are checked out via RAII (`CurlHandle`) and automatically returned to the pool on destruction — safe for concurrent use across async requests. |

### `wdk::utilities`

| Component | Responsibility |
|---|---|
| `http` | Builds and executes signed HTTP requests (`execute_request` / `execute_request_async`), parses query strings, and injects required headers. |
| `openapi` | Implements Webull's canonical request-signing scheme (parameter sorting, body hashing, HMAC signature). |
| `cryptography` | Low-level primitives: HMAC-SHA1, MD5, and cryptographically-seeded nonce generation. |
| `json` | Thin, exception-safe wrapper around `nlohmann::json` file I/O. |
| `time` | UTC timestamp formatting (`YYYY-MM-DDTHH:MM:SSZ`) using `std::chrono::utc_clock`. |

### `wdk::client`

| Component | Responsibility |
|---|---|
| `TradingClient` | Account listing/balance/positions and the order lifecycle: `preview_order`, `place_order`, `modify_order`, `cancel_order` — each with an async twin. |
| `MarketClient` | `fetch_tick_data` / `fetch_tick_data_async` for streaming-style tick snapshots, filtered by symbol, category, count, and trading session. |

---

## Installation

### Prerequisites

| Requirement | Notes |
|---|---|
| **Compiler** | GCC with C++23 support (`std::format`, `std::chrono::utc_clock`, deducing `this`-era stdlib) |
| **CMake** | ≥ 3.25 (CMakePresets v8) |
| **Ninja** | Recommended generator for fast incremental builds |
| **libcurl** | HTTP transport layer |
| **OpenSSL** | HMAC-SHA1 / MD5 signing primitives |
| **nlohmann/json** | JSON parsing & serialization |
| **spdlog** | Structured logging |

### Build

The project ships with a self-contained build script that wraps CMake + Ninja:

```bash
# Standard debug build
./build.sh

# Optimized release build, clean from scratch
./build.sh --build-type Release --clean

# View all options
./build.sh --help
```

<details>
<summary><strong>What <code>build.sh</code> does under the hood</strong></summary>

1. Verifies `cmake` and `ninja` are available on `PATH`.
2. Configures into `out/build/<build-type>-ninja` with `CMAKE_EXPORT_COMPILE_COMMANDS=ON` (for editor tooling / `clangd`).
3. Builds the static library (`libWebull-SDK.a`) and the bundled demo binary using all available CPU cores (`nproc`).
4. Prints the resolved artifact paths on success.

</details>

Alternatively, use the predefined **CMake Presets**:

```bash
cmake --preset debug-ninja      # fast incremental debug builds (recommended)
cmake --preset release          # optimized Ninja build
cmake --build --preset debug-ninja
```

### Running the Demo

A prebuilt example binary, `Webull-SDK-Demo`, demonstrates the complete client surface end-to-end:

```bash
./run.sh
```

`run.sh` performs a pre-flight check for the compiled binary, ensures it's executable, and streams its output with clear success/failure reporting.

---

## Quick Start

### 1 · Provide Credentials

Create a `secret.json` containing your Webull OpenAPI app credentials:

```json
{
  "id": "your-app-id",
  "key": "your-app-key",
  "secret": "your-app-secret"
}
```

### 2 · Bootstrap the Application

```cpp
#include "core/application.hpp"
#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    Application application;

    try {
        application.run();
    } catch (const std::exception& e) {
        spdlog::critical("[Main] Failed to run the application: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

On first run, if no valid token is cached, the SDK will log a prompt to approve the login from the **Webull Mobile App** and poll until the session is confirmed.

### 3 · Use the Trading Client

```cpp
wdk::core::CurlPool   pool(10);
wdk::core::Secret     secret("secret.json");
wdk::core::Token      token("token.json", pool, secret, "api.webull.co.th");

wdk::client::TradingClient client(pool, secret, "api.webull.co.th", token.get_handle());

std::string account_id = client.get_account_id();

auto balance = client.fetch_account_balance(account_id);
if (balance.http_code == 200L) {
    std::cout << balance.message << std::endl;
}
```

### 4 · Place an Order Asynchronously

```cpp
auto future = client.place_order_async({
    .account_id              = account_id,
    .combo_type              = "NORMAL",
    .client_order_id         = wdk::utilities::generate_nonce(26),
    .instrument_type         = "EQUITY",
    .market                  = "US",
    .symbol                  = "AAPL",
    .order_type              = "LIMIT",
    .entrust_type            = "QTY",
    .support_trading_session = "CORE",
    .time_in_force           = "DAY",
    .side                    = "BUY",
    .quantity                = 1.0,
    .limit_price             = 200.00
});

wdk::utilities::Response result = future.get();
```

### 5 · Fetch Market Data

```cpp
wdk::client::MarketClient market(pool, secret, "api.webull.co.th", token.get_handle());

auto tick_future = market.fetch_tick_data_async({
    .symbol          = "AAPL",
    .category        = "US_STOCK",
    .count           = 2,
    .trading_session = "PRE"
});

wdk::utilities::Response ticks = tick_future.get();
```

---

## API Reference

### `wdk::core::Secret`

```cpp
Secret(const std::filesystem::path& secret_path);

const std::string& get_id()     const;
const std::string& get_key()    const;
const std::string& get_secret() const;
```
Loads and exposes the three credential fields required for OpenAPI request signing. Copyable, immutable after load.

---

### `wdk::core::Token`

```cpp
Token(const std::filesystem::path& token_path,
      CurlPool& pool, const Secret& secret, const std::string_view& host);

void        generate(CurlPool& pool, const Secret& secret, const std::string_view& host);
void        verify  (CurlPool& pool, const Secret& secret, const std::string_view& host);
std::string get_handle() const;
std::string get_status() const;
bool        is_valid()   const;
```

| Method | Behavior |
|---|---|
| **Constructor** | Loads any cached token from disk, then `verify`s it. If invalid or not `NORMAL`, calls `generate` and polls (5s interval) while status remains `PENDING` — waiting for mobile-app approval — before persisting the result. |
| `generate` | Requests a fresh token via `POST /openapi/auth/token/create`. |
| `verify` | Validates the current token via `POST /openapi/auth/token/check`. |

Move-only (`Token(Token&&) = default`), non-copyable — a token represents a unique live session.

---

### `wdk::core::CurlPool`

```cpp
explicit CurlPool(size_t pool_size = 10);
[[nodiscard]] CurlHandle acquire();
```

A thread-safe object pool of `CURL*` handles. `acquire()` blocks (via `std::condition_variable`) until a handle is available, returning a `CurlHandle` — a `std::unique_ptr<CURL, CurlReleaser>` that automatically returns the handle to the pool on scope exit. Non-copyable, intended to be shared across clients via reference.

---

### `wdk::client::TradingClient`

```cpp
TradingClient(CurlPool& pool, const Secret& secret,
              std::string_view host, std::string_view token);
```

#### Account Operations

| Method | Async Variant | Endpoint |
|---|---|---|
| `fetch_account_list()` | `fetch_account_list_async()` | `GET /openapi/account/list` |
| `get_account_id()` | `get_account_id_async()` | derived from account list (cached after first call) |
| `fetch_account_balance(account_id)` | `fetch_account_balance_async(...)` | `GET /openapi/assets/balance` |
| `fetch_account_position(account_id)` | `fetch_account_position_async(...)` | `GET /openapi/assets/positions` |

#### Order Lifecycle

| Method | Async Variant | Endpoint |
|---|---|---|
| `preview_order(req)` | `preview_order_async(req)` | `POST /openapi/trade/order/preview` |
| `place_order(req)` | `place_order_async(req)` | `POST /openapi/trade/order/place` |
| `modify_order(req)` | `modify_order_async(req)` | `POST /openapi/trade/order/replace` |
| `cancel_order(req)` | `cancel_order_async(req)` | `POST /openapi/trade/order/cancel` |

#### `OrderRequest`

```cpp
struct OrderRequest {
    std::string           account_id;
    std::string           combo_type;
    std::string           client_order_id;
    std::string           instrument_type;
    std::string           market;
    std::string           symbol;
    std::string           order_type;
    std::string           entrust_type;
    std::string           support_trading_session;
    std::string           time_in_force;
    std::string           side;
    std::optional<double> quantity;
    std::optional<double> limit_price;
    std::optional<double> stop_price;
};
```
All fields default to empty/`nullopt`; only populated fields are serialized into the outgoing JSON payload — `modify_order` and `cancel_order` only require the subset relevant to their operation (e.g. `client_order_id`, and any fields being changed).

---

### `wdk::client::MarketClient`

```cpp
MarketClient(CurlPool& pool, const Secret& secret,
             std::string_view host = "", std::string_view token = "");

[[nodiscard]] wdk::utilities::Response fetch_tick_data(const MarketRequest& request);
[[nodiscard]] std::future<wdk::utilities::Response> fetch_tick_data_async(const MarketRequest& request);
```

#### `MarketRequest`

```cpp
struct MarketRequest {
    std::string           symbol;
    std::string           category;
    std::optional<size_t> count;
    std::string           trading_session;
};
```
Calls `GET /openapi/market-data/stock/tick`, with non-empty fields appended as URL query parameters.

---

### `wdk::utilities::http`

```cpp
enum class HttpMethod : bool { GET = false, POST = true };

struct Response {
    long        http_code;
    std::string message;
};

[[nodiscard]] Response execute_request(
    CurlPool& pool, const Secret& secret,
    std::string_view host, std::string_view path, HttpMethod method,
    std::string_view body_str = "", std::string_view token = "");

[[nodiscard]] std::future<Response> execute_request_async(/* ... */);

[[nodiscard]] curl_slist* generate_headers(
    const Secret& secret, std::string_view timestamp = "",
    std::string_view nonce = "", std::string_view signature = "",
    std::string_view token = "");
```

The core transport function used by every client method. It:
1. Acquires a pooled CURL handle.
2. Generates a UTC timestamp and random nonce.
3. Parses any inline query string out of `path`.
4. Computes the request signature via `wdk::utilities::generate_signature`.
5. Attaches required headers (`x-app-key`, `x-timestamp`, `x-signature-*`, `x-access-token`, `x-version`).
6. Executes the request and returns the HTTP status code with the raw response body.

---

## Security Model

Every request is authenticated using Webull's **HMAC-SHA1 canonical signing scheme**:

```
sign_string = request_path + "&" + canonical_params [+ "&" + MD5(body)]
signature   = HMAC-SHA1(app_secret + "&", url_encode(sign_string))
```

Where `canonical_params` is the alphabetically-sorted, `&`-joined set of:
- `host`, `x-app-key`, `x-signature-algorithm`, `x-signature-nonce`, `x-signature-version`, `x-timestamp`
- any URL query parameters present on the request

This signature, together with a fresh nonce and timestamp per-request, is transmitted via the `x-signature`, `x-signature-nonce`, and `x-timestamp` headers — preventing replay and tampering.

> **⚠️ Credential Handling**
> `secret.json` and `token.json` contain sensitive material. Never commit them to version control, and restrict filesystem permissions accordingly. The SDK reads and persists these files in plaintext at the paths you configure.

---

## Project Layout

```
Webull-SDK/
├── include/
│   ├── core/          # Secret, Token, CurlPool
│   ├── client/         # TradingClient, MarketClient
│   └── utilities/      # http, json, cryptography, openapi, time
├── src/                 # Implementation files mirroring include/
├── examples/
│   ├── bin/            # Compiled demo binary
│   └── res/            # secret.json / token.json (gitignored)
├── out/build/           # CMake build trees (per-preset)
├── CMakePresets.json
├── build.sh
└── run.sh
```

---

## License

This project is distributed under the **MIT License**. See `LICENSE` for details.

<div align="center">

Built with C++23 — for traders who'd rather compile than click.

</div>
