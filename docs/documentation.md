<div align="center">

# WDK — Webull Developer Kit

### A Modern C++23 Client for the Webull OpenAPI

**Asynchronous · Thread-Safe · Header-Light · Cryptographically Signed**

Built on `libcurl` · `nlohmann/json` · `OpenSSL` · `spdlog`

---

[![C++](https://img.shields.io/badge/C%2B%2B-23-00599C?style=flat-square&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/23)
[![Build](https://img.shields.io/badge/build-CMake%20%2B%20Ninja-064F8C?style=flat-square&logo=cmake)](https://cmake.org/)
[![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)](#license)

</div>

---

## Table of Contents

1. [Overview](#1-overview)
2. [Prerequisites](#2-prerequisites)
3. [Installation](#3-installation)
   - [3.1 Building from Source](#31-building-from-source)
   - [3.2 Integration as a Third-Party Library](#32-integration-as-a-third-party-library)
4. [Configuration](#4-configuration)
   - [4.1 Credentials File](#41-credentials-file)
   - [4.2 Token File](#42-token-file)
   - [4.3 API Endpoints](#43-api-endpoints)
5. [Architecture](#5-architecture)
   - [5.1 Layered Design](#51-layered-design)
   - [5.2 Concurrency Model](#52-concurrency-model)
   - [5.3 Authentication Flow](#53-authentication-flow)
   - [5.4 Request Signing](#54-request-signing)
   - [5.5 Project Structure](#55-project-structure)
6. [Core API Reference — `wdk::core`](#6-core-api-reference--wdkcore)
   - [6.1 `Credentials`](#61-credentials)
   - [6.2 `Token`](#62-token)
   - [6.3 `CurlPool`](#63-curlpool)
   - [6.4 `ThreadPool`](#64-threadpool)
   - [6.5 `RateLimiter`](#65-ratelimiter)
7. [Utilities API Reference — `wdk::utilities`](#7-utilities-api-reference--wdkutilities)
   - [7.1 HTTP Layer](#71-http-layer)
   - [7.2 Cryptography](#72-cryptography)
   - [7.3 OpenAPI Signing](#73-openapi-signing)
   - [7.4 Time](#74-time)
   - [7.5 JSON](#75-json)
8. [Client API Reference — `wdk::client`](#8-client-api-reference--wdkclient)
   - [8.1 `MarketClient`](#81-marketclient)
   - [8.2 `TradingClient`](#82-tradingclient)
9. [Data Layer Reference — `wdk::data`](#9-data-layer-reference--wdkdata)
   - [9.1 Data Structures](#91-data-structures)
   - [9.2 Conversion Functions](#92-conversion-functions)
10. [Usage Guide](#10-usage-guide)
    - [10.1 Initialization](#101-initialization)
    - [10.2 Market Data](#102-market-data)
    - [10.3 Trading Operations](#103-trading-operations)
    - [10.4 Account Management](#104-account-management)
11. [Error Handling](#11-error-handling)
12. [Build System Reference](#12-build-system-reference)
13. [License](#13-license)

---

## 1. Overview

WDK (**Webull Developer Kit**) is a native C++23 client library for the Webull OpenAPI. It provides fully typed, asynchronous access to market data and trading operations through a three-layer architecture that separates transport, signing, and business logic.

The library was designed with the following principles:

- **Zero dynamic dispatch in the hot path.** All client objects are instantiated by the caller; no virtual tables are used in the core I/O path.
- **Cooperative ownership.** All heavyweight resources (`CurlPool`, `ThreadPool`, `Token`) are managed through `std::unique_ptr` by the consuming application. Clients receive non-owning references.
- **Symmetric async/sync surface.** Every client method has both a blocking synchronous variant and a `std::future`-returning asynchronous variant. The caller selects the scheduling model.
- **Request signing encapsulated from business logic.** HMAC-SHA1 signing, nonce generation, and UTC timestamping are performed inside `wdk::utilities::execute_request` and are never visible to the calling code.

---

## 2. Prerequisites

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

---

## 3. Installation

### 3.1 Building from Source

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

### 3.2 Integration as a Third-Party Library

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

---

## 4. Configuration

### 4.1 Credentials File

The SDK authenticates requests using an application identity issued by Webull. Credentials are stored in a JSON file and loaded at startup via `wdk::core::Credentials`.

Default location: `examples/res/credentials.json`

```json
{
    "id":     "<your-app-id>",
    "key":    "<your-app-key>",
    "secret": "<your-app-secret>"
}
```

| Field | Type | Description |
|---|---|---|
| `id` | `string` | Application identifier (used for `x-app-id` context) |
| `key` | `string` | Public application key (sent as `x-app-key` header) |
| `secret` | `string` | Private signing secret (used for HMAC-SHA1 key derivation) |

> The `secret` field is never transmitted over the network. It is used exclusively to derive the HMAC-SHA1 signing key of the form `<secret>&`.

### 4.2 Token File

Session tokens are persisted to disk to avoid re-authentication on each restart. Once a token has been approved and activated, it is written back to this file automatically.

Default location: `examples/res/token.json`

```json
{
    "token": "<session-token-string>"
}
```

If the stored token fails verification at startup, the SDK will automatically request a new token and block until the user approves the login via the Webull mobile application. The newly activated token is then written back to this file.

### 4.3 API Endpoints

WDK supports both the UAT (test) environment and the production environment. The active endpoint is selected in `application.hpp`:

```cpp
// UAT (test) endpoint
static constexpr std::string_view HOST { "th-api.uat.webullbroker.com" };

// Production endpoint
static constexpr std::string_view HOST { "api.webull.co.th" };
```

> Ensure you are targeting the UAT endpoint for all development and testing. Requests to the production endpoint will execute against live accounts.

---

## 5. Architecture

### 5.1 Layered Design

The codebase is organized into three discrete namespaces, each with a singular responsibility:

```
┌──────────────────────────────────────────────────────────────────────┐
│                           wdk::client                                │
│        TradingClient                   │          MarketClient       │
│  (orders · accounts · instruments)    │  (tick · snapshot · OHLCV)  │
└───────────────┬──────────────────────────────────────┬──────────────┘
                │                                      │
┌───────────────▼──────────────────────────────────────▼──────────────┐
│                          wdk::utilities                              │
│   http (execute_request)  │  openapi (signing)  │  cryptography      │
│   json (read/write)       │  time (UTC stamps)  │                    │
└───────────────┬──────────────────────────────────────────────────────┘
                │
┌───────────────▼──────────────────────────────────────────────────────┐
│                            wdk::core                                 │
│    Credentials  │  Token  │  CurlPool  │  ThreadPool  │  RateLimiter │
└──────────────────────────────────────────────────────────────────────┘
```

**`wdk::core`** — Infrastructure primitives. This layer manages all stateful, long-lived resources: connection pools, worker threads, session tokens, and application credentials. Nothing in this layer is aware of business-domain concepts.

**`wdk::utilities`** — Stateless functional utilities. All HTTP dispatch, HMAC-SHA1 signing, nonce generation, UTC timestamping, and JSON I/O reside here. Functions in this layer are pure or near-pure; they take all required inputs as parameters and return results.

**`wdk::client`** — Business domain clients. `MarketClient` and `TradingClient` hold non-owning references to `wdk::core` primitives, construct typed request objects, and delegate I/O to `wdk::utilities`. This layer contains no I/O logic of its own.

**`wdk::data`** — Typed response model. Structured C++ types representing API responses, and converter functions that parse raw `Response` objects into those types.

### 5.2 Concurrency Model

All I/O operations are dispatched through `wdk::core::ThreadPool`, which maintains a pool of `std::jthread` workers. Each worker processes tasks from a shared `std::queue<std::move_only_function<void()>>`.

Callers receive `std::future<wdk::utilities::Response>` objects. Futures are fulfilled on whichever worker thread executes the corresponding task.

`CurlPool` provides a blocking `acquire()` method: if all handles are in use, the calling thread waits on a `std::condition_variable` until a handle is returned. This provides implicit back-pressure against request bursts that would otherwise saturate the connection pool.

All CURL handles in the pool share a single `CURLSH*` handle configured to cache DNS resolutions and SSL sessions across connections. This eliminates redundant TLS handshakes and DNS round-trips when multiple requests target the same host concurrently.

HTTP/2 multiplexing is enabled per handle via `CURL_HTTP_VERSION_2TLS`. TCP keep-alive is configured with an idle timeout of 60 seconds and a probe interval of 30 seconds.

### 5.3 Authentication Flow

```
Application startup
       │
       ▼
Token::Token(token_path, pool, credentials, host)
       │
       ├── Load token from disk
       │
       ├── Token::verify()
       │       │
       │       ├── is_valid() && status == "NORMAL"  ──► Return (token is active)
       │       │
       │       └── Otherwise ─────────────────────────► Token::generate()
       │                                                      │
       │                                                      ▼
       │                                              POST /openapi/auth/token/create
       │                                                      │
       │                                                      ▼
       │                                              Poll Token::verify() every 5s
       │                                              until status != "PENDING"
       │                                                      │
       │                                                      ▼
       │                                              Persist token to disk
       │
       └── Throw std::runtime_error if token fails to reach "NORMAL"
```

The `Token` constructor is **blocking by design**: the application cannot proceed until a valid, activated session token is available. This guarantees that all subsequent client operations have a usable token handle.

### 5.4 Request Signing

Each outgoing request is signed using the Webull OpenAPI HMAC-SHA1 scheme. The signing procedure is encapsulated in `wdk::utilities::generate_signature` and proceeds as follows:

1. Assemble a canonical parameter set comprising the fixed protocol headers (`host`, `x-app-key`, `x-signature-algorithm`, `x-signature-nonce`, `x-signature-version`, `x-timestamp`) merged with any query string parameters extracted from the request path.
2. Sort the parameter set lexicographically by key.
3. Serialize the sorted set to a `key=value&key=value` string.
4. Prepend the request path to form the sign string: `<path>&<canonical>`.
5. If a request body is present, append `&<MD5(body)>` (uppercase hex).
6. URL-encode the complete sign string using `curl_easy_escape`.
7. Derive the signing key as `<app_secret>&`.
8. Compute `HMAC-SHA1(signing_key, url_encoded_sign_string)` and Base64-encode the result.

The signature is transmitted in the `x-signature` HTTP header. The nonce is a 26-character numeric string generated by a thread-local Mersenne Twister (`std::mt19937_64`) seeded from `std::random_device`.

### 5.5 Project Structure

```
Webull-SDK/
├── include/
│   ├── client/
│   │   ├── market.hpp          # MarketClient declaration
│   │   └── trading.hpp         # TradingClient, OrderRequest, QueryRequest
│   ├── core/
│   │   ├── credentials.hpp     # Credentials
│   │   ├── curl_pool.hpp       # CurlPool
│   │   ├── rate_limiter.hpp    # RateLimiter
│   │   ├── thread_pool.hpp     # ThreadPool
│   │   └── token.hpp           # Token
│   ├── data/
│   │   └── data.hpp            # Typed response structs + converters
│   └── utilities/
│       ├── cryptography.hpp    # HMAC-SHA1, MD5, nonce
│       ├── http.hpp            # execute_request, Response, HttpMethod
│       ├── json.hpp            # read, write, field extractors
│       ├── openapi.hpp         # generate_signature
│       └── time.hpp            # get_utc_timestamp
├── src/
│   ├── client/
│   │   ├── market.cpp
│   │   └── trading.cpp
│   ├── core/
│   │   ├── credentials.cpp
│   │   ├── curl_pool.cpp
│   │   ├── rate_limiter.cpp
│   │   ├── thread_pool.cpp
│   │   └── token.cpp
│   ├── data/
│   │   └── data.cpp
│   └── utilities/
│       ├── cryptography.cpp
│       ├── http.cpp
│       ├── json.cpp
│       ├── openapi.cpp
│       └── time.cpp
├── examples/
│   ├── bin/                    # Compiled demo binary
│   ├── res/
│   │   ├── credentials.json    # Application credentials (not committed)
│   │   └── token.json          # Session token cache (not committed)
│   └── src/
│       └── core/
│           └── application.cpp # Demo application
├── lib/
│   └── nlohmann/               # Bundled nlohmann/json
├── CMakeLists.txt
├── CMakePresets.json
├── build.sh
└── run.sh
```

---

## 6. Core API Reference — `wdk::core`

### 6.1 `Credentials`

**Header:** `<core/credentials.hpp>`

Loads and holds the application identity read from a JSON credentials file. The object is immutable after construction.

```cpp
class Credentials {
public:
    explicit Credentials(const std::filesystem::path& credentials_path);

    [[nodiscard]] const std::string& get_id()     const;
    [[nodiscard]] const std::string& get_key()    const;
    [[nodiscard]] const std::string& get_secret() const;
};
```

**Constructor**

`Credentials(const std::filesystem::path& credentials_path)`

Reads the JSON file at `credentials_path` and extracts the `id`, `key`, and `secret` fields. Logs a critical error and throws `std::runtime_error` if the file cannot be parsed.

**Member Functions**

| Function | Return Type | Description |
|---|---|---|
| `get_id()` | `const std::string&` | Returns the application identifier |
| `get_key()` | `const std::string&` | Returns the public application key |
| `get_secret()` | `const std::string&` | Returns the private signing secret |

`Credentials` is copyable. Copy semantics are meaningful when multiple subsystems require independent access to credential data.

---

### 6.2 `Token`

**Header:** `<core/token.hpp>`

Manages the lifecycle of a Webull API session token. Handles token verification, automatic re-generation when expired, and persistence to disk.

```cpp
class Token {
public:
    Token(
        const std::filesystem::path& token_path,
              CurlPool&              pool,
        const Credentials&           credentials,
        const std::string_view&      host);

    void generate(CurlPool& pool, const Credentials& credentials, const std::string_view& host);
    void verify  (CurlPool& pool, const Credentials& credentials, const std::string_view& host);

    [[nodiscard]] std::string get_handle() const;
    [[nodiscard]] std::string get_status() const;
    [[nodiscard]] bool        is_valid()   const;
};
```

**Constructor**

`Token(token_path, pool, credentials, host)`

This constructor is **blocking**. It performs the following sequence synchronously:

1. Load any existing token from `token_path`.
2. Call `verify()`. If the token is valid and its status is `"NORMAL"`, return immediately.
3. Otherwise, call `generate()` to request a new token from the API.
4. Poll `verify()` every 5 seconds while the status is `"PENDING"`, logging a prompt to approve the login in the Webull mobile application.
5. On successful activation, persist the new token to `token_path`.
6. If activation fails, throw `std::runtime_error`.

`Token` is move-constructible but not copyable.

**Member Functions**

| Function | Return Type | Description |
|---|---|---|
| `generate(pool, credentials, host)` | `void` | Issues a token creation request (`POST /openapi/auth/token/create`) and stores the returned token and status |
| `verify(pool, credentials, host)` | `void` | Issues a token verification request (`POST /openapi/auth/token/check`) and updates the internal status |
| `get_handle()` | `std::string` | Returns the raw token string for use in `x-access-token` headers |
| `get_status()` | `std::string` | Returns the current token status (`"NORMAL"`, `"PENDING"`, etc.) |
| `is_valid()` | `bool` | Returns `true` if the token string is non-empty |

**Token Lifecycle Endpoints**

| Endpoint | Method | Description |
|---|---|---|
| `/openapi/auth/token/create` | POST | Request a new session token |
| `/openapi/auth/token/check` | POST | Verify and refresh the status of an existing token |

---

### 6.3 `CurlPool`

**Header:** `<core/curl_pool.hpp>`

A bounded pool of reusable `CURL*` handles. Handles are reset and reconfigured upon each acquisition. All handles share a common `CURLSH*` for DNS and SSL session caching.

```cpp
class CurlPool {
public:
    using CurlReleaser = std::function<void(CURL*)>;
    using CurlHandle   = std::unique_ptr<CURL, CurlReleaser>;

    explicit CurlPool(size_t pool_size = 10uz);
    ~CurlPool();

    [[nodiscard]] CurlHandle acquire();
};
```

**Constructor**

`CurlPool(size_t pool_size = 10)`

Allocates `pool_size` CURL easy handles and a shared handle configured to cache DNS resolutions and SSL sessions. Logs a critical error for any handle that fails to initialize.

**Member Functions**

| Function | Return Type | Description |
|---|---|---|
| `acquire()` | `CurlHandle` | Blocks until a handle is available, resets and reconfigures it, and returns it wrapped in a `unique_ptr` with an auto-release deleter |

The `CurlHandle` returned by `acquire()` is an `std::unique_ptr<CURL, CurlReleaser>`. When this `unique_ptr` goes out of scope, the deleter calls `CurlPool::release()`, returning the raw handle to the pool and notifying one waiting thread.

Each acquired handle is configured with:

- `CURLOPT_TCP_KEEPALIVE`: enabled
- `CURLOPT_TCP_KEEPIDLE`: 60 seconds
- `CURLOPT_TCP_KEEPINTVL`: 30 seconds
- `CURLOPT_HTTP_VERSION`: `CURL_HTTP_VERSION_2TLS`
- `CURLOPT_SHARE`: the shared `CURLSH*` handle

`CurlPool` is non-copyable and non-movable.

---

### 6.4 `ThreadPool`

**Header:** `<core/thread_pool.hpp>`

A general-purpose, fixed-size task executor. Workers are `std::jthread` instances that consume tasks from a shared queue.

```cpp
class ThreadPool {
public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;
};
```

**Constructor**

`ThreadPool(size_t threads = std::thread::hardware_concurrency())`

Spawns `threads` worker threads. Each thread waits on a `std::condition_variable` until a task is available or a stop is requested.

**Member Functions**

`enqueue(F&& f, Args&&... args) -> std::future<...>`

Wraps the callable `f` and its arguments in a `std::packaged_task`, pushes it onto the task queue, notifies one worker, and returns the associated `std::future`. Throws `std::runtime_error` if called after the pool has been stopped.

**Destructor**

Sets the internal `stop_` flag, notifies all workers via the condition variable, and allows the `std::jthread` destructors to join each worker via their cooperative stop tokens.

`ThreadPool` is non-copyable and non-movable.

---

### 6.5 `RateLimiter`

**Header:** `<core/rate_limiter.hpp>`

A token-bucket rate limiter suitable for enforcing API call quotas. The bucket refills to `max_tokens` at the configured `refill_interval`.

```cpp
class RateLimiter {
public:
    RateLimiter(size_t max_tokens, std::chrono::milliseconds refill_interval);

    void acquire(size_t tokens = 1uz);
};
```

**Constructor**

`RateLimiter(size_t max_tokens, std::chrono::milliseconds refill_interval)`

Initializes the bucket to full capacity. Sets `next_refill_` to `now() + refill_interval`.

**Member Functions**

| Function | Description |
|---|---|
| `acquire(size_t tokens = 1)` | Blocks the calling thread until `tokens` tokens are available. If the bucket is depleted, the thread waits until `next_refill_` and replenishes the bucket to `max_tokens`. |

---

## 7. Utilities API Reference — `wdk::utilities`

### 7.1 HTTP Layer

**Header:** `<utilities/http.hpp>`

#### `enum class HttpMethod`

```cpp
enum class HttpMethod : bool {
    GET  = false,
    POST = true
};
```

#### `struct Response`

```cpp
struct Response {
    long        http_code { 0L };
    std::string message   { "" };
};
```

| Field | Type | Description |
|---|---|---|
| `http_code` | `long` | HTTP response status code, or a `CURLcode` error value on transport failure |
| `message` | `std::string` | Raw response body as received from the server |

#### `execute_request`

```cpp
[[nodiscard]] Response execute_request(
          wdk::core::CurlPool&    pool,
    const wdk::core::Credentials& credentials,
          std::string_view        host,
          std::string_view        path,
          HttpMethod              method,
          std::string_view        body_str = "",
          std::string_view        token    = "");
```

The primary I/O function. Performs the following steps on each invocation:

1. Acquire a handle from `pool` (blocking if all handles are in use).
2. Generate a UTC timestamp and a 26-character numeric nonce.
3. Parse query parameters from `path` to construct the signing parameter set.
4. Compute the HMAC-SHA1 signature via `generate_signature`.
5. Construct the full URL and configure the CURL handle.
6. Build HTTP headers via `generate_headers` (wrapped in a scoped `unique_ptr` for automatic `curl_slist_free_all`).
7. Execute the request with `curl_easy_perform`.
8. On HTTP 429, apply exponential backoff (`500ms × 2^attempt`) and retry up to 3 times.
9. Return the `Response` containing the status code and body.

**Retry Behavior**

| Condition | Action |
|---|---|
| HTTP 429 (Rate Limited) | Backoff 500ms, 1000ms, 2000ms; retry up to 3 times |
| HTTP 200 | Return immediately |
| Any other HTTP code | Return with the received code and body |
| `CURLE_*` error | Return with the CURLcode and a JSON error body |

#### `generate_headers`

```cpp
[[nodiscard]] curl_slist* generate_headers(
    const wdk::core::Credentials& credentials,
          std::string_view        timestamp = "",
          std::string_view        nonce     = "",
          std::string_view        signature = "",
          std::string_view        token     = "");
```

Constructs and returns a `curl_slist*` containing all required Webull API headers. The caller is responsible for freeing this list via `curl_slist_free_all`.

In practice, `execute_request` wraps the raw pointer in a `unique_ptr` with a custom deleter, so callers of the high-level API never manage this memory directly.

**Headers Produced**

| Header | Value |
|---|---|
| `Accept` | `application/json` |
| `Content-Type` | `application/json` |
| `User-Agent` | `WebullBot/1.0 (C++23 Client)` |
| `x-app-key` | `credentials.get_key()` |
| `x-timestamp` | ISO 8601 UTC timestamp |
| `x-signature-version` | `1.0` |
| `x-signature-algorithm` | `HMAC-SHA1` |
| `x-signature-nonce` | 26-character numeric nonce |
| `x-access-token` | Session token (omitted if empty) |
| `x-version` | `v2` |
| `x-signature` | Base64-encoded HMAC-SHA1 signature |

---

### 7.2 Cryptography

**Header:** `<utilities/cryptography.hpp>`

#### `compute_hmac_sha1`

```cpp
[[nodiscard]] std::string compute_hmac_sha1(std::string_view key, std::string_view message);
```

Computes the HMAC-SHA1 of `message` using `key` and returns the result as a Base64-encoded string (no line breaks). Uses OpenSSL's `HMAC()` and a `BIO_f_base64` chain with `BIO_FLAGS_BASE64_NO_NL`.

#### `compute_md5`

```cpp
[[nodiscard]] std::string compute_md5(std::string_view data);
```

Computes the MD5 digest of `data` using OpenSSL's EVP interface and returns the result as an uppercase hexadecimal string. Used to include the request body fingerprint in the signing payload.

#### `generate_nonce`

```cpp
[[nodiscard]] std::string generate_nonce(size_t length = 26uz);
```

Generates a numeric string of `length` digits using a thread-local `std::mt19937_64` seeded from `std::random_device`. Thread-local storage ensures that concurrent threads do not contend on a shared PRNG state.

---

### 7.3 OpenAPI Signing

**Header:** `<utilities/openapi.hpp>`

#### `generate_signature`

```cpp
[[nodiscard]] std::string generate_signature(
          CURL*                                             curl,
          std::string_view                                  app_key,
          std::string_view                                  app_secret,
          std::string_view                                  nonce,
          std::string_view                                  timestamp,
          std::string_view                                  host,
          std::string_view                                  request_path,
    const std::vector<std::pair<std::string, std::string>>& query_params,
          std::string_view                                  request_body);
```

Constructs the canonical signing string and returns a Base64-encoded HMAC-SHA1 signature. A CURL handle is required solely to call `curl_easy_escape` for percent-encoding. The signing procedure is described in [Section 5.4](#54-request-signing).

---

### 7.4 Time

**Header:** `<utilities/time.hpp>`

#### `get_utc_timestamp`

```cpp
[[nodiscard]] std::string get_utc_timestamp();
```

Returns the current UTC time formatted as `YYYY-MM-DDTHH:MM:SSZ` using `std::chrono::utc_clock` and `std::format`. The timestamp is truncated to second precision via `std::chrono::floor<std::chrono::seconds>`.

---

### 7.5 JSON

**Header:** `<utilities/json.hpp>`

#### `read`

```cpp
[[nodiscard]] nlohmann::json read(const std::filesystem::path& input_path);
```

Opens and parses the JSON file at `input_path`. Returns an empty `nlohmann::json` object on failure (file not found, stream error, or parse error) and logs a structured error message. Does not throw.

#### `write`

```cpp
void write(const nlohmann::json& json, const std::filesystem::path& output_path);
```

Serializes `json` to `output_path` with 4-space indentation. Creates parent directories as needed. Logs errors and does not throw.

#### `get_string_from_json`

```cpp
[[nodiscard]] std::string get_string_from_json(const nlohmann::json& json_obj, const char* key);
```

Returns the string value of `key` in `json_obj`, or an empty string if the key is absent or its value is not a string. Logs at debug level on miss.

#### `get_size_t_from_json`

```cpp
[[nodiscard]] size_t get_size_t_from_json(const nlohmann::json& json_obj, const char* key);
```

Returns the `size_t` value of `key`. Handles integer values directly, and string values via `std::from_chars`. Returns `0` on absent, null, or unconvertible values.

---

## 8. Client API Reference — `wdk::client`

### 8.1 `MarketClient`

**Header:** `<client/market.hpp>`

Provides access to real-time and historical market data endpoints. Each operation has a synchronous and an asynchronous variant.

```cpp
class MarketClient {
public:
    MarketClient(
              wdk::core::CurlPool&    pool,
              wdk::core::ThreadPool&  thread_pool,
        const wdk::core::Credentials& credentials,
              std::string_view        host  = "",
              std::string_view        token = "");
};
```

`MarketClient` is non-copyable. It stores non-owning references to `pool`, `thread_pool`, and `credentials`; these objects must outlive the client.

#### `struct MarketRequest`

Defined as a nested type within `MarketClient`:

```cpp
struct MarketRequest {
    std::string            symbol                 { "" };
    std::string            symbols                { "" };
    std::string            category               { "" };
    std::string            timespan               { "" };
    std::optional<size_t>  count                  { std::nullopt };
    std::optional<bool>    real_time_required     { std::nullopt };
    std::string            trading_sessions       { "" };
    std::optional<uint8_t> depth                  { std::nullopt };
    std::optional<bool>    extended_hour_required { std::nullopt };
    std::optional<bool>    overnight_required     { std::nullopt };
};
```

All fields default to empty or `std::nullopt`. Only the fields relevant to a specific endpoint need to be populated; optional fields are omitted from the query string if not set.

**`MarketRequest` Field Reference**

| Field | Type | Description |
|---|---|---|
| `symbol` | `string` | Ticker symbol for single-symbol endpoints, e.g., `"AAPL"`. Used by `fetch_tick_data`, `fetch_quotes_data`, `fetch_historical_bars_data`, and `fetch_footprint_data` |
| `symbols` | `string` | Comma-separated list of ticker symbols for multi-symbol endpoints, e.g., `"AAPL,NVDA,MSFT"`. Used by `fetch_snapshot_data` and `fetch_historical_batch_bars_data` |
| `category` | `string` | Instrument category. Use `"US_STOCK"` for US equities |
| `timespan` | `string` | Bar interval for bar and footprint endpoints. Valid values: `"M1"`, `"M5"`, `"M15"`, `"M30"`, `"H"`, `"D"` |
| `count` | `optional<size_t>` | Maximum number of records to return. Omitted from the request if not set |
| `real_time_required` | `optional<bool>` | When `true`, the response includes the latest incomplete (real-time) bar in addition to closed bars. Applies to bar and footprint endpoints |
| `trading_sessions` | `string` | Session filter. Valid values: `"PRE"`, `"RTH"`, `"ATH"`, `"OVN"` |
| `depth` | `optional<uint8_t>` | Number of order book levels to return. Applies exclusively to `fetch_quotes_data` |
| `extended_hour_required` | `optional<bool>` | When `true`, the snapshot response includes extended-hours price and volume fields. Applies to `fetch_snapshot_data` |
| `overnight_required` | `optional<bool>` | When `true`, the response includes overnight session (`ovn_*`) fields. Applies to `fetch_snapshot_data` and `fetch_quotes_data` |

`symbol` and `symbols` are mutually exclusive by convention: single-symbol endpoints ignore `symbols`, and multi-symbol endpoints ignore `symbol`. Populate only the field appropriate to the target method. Optional fields left at `std::nullopt` are not appended to the query string.


#### Market Data Methods

| Method | Endpoint | HTTP | Description |
|---|---|---|---|
| `fetch_tick_data` | `/openapi/market-data/stock/tick` | GET | Recent tick trades for a single symbol |
| `fetch_snapshot_data` | `/openapi/market-data/stock/snapshot` | GET | Real-time price snapshot for one or more symbols |
| `fetch_quotes_data` | `/openapi/market-data/stock/quotes` | GET | Level 1/2 order book (bid/ask) for a single symbol |
| `fetch_footprint_data` | `/openapi/market-data/stock/footprint` | GET | Volume footprint bars (buy/sell breakdown per price level) |
| `fetch_historical_bars_data` | `/openapi/market-data/stock/bars` | GET | OHLCV bars for a single symbol |
| `fetch_historical_batch_bars_data` | `/openapi/market-data/stock/batch-bars` | POST | OHLCV bars for multiple symbols (comma-separated) |

Each method has a corresponding `*_async` variant that returns `std::future<wdk::utilities::Response>`.

**`symbol` vs `symbols`**

- Methods operating on a single symbol (`fetch_tick_data`, `fetch_quotes_data`, `fetch_historical_bars_data`, `fetch_footprint_data`) use `MarketRequest::symbol`.
- Methods that accept multiple symbols (`fetch_snapshot_data`, `fetch_historical_batch_bars_data`) use `MarketRequest::symbols` as a comma-separated list (e.g., `"AAPL,NVDA"`).

**Timespan Values (for bar/footprint endpoints)**

| Value | Description |
|---|---|
| `"M1"` | 1-minute bars |
| `"M5"` | 5-minute bars |
| `"M15"` | 15-minute bars |
| `"M30"` | 30-minute bars |
| `"H1"` | 1-hour bars |
| `"D1"` | Daily bars |

**Trading Session Values**

| Value | Description |
|---|---|
| `"PRE"` | Pre-market session |
| `"CORE"` | Regular trading session |
| `"POST"` | After-hours session |
| `"ALL_DAY"` | All sessions combined |

---

### 8.2 `TradingClient`

**Header:** `<client/trading.hpp>`

Provides order management, account, and instrument query operations.

```cpp
class TradingClient {
public:
    TradingClient(
              wdk::core::CurlPool&    pool,
              wdk::core::ThreadPool&  thread_pool,
        const wdk::core::Credentials& credentials,
              std::string_view        host,
              std::string_view        token);
};
```

#### `struct OrderRequest`

```cpp
struct OrderRequest {
    std::string           account_id      { "" };
    std::string           combo_type      { "" };
    std::string           client_order_id { "" };
    std::string           instrument_type { "" };
    std::string           market          { "" };
    std::string           symbol          { "" };
    std::string           order_type      { "" };
    std::string           entrust_type    { "" };
    std::string           trading_session { "" };
    std::string           time_in_force   { "" };
    std::string           side            { "" };
    std::optional<double> quantity        { std::nullopt };
    std::optional<double> limit_price     { std::nullopt };
    std::optional<double> stop_price      { std::nullopt };
};
```

**`OrderRequest` Field Reference**

| Field | Type | Description |
|---|---|---|
| `account_id` | `string` | Brokerage account identifier. Obtain via `get_account_id()` |
| `combo_type` | `string` | Order combo type. Use `"NORMAL"` for single-leg orders |
| `client_order_id` | `string` | Client-assigned unique order identifier (nonce). Generate via `wdk::utilities::generate_nonce()` |
| `instrument_type` | `string` | Asset class. Use `"EQUITY"` for stocks |
| `market` | `string` | Market identifier, e.g., `"US"` |
| `symbol` | `string` | Ticker symbol, e.g., `"AAPL"`, `"NVDA"` |
| `order_type` | `string` | `"LIMIT"`, `"MARKET"`, `"STOP"`, `"STOP_LIMIT"` |
| `entrust_type` | `string` | Entrustment type. Use `"QTY"` for quantity-based orders |
| `trading_session` | `string` | `"PRE"`, `"RTH"`, `"ATH"`, `"OVN"` |
| `time_in_force` | `string` | `"DAY"`, `"GTC"`, `"IOC"`, `"FOK"` |
| `side` | `string` | `"BUY"` or `"SELL"` |
| `quantity` | `optional<double>` | Number of shares |
| `limit_price` | `optional<double>` | Limit price. Required for `LIMIT` and `STOP_LIMIT` orders |
| `stop_price` | `optional<double>` | Stop price. Required for `STOP` and `STOP_LIMIT` orders |

Numeric fields `quantity`, `limit_price`, and `stop_price` are serialized as strings in the JSON payload: `quantity` with default precision, `limit_price` and `stop_price` with exactly two decimal places.

#### `struct QueryRequest`

```cpp
struct QueryRequest {
    std::string           symbols            { "" };
    std::string           category           { "" };
    std::string           status             { "" };
    std::string           last_instrument_id { "" };
    std::string           account_id         { "" };
    std::string           start_date         { "" };
    std::optional<size_t> page_size          { std::nullopt };
    std::string           last_client_id     { "" };
    std::string           client_order_id    { "" };
};
```

#### Order Management Methods

| Method | Endpoint | HTTP | Description |
|---|---|---|---|
| `preview_order` | `/openapi/trade/order/preview` | POST | Simulate an order and receive estimated costs without placing it |
| `place_order` | `/openapi/trade/order/place` | POST | Submit an order for execution |
| `modify_order` | `/openapi/trade/order/replace` | POST | Modify the quantity, price, or time-in-force of an open order |
| `cancel_order` | `/openapi/trade/order/cancel` | POST | Cancel an open order by its `client_order_id` |

Each method has a corresponding `*_async` variant.

**`modify_order`** accepts only the mutable fields: `account_id`, `client_order_id`, `quantity`, `limit_price`, `stop_price`, and `time_in_force`. Only fields that are set are included in the modification payload.

**`cancel_order`** requires only `account_id` and `client_order_id`.

#### Order Query Methods

| Method | Endpoint | HTTP | Description |
|---|---|---|---|
| `fetch_stock_instrument` | `/openapi/instrument/stock/list` | GET | Retrieve instrument metadata for one or more symbols |
| `fetch_order_history` | `/openapi/trade/order/history` | GET | Retrieve historical orders for an account |
| `fetch_open_order` | `/openapi/trade/order/open` | GET | Retrieve currently open orders for an account |
| `fetch_order_detail` | `/openapi/trade/order/detail` | GET | Retrieve the details of a specific order by `client_order_id` |

#### Account Methods

| Method | Endpoint | HTTP | Description |
|---|---|---|---|
| `fetch_account_list` | `/openapi/account/list` | GET | Retrieve all brokerage accounts associated with the token |
| `fetch_account_balance` | `/openapi/assets/balance` | GET | Retrieve cash and buying power for an account |
| `fetch_account_position` | `/openapi/assets/positions` | GET | Retrieve open positions for an account |
| `get_account_id()` | — | — | Convenience method: calls `fetch_account_list`, parses the first account ID, caches it, and returns it |

**`get_account_id()`** caches the result internally. The first call issues a network request; subsequent calls return the cached value synchronously.

---

## 9. Data Layer Reference — `wdk::data`

**Header:** `<data/data.hpp>`

### 9.1 Data Structures

#### `TickData`

Represents the most recent trade tick for a single symbol.

| Field | Type | Description |
|---|---|---|
| `symbol` | `string` | Ticker symbol |
| `instrument_id` | `string` | Internal instrument identifier |
| `volume` | `string` | Trade volume |
| `side` | `string` | Trade side (`"BUY"` / `"SELL"`) |
| `trading_sessions` | `string` | Session in which the tick occurred |

#### `SnapshotData`

Represents a full market snapshot for a single instrument, including regular, extended-hours, and overnight session data.

Key fields include: `symbol`, `instrument_id`, `price`, `open`, `close`, `high`, `low`, `volume`, `change`, `change_ratio`, `pre_close`, `last_trade_time`, `ask`, `ask_size`, `bid`, `bid_size`, extended-hour OHLCV and bid/ask fields, and overnight session (`ovn_*`) equivalents.

#### `QuoteLevel`

One level of the order book.

| Field | Type | Description |
|---|---|---|
| `price` | `string` | Price at this level |
| `size` | `string` | Aggregate size at this price |

#### `QuotesData`

The full order book for a single symbol.

| Field | Type | Description |
|---|---|---|
| `symbol` | `string` | Ticker symbol |
| `instrument_id` | `string` | Internal instrument identifier |
| `quote_time` | `size_t` | Unix timestamp (ms) of the quote |
| `asks` | `vector<QuoteLevel>` | Ask side, best price first |
| `bids` | `vector<QuoteLevel>` | Bid side, best price first |

#### `FootPrintBar`

One OHLCV footprint bar with per-price-level buy/sell volume breakdown.

| Field | Type | Description |
|---|---|---|
| `time` | `string` | Bar open time |
| `trading_session` | `string` | Session identifier |
| `total` | `string` | Total volume |
| `delta` | `string` | Buy volume minus sell volume |
| `buy_total` | `string` | Aggregate buy volume |
| `sell_total` | `string` | Aggregate sell volume |
| `buy_detail` | `map<string, string>` | Buy volume keyed by price level |
| `sell_detail` | `map<string, string>` | Sell volume keyed by price level |

#### `FootPrintData`

Collection of `FootPrintBar` objects for one symbol.

#### `Bar`

One OHLCV bar.

| Field | Type | Description |
|---|---|---|
| `time` | `string` | Bar open time |
| `open` | `string` | Opening price |
| `high` | `string` | High price |
| `low` | `string` | Low price |
| `close` | `string` | Closing price |
| `volume` | `string` | Volume |
| `trading_session` | `string` | Session identifier |

#### `HistoricalBarsData`

Collection of `Bar` objects for one symbol.

---

### 9.2 Conversion Functions

All conversion functions accept a `wdk::utilities::Response` by value, parse `response.message` with `nlohmann::json::parse(..., nullptr, false)` (no-throw mode), and return a default-constructed result on parse failure.

| Function | Input | Return Type | Description |
|---|---|---|---|
| `convert_response_to_tick_data` | `Response` | `TickData` | Extracts the first tick from `result[]` array |
| `convert_response_to_snapshot_data` | `Response` | `SnapshotData` | Handles multiple JSON schemas (array root, `result[]`, `data[]`, bare object) |
| `convert_response_to_snapshot_vector` | `Response` | `vector<SnapshotData>` | Batch snapshot conversion supporting the same schema variants |
| `convert_response_to_quotes_data` | `Response` | `QuotesData` | Parses `asks[]` and `bids[]` arrays into `vector<QuoteLevel>` |
| `convert_response_to_footprint_vector` | `Response` | `vector<FootPrintData>` | Parses footprint bars including `buy_detail` and `sell_detail` maps |
| `convert_response_to_historical_bars_vector` | `Response` | `vector<HistoricalBarsData>` | Handles both single-symbol array and multi-symbol `result[]` schemas |

> All monetary and volume values are returned as `std::string` exactly as received from the API. This preserves full precision and avoids floating-point representation issues. Callers requiring numeric operations should use an appropriate decimal arithmetic library.

---

## 10. Usage Guide

### 10.1 Initialization

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

### 10.2 Market Data

#### Tick Data

```cpp
wdk::client::MarketClient market_client(
    *curl_pool, *thread_pool, *credentials, HOST, token->get_handle()
);

std::future<wdk::utilities::Response> future = market_client.fetch_tick_data_async({
    .symbol           { "AAPL" },
    .category         { "US_STOCK" },
    .count            { 2uz },
    .trading_sessions { "PRE" }
});

wdk::utilities::Response response = future.get();

if (response.http_code == 200L) {
    wdk::data::TickData tick = wdk::data::convert_response_to_tick_data(response);
    // Access tick.symbol, tick.volume, tick.side, etc.
}
```

#### Multi-Symbol Snapshot

```cpp
std::future<wdk::utilities::Response> future = market_client.fetch_snapshot_data_async({
    .symbols                { "AAPL,NVDA,MSFT" },
    .category               { "US_STOCK" },
    .extended_hour_required { false },
    .overnight_required     { false }
});

wdk::utilities::Response response = future.get();

if (response.http_code == 200L) {
    std::vector<wdk::data::SnapshotData> snapshots =
        wdk::data::convert_response_to_snapshot_vector(response);

    for (const auto& snap : snapshots) {
        // Access snap.symbol, snap.price, snap.change_ratio, etc.
    }
}
```

#### Level 2 Order Book

```cpp
std::future<wdk::utilities::Response> future = market_client.fetch_quotes_data_async({
    .symbol             { "AAPL" },
    .category           { "US_STOCK" },
    .depth              { 5u },
    .overnight_required { false }
});

wdk::utilities::Response response = future.get();

if (response.http_code == 200L) {
    wdk::data::QuotesData quotes = wdk::data::convert_response_to_quotes_data(response);
    // Access quotes.asks and quotes.bids (vectors of QuoteLevel)
}
```

#### Historical OHLCV Bars — Single Symbol

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

#### Historical OHLCV Bars — Batch (Multiple Symbols)

The batch endpoint uses a POST request with a JSON body. Pass a comma-separated list to `symbols`:

```cpp
std::future<wdk::utilities::Response> future = market_client.fetch_historical_batch_bars_data_async({
    .symbols              { "AAPL,NVDA,TSLA" },
    .category             { "US_STOCK" },
    .timespan             { "D1" },
    .count                { 30uz },
    .real_time_required   { false },
    .trading_sessions     { "CORE" }
});
```

---

### 10.3 Trading Operations

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

### 10.4 Account Management

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

## 11. Error Handling

### HTTP Status Codes

| Code | Meaning | SDK Behavior |
|---|---|---|
| `200` | Success | Return `Response` with body |
| `400` | Bad Request | Return `Response`; error body is logged |
| `401` | Unauthorized | Return `Response`; check token validity and credentials |
| `403` | Forbidden | Return `Response`; check account permissions |
| `404` | Not Found | Return `Response` |
| `429` | Rate Limited | Automatic exponential backoff; retry up to 3 times (500ms / 1000ms / 2000ms) |
| `500+` | Server Error | Return `Response`; error body is logged |

After 3 failed retries due to rate limiting, `execute_request` returns a synthetic `Response` with `http_code = 429` and a JSON error body.

### Response Checking Pattern

All client methods return `wdk::utilities::Response`. The canonical pattern for consuming a response is:

```cpp
wdk::utilities::Response response = /* ... */;

if (response.http_code == 200L) {
    // Parse response.message as JSON or pass to a converter function
    auto json = nlohmann::json::parse(response.message);
    // ...
} else {
    spdlog::error("Request failed with HTTP {}: {}", response.http_code, response.message);
}
```

### Transport Errors

If `curl_easy_perform` returns a non-`CURLE_OK` code, `http_code` is set to the CURLcode value and `message` contains a JSON body of the form `{"error": "curl is nullptr"}` or equivalent. These values are always less than 100, which distinguishes them from valid HTTP status codes.

### Constructor Exceptions

| Constructor | Exception | Condition |
|---|---|---|
| `Credentials` | `std::runtime_error` | JSON file cannot be parsed or fields are missing |
| `Token` | `std::runtime_error` | Token fails to reach `"NORMAL"` status |
| `ThreadPool::enqueue` | `std::runtime_error` | Enqueue called after pool destruction |

---

## 12. Build System Reference

### CMake Presets

WDK ships with `CMakePresets.json` defining the following presets:

| Preset Name | Display Name | Generator | Build Type | Use Case |
|---|---|---|---|---|
| `debug` | GCC Debug (Make) | Make | Debug | Compatibility fallback |
| `debug-ninja` | GCC Debug (Ninja) | Ninja | Debug | **Recommended** for development |
| `release` | GCC Release (Ninja) | Ninja | Release | Production distribution |

All presets inherit from a `base` preset that sets `CC=gcc`, `CXX=g++`, and enables `CMAKE_EXPORT_COMPILE_COMMANDS=ON` for tooling integration (e.g., `clangd`).

Build output is placed under `out/build/<preset-name>/`. The install prefix is `out/install/<preset-name>/`.

### Build Script Reference

`build.sh` is a convenience wrapper around CMake. It performs the following in order:

1. Validates that `cmake` and `ninja` are on `PATH`.
2. Optionally removes the build directory (on `--clean`).
3. Runs `cmake -B <build-dir> -G Ninja -DCMAKE_BUILD_TYPE=<type>`.
4. Runs `cmake --build <build-dir> --parallel <nproc>`.

The script uses strict mode (`set -euo pipefail`) and installs a `trap` to report the exit code on failure.

### Run Script Reference

`run.sh` locates the demo binary at `examples/bin/Webull-SDK-Demo`, checks that it exists and is executable, launches it with any arguments forwarded via `"$@"`, and reports the exit code. It does not re-build; call `build.sh` first.

### Compile Commands Export

`CMAKE_EXPORT_COMPILE_COMMANDS=ON` is set in all presets. This generates `out/build/<preset>/compile_commands.json`, which can be symlinked to the project root for use with `clangd` or other LSP servers:

```sh
ln -sf out/build/debug-ninja/compile_commands.json compile_commands.json
```

---

## 13. License

WDK is released under the MIT License.

```
MIT License

Copyright (c) 2025 Pooh555

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

The full license text is available at: [https://github.com/Pooh555/Webull-SDK/blob/main/LICENSE](https://github.com/Pooh555/Webull-SDK/blob/main/LICENSE)

---

<div align="center">

*WDK — Webull Developer Kit*  
*Authored by Pooh555*

</div>
