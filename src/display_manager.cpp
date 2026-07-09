#include "display_manager.h"

static constexpr uint16_t C_BLACK  = 0x0000;
static constexpr uint16_t C_WHITE  = 0xFFFF;
static constexpr uint16_t C_YELLOW = 0xFFE0;
static constexpr uint16_t C_RED    = 0xF800;
static constexpr uint16_t C_GREEN  = 0x07E0;

void Renderer::begin() {
    M5.Lcd.setRotation(3);
    M5.Lcd.fillScreen(C_BLACK);
    _spr.setColorDepth(16);
    _spr.createSprite(W, H);
    _spr.fillSprite(C_BLACK);
    _spr.setTextWrap(false);

    _u8f.begin(_spr);
    _u8f.setFont(u8g2_font_wqy16_t_gb2312b);
    _u8f.setFontMode(1);
    _u8f.setFontDirection(0);
    _utf8Ascent = _u8f.getFontAscent();
    _utf8LineH  = _u8f.getFontAscent() - _u8f.getFontDescent() + 2;
}

void Renderer::clear(uint16_t color) {
    _spr.fillSprite(color);
}

void Renderer::present() {
    _spr.pushSprite(0, 0);
}

void Renderer::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    _spr.fillRect(x, y, w, h, color);
}

void Renderer::text(int16_t x, int16_t y, const char* s, uint16_t color, uint8_t font) {
    _spr.setTextFont(font);
    _spr.setTextColor(color);
    _spr.setCursor(x, y);
    _spr.print(s);
}

int16_t Renderer::textWidth(const char* s, uint8_t font) {
    _spr.setTextFont(font);
    return _spr.textWidth(s);
}

void Renderer::textCentered(int16_t cy, const char* s, uint16_t color, uint8_t font) {
    int16_t w = textWidth(s, font);
    text((W - w) / 2, cy, s, color, font);
}

static uint8_t utf8Len(uint8_t b) {
    if ((b & 0x80) == 0x00) return 1;
    if ((b & 0xE0) == 0xC0) return 2;
    if ((b & 0xF0) == 0xE0) return 3;
    if ((b & 0xF8) == 0xF0) return 4;
    return 1;
}

static uint16_t utf8Codepoint(const char* s) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(s);
    if ((p[0] & 0x80) == 0x00) return p[0];
    if ((p[0] & 0xE0) == 0xC0) return ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
    if ((p[0] & 0xF0) == 0xE0) return ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
    return 0;
}

const uint8_t* Renderer::fontForGlyph(const char* glyph) {
    uint16_t cp = utf8Codepoint(glyph);
    _u8f.setFont(u8g2_font_wqy16_t_gb2312b);
    if (u8g2_IsGlyph(&_u8f.u8g2, cp)) return u8g2_font_wqy16_t_gb2312b;

    return u8g2_font_wqy16_t_gb2312b;
}

int16_t Renderer::utf8GlyphWidth(const char* glyph) {
    _u8f.setFont(fontForGlyph(glyph));
    int16_t w = _u8f.getUTF8Width(glyph);
    if (w <= 0) w = 16;
    return w;
}

void Renderer::utf8Text(int16_t x, int16_t y, const char* s, uint16_t color) {
    _u8f.setFontMode(1);
    _u8f.setFontDirection(0);
    _u8f.setForegroundColor(color);

    int16_t cx = x;
    for (int i = 0; s[i] != '\0';) {
        uint8_t len = utf8Len((uint8_t)s[i]);
        char glyph[5] = {0, 0, 0, 0, 0};
        for (uint8_t k = 0; k < len && s[i + k] != '\0'; k++) glyph[k] = s[i + k];

        _u8f.setFont(fontForGlyph(glyph));
        _u8f.setCursor(cx, y + _utf8Ascent);
        _u8f.print(glyph);
        cx += utf8GlyphWidth(glyph);
        i += len;
    }
}

void Renderer::utf8TextCentered(int16_t y, const char* s, uint16_t color) {
    int16_t w = utf8TextWidth(s);
    utf8Text((W - w) / 2, y, s, color);
}

int16_t Renderer::utf8TextWidth(const char* s) {
    int16_t w = 0;
    for (int i = 0; s[i] != '\0';) {
        uint8_t len = utf8Len((uint8_t)s[i]);
        char glyph[5] = {0, 0, 0, 0, 0};
        for (uint8_t k = 0; k < len && s[i + k] != '\0'; k++) glyph[k] = s[i + k];
        w += utf8GlyphWidth(glyph);
        i += len;
    }
    return w;
}

