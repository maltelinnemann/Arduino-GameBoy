#ifndef HOMESCREEN_H
#define HOMESCREEN_H

#include "game_base.h"

#define HOMESCREEN_MAX_GAMES 5

enum class HSState : uint8_t {
    BOOT_ANIMATION,
    IDLE,
    SELECTING
};

struct GameEntry {
    Game* game;
    const uint8_t* icon;
};

struct Particle {
    float x, y;
    float vx, vy;
    uint8_t life;
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

    HSState _state;
    unsigned long _stateStartMs;
    unsigned long _animPhaseMs;
    uint8_t _animPhase;
    uint8_t _revealChars;
    bool _idleBlink;

    static const int MAX_PARTICLES = 30;
    Particle _particles[MAX_PARTICLES];

    void drawCard(int idx, int centerX);

    void drawBootAnimation();
    void drawIdle();
    void drawSelection();
    void spawnParticles(int cx, int cy, int count);
    void updateParticles();
};

#endif
