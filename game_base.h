#ifndef GAME_BASE_H
#define GAME_BASE_H

#include <Arduino.h>
#include <U8g2lib.h>

struct InputState;

enum class GameResult : uint8_t {
    CONTINUE,
    GAME_SELECTED,
    EXIT_TO_MENU
};

class Game {
public:
    virtual ~Game() {}

    virtual void setup() = 0;
    virtual GameResult loop(InputState& input) = 0;
    virtual const char* getName() const = 0;
};

#endif
