#ifndef SPACE_INVADERS_H
#define SPACE_INVADERS_H

#include "game_base.h"

enum class EnemyType : uint8_t {
    BASIC,
    SINGLE_SHOT,
    MULTI_SHOT
};

enum class SIState : uint8_t {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER
};

struct Bullet {
    float x, y;
    float speedY;
    bool active;
};

struct Enemy {
    float x, y;
    float speedX, speedY;
    uint8_t w, h;
    EnemyType type;
    bool alive;
    unsigned long nextFireMs;
};

struct PowerUp {
    float x, y;
    float speedY;
    bool active;
    unsigned long spawnTime;
};

class SpaceInvadersGame : public Game {
public:
    SpaceInvadersGame();

    virtual void setup() override;
    virtual GameResult loop(InputState& input) override;
    virtual const char* getName() const override { return "Infinity"; }

private:
    SIState _state;
    int _menuSelection;

    // Player
    float _playerX, _playerY;
    int _lives;
    unsigned long _invulnerableUntil;
    bool _invulnVisible;

    // Bullet pools
    static const int MAX_PLAYER_BULLETS = 3;
    static const int MAX_ENEMY_BULLETS = 15;
    Bullet _playerBullets[MAX_PLAYER_BULLETS];
    Bullet _enemyBullets[MAX_ENEMY_BULLETS];
    unsigned long _lastShotMs;

    // Enemies
    static const int MAX_ENEMIES = 20;
    Enemy _enemies[MAX_ENEMIES];

    // PowerUp
    PowerUp _powerUp;

    // Wave
    int _wave;
    int _enemiesSpawned;
    int _maxEnemiesWave;
    unsigned long _lastSpawnMs;
    int _spawnIntervalMs;
    int _aliveCount;

    // Score
    uint32_t _score;
    uint32_t _highScore;

    // Pause
    int _pauseSelection;
    unsigned long _pauseMoveTime;
    static const int PAUSE_OPTIONS = 3;
    const char* _pauseLabels[PAUSE_OPTIONS];

    // Game Over
    unsigned long _gameOverTime;
    unsigned long _pauseEnteredTime;
    unsigned long _menuEnteredTime;

    void drawMenu();
    void drawGame();
    void drawPlaying();
    void drawPause();
    void drawGameOver();
    void drawUI();
    void drawShip();

    void startGame();
    void spawnWave();
    void spawnEnemy();
    void spawnPowerUp(float x, float y);
    void firePlayerBullet();
    void fireEnemyBullet(int idx);
    void updateBullets();
    void updateEnemies();
    void updatePowerUp();
    void checkCollisions();
    int getWaveSpawnInterval() const;
    float getSpeedMultiplier() const;
};

#endif
