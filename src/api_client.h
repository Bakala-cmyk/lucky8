#pragma once
#include <Arduino.h>

class ApiClient {
public:
    bool   connectWiFi();
    String fetchFortune();

private:
    String buildPrompt();
};
