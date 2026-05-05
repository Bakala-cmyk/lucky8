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
    // Truth-or-Dare (truth only) question for the given gender.
    // Reuses Fortune.text for the question; mood is set to a fitting mascot.
    bool fetchTruthQuestion(bool isMale, Fortune& out);

private:
    String buildUserPrompt();
    Mood   parseMood(const char* s);
    // Shared HTTP/JSON request runner. Returns true and fills out.text/mood
    // on success. If expectMood=false, treats response as {text:"..."} only.
    bool doChat(const char* sysPrompt, const String& userPrompt,
                bool expectMood, Fortune& out);
};
