#pragma once
#include <Arduino.h>
#include "display_manager.h"
#include "sound_player.h"
#include "api_client.h"

void setAnimationLanguage(ResponseLanguage lang);
ResponseLanguage animationLanguage();

class Animation {
public:
    virtual ~Animation() = default;
    virtual void enter(Renderer& r, SoundPlayer& s) = 0;
    virtual void update(Renderer& r, SoundPlayer& s, uint32_t nowMs) = 0;
    virtual bool done() const { return false; }
};

// Boot-only language menu: A -> Chinese, B -> English.
class LanguageSelectAnim : public Animation {
public:
    void enter(Renderer&, SoundPlayer&) override;
    void update(Renderer&, SoundPlayer&, uint32_t) override;
};

// Boot-only menu: A → fortune mode, B → truth mode.
class ModeSelectAnim : public Animation {
public:
    void enter(Renderer&, SoundPlayer&) override;
    void update(Renderer&, SoundPlayer&, uint32_t) override;
};

// Slot-machine style Male/Female pick with screen-flash. Auto-advances
// after the result locks in for a brief celebration.
class GenderPickAnim : public Animation {
public:
    void enter(Renderer&, SoundPlayer&) override;
    void update(Renderer&, SoundPlayer&, uint32_t) override;
    bool done() const override { return _done; }
    bool resultIsMale() const { return _resultMale; }
    bool locked() const { return _phase == Phase::LOCKED; }
    void advance() { if (_phase == Phase::LOCKED) _done = true; }
private:
    enum class Phase { SPIN, LOCKED };
    Phase    _phase        = Phase::SPIN;
    uint32_t _startMs      = 0;
    uint32_t _lockMs       = 0;
    uint32_t _lastTickMs   = 0;
    uint16_t _tickInterval = 60;
    bool     _showMale     = true;
    bool     _resultMale   = true;
    bool     _flashOn      = true;
    bool     _done         = false;
    bool     _lockJinglePlayed = false;
};

class IdleAnim : public Animation {
public:
    void enter(Renderer&, SoundPlayer&) override;
    void update(Renderer&, SoundPlayer&, uint32_t) override;
private:
    static constexpr int N_PELLETS = 14;
    float    _pacX    = 20.0f;
    int      _dir     = 0;           // 0 right, 1 left
    uint16_t _pacColor= 0xFFE0;      // yellow default
    bool     _pellets[N_PELLETS];    // alive flags
    int      _pelletX[N_PELLETS];    // x positions

    void respawn();
};

class ConnectingAnim : public Animation {
public:
    void enter(Renderer&, SoundPlayer&) override;
    void update(Renderer&, SoundPlayer&, uint32_t) override;
    // Called when WiFi result is known — accelerates Pac-Man into ghost.
    void triggerCaught(SoundPlayer& s);
    // True once the eat-flourish has played long enough to transition.
    bool readyToAdvance() const;
private:
    enum class Phase { CHASE, CATCHUP, EATEN };
    Phase    _phase    = Phase::CHASE;
    float    _pacX     = -20.0f;
    float    _ghostX   = 40.0f;
    uint32_t _eatenMs  = 0;
};

class LoadingAnim : public Animation {
public:
    void enter(Renderer&, SoundPlayer&) override;
    void update(Renderer&, SoundPlayer&, uint32_t) override;
private:
    uint32_t _startMs = 0;
    struct Ghost { float x, y, vx, vy; };
    Ghost _ghosts[4];
};

// DisplayingAnim has three sub-phases:
//   TYPING: chars appear one at a time + click sound
//   HOLD:   mood jingle plays, mood mascot breathes, wait until time up or BtnA
//   EAT:    Pac-Man walks right→left eating each char (waka) and erases
class DisplayingAnim : public Animation {
public:
    DisplayingAnim(const Fortune& f);
    void enter(Renderer&, SoundPlayer&) override;
    void update(Renderer&, SoundPlayer&, uint32_t) override;
    bool done() const override { return _phase == Phase::FINISHED; }

    // BtnA gestures (called from main.cpp).
    void onShortPress();   // → EAT (works in TYPING/HOLD/SCROLL)
    void onLongPress();    // → SCROLL (only if overflow, only in HOLD)
    void setScrollHeld(bool h) { _scrollHeld = h; }
    bool overflows() const { return _maxScrollY > 0; }

private:
    enum class Phase { TYPING, HOLD, SCROLL, EAT, FINISHED };

    Fortune  _f;
    Phase    _phase    = Phase::TYPING;
    uint32_t _phaseStartMs = 0;

    // One visible UTF-8 glyph or ASCII space with its laid-out bounding box.
    struct Glyph {
        char    text[5];
        uint8_t len;
        int16_t x, y;
        int16_t w;
        bool    whitespace;
    };
    static constexpr int MAX_GLYPHS = 200;
    Glyph _glyphs[MAX_GLYPHS];
    int   _glyphCount = 0;

    // TYPING
    int      _typedIdx = 0;
    uint32_t _lastTypeMs = 0;
    bool     _jingleKicked = false;
    uint32_t _typingDoneMs = 0;

    // HOLD
    bool     _jinglePlayed = false;

    // EAT
    int      _eatIdx = -1;          // index of next char to eat (right to left)
    int16_t  _prevLineY = 0;        // y of the line currently being eaten
    float    _pacX   = 0;
    uint32_t _lastEatMs = 0;

    // SCROLL — _scrollY is pixels the content has been moved up.
    // Visible glyph y on screen = g.y - _scrollY.
    // After hitting bottom we pause briefly, snap back to top, and loop.
    int16_t  _scrollY            = 0;
    int16_t  _maxScrollY         = 0;
    uint32_t _lastScrollMs       = 0;
    bool     _scrollPaused       = false;
    uint32_t _scrollPauseStartMs = 0;
    bool     _scrollHeld         = false;   // BtnA currently held → progress scroll

    void layoutText(Renderer& r);
    void drawAllTyped(Renderer& r);
    void drawMascot(Renderer& r, uint32_t now);
};

class ErrorAnim : public Animation {
public:
    ErrorAnim(const String& msg) : _msg(msg) {}
    void enter(Renderer&, SoundPlayer&) override;
    void update(Renderer&, SoundPlayer&, uint32_t) override;
private:
    String   _msg;
    uint32_t _startMs = 0;
};
