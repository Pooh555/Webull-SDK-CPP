#include <core/curl_pool.hpp>

#include <spdlog/spdlog.h>

namespace wdk::core {

CurlPool::CurlPool(size_t pool_size) {
    for (size_t i { 0uz }; i < pool_size; ++i) {
        CURL* handle = curl_easy_init();

        if (handle) {
            handles_.push(handle);
        } else {
            spdlog::critical("[CurlPool] Failed to initialize a CURL handle");
        }
    }
    
    spdlog::info("[CurlPool] Successfully initialized connection pool with {} handles", handles_.size());
}

CurlPool::~CurlPool() {
    std::lock_guard<std::mutex> lock(mutex_);

    shutdown_ = true;

    condition_.notify_all();

    while (!handles_.empty()) {
        curl_easy_cleanup(handles_.front());
        handles_.pop();
    }

    spdlog::debug("[CurlPool] Cleaned up all CURL handles");
}

CurlPool::CurlHandle CurlPool::acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    condition_.wait(lock, [this] { return !handles_.empty() || shutdown_; });

    if (shutdown_ || handles_.empty()) {
        return CurlHandle(nullptr, [](CURL*) {});
    }

    CURL* handle = handles_.front();
    handles_.pop();

    curl_easy_reset(handle);

    return CurlHandle(handle, [this](CURL* h) { this->release(h); });
}

void CurlPool::release(CURL* handle) {
    if (!handle) return;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        handles_.push(handle);
    }
    
    condition_.notify_one();
}

}