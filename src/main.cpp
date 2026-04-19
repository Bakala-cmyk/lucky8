#include <M5StickCPlus.h>
#include <WiFi.h>
#include "config.h"
#include "state_machine.h"
#include "shake_detector.h"
#include "api_client.h"
#include "display_manager.h"

static StateMachine  sm;
static ShakeDetector shaker;
static ApiClient     api;
static DisplayManager display;

static String   currentMessage;
static uint32_t displayStartMs = 0;

void setup() {
    Serial.begin(115200);
    M5.begin();
    M5.Imu.Init();
    randomSeed(esp_random());

    display.begin();
    display.showIdle();
    Serial.println("[Boot] Ready — shake or press BtnA");
}

void loop() {
    M5.update();

    switch (sm.current()) {

    case AppState::IDLE:
        if (shaker.update() || M5.BtnA.wasPressed()) {
            sm.transition(AppState::CONNECTING);
            display.showConnecting();
        }
        delay(20);
        break;

    case AppState::CONNECTING:
        if (api.connectWiFi()) {
            sm.transition(AppState::LOADING);
            display.showLoading();
        } else {
            sm.transition(AppState::ERROR);
            display.showError("WiFi failed.\nCheck SSID/password.");
        }
        break;

    case AppState::LOADING:
        currentMessage = api.fetchFortune();
        if (currentMessage.length() > 0) {
            sm.transition(AppState::DISPLAYING);
            display.showMessage(currentMessage);
            displayStartMs = millis();
        } else {
            sm.transition(AppState::ERROR);
            display.showError("API error.\nCheck key or network.");
        }
        break;

    case AppState::DISPLAYING:
        if (M5.BtnA.wasPressed() ||
            millis() - displayStartMs > (uint32_t)(MSG_DISPLAY_SECONDS * 1000UL)) {
            WiFi.disconnect(true);
            sm.transition(AppState::IDLE);
            display.showIdle();
        }
        delay(100);
        break;

    case AppState::ERROR:
        if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed()) {
            WiFi.disconnect(true);
            sm.transition(AppState::IDLE);
            display.showIdle();
        }
        delay(100);
        break;
    }
}
