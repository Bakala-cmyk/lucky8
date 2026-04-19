#pragma once
#include <Arduino.h>

class DisplayManager {
public:
    void begin();
    void showIdle();
    void showConnecting();
    void showLoading();
    void showMessage(const String& msg);
    void showError(const String& msg);

private:
    void clearScreen();
    void printWrapped(const String& text, int16_t x, int16_t y, uint8_t font, uint16_t color);
};
