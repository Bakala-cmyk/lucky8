#pragma once
#include <Arduino.h>

class ShakeDetector {
public:
    // Call every ~20ms in loop(). Returns true once per confirmed shake.
    bool update();
};
