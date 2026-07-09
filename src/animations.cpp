#include "animations.h"
#include <cstring>

static constexpr uint16_t C_BLACK   = 0x0000;
static constexpr uint16_t C_WHITE   = 0xFFFF;
static constexpr uint16_t C_YELLOW  = 0xFFE0;
static constexpr uint16_t C_RED     = 0xF800;
static constexpr uint16_t C_PINK    = 0xFC18;
static constexpr uint16_t C_CYAN    = 0x07FF;
static constexpr uint16_t C_ORANGE  = 0xFC00;
static constexpr uint16_t C_BLUE    = 0x001F;
static constexpr uint16_t C_GREY    = 0x7BEF;
static constexpr uint16_t C_DARKBLUE= 0x0010;

static ResponseLanguage g_animLang = ResponseLanguage::CHINESE;

void setAnimationLanguage(ResponseLanguage lang) {
    g_animLang = lang;
}

ResponseLanguage animationLanguage() {
    return g_animLang;
}

static bool zh() {
    return g_animLang == ResponseLanguage::CHINESE;
}

static void label(Renderer& r, int16_t x, int16_t y, const char* cn, const char* en,
                  uint16_t color, uint8_t font = 2) {
    if (zh()) r.utf8Text(x, y, cn, color);
    else      r.text(x, y, en, color, font);
}

static void labelCentered(Renderer& r, int16_t y, const char* cn, const char* en,
                          uint16_t color, uint8_t font = 2) {
    if (zh()) r.utf8TextCentered(y, cn, color);
    else      r.textCentered(y, en, color, font);
}

// =================== LanguageSelectAnim ===================
void LanguageSelectAnim::enter(Renderer&, SoundPlayer& s) {
    s.play(snd::IDLE_WAKA, snd::IDLE_WAKA_LEN, true);
}

void LanguageSelectAnim::update(Renderer& r, SoundPlayer&, uint32_t now) {
    r.clear(C_BLACK);

    bool blink = ((now / 500) % 2) == 0;
    r.textCentered(8, "PICK LANGUAGE", blink ? C_YELLOW : C_ORANGE, 2);

    int row1 = 50;
    r.drawPacman(22, row1 + 6, 9, 0, ((now / 160) % 2) == 0, C_YELLOW);
    r.text(40, row1, "A:", C_WHITE, 2);
    r.utf8Text(70, row1 - 1, "中文", C_YELLOW);

    int row2 = 90;
    r.drawGhost(22, row2 + 6, 9, C_CYAN, C_WHITE, ((now / 250) % 2) ? 0 : 1);
    r.text(40, row2, "B:", C_WHITE, 2);
    r.text(70, row2, "ENGLISH", C_CYAN, 2);

    r.textCentered(122, "press a button to pick", C_GREY, 1);
    r.present();
}

// =================== ModeSelectAnim ===================
void ModeSelectAnim::enter(Renderer&, SoundPlayer& s) {
    s.play(snd::IDLE_WAKA, snd::IDLE_WAKA_LEN, true);
}

void ModeSelectAnim::update(Renderer& r, SoundPlayer&, uint32_t now) {
    r.clear(C_BLACK);

    bool blink = ((now / 500) % 2) == 0;
    labelCentered(r, 8, "选择模式", "PICK A MODE", blink ? C_YELLOW : C_ORANGE, 2);

    // Option A: Fortune (yellow pacman + label)
    int row1 = 50;
    r.drawPacman(22, row1 + 6, 9, 0, ((now / 160) % 2) == 0, C_YELLOW);
    r.text(40, row1, "A:",       C_WHITE, 2);
    label(r, 70, row1, "求签", "FORTUNE", C_YELLOW, 2);

    // Option B: Truth or Dare (pink ghost + label)
    int row2 = 90;
    r.drawGhost(22, row2 + 6, 9, C_PINK, C_WHITE, ((now / 250) % 2) ? 0 : 1);
    r.text(40, row2, "B:",       C_WHITE, 2);
    label(r, 70, row2, "真心话", "TRUTH", C_PINK, 2);

    labelCentered(r, 122, "按键选择", "press a button to pick", C_GREY, 1);

    r.present();
}

