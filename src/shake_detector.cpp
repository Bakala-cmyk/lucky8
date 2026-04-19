#include "shake_detector.h"
#include <M5StickCPlus.h>
#include "config.h"

static uint32_t _shakeStartMs  = 0;
static bool     _inMotion      = false;
static uint32_t _lastTriggerMs = 0;

bool ShakeDetector::update() {
    float ax, ay, az;
    M5.Imu.getAccelData(&ax, &ay, &az);

    float magnitude = sqrtf(ax * ax + ay * ay + az * az);
    float excess    = fabsf(magnitude - 1.0f);

    uint32_t now = millis();

    if (now - _lastTriggerMs < SHAKE_COOLDOWN_MS) return false;

    if (excess > SHAKE_THRESHOLD) {
        if (!_inMotion) {
            _inMotion     = true;
            _shakeStartMs = now;
        } else if (now - _shakeStartMs >= SHAKE_DURATION_MS) {
            _inMotion      = false;
            _lastTriggerMs = now;
            Serial.println("[Shake] Confirmed shake!");
            return true;
        }
    } else {
        _inMotion = false;
    }
    return false;
}
