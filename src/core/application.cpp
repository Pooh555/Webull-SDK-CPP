#include "application.hpp"

Application::Application() {
    curl   = std::make_unique<Curl>();
    secret = std::make_unique<Secret>(SECRET_PATH);
    token  = std::make_unique<Token>();
}

Application::~Application() {}

void Application::run() {
    token->generate(curl->get_handle(), *secret.get());
    token->verify(curl->get_handle(), *secret.get());
}