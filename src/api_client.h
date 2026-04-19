#pragma once
#include <Arduino.h>

enum class Mood : uint8_t {
    LUCKY, WARNING, CALM, BOLD, LOVE
};

struct Fortune {
    String text;
    Mood   mood = Mood::CALM;
};

class ApiClient {
public:
    bool connectWiFi();
    // Returns true on success; fills `out`.
    // On parse/network failure, returns false.
    bool fetchFortune(Fortune& out);

private:
    String buildUserPrompt();
    Mood   parseMood(const char* s);
};
