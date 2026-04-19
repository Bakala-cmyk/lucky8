#pragma once
#include <Arduino.h>
#include <M5StickCPlus.h>

class Renderer {
public:
    static constexpr int16_t W = 240;
    static constexpr int16_t H = 135;

    void begin();
    void clear(uint16_t color);
    void present();                 // push buffer to LCD

    // --- Primitives (operate on framebuffer) ---
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void text(int16_t x, int16_t y, const char* s, uint16_t color, uint8_t font = 2);
    void textCentered(int16_t cy, const char* s, uint16_t color, uint8_t font = 2);
    int16_t textWidth(const char* s, uint8_t font = 2);

    // --- Pac-Man art ---
    // dir: 0 right, 1 left, 2 up, 3 down
    void drawPacman(int16_t cx, int16_t cy, int16_t r, int dir, bool mouthOpen,
                    uint16_t color = 0xFFE0 /* yellow */);
    void drawGhost(int16_t cx, int16_t cy, int16_t r, uint16_t body, uint16_t pupil,
                   int eyeDir /* 0 right, 1 left, 2 fwd */);
    void drawPellet(int16_t cx, int16_t cy);
    void drawPowerPellet(int16_t cx, int16_t cy, bool lit);
    void drawCherry(int16_t cx, int16_t cy);
    // "+200" bonus text in yellow arcade style
    void drawBonus(int16_t cx, int16_t cy, const char* s);
    // death animation frame 0..5 at (cx,cy) with radius r
    void drawDeathFrame(int16_t cx, int16_t cy, int16_t r, int frame);

    TFT_eSprite& sprite() { return _spr; }

private:
    TFT_eSprite _spr{&M5.Lcd};
};
