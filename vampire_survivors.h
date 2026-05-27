#ifndef VAMPIRE_SURVIVORS_H
#define VAMPIRE_SURVIVORS_H

#include "game_base.h"

enum class VSState : uint8_t {
    MENU,
    PLAYING,
    UPGRADE_SELECT,
    PAUSED,
    GAME_OVER
};

struct VSPlayer {
    float x, y;
    int hp, maxHp;
    float moveSpeed;
    int fireRateMs;
    int bulletCount;
    int damage;
    bool spreadShot;
    bool backShot;
    unsigned long lastShotMs;
    unsigned long invulnerableUntil;
    uint8_t facingDir; // 0 right, 1 up, 2 left, 3 down
};

struct VSEnemy {
    float x, y;
    float vx, vy;
    int hp;
    uint8_t type; // 0 basic, 1 fast, 2 tank, 3 shooter
    bool alive;
    unsigned long lastShotMs;
    unsigned long lastMoveMs;
};

struct VSBullet {
    float x, y;
    float vx, vy;
    bool active;
    bool playerOwned;
    int damage;
};

struct VSUpgrade {
    uint8_t id;
    const char* name;
    const char* desc;
};

class VampireSurvivorsGame : public Game {
public:
    VampireSurvivorsGame();

    virtual void setup() override;
    virtual GameResult loop(InputState& input) override;
    virtual const char* getName() const override { return "Godly"; }

private:
    VSState _state;
    int _menuSelection;
    unsigned long _menuEnteredTime;

    VSPlayer _player;

    static const int MAX_ENEMIES = 15;
    VSEnemy _enemies[MAX_ENEMIES];

    static const int MAX_BULLETS = 20;
    VSBullet _bullets[MAX_BULLETS];

    int _room;
    int _enemiesToSpawn;
    int _enemiesSpawned;
    int _aliveCount;
    unsigned long _lastSpawnMs;
    int _spawnIntervalMs;

    uint32_t _highScore; // best room reached

    // Pause
    int _pauseSelection;
    unsigned long _pauseMoveTime;
    static const int PAUSE_OPTIONS = 3;
    const char* _pauseLabels[PAUSE_OPTIONS];
    unsigned long _pauseEnteredTime;

    // Upgrade selection
    static const int UPGRADE_OPTIONS = 3;
    VSUpgrade _upgrades[UPGRADE_OPTIONS];
    int _upgradeSelection;
    unsigned long _upgradeMoveTime;

    unsigned long _gameOverTime;

    // Available upgrade pool
    static const int UPGRADE_POOL_SIZE = 7;
    static const VSUpgrade UPGRADE_POOL[UPGRADE_POOL_SIZE];

    void drawMenu();
    void drawGame();
    void drawPlaying();
    void drawUpgradeSelect();
    void drawPause();
    void drawGameOver();
    void drawUI();
    void drawPlayer();
    void drawEnemies();
    void drawBullets();

    void startGame();
    void startRoom();
    void spawnEnemy();
    void updatePlayer(InputState& input);
    void updateEnemies();
    void updateBullets();
    void firePlayerBullets(float dx, float dy);
    void fireEnemyBullet(const VSEnemy& e);
    void checkCollisions();
    void pickUpgrades();
    void applyUpgrade(uint8_t id);
    int findNearestEnemy(float& outDx, float& outDy);
};

#endif
