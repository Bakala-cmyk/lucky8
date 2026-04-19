#include "display_manager.h"
#include <M5StickCPlus.h>

// Landscape 240x135, Font2 = 16px, ~14 chars per 240px line at scale 1
static const uint8_t FONT_SIZE  = 2;
static const int16_t LINE_H     = 18;  // pixel height per line for Font2
static const int16_t MARGIN     = 4;
static const int16_t MAX_WIDTH  = 232; // 240 - 2*MARGIN

void DisplayManager::begin() {
    M5.Lcd.setRotation(3);   // landscape: 240 wide x 135 tall
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setTextWrap(false);
}

void DisplayManager::clearScreen() {
    M5.Lcd.fillScreen(TFT_BLACK);
}

void DisplayManager::showIdle() {
    clearScreen();
    M5.Lcd.setTextFont(FONT_SIZE);
    M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Lcd.setTextSize(1);

    // Center "Shake me!" vertically and horizontally
    const char* line1 = "~ Shake me! ~";
    const char* line2 = "Life Coach";
    int16_t x1 = (240 - strlen(line1) * 14) / 2;
    int16_t x2 = (240 - strlen(line2) * 14) / 2;
    M5.Lcd.setCursor(x1, 45);
    M5.Lcd.print(line1);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.setCursor(x2, 70);
    M5.Lcd.print(line2);
}

void DisplayManager::showConnecting() {
    clearScreen();
    M5.Lcd.setTextFont(FONT_SIZE);
    M5.Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(MARGIN, 55);
    M5.Lcd.print("Connecting WiFi...");
}

void DisplayManager::showLoading() {
    clearScreen();
    M5.Lcd.setTextFont(FONT_SIZE);
    M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(MARGIN, 55);
    M5.Lcd.print("Asking the cosmos...");
}

void DisplayManager::showMessage(const String& msg) {
    clearScreen();
    M5.Lcd.setTextFont(FONT_SIZE);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.setTextSize(1);
    printWrapped(msg, MARGIN, MARGIN + LINE_H, FONT_SIZE, TFT_WHITE);
}

void DisplayManager::showError(const String& msg) {
    clearScreen();
    M5.Lcd.setTextFont(FONT_SIZE);
    M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(MARGIN, MARGIN);
    M5.Lcd.print("Error:");
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    printWrapped(msg, MARGIN, MARGIN + LINE_H + 4, FONT_SIZE, TFT_WHITE);
    M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Lcd.setCursor(MARGIN, 115);
    M5.Lcd.print("Press A to retry");
}

// Word-wrap text by splitting on spaces, fitting within MAX_WIDTH pixels.
void DisplayManager::printWrapped(const String& text, int16_t x, int16_t y,
                                   uint8_t font, uint16_t color) {
    M5.Lcd.setTextFont(font);
    M5.Lcd.setTextColor(color, TFT_BLACK);

    // Approximate char width for Font2 size1 = ~14px per character
    const int16_t charW = 14;
    int16_t       curX  = x;
    int16_t       curY  = y;

    String word  = "";
    String token = "";

    for (int i = 0; i <= (int)text.length(); i++) {
        char c = (i < (int)text.length()) ? text[i] : ' ';

        if (c == ' ' || c == '\n' || i == (int)text.length()) {
            // Check if word fits on current line
            int16_t wordW = token.length() * charW;
            if (curX + wordW > MAX_WIDTH) {
                curY += LINE_H;
                curX  = x;
                if (curY > 125) break;  // off screen
            }
            M5.Lcd.setCursor(curX, curY);
            M5.Lcd.print(token);
            curX += wordW + charW;  // +charW for the space
            token = "";
            if (c == '\n') { curY += LINE_H; curX = x; }
        } else {
            token += c;
        }
    }
}
