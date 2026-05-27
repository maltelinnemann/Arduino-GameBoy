#ifndef HOMESCREEN_H
#define HOMESCREEN_H

#include "game_base.h"

#define HOMESCREEN_MAX_GAMES 5

struct GameEntry {
    Game* game;
    const uint8_t* icon;
};

class Homescreen {
public:
    Homescreen();

    void addGame(Game* game, const uint8_t* icon);
    void setup();
    GameResult loop(InputState& input);
    Game* getSelectedGame() const;

private:
    GameEntry _entries[HOMESCREEN_MAX_GAMES];
    int _numGames;
    int _selectedIndex;
    float _scrollPos;
    unsigned long _lastMoveTime;
    unsigned long _lastFrameTime;

    void draw();
    void drawCard(int idx, int centerX);
};

#endif
