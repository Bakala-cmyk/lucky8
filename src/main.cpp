#include <M5StickCPlus.h>
#include <WiFi.h>
#include "config.h"
#include "state_machine.h"
#include "shake_detector.h"
#include "api_client.h"
#include "display_manager.h"
#include "sound_player.h"
#include "animations.h"

static StateMachine  sm;
static ShakeDetector shaker;
static ApiClient     api;
static Renderer      renderer;
static SoundPlayer   sound;

static Animation* currentAnim = nullptr;
static String     lastError;

// Worker task plumbing: run WiFi connect / API fetch on core 0
// so the animation loop on core 1 keeps ticking.
static TaskHandle_t  workerHandle = nullptr;
static volatile int  workerResult = -1;  // -1 running, 0 fail, 1 ok
static Fortune       workerFortune;

static void connectTask(void*) {
    bool ok = api.connectWiFi();
    workerResult = ok ? 1 : 0;
    workerHandle = nullptr;
    vTaskDelete(nullptr);
}

static void fetchTask(void*) {
    Fortune f;
    bool ok = api.fetchFortune(f);
    if (ok) workerFortune = f;
    workerResult = ok ? 1 : 0;
    workerHandle = nullptr;
    vTaskDelete(nullptr);
}

static void startWorker(TaskFunction_t fn, const char* name) {
    if (workerHandle != nullptr) return;
    workerResult = -1;
    xTaskCreatePinnedToCore(fn, name, 8192, nullptr, 1, &workerHandle, 0);
}

static void setState(AppState next) {
    sm.transition(next);

    if (currentAnim) { delete currentAnim; currentAnim = nullptr; }
    sound.stop();

    switch (next) {
        case AppState::IDLE:
            currentAnim = new IdleAnim();
            break;
        case AppState::CONNECTING:
            currentAnim = new ConnectingAnim();
            startWorker(connectTask, "conn");
            break;
        case AppState::LOADING:
            currentAnim = new LoadingAnim();
            startWorker(fetchTask, "fetch");
            break;
        case AppState::DISPLAYING:
            currentAnim = new DisplayingAnim(workerFortune, MSG_DISPLAY_SECONDS);
            break;
        case AppState::ERROR:
            currentAnim = new ErrorAnim(lastError);
            break;
    }
    if (currentAnim) currentAnim->enter(renderer, sound);
}

void setup() {
    Serial.begin(115200);
    delay(50);
    M5.begin();
    M5.Imu.Init();
    randomSeed(esp_random());

    renderer.begin();
    sound.begin();

    setState(AppState::IDLE);
    Serial.println("[Boot] Ready — shake or press BtnA");
}

void loop() {
    M5.update();
    uint32_t now = millis();

    // ---- State transitions ----
    switch (sm.current()) {
        case AppState::IDLE:
            if (shaker.update() || M5.BtnA.wasPressed()) setState(AppState::CONNECTING);
            break;

        case AppState::CONNECTING: {
            auto* c = static_cast<ConnectingAnim*>(currentAnim);
            if (workerResult == 1) c->triggerCaught(sound);
            else if (workerResult == 0) {
                lastError = "WiFi failed.";
                setState(AppState::ERROR);
                break;
            }
            if (c->readyToAdvance()) setState(AppState::LOADING);
            break;
        }

        case AppState::LOADING:
            if (workerResult == 1) {
                setState(AppState::DISPLAYING);
            } else if (workerResult == 0) {
                lastError = "API error.";
                setState(AppState::ERROR);
            }
            break;

        case AppState::DISPLAYING: {
            auto* d = static_cast<DisplayingAnim*>(currentAnim);
            if (M5.BtnA.wasPressed()) d->skipToEat();
            if (d->done()) { WiFi.disconnect(true); setState(AppState::IDLE); }
            break;
        }

        case AppState::ERROR:
            if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed()) {
                WiFi.disconnect(true);
                setState(AppState::IDLE);
            }
            break;
    }

    // ---- Animation + sound tick ----
    if (currentAnim) currentAnim->update(renderer, sound, now);
    sound.update();

    // ---- Frame pacing @ ~30fps ----
    static uint32_t lastFrame = 0;
    uint32_t elapsed = millis() - lastFrame;
    if (elapsed < 33) delay(33 - elapsed);
    lastFrame = millis();
}
