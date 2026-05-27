#ifndef ORIGIN_H
#define ORIGIN_H

#include "game_base.h"

enum class OGState : uint8_t {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER
};

struct OGSegment {
    int8_t x, y; // grid cell (0-15, 0-7)
};

class OriginGame : public Game {
public:
    OriginGame();

    virtual void setup() override;
    virtual GameResult loop(InputState& input) override;
    virtual const char* getName() const override { return "Origin"; }

private:
    OGState _state;
    int _menuSelection;
    unsigned long _menuEnteredTime;

    static const int MAX_LENGTH = 100;
    OGSegment _snake[MAX_LENGTH];
    int _length;
    int8_t _dirX, _dirY;
    int8_t _nextDirX, _nextDirY;
    int8_t _foodX, _foodY;
    unsigned long _lastMoveMs;
    int _moveIntervalMs;
    bool _boost;
    int _foodEaten;
    uint32_t _highScore;

    // Pause
    int _pauseSelection;
    unsigned long _pauseMoveTime;
    static const int PAUSE_OPTIONS = 3;
    const char* _pauseLabels[PAUSE_OPTIONS];
    unsigned long _pauseEnteredTime;
    unsigned long _gameOverTime;

    void drawMenu();
    void drawGame();
    void drawPlaying();
    void drawPause();
    void drawGameOver();

    void startGame();
    void spawnFood();
    void moveSnake();
    bool isOccupied(int8_t x, int8_t y);
    void updateDirection(InputState& input);
};

#endif
