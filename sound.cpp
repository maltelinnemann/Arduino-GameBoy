#include "sound.h"
#include "hardware.h"

void SoundManager::init(int pin) {
    _pin = pin;
    _active = false;
    _endTime = 0;
}

void SoundManager::beep(unsigned int durationMs) {
    digitalWrite(_pin, HIGH);
    _active = true;
    _endTime = millis() + durationMs;
}

void SoundManager::update() {
    if (_active && millis() >= _endTime) {
        digitalWrite(_pin, LOW);
        _active = false;
    }
}

bool SoundManager::isPlaying() const {
    return _active;
}
