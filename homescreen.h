#ifndef HOMESCREEN_H
#define HOMESCREEN_H

#include "game_base.h"

#define HOMESCREEN_MAX_GAMES 5

class Homescreen {
public:
    Homescreen();

    void addGame(Game* game);
    void setup();
    GameResult loop(InputState& input);
    Game* getSelectedGame() const;

private:
    Game* _games[HOMESCREEN_MAX_GAMES];
    int _numGames;
    int _selectedIndex;
    unsigned long _lastMoveTime;

    void draw();
};

#endif
