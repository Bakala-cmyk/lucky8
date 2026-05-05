#pragma once
#include <Arduino.h>

struct Note {
    uint16_t freq;   // Hz. 0 = rest (silent for durMs)
    uint16_t durMs;
};

class SoundPlayer {
public:
    void begin();
    // seq must live at least as long as playback. Use `constexpr` arrays.
    void play(const Note* seq, size_t len, bool loop = false);
    void stop();
    void update();              // call every loop tick
    bool playing() const { return _seq != nullptr; }

    void setMuted(bool m);
    bool toggleMuted();         // returns new muted state
    bool isMuted() const { return _muted; }

private:
    const Note* _seq     = nullptr;
    size_t      _len     = 0;
    size_t      _idx     = 0;
    uint32_t    _noteStartMs = 0;
    bool        _loop    = false;
    bool        _muted   = false;

    void startNote(size_t i);
};

// ------- Predefined sequences -------
namespace snd {
    extern const Note SIREN[];          extern const size_t SIREN_LEN;
    extern const Note CAUGHT[];         extern const size_t CAUGHT_LEN;
    extern const Note POWER_PELLET[];   extern const size_t POWER_PELLET_LEN;
    extern const Note SCARED_BGM[];     extern const size_t SCARED_BGM_LEN;

    extern const Note JINGLE_LUCKY[];   extern const size_t JINGLE_LUCKY_LEN;
    extern const Note JINGLE_WARNING[]; extern const size_t JINGLE_WARNING_LEN;
    extern const Note JINGLE_CALM[];    extern const size_t JINGLE_CALM_LEN;
    extern const Note JINGLE_BOLD[];    extern const size_t JINGLE_BOLD_LEN;
    extern const Note JINGLE_LOVE[];    extern const size_t JINGLE_LOVE_LEN;

    extern const Note TYPE_CLICK[];     extern const size_t TYPE_CLICK_LEN;
    extern const Note EAT_WAKA[];       extern const size_t EAT_WAKA_LEN;
    extern const Note DEATH[];          extern const size_t DEATH_LEN;
    extern const Note IDLE_WAKA[];      extern const size_t IDLE_WAKA_LEN;
}
