#include <M5StickCPlus.h>
#include <WiFi.h>
#include "config.h"
#include "state_machine.h"
#include "shake_detector.h"
#include "api_client.h"
#include "display_manager.h"
#include "sound_player.h"
#include "animations.h"

enum class Mode   { FORTUNE, TRUTH };
enum class Gender { MALE, FEMALE };

static StateMachine  sm;
static ShakeDetector shaker;
static ApiClient     api;
static Renderer      renderer;
static SoundPlayer   sound;

static Animation* currentAnim = nullptr;
static String     lastError;
static Mode       g_mode   = Mode::FORTUNE;
static Gender     g_gender = Gender::MALE;
static ResponseLanguage g_lang = ResponseLanguage::CHINESE;

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
    bool ok = (g_mode == Mode::TRUTH)
                ? api.fetchTruthQuestion(g_gender == Gender::MALE, g_lang, f)
                : api.fetchFortune(g_lang, f);
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
        case AppState::LANGUAGE_SELECT:
            currentAnim = new LanguageSelectAnim();
            break;
        case AppState::MODE_SELECT:
            currentAnim = new ModeSelectAnim();
            break;
        case AppState::IDLE:
            currentAnim = new IdleAnim();
            break;
        case AppState::CONNECTING:
            currentAnim = new ConnectingAnim();
            startWorker(connectTask, "conn");
            break;
        case AppState::GENDER_PICK:
            currentAnim = new GenderPickAnim();
            break;
        case AppState::LOADING:
            currentAnim = new LoadingAnim();
            startWorker(fetchTask, "fetch");
            break;
        case AppState::DISPLAYING:
            currentAnim = new DisplayingAnim(workerFortune);
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

    setState(AppState::LANGUAGE_SELECT);
    Serial.println("[Boot] Ready — pick language (A=Chinese, B=English)");
}

void loop() {
    M5.update();
    uint32_t now = millis();

    // ---- Global mute toggle (BtnB) — disabled in selection screens where B is an option ----
    if (sm.current() != AppState::LANGUAGE_SELECT &&
        sm.current() != AppState::MODE_SELECT &&
        M5.BtnB.wasPressed()) {
        bool muted = sound.toggleMuted();
        Serial.printf("[Mute] %s\n", muted ? "ON" : "OFF");
    }

    // ---- State transitions ----
    switch (sm.current()) {
        case AppState::LANGUAGE_SELECT:
            if (M5.BtnA.wasPressed()) {
                g_lang = ResponseLanguage::CHINESE;
                setAnimationLanguage(g_lang);
                Serial.println("[Language] CHINESE");
                setState(AppState::MODE_SELECT);
            } else if (M5.BtnB.wasPressed()) {
                g_lang = ResponseLanguage::ENGLISH;
                setAnimationLanguage(g_lang);
                Serial.println("[Language] ENGLISH");
                setState(AppState::MODE_SELECT);
            }
            break;

        case AppState::MODE_SELECT:
            if (M5.BtnA.wasPressed()) {
                g_mode = Mode::FORTUNE;
                Serial.println("[Mode] FORTUNE");
                setState(AppState::IDLE);
            } else if (M5.BtnB.wasPressed()) {
                g_mode = Mode::TRUTH;
                Serial.println("[Mode] TRUTH");
                setState(AppState::IDLE);
            }
            break;

        case AppState::IDLE:
            if (shaker.update() || M5.BtnA.wasPressed()) setState(AppState::CONNECTING);
            break;

        case AppState::CONNECTING: {
            auto* c = static_cast<ConnectingAnim*>(currentAnim);
            if (workerResult == 1) c->triggerCaught(sound);
            else if (workerResult == 0) {
                lastError = (g_lang == ResponseLanguage::CHINESE) ? "WiFi失败" : "WiFi failed.";
                setState(AppState::ERROR);
                break;
            }
            if (c->readyToAdvance()) {
                if (g_mode == Mode::TRUTH) setState(AppState::GENDER_PICK);
                else                       setState(AppState::LOADING);
            }
            break;
        }

        case AppState::GENDER_PICK: {
            auto* g = static_cast<GenderPickAnim*>(currentAnim);
            if (g->locked() && M5.BtnA.wasPressed()) g->advance();
            if (g->done()) {
                g_gender = g->resultIsMale() ? Gender::MALE : Gender::FEMALE;
                Serial.printf("[Gender] %s\n", g->resultIsMale() ? "MALE" : "FEMALE");
                setState(AppState::LOADING);
            }
            break;
        }

        case AppState::LOADING:
            if (workerResult == 1) {
                setState(AppState::DISPLAYING);
            } else if (workerResult == 0) {
                lastError = (g_lang == ResponseLanguage::CHINESE) ? "接口错误" : "API error.";
                setState(AppState::ERROR);
            }
            break;

        case AppState::DISPLAYING: {
            auto* d = static_cast<DisplayingAnim*>(currentAnim);
            // Report A-held state every frame so SCROLL only progresses while held.
            d->setScrollHeld(M5.BtnA.isPressed());

            // Self-tracked press timing — robust against frame-poll race that can
            // happen if the user releases just past the threshold (the M5 pressedFor
            // poll may miss it between frames and misclassify as a short press).
            static uint32_t pressStartMs = 0;
            static bool     aHoldFired   = false;
            constexpr uint32_t LONG_MS   = 300;

            if (M5.BtnA.wasPressed()) {
                pressStartMs = now;
                aHoldFired   = false;
            }
            if (M5.BtnA.isPressed() && !aHoldFired && (now - pressStartMs) >= LONG_MS) {
                aHoldFired = true;
                d->onLongPress();
            }
            if (M5.BtnA.wasReleased()) {
                uint32_t held = now - pressStartMs;
                if (held < LONG_MS) d->onShortPress();
                aHoldFired = false;
            }
            if (d->done()) { WiFi.disconnect(true); setState(AppState::MODE_SELECT); }
            break;
        }

        case AppState::ERROR:
            if (M5.BtnA.wasPressed()) {
                WiFi.disconnect(true);
                setState(AppState::MODE_SELECT);
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