// --- Pac-Man ---
// Draw yellow disk, then cut a wedge by overwriting with black triangle in `dir`.
void Renderer::drawPacman(int16_t cx, int16_t cy, int16_t r, int dir, bool mouthOpen, uint16_t color) {
    _spr.fillCircle(cx, cy, r, color);
    // a single dark eye for character
    int ex = cx, ey = cy - r/2;
    switch (dir) {
        case 0: ex = cx + r/4; break;   // facing right → eye offset right
        case 1: ex = cx - r/4; break;
        case 2: ex = cx;       ey = cy - r/2; break;
        case 3: ex = cx;       ey = cy + r/3; break;
    }
    _spr.fillCircle(ex, ey, 1, C_BLACK);

    if (!mouthOpen) return;
    // Wedge triangle — apex at center, base out in `dir`.
    int bx1, by1, bx2, by2;
    int rr = (int)(r * 1.3f);
    int off = (int)(r * 0.65f);
    switch (dir) {
        default: case 0: // right
            bx1 = cx + rr; by1 = cy - off;
            bx2 = cx + rr; by2 = cy + off;
            break;
        case 1: // left
            bx1 = cx - rr; by1 = cy - off;
            bx2 = cx - rr; by2 = cy + off;
            break;
        case 2: // up
            bx1 = cx - off; by1 = cy - rr;
            bx2 = cx + off; by2 = cy - rr;
            break;
        case 3: // down
            bx1 = cx - off; by1 = cy + rr;
            bx2 = cx + off; by2 = cy + rr;
            break;
    }
    _spr.fillTriangle(cx, cy, bx1, by1, bx2, by2, C_BLACK);
}

// Ghost centered at (cx, cy) with outer radius r.
// Height = r*2, width = r*2. Dome on top, rect body, wavy bottom.
void Renderer::drawGhost(int16_t cx, int16_t cy, int16_t r, uint16_t body, uint16_t pupil, int eyeDir) {
    int16_t x = cx - r;
    int16_t y = cy - r;
    int16_t w = r * 2;
    int16_t h = r * 2;

    // dome (top half)
    _spr.fillCircle(cx, y + r, r, body);
    // body rect
    _spr.fillRect(x, y + r, w, h - r - 3, body);

    // wavy bottom (3 valleys)
    int16_t footY = y + h;
    int16_t segW = w / 3;
    for (int i = 0; i < 3; i++) {
        int16_t sx = x + i * segW;
        _spr.fillTriangle(sx, footY - 3, sx + segW/2, footY, sx + segW, footY - 3, C_BLACK);
    }

    // eyes (whites)
    int16_t eyeR = r / 3;
    int16_t eyeY = y + r - 1;
    int16_t eyeLx = cx - r/2;
    int16_t eyeRx = cx + r/2;
    _spr.fillCircle(eyeLx, eyeY, eyeR, C_WHITE);
    _spr.fillCircle(eyeRx, eyeY, eyeR, C_WHITE);

    // pupils shifted by eyeDir
    int dx = 0;
    if (eyeDir == 0) dx = 1;
    else if (eyeDir == 1) dx = -1;
    _spr.fillCircle(eyeLx + dx, eyeY, eyeR / 2 + 1, pupil);
    _spr.fillCircle(eyeRx + dx, eyeY, eyeR / 2 + 1, pupil);
}

void Renderer::drawPellet(int16_t cx, int16_t cy) {
    _spr.fillCircle(cx, cy, 1, C_WHITE);
}

void Renderer::drawPowerPellet(int16_t cx, int16_t cy, bool lit) {
    if (lit) _spr.fillCircle(cx, cy, 4, C_WHITE);
    else     _spr.drawCircle(cx, cy, 4, C_WHITE);
}

void Renderer::drawCherry(int16_t cx, int16_t cy) {
    // stem
    _spr.drawLine(cx - 2, cy - 8, cx + 2, cy - 10, C_GREEN);
    _spr.drawLine(cx + 2, cy - 10, cx + 6, cy - 8, C_GREEN);
    // cherries
    _spr.fillCircle(cx - 3, cy, 4, C_RED);
    _spr.fillCircle(cx + 5, cy - 1, 4, C_RED);
    // highlights
    _spr.drawPixel(cx - 4, cy - 1, C_WHITE);
    _spr.drawPixel(cx + 4, cy - 2, C_WHITE);
}

void Renderer::drawBonus(int16_t cx, int16_t cy, const char* s) {
    text(cx - textWidth(s, 2) / 2, cy - 8, s, C_YELLOW, 2);
}

// Death: frame 0 = closed mouth circle; 1..4 = mouth opens wider; 5 = lines radiating.
void Renderer::drawDeathFrame(int16_t cx, int16_t cy, int16_t r, int frame) {
    if (frame <= 0) {
        _spr.fillCircle(cx, cy, r, C_YELLOW);
        return;
    }
    if (frame >= 5) {
        // radiating lines
        for (int a = 0; a < 360; a += 45) {
            float rad = a * 3.14159f / 180.0f;
            int x1 = cx + (int)(cosf(rad) * r * 0.3f);
            int y1 = cy + (int)(sinf(rad) * r * 0.3f);
            int x2 = cx + (int)(cosf(rad) * r * 1.2f);
            int y2 = cy + (int)(sinf(rad) * r * 1.2f);
            _spr.drawLine(x1, y1, x2, y2, C_YELLOW);
        }
        return;
    }
    // frames 1..4: progressively larger mouth opening from top
    _spr.fillCircle(cx, cy, r, C_YELLOW);
    int openDeg = 20 + frame * 40;  // 60, 100, 140, 180
    int off = (int)(r * tanf(openDeg * 3.14159f / 360.0f));
    if (off < 2) off = 2;
    // two triangles meeting at center, mouth facing up
    int rr = r + 2;
    _spr.fillTriangle(cx, cy, cx - off, cy - rr, cx + off, cy - rr, C_BLACK);
}
