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
        if (millis() - start > 15000) {
            Serial.println("[WiFi] Timeout");
            return false;
        }
        delay(250);
    }
    Serial.printf("[WiFi] Connected, IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
}

String ApiClient::buildPrompt() {
    const char* prompts[] = {
        "Give me today's fortune.",
        "Inspire me for today.",
        "What's my life advice for today?",
        "Give me a powerful motto for this moment.",
    };
    return String(prompts[random(0, 4)]);
}

String ApiClient::fetchFortune() {
    WiFiClientSecure wcs;
    wcs.setInsecure();  // personal device — skip cert pinning

    HTTPClient http;
    http.useHTTP10(true);  // avoid chunked transfer for stream parsing

    String url = String("https://") + OPENAI_API_HOST + OPENAI_API_PATH;
    if (!http.begin(wcs, url)) {
        Serial.println("[API] http.begin failed");
        return "";
    }

    http.addHeader("Content-Type",  "application/json");
    http.addHeader("Authorization", String("Bearer ") + OPENAI_API_KEY);

    // Build request
    JsonDocument req;
    req["model"]      = OPENAI_MODEL;
    req["max_tokens"] = 60;

    JsonArray messages = req["messages"].to<JsonArray>();

    JsonObject sys = messages.add<JsonObject>();
    sys["role"]    = "system";
    sys["content"] = "You are a fortune-teller life coach. "
                     "Reply with ONE motivational sentence under 20 words. "
                     "Be vivid, poetic, and inspiring. No quotes, no labels.";

    JsonObject usr = messages.add<JsonObject>();
    usr["role"]    = "user";
    usr["content"] = buildPrompt();

    String body;
    serializeJson(req, body);
    Serial.printf("[API] POST body: %s\n", body.c_str());

    int code = http.POST(body);
    Serial.printf("[API] HTTP %d\n", code);
    Serial.printf("[API] Free heap: %u\n", ESP.getFreeHeap());

    if (code != 200) {
        Serial.printf("[API] Error body: %s\n", http.getString().c_str());
        http.end();
        return "";
    }

    JsonDocument resp;
    DeserializationError err = deserializeJson(resp, http.getStream());
    http.end();

    if (err) {
        Serial.printf("[API] JSON parse error: %s\n", err.c_str());
        return "";
    }

    String result = resp["choices"][0]["message"]["content"].as<String>();
    Serial.printf("[API] Fortune: %s\n", result.c_str());
    return result;
}