// =================== GenderPickAnim ===================
void GenderPickAnim::enter(Renderer&, SoundPlayer& s) {
    _startMs           = millis();
    _lastTickMs        = _startMs;
    _tickInterval      = 60;
    _phase             = Phase::SPIN;
    _showMale          = (random(0, 2) == 0);
    _resultMale        = (random(0, 2) == 0);
    _flashOn           = true;
    _done              = false;
    _lockJinglePlayed  = false;
    s.play(snd::POWER_PELLET, snd::POWER_PELLET_LEN);
}

void GenderPickAnim::update(Renderer& r, SoundPlayer& s, uint32_t now) {
    if (_phase == Phase::SPIN) {
        if (now - _lastTickMs >= _tickInterval) {
            _lastTickMs = now;
            _showMale = !_showMale;
            _flashOn  = !_flashOn;
            _tickInterval = (uint16_t)(_tickInterval + 10);  // gentler slowdown
        }
        if (_tickInterval > 280 || (now - _startMs) > 4000) {
            _phase   = Phase::LOCKED;
            _lockMs  = now;
            _showMale = _resultMale;
            _flashOn  = true;
        }
    } else { // LOCKED — wait for button press, gentle blink only
        if (!_lockJinglePlayed) {
            _lockJinglePlayed = true;
            s.play(snd::CAUGHT, snd::CAUGHT_LEN);
        }
        _flashOn = ((now / 500) % 2) == 0;   // slow, calm
    }

    uint16_t bg     = _showMale ? C_DARKBLUE : C_PINK;
    uint16_t accent = _showMale ? C_CYAN     : C_WHITE;
    if (!_flashOn) bg = C_BLACK;

    r.clear(bg);

    if (zh()) {
        r.utf8TextCentered(40, _showMale ? "男生" : "女生", accent);
    } else {
        r.textCentered(40, _showMale ? "MALE" : "FEMALE", accent, 4);
    }

    // Mascot below
    int my = 100;
    if (_showMale) r.drawPacman(Renderer::W/2, my, 12, 0, ((now/200)%2)==0, C_YELLOW);
    else           r.drawCherry(Renderer::W/2, my);

    if (_phase == Phase::LOCKED) {
        labelCentered(r, 8, "已选中", "LOCKED!", C_YELLOW, 2);
        if (((now / 500) % 2) == 0) {
            labelCentered(r, 122, "按A继续", "press A to continue", C_GREY, 1);
        }
    } else {
        labelCentered(r, 15, "抽选中", "spinning", C_GREY, 1);
    }

    r.present();
}

// =================== IdleAnim ===================
static const uint16_t PAC_COLORS[] = {
    0xFFE0,  // yellow
    0x07FF,  // cyan
    0xFC18,  // pink
    0xFC00,  // orange
    0x07E0,  // green
    0xFFFF,  // white
    0xF81F,  // magenta
};
static constexpr int PAC_COLOR_COUNT = sizeof(PAC_COLORS) / sizeof(PAC_COLORS[0]);

void IdleAnim::respawn() {
    for (int i = 0; i < N_PELLETS; i++) _pellets[i] = true;
    // pick a new random color different from current
    uint16_t prev = _pacColor;
    do { _pacColor = PAC_COLORS[random(0, PAC_COLOR_COUNT)]; } while (_pacColor == prev);
}

void IdleAnim::enter(Renderer& r, SoundPlayer& s) {
    // lay out pellets evenly across the bottom row
    const int startX = 20;
    const int spacing = 16;
    for (int i = 0; i < N_PELLETS; i++) {
        _pelletX[i] = startX + i * spacing;
        _pellets[i] = true;
    }
    _pacColor = PAC_COLORS[random(0, PAC_COLOR_COUNT)];
    _pacX = 10.0f;
    _dir  = 0;
    s.play(snd::IDLE_WAKA, snd::IDLE_WAKA_LEN, true);
}

