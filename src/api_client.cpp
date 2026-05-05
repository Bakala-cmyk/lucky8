#include "api_client.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

bool ApiClient::connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return true;
    Serial.printf("[WiFi] Connecting to %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > 15000) { Serial.println("[WiFi] Timeout"); return false; }
        delay(250);
    }
    Serial.printf("[WiFi] Connected, IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
}

String ApiClient::buildUserPrompt() {
    const char* prompts[] = {
        "Give me today's fortune.",
        "Inspire me for today.",
        "What's my life advice right now?",
        "Give me a powerful motto for this moment.",
        "What kind of day should I expect?",
        "Tell me a secret of the universe.",
        "Share a surprising bit of wisdom.",
        "What should I remember today?",
        "Reveal a small truth that could change my day.",
        "What is the vibe of this hour?",
        "Bless me with a short oracle.",
        "Deliver a verse only I needed to hear.",
        "Pass along a cosmic nudge.",
        "Whisper a strange little courage.",
        "Give me one line that hits like lightning.",
    };
    const int N = sizeof(prompts) / sizeof(prompts[0]);
    String p = String(prompts[random(0, N)]);
    // Append a random nonce to defeat any server-side response cache and
    // nudge the model toward a different sample each call.
    p += " [seed:";
    p += (uint32_t)esp_random();
    p += "]";
    return p;
}

Mood ApiClient::parseMood(const char* s) {
    if (!s) return Mood::CALM;
    if      (!strcasecmp(s, "lucky"))   return Mood::LUCKY;
    else if (!strcasecmp(s, "warning")) return Mood::WARNING;
    else if (!strcasecmp(s, "calm"))    return Mood::CALM;
    else if (!strcasecmp(s, "bold"))    return Mood::BOLD;
    else if (!strcasecmp(s, "love"))    return Mood::LOVE;
    return Mood::CALM;
}

bool ApiClient::doChat(const char* sysPrompt, const String& userPrompt,
                       bool expectMood, Fortune& out) {
    WiFiClientSecure wcs;
    wcs.setInsecure();

    HTTPClient http;
    http.useHTTP10(true);
    http.setTimeout(15000);

    String url = String("https://") + OPENAI_API_HOST + OPENAI_API_PATH;
    if (!http.begin(wcs, url)) { Serial.println("[API] http.begin failed"); return false; }

    http.addHeader("Content-Type",  "application/json");
    http.addHeader("Authorization", String("Bearer ") + OPENAI_API_KEY);

    JsonDocument req;
    req["model"]       = OPENAI_MODEL;
    req["max_tokens"]  = 100;
    req["temperature"] = 1.2;
    JsonObject rf = req["response_format"].to<JsonObject>();
    rf["type"] = "json_object";

    JsonArray messages = req["messages"].to<JsonArray>();
    JsonObject sys = messages.add<JsonObject>();
    sys["role"]    = "system";
    sys["content"] = sysPrompt;
    JsonObject usr = messages.add<JsonObject>();
    usr["role"]    = "user";
    usr["content"] = userPrompt;

    String body;
    serializeJson(req, body);
    Serial.printf("[API] POST body: %s\n", body.c_str());

    int code = http.POST(body);
    Serial.printf("[API] HTTP %d | free heap: %u\n", code, ESP.getFreeHeap());

    if (code != 200) {
        Serial.printf("[API] Error body: %s\n", http.getString().c_str());
        http.end();
        return false;
    }

    JsonDocument resp;
    DeserializationError err = deserializeJson(resp, http.getStream());
    http.end();
    if (err) { Serial.printf("[API] outer JSON parse error: %s\n", err.c_str()); return false; }

    const char* content = resp["choices"][0]["message"]["content"];
    if (!content) { Serial.println("[API] no content field"); return false; }
    Serial.printf("[API] content: %s\n", content);

    JsonDocument inner;
    err = deserializeJson(inner, content);
    if (err) {
        Serial.printf("[API] inner JSON parse error: %s — falling back to raw text\n", err.c_str());
        out.text = String(content);
        return out.text.length() > 0;
    }

    if (expectMood) out.mood = parseMood(inner["mood"] | "calm");
    const char* txt = inner["text"] | "";
    out.text = String(txt);
    out.text.trim();
    if (out.text.length() == 0) return false;

    Serial.printf("[API] mood=%d text=%s\n", (int)out.mood, out.text.c_str());
    return true;
}

bool ApiClient::fetchFortune(Fortune& out) {
    const char* sys =
        "You are a fortune-teller life coach for an arcade-style oracle device. "
        "Return STRICT JSON only, no markdown, no code fences, no extra text. "
        "Schema: {\"mood\":\"<one of: lucky, warning, calm, bold, love>\",\"text\":\"<one motivational sentence, max 18 words, no quotes>\"}. "
        "Pick mood to match the vibe of the text. Keep text vivid and poetic. "
        "Each call must produce a FRESH, DIFFERENT sentence — never repeat phrasing, "
        "imagery, or themes from generic fortune-cookie templates. Surprise the reader.";
    return doChat(sys, buildUserPrompt(), /*expectMood=*/true, out);
}

bool ApiClient::fetchTruthQuestion(bool isMale, Fortune& out) {
    const char* sys =
        "You generate Truth-or-Dare questions for a portable arcade device, "
        "but ONLY 'truth' questions (never dare). Return STRICT JSON only, no "
        "markdown, no code fences, no extra text. Schema: {\"text\":\"<one truth "
        "question, max 22 words, no quotes>\"}. The question is for casual play "
        "between male and female friends to warm up conversation — flirty-but-"
        "respectful is welcome, can be playful or mildly spicy, but never "
        "explicit/NSFW. Each call must produce a FRESH, DIFFERENT question — "
        "no clichés, surprise the reader.";
    String userPrompt = String("The player just spun and got: ") +
                        (isMale ? "MALE" : "FEMALE") +
                        ". Ask one truth question tailored to that. [seed:" +
                        (uint32_t)esp_random() + "]";
    // Default mascot mood: BOLD for male, LOVE for female.
    out.mood = isMale ? Mood::BOLD : Mood::LOVE;
    return doChat(sys, userPrompt, /*expectMood=*/false, out);
}
