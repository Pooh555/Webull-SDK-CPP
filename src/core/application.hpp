#pragma once

#include "curl.hpp"
#include "token.hpp"
#include "secret/secret.hpp"

#include <memory>


class Application {
public:
    Application();
    ~Application();

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    void run();
private:
    const std::string SECRET_PATH = "/home/Pooh555/programming/Webull-Trading-Bot/secret.json";

    std::unique_ptr<Curl>   curl;
    std::unique_ptr<Secret> secret;
    std::unique_ptr<Token>  token;
};