void IdleAnim::update(Renderer& r, SoundPlayer& s, uint32_t now) {
    const float speed = 1.3f;
    const int   pacY  = 100;
    const int   rightEdge = Renderer::W - 12;
    const int   leftEdge  = 12;

    // move
    if (_dir == 0) _pacX += speed;
    else           _pacX -= speed;

    // eat pellet when pac reaches it (by direction)
    for (int i = 0; i < N_PELLETS; i++) {
        if (!_pellets[i]) continue;
        if (_dir == 0 && _pacX >= _pelletX[i] - 2) _pellets[i] = false;
        if (_dir == 1 && _pacX <= _pelletX[i] + 2) _pellets[i] = false;
    }

    // finished a row → respawn pellets + new pac color + flip direction
    if (_dir == 0 && _pacX > rightEdge) {
        respawn();
        _dir  = 1;
        _pacX = rightEdge;
    } else if (_dir == 1 && _pacX < leftEdge) {
        respawn();
        _dir  = 0;
        _pacX = leftEdge;
    }

    bool mouthOpen = ((now / 160) % 2) == 0;

    r.clear(C_BLACK);

    // Two alternating lines that together never go both-dark.
    bool topOn = ((now / 500) % 2) == 0;
    if (topOn) labelCentered(r, 12, "~ 摇一摇 ~", "~ SHAKE ME! ~", C_YELLOW, 2);
    else       r.textCentered(42, "~ lucky8! ~",   C_CYAN,   2);

    // pellets
    for (int i = 0; i < N_PELLETS; i++) {
        if (_pellets[i]) r.drawPellet(_pelletX[i], pacY);
    }

    // pacman
    r.drawPacman((int)_pacX, pacY, 9, _dir, mouthOpen, _pacColor);

    r.present();
}

// =================== ConnectingAnim ===================
void ConnectingAnim::enter(Renderer& r, SoundPlayer& s) {
    _phase   = Phase::CHASE;
    _pacX    = -20.0f;
    _ghostX  = 40.0f;
    _eatenMs = 0;
    s.play(snd::SIREN, snd::SIREN_LEN, true);
}

void ConnectingAnim::triggerCaught(SoundPlayer& s) {
    if (_phase != Phase::CHASE) return;
    _phase = Phase::CATCHUP;
}

bool ConnectingAnim::readyToAdvance() const {
    return _phase == Phase::EATEN && (millis() - _eatenMs) > 600;
}

void ConnectingAnim::update(Renderer& r, SoundPlayer& s, uint32_t now) {
    const int laneY = 90;

    if (_phase == Phase::CHASE) {
        _pacX   += 1.8f;
        _ghostX += 1.3f;
        if (_pacX > Renderer::W + 20) { _pacX = -30; _ghostX = _pacX + 60; }
        if (_ghostX - _pacX < 20) _ghostX = _pacX + 20;
    } else if (_phase == Phase::CATCHUP) {
        // Pac-Man accelerates; ghost slows. Close the gap quickly.
        _pacX   += 3.6f;
        _ghostX += 0.6f;
        if (_pacX + 6 >= _ghostX) {
            // Caught! Snap pac onto ghost location; transition to EATEN.
            _pacX = _ghostX;
            _phase = Phase::EATEN;
            _eatenMs = now;
            s.play(snd::CAUGHT, snd::CAUGHT_LEN);
        }
    }
    // Phase::EATEN: pacman holds position, bonus floats up.

    r.clear(C_BLACK);

    // CONNECTING banner centered vertically, slightly above the chase lane
    int dots = (now / 400) % 4;
    char banner[24];
    const char* suffix = dots==0 ? "" : (dots==1 ? "." : (dots==2 ? ".." : "..."));
    if (zh()) {
        snprintf(banner, sizeof(banner), "连接中%s", suffix);
        r.utf8TextCentered(38, banner, C_YELLOW);
    } else {
        snprintf(banner, sizeof(banner), "CONNECTING%s", suffix);
        r.textCentered(40, banner, C_YELLOW, 2);
    }

    // dashed running track
    for (int px = 0; px < Renderer::W; px += 12) {
        r.fillRect(px, laneY + 20, 6, 1, C_GREY);
    }

    if (_phase != Phase::EATEN) {
        r.drawGhost((int)_ghostX, laneY, 10, C_RED, C_BLUE, 0);
    }
    bool mouthOpen = ((now / 120) % 2) == 0;
    r.drawPacman((int)_pacX, laneY, 10, 0, mouthOpen, C_YELLOW);

    if (_phase == Phase::EATEN) {
        int riseY = laneY - 20 - (int)((now - _eatenMs) / 18);
        if (riseY > 20) r.drawBonus((int)_pacX, riseY, "+100");
    }

    r.present();
}

