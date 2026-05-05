#include "sound_player.h"
#include <M5StickCPlus.h>

void SoundPlayer::begin() {
    M5.Beep.begin();
    M5.Beep.setVolume(4);  // 0..10
    M5.Beep.mute();
}

void SoundPlayer::startNote(size_t i) {
    _idx = i;
    _noteStartMs = millis();
    if (_muted || _seq[i].freq == 0) M5.Beep.mute();
    else                             M5.Beep.tone(_seq[i].freq);
}

void SoundPlayer::setMuted(bool m) {
    _muted = m;
    if (m) M5.Beep.mute();
}

bool SoundPlayer::toggleMuted() {
    setMuted(!_muted);
    return _muted;
}

void SoundPlayer::play(const Note* seq, size_t len, bool loop) {
    if (len == 0 || seq == nullptr) return;
    _seq  = seq;
    _len  = len;
    _loop = loop;
    startNote(0);
}

void SoundPlayer::stop() {
    _seq = nullptr;
    _len = 0;
    _idx = 0;
    M5.Beep.mute();
}

void SoundPlayer::update() {
    if (!_seq) return;
    uint32_t elapsed = millis() - _noteStartMs;
    if (elapsed < _seq[_idx].durMs) return;

    size_t next = _idx + 1;
    if (next >= _len) {
        if (_loop) next = 0;
        else       { stop(); return; }
    }
    startNote(next);
}

// ---------- Sequences ----------
namespace snd {
    const Note SIREN[] = {
        {220, 180}, {330, 180}, {220, 180}, {330, 180},
    };
    const size_t SIREN_LEN = sizeof(SIREN) / sizeof(Note);

    const Note CAUGHT[] = {
        {523, 70}, {659, 70}, {784, 70}, {1046, 160},
    };
    const size_t CAUGHT_LEN = sizeof(CAUGHT) / sizeof(Note);

    const Note POWER_PELLET[] = {
        {200, 35}, {300, 35}, {400, 35}, {500, 35}, {600, 35}, {800, 60},
    };
    const size_t POWER_PELLET_LEN = sizeof(POWER_PELLET) / sizeof(Note);

    const Note SCARED_BGM[] = {
        {440, 120}, {494, 120}, {523, 120}, {494, 120},
    };
    const size_t SCARED_BGM_LEN = sizeof(SCARED_BGM) / sizeof(Note);

    const Note JINGLE_LUCKY[] = {
        {523, 120}, {659, 120}, {784, 120}, {1046, 260},
    };
    const size_t JINGLE_LUCKY_LEN = sizeof(JINGLE_LUCKY) / sizeof(Note);

    const Note JINGLE_WARNING[] = {
        {220, 150}, {330, 150}, {220, 150}, {330, 150}, {220, 250},
    };
    const size_t JINGLE_WARNING_LEN = sizeof(JINGLE_WARNING) / sizeof(Note);

    const Note JINGLE_CALM[] = {
        {349, 220}, {440, 220}, {523, 400},
    };
    const size_t JINGLE_CALM_LEN = sizeof(JINGLE_CALM) / sizeof(Note);

    const Note JINGLE_BOLD[] = {
        {200, 45}, {400, 45}, {600, 45}, {800, 45}, {1000, 90}, {1046, 200},
    };
    const size_t JINGLE_BOLD_LEN = sizeof(JINGLE_BOLD) / sizeof(Note);

    const Note JINGLE_LOVE[] = {
        {659, 90}, {0, 40}, {659, 90}, {0, 90}, {440, 280},
    };
    const size_t JINGLE_LOVE_LEN = sizeof(JINGLE_LOVE) / sizeof(Note);

    const Note TYPE_CLICK[] = { {1200, 8} };
    const size_t TYPE_CLICK_LEN = sizeof(TYPE_CLICK) / sizeof(Note);

    const Note EAT_WAKA[] = { {500, 30}, {250, 30} };
    const size_t EAT_WAKA_LEN = sizeof(EAT_WAKA) / sizeof(Note);

    const Note DEATH[] = {
        {523, 70}, {494, 70}, {440, 70}, {392, 70}, {349, 70},
        {329, 70}, {293, 70}, {261, 70}, {196, 180},
    };
    const size_t DEATH_LEN = sizeof(DEATH) / sizeof(Note);

    const Note IDLE_WAKA[] = {
        {500, 60}, {250, 60}, {0, 1500},
    };
    const size_t IDLE_WAKA_LEN = sizeof(IDLE_WAKA) / sizeof(Note);
}
