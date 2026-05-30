#ifndef ETERNAL_H
#define ETERNAL_H

#include "game_base.h"

enum class ETState : uint8_t {
    MENU,
    PLAYING,
    UPGRADE,
    PAUSED,
    GAME_OVER
};

struct ETEnemy {
    float x, altitude;
    float vx;
    uint8_t type;   // 0=bird, 1=UFO, 2=satellite
    bool alive;
    uint8_t animTimer;
};

struct ETCoin {
    float x, altitude;
    bool alive;
    uint8_t animTimer;
};

struct ETParticle {
    float x, y, vx, vy;
    uint8_t life;
};

class EternalGame : public Game {
public:
    EternalGame();

    virtual void setup() override;
    virtual GameResult loop(InputState& input) override;
    virtual const char* getName() const override { return "Eternal"; }

private:
    ETState _state;
    int _menuSelection;

    // Player
    float _x, _altitude;
    float _vx, _vy;
    float _fuel;
    int _hp;
    uint32_t _coinCount;
    uint8_t _rocketType;    // 0-3
    uint8_t _fuelLevel;     // 0-2
    uint8_t _thrustLevel;   // 0-2
    uint8_t _hullLevel;     // 0-2
    bool _thrusting;
    bool _grounded;
    unsigned long _groundedSince;
    unsigned long _invulnUntil;

    // Derived stats (recalculated after upgrades)
    float _maxFuel;
    float _thrustPower;
    int _maxHp;
    float _termVelUp;

    // Camera
    float _scrollY;

    // Object pools
    static const int MAX_ENEMIES = 6;
    ETEnemy _enemies[MAX_ENEMIES];
    static const int MAX_COINS = 10;
    ETCoin _coins[MAX_COINS];
    static const int MAX_PARTICLES = 20;
    ETParticle _particles[MAX_PARTICLES];

    // Spawning
    unsigned long _lastSpawnMs;
    float _nextCoinAlt;
    float _nextEnemyAlt;

    // Timing
    unsigned long _lastFrameMs;

    // Upgrade screen
    int _upgradeSelection;
    unsigned long _upgradeMoveTime;

    // Pause
    int _pauseSelection;
    unsigned long _pauseMoveTime;
    unsigned long _pauseEnteredTime;
    static const int PAUSE_OPTIONS = 3;
    const char* _pauseLabels[PAUSE_OPTIONS];

    // High score
    uint32_t _highScore;
    unsigned long _gameOverTime;

    void startGame();
    void recalcStats();
    void applyPhysics(InputState& input);
    void updateCamera();
    void spawnObjects();
    void updateEnemies();
    void updateCoins();
    void checkCollisions();
    void spawnParticles(float x, float y, int count);

    // Drawing
    void drawMenu();
    void drawGame();
    void drawPlaying();
    void drawPause();
    void drawUpgrade();
    void drawGameOver();

    void drawRocket(int cx, int topY);
    void drawFlame(int cx, int y, int h);
    void drawEnemySprite(int sx, int sy, uint8_t type, uint8_t timer);
    void drawCoinSprite(int sx, int sy, uint8_t timer);
    void drawCloud(int sx, int sy, uint8_t size);
    void drawBackground();
    void drawHUD();

    float worldToScreenY(float worldAlt) const;
    float playerScreenY() const;

    // Rocket data
    static const char* _rocketNames[4];
    static const int _rocketCosts[4];
    static const float _baseMaxFuel[4];
    static const float _baseThrust[4];
    static const int _baseMaxHp[4];
    static const int _bodyW[4];
    static const int _bodyH[4];
    static const int _noseH[4];
    static const int _upgradeCosts[3];

    int totalUpgrades() const;
    int upgradeCost(int currentLevel) const;
};

#endif