// =================== LoadingAnim ===================
void LoadingAnim::enter(Renderer& r, SoundPlayer& s) {
    _startMs = millis();
    for (int i = 0; i < 4; i++) {
        _ghosts[i].x  = 30 + i * 55;
        _ghosts[i].y  = 60 + (i % 2) * 30;
        _ghosts[i].vx = (i % 2 ? 1 : -1) * (1.0f + (i * 0.25f));
        _ghosts[i].vy = (i < 2 ? 1 : -1) * 0.6f;
    }
    s.play(snd::POWER_PELLET, snd::POWER_PELLET_LEN);
}

void LoadingAnim::update(Renderer& r, SoundPlayer& s, uint32_t now) {
    // After power-pellet jingle ends, switch to looping scared BGM.
    if (!s.playing()) {
        s.play(snd::SCARED_BGM, snd::SCARED_BGM_LEN, true);
    }

    // advance ghosts
    for (int i = 0; i < 4; i++) {
        _ghosts[i].x += _ghosts[i].vx;
        _ghosts[i].y += _ghosts[i].vy;
        if (_ghosts[i].x < 15 || _ghosts[i].x > Renderer::W - 15) _ghosts[i].vx = -_ghosts[i].vx;
        if (_ghosts[i].y < 45 || _ghosts[i].y > Renderer::H - 15) _ghosts[i].vy = -_ghosts[i].vy;
    }

    r.clear(C_BLACK);

    // banner with marquee dots
    int dots = (now / 400) % 4;
    char banner[32];
    const char* suffix = dots==0 ? "" : (dots==1 ? "." : (dots==2 ? ".." : "..."));
    if (zh()) {
        snprintf(banner, sizeof(banner), "询问宇宙%s", suffix);
        r.utf8TextCentered(8, banner, C_CYAN);
    } else {
        snprintf(banner, sizeof(banner), "ASKING THE COSMOS%s", suffix);
        r.textCentered(10, banner, C_CYAN, 2);
    }

    // center power pellet — pulsing
    bool pulse = ((now / 150) % 2) == 0;
    r.drawPowerPellet(Renderer::W/2, Renderer::H/2 + 5, pulse);

    // four scared (blue) ghosts
    for (int i = 0; i < 4; i++) {
        r.drawGhost((int)_ghosts[i].x, (int)_ghosts[i].y, 9, C_DARKBLUE, C_WHITE, 2);
    }

    r.present();
}

// =================== DisplayingAnim ===================
DisplayingAnim::DisplayingAnim(const Fortune& f) : _f(f) {}

static uint8_t utf8GlyphLen(uint8_t b) {
    if ((b & 0x80) == 0x00) return 1;
    if ((b & 0xE0) == 0xC0) return 2;
    if ((b & 0xF0) == 0xE0) return 3;
    if ((b & 0xF8) == 0xF0) return 4;
    return 1;
}

