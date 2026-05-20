#ifndef DINO_JUMP_H
#define DINO_JUMP_H

#include "game_base.h"

enum class DJState : uint8_t {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER
};

struct DJPlayer {
    float x, y;
    float vy;
    bool onGround;
    int animFrame;
    unsigned long lastAnimTime;
};

struct Obstacle {
    float x, y;
    uint8_t w, h;
    uint8_t type; // 0 small, 1 tall, 2 double
    bool active;
};

class DinoJumpGame : public Game {
public:
    DinoJumpGame();

    virtual void setup() override;
    virtual GameResult loop(InputState& input) override;
    virtual const char* getName() const override { return "Dino Jump"; }

private:
    DJState _state;
    int _menuSelection;

    DJPlayer _player;
    static const int MAX_OBSTACLES = 6;
    Obstacle _obstacles[MAX_OBSTACLES];

    float _speed;
    unsigned long _lastSpawnMs;
    int _spawnIntervalMs;
    unsigned long _lastSpeedIncreaseMs;

    uint32_t _score;
    uint32_t _highScore;

    int _pauseSelection;
    static const int PAUSE_OPTIONS = 3;
    const char* _pauseLabels[PAUSE_OPTIONS];

    unsigned long _gameOverTime;
    unsigned long _pauseEnteredTime;

    static constexpr float GRAVITY = 0.55f;
    static constexpr float JUMP_VEL = -6.5f;
    static constexpr float GROUND_Y = 56.0f;
    static constexpr float PLAYER_X = 20.0f;

    void drawMenu();
    void drawPlaying();
    void drawPause();
    void drawGameOver();
    void drawPhallus(float px, float py);
    void drawGround();
    void drawScore();
    void drawObstacle(const Obstacle& obs);

    void startGame();
    void spawnObstacle();
    void updatePlayer(InputState& input);
    void updateObstacles();
    void checkCollisions();
    bool collidesWith(const Obstacle& obs) const;
    void saveHighScore();
};

#endif
