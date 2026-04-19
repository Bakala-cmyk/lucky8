#pragma once
#include <Arduino.h>
#include "display_manager.h"
#include "sound_player.h"
#include "api_client.h"

class Animation {
public:
    virtual ~Animation() = default;
    virtual void enter(Renderer& r, SoundPlayer& s) = 0;
    virtual void update(Renderer& r, SoundPlayer& s, uint32_t nowMs) = 0;
    virtual bool done() const { return false; }
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
    DisplayingAnim(const Fortune& f, uint32_t holdSeconds);
    void enter(Renderer&, SoundPlayer&) override;
    void update(Renderer&, SoundPlayer&, uint32_t) override;
    bool done() const override { return _phase == Phase::FINISHED; }
    void skipToEat();   // called if BtnA during TYPING/HOLD

private:
    enum class Phase { TYPING, HOLD, EAT, FINISHED };

    Fortune  _f;
    uint32_t _holdMs;
    Phase    _phase    = Phase::TYPING;
    uint32_t _phaseStartMs = 0;

    // per-char laid out bounding boxes
    struct Glyph { char c; int16_t x, y; int16_t w; };
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