static bool isAsciiBreak(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

void DisplayingAnim::layoutText(Renderer& r) {
    const bool chinese = zh();
    const int lineH   = chinese ? r.utf8LineHeight() : 22;
    const int maxColW = chinese ? (Renderer::W - 42) : (Renderer::W - 10);
    const int startX  = 5;
    const int startY  = chinese ? 6 : 8;

    _glyphCount = 0;

    const String& s = _f.text;
    const int n = s.length();
    int i = 0;
    int curX = startX;
    int curRow = 0;

    if (!chinese) {
        while (i < n && _glyphCount < MAX_GLYPHS) {
            char first = s[i];
            if (first == '\n' || first == '\r') {
                curRow++;
                curX = startX;
                i++;
                continue;
            }
            if (first == ' ' || first == '\t') {
                i++;
                continue;
            }

            int wordStart = i;
            int wordW = 0;
            while (i < n && !isAsciiBreak(s[i])) {
                char buf[2] = {s[i], 0};
                int16_t cw = r.textWidth(buf, 2);
                wordW += (cw > 0) ? cw : 8;
                i++;
            }

            if (curX > startX && (curX + wordW) > (startX + maxColW)) {
                curRow++;
                curX = startX;
            }

            for (int k = wordStart; k < i && _glyphCount < MAX_GLYPHS; k++) {
                char buf[2] = {s[k], 0};
                int16_t cw = r.textWidth(buf, 2);
                if (cw <= 0) cw = 8;

                Glyph& g = _glyphs[_glyphCount++];
                g.text[0] = s[k];
                g.text[1] = '\0';
                g.len = 1;
                g.x = curX;
                g.y = startY + curRow * lineH;
                g.w = cw;
                g.whitespace = false;
                curX += cw;
            }

            int spaceW = 6;
            if (i < n && (s[i] == ' ' || s[i] == '\t') &&
                curX + spaceW <= startX + maxColW && _glyphCount < MAX_GLYPHS) {
                Glyph& g = _glyphs[_glyphCount++];
                g.text[0] = ' ';
                g.text[1] = '\0';
                g.len = 1;
                g.x = curX;
                g.y = startY + curRow * lineH;
                g.w = spaceW;
                g.whitespace = true;
                curX += spaceW;
            }
            while (i < n && (s[i] == ' ' || s[i] == '\t')) i++;
        }
        return;
    }

    while (i < n && _glyphCount < MAX_GLYPHS) {
        char first = s[i];
        if (first == '\n' || first == '\r') {
            curRow++;
            curX = startX;
            i++;
            continue;
        }

        uint8_t len = utf8GlyphLen((uint8_t)first);
        if (i + len > n) len = 1;

        char buf[5] = {0, 0, 0, 0, 0};
        for (uint8_t k = 0; k < len; k++) buf[k] = s[i + k];

        bool whitespace = (len == 1 && isAsciiBreak(first));
        int16_t w = whitespace ? 6 : r.utf8TextWidth(buf);
        if (w <= 0) w = (len == 1) ? 8 : 16;

        if (!whitespace && curX > startX && (curX + w) > (startX + maxColW)) {
            curRow++;
            curX = startX;
        }

        if (!(whitespace && curX == startX)) {
            Glyph& g = _glyphs[_glyphCount++];
            memcpy(g.text, buf, sizeof(g.text));
            g.len = len;
            g.x = curX;
            g.y = startY + curRow * lineH;
            g.w = w;
            g.whitespace = whitespace;
            curX += w;
        }

        i += len;
    }
}

void DisplayingAnim::enter(Renderer& r, SoundPlayer& s) {
    layoutText(r);
    _phase         = Phase::TYPING;
    _phaseStartMs  = millis();
    _typedIdx      = 0;
    _lastTypeMs    = 0;
    _jinglePlayed  = false;
    _eatIdx        = _glyphCount - 1;
    _prevLineY     = (_glyphCount > 0) ? _glyphs[_glyphCount - 1].y : 0;
    _pacX          = Renderer::W + 12;
    _lastEatMs     = 0;
    _scrollY            = 0;
    _lastScrollMs       = 0;
    _scrollPaused       = false;
    _scrollPauseStartMs = 0;

    // visible text area: y = 0..textBottomY (above mascot/hint)
    const int textBottomY = 100;
    int lastY = (_glyphCount > 0) ? _glyphs[_glyphCount - 1].y : 0;
    int contentBottom = lastY + (zh() ? r.utf8LineHeight() : 22);
    _maxScrollY = (contentBottom > textBottomY) ? (contentBottom - textBottomY) : 0;
}

void DisplayingAnim::drawAllTyped(Renderer& r) {
    const int topClip    = -2;
    const int bottomClip = 100;        // keep text out of mascot/hint band
    for (int i = 0; i < _typedIdx && i < _glyphCount; i++) {
        int sy = _glyphs[i].y - _scrollY;
        if (sy < topClip || sy > bottomClip) continue;
        if (!_glyphs[i].whitespace) {
            if (zh()) r.utf8Text(_glyphs[i].x, sy, _glyphs[i].text, C_WHITE);
            else      r.text(_glyphs[i].x, sy, _glyphs[i].text, C_WHITE, 2);
        }
    }
}

void DisplayingAnim::drawMascot(Renderer& r, uint32_t now) {
    // Right side vertical bounce
    int mx = Renderer::W - 22;
    int my = Renderer::H - 22 + (((now / 400) % 2) == 0 ? 0 : -2);

    switch (_f.mood) {
        case Mood::LUCKY:
            r.drawGhost(mx, my, 10, C_DARKBLUE, C_WHITE, 1);
            break;
        case Mood::WARNING:
            r.drawGhost(mx, my, 10, C_RED, C_WHITE, ((now/300)%2)?0:1);
            break;
        case Mood::CALM:
            r.drawPacman(mx, my, 10, 0, false, C_YELLOW);
            // "Z Z" floating
            {
                int zy = my - 14 - (int)((now / 250) % 8);
                r.text(mx + 10, zy, "z", C_WHITE, 2);
            }
            break;
        case Mood::BOLD: {
            bool lit = ((now/180)%2) == 0;
            r.drawPowerPellet(mx, my, lit);
            break;
        }
        case Mood::LOVE:
            r.drawCherry(mx, my);
            break;
    }
}

void DisplayingAnim::onShortPress() {
    if (_phase == Phase::TYPING || _phase == Phase::HOLD || _phase == Phase::SCROLL) {
        _phase = Phase::EAT;
        _phaseStartMs = millis();
        _typedIdx = _glyphCount;
        _eatIdx   = _glyphCount - 1;
        _prevLineY = (_glyphCount > 0) ? _glyphs[_glyphCount - 1].y : 0;
        _pacX     = Renderer::W + 12;
        _lastEatMs = 0;
    }
}

void DisplayingAnim::onLongPress() {
    if (_phase == Phase::HOLD && _maxScrollY > 0) {
        _phase = Phase::SCROLL;
        _phaseStartMs = millis();
        _lastScrollMs = millis();
    }
}

void DisplayingAnim::update(Renderer& r, SoundPlayer& s, uint32_t now) {
    if (_phase == Phase::TYPING) {
        // emit one char every 28ms
        if (now - _lastTypeMs >= 28 && _typedIdx < _glyphCount) {
            _typedIdx++;
            _lastTypeMs = now;
            if (!_glyphs[_typedIdx - 1].whitespace) {
                s.play(snd::TYPE_CLICK, snd::TYPE_CLICK_LEN);
            }
        }
        if (_typedIdx >= _glyphCount) {
            _phase        = Phase::HOLD;
            _phaseStartMs = now;
            _typingDoneMs = now;
        }
    }

    if (_phase == Phase::HOLD) {
        if (!_jinglePlayed && !s.playing()) {
            _jinglePlayed = true;
            const Note* seq = nullptr; size_t len = 0;
            switch (_f.mood) {
                case Mood::LUCKY:   seq = snd::JINGLE_LUCKY;   len = snd::JINGLE_LUCKY_LEN;   break;
                case Mood::WARNING: seq = snd::JINGLE_WARNING; len = snd::JINGLE_WARNING_LEN; break;
                case Mood::CALM:    seq = snd::JINGLE_CALM;    len = snd::JINGLE_CALM_LEN;    break;
                case Mood::BOLD:    seq = snd::JINGLE_BOLD;    len = snd::JINGLE_BOLD_LEN;    break;
                case Mood::LOVE:    seq = snd::JINGLE_LOVE;    len = snd::JINGLE_LOVE_LEN;    break;
            }
            if (seq) s.play(seq, len);
        }
        // No auto-advance; wait for short-press (→ EAT) or long-press (→ SCROLL).
    }

    if (_phase == Phase::SCROLL) {
        if (_scrollPaused) {
            // Hold at bottom for 1s, then snap back to top. Whether scrolling
            // resumes or stays parked at the top is decided by _scrollHeld.
            if (now - _scrollPauseStartMs > 1000) {
                _scrollY      = 0;
                _scrollPaused = false;
                _lastScrollMs = now;
            }
        } else if (_scrollHeld) {
            // Only progress while BtnA is held. ~30 px/sec.
            if (_lastScrollMs == 0) _lastScrollMs = now;
            float dt = (float)(now - _lastScrollMs);
            _lastScrollMs = now;
            _scrollY = (int16_t)(_scrollY + (int)(dt * 0.030f + 0.5f));
            if (_scrollY >= _maxScrollY) {
                _scrollY            = _maxScrollY;
                _scrollPaused       = true;
                _scrollPauseStartMs = now;
            }
        } else {
            _lastScrollMs = 0;   // pause time tracking when not held
        }
    }

    if (_phase == Phase::EAT) {
        // Line-change detection: when the next char to eat lives on a
        // different line than _prevLineY, teleport pac-man back to the
        // right edge so the new line is fully traversed.
        if (_eatIdx >= 0 && _glyphs[_eatIdx].y != _prevLineY) {
            _prevLineY = _glyphs[_eatIdx].y;
            _pacX      = Renderer::W + 12;
            _lastEatMs = 0;   // let dt reset for smooth motion
        }

        float dt = (_lastEatMs == 0) ? 33.0f : (float)(now - _lastEatMs);
        _lastEatMs = now;
        _pacX -= dt * 0.18f;  // ~180 px/sec → 240/180 ≈ 1.3s per line

        // Auto-scroll to keep the line being eaten on screen.
        // Target: keep _prevLineY at ~y=40 in visible coords.
        if (_glyphCount > 0) {
            int desiredScroll = _prevLineY - 40;
            if (desiredScroll < 0) desiredScroll = 0;
            if (desiredScroll > _maxScrollY) desiredScroll = _maxScrollY;
            _scrollY = (int16_t)desiredScroll;
        }

        // Eat the rightmost remaining char when pac-man passes its right edge,
        // but only if it belongs to the line we're currently traversing.
        if (_eatIdx >= 0 && _glyphs[_eatIdx].y == _prevLineY) {
            int eatX = _glyphs[_eatIdx].x + _glyphs[_eatIdx].w;
            if (_pacX <= eatX + 4) {
                if (!_glyphs[_eatIdx].whitespace) {
                    s.play(snd::EAT_WAKA, snd::EAT_WAKA_LEN);
                }
                _eatIdx--;
            }
        }
        // Fully finished when all chars eaten AND pac has walked off the left.
        if (_eatIdx < 0 && _pacX < -20) { _phase = Phase::FINISHED; return; }
    }

    // === Draw ===
    r.clear(C_BLACK);

    if (_phase == Phase::TYPING || _phase == Phase::HOLD || _phase == Phase::SCROLL) {
        drawAllTyped(r);
        if (_phase == Phase::HOLD) {
            drawMascot(r, now);
            bool blink = ((now / 500) % 2) == 0;
            if (blink) {
                if (_maxScrollY > 0) {
                    label(r, 5, Renderer::H - 15, "长按A滚动", "hold A: scroll", C_GREY, 1);
                } else {
                    label(r, 5, Renderer::H - 15, "按A吞字", "press A", C_GREY, 1);
                }
            }
        } else if (_phase == Phase::SCROLL) {
            // mascot still visible while scrolling
            drawMascot(r, now);
            if (((now / 500) % 2) == 0) {
                label(r, 5, Renderer::H - 15, "点按A吞字", "tap A: eat", C_GREY, 1);
            }
        }
    } else if (_phase == Phase::EAT) {
        // redraw still-visible glyphs (indices 0.._eatIdx inclusive), with scroll offset
        const int topClip    = -2;
        const int bottomClip = 100;
        for (int i = 0; i <= _eatIdx && i < _glyphCount; i++) {
            int sy = _glyphs[i].y - _scrollY;
            if (sy < topClip || sy > bottomClip) continue;
            if (!_glyphs[i].whitespace) {
                if (zh()) r.utf8Text(_glyphs[i].x, sy, _glyphs[i].text, C_WHITE);
                else      r.text(_glyphs[i].x, sy, _glyphs[i].text, C_WHITE, 2);
            }
        }
        // pacman moving left; sit on the line currently being eaten (scrolled)
        int pacY = (_prevLineY - _scrollY) + 8;
        bool mouthOpen = ((now / 100) % 2) == 0;
        r.drawPacman((int)_pacX, pacY, 9, 1, mouthOpen, C_YELLOW);
    }

    r.present();
}

// =================== ErrorAnim ===================
void ErrorAnim::enter(Renderer& r, SoundPlayer& s) {
    _startMs = millis();
    s.play(snd::DEATH, snd::DEATH_LEN);
}

void ErrorAnim::update(Renderer& r, SoundPlayer& s, uint32_t now) {
    uint32_t t = now - _startMs;
    int frame = t / 80;
    if (frame > 5) frame = 5;

    r.clear(C_BLACK);

    r.drawDeathFrame(Renderer::W / 2, 45, 14, frame);

    if (frame >= 5) {
        labelCentered(r, 80, "出错了", "GAME OVER", C_RED, 2);
        if (_msg.length() > 0) {
            // only the first line of the error
            String line = _msg;
            int nl = line.indexOf('\n');
            if (nl >= 0) line = line.substring(0, nl);
            r.textCentered(100, line.c_str(), C_WHITE, 1);
        }
        labelCentered(r, 115, "按A重试", "Press A to retry", C_GREY, 1);
    }

    r.present();
}
