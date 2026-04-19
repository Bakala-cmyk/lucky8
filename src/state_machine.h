#pragma once
#include <Arduino.h>

enum class AppState {
    IDLE,
    CONNECTING,
    LOADING,
    DISPLAYING,
    ERROR
};

class StateMachine {
public:
    AppState current() const { return _state; }
    uint32_t stateEnteredAt() const { return _enteredAt; }

    void transition(AppState next) {
        _state     = next;
        _enteredAt = millis();
    }

private:
    AppState _state     = AppState::IDLE;
    uint32_t _enteredAt = 0;
};
