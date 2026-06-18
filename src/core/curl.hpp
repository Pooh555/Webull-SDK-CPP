#pragma once

#include <curl/curl.h>

class Curl {
    public:
        Curl();
        ~Curl();

        Curl(const Curl&)            = delete;
        Curl& operator=(const Curl&) = delete;

        [[nodiscard]] CURL* get_handle() const { return curl; }
    private:
        CURL* curl = nullptr;
};