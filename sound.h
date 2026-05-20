#ifndef SOUND_H
#define SOUND_H

#include <Arduino.h>

class SoundManager {
public:
    void init(int pin);
    void beep(unsigned int durationMs);
    void update();
    bool isPlaying() const;

private:
    int _pin;
    bool _active;
    unsigned long _endTime;
};

#endif
