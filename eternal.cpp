#include "eternal.h"
#include "hardware.h"
#include "sound.h"
#include "bitmaps.h"
#include <math.h>

extern SoundManager soundManager;

// ==================== ROCKET DATA ====================

const char* EternalGame::_rocketNames[4] = {
    "Firefly", "Star Chaser", "Nebula", "Cosmos"
};
const int EternalGame::_rocketCosts[4] = { 0, 20, 50, 100 };
const float EternalGame::_baseMaxFuel[4] = { 150.0f, 220.0f, 300.0f, 400.0f };
const float EternalGame::_baseThrust[4] = { 0.24f, 0.32f, 0.40f, 0.50f };
const int EternalGame::_baseMaxHp[4] = { 3, 5, 7, 10 };
const int EternalGame::_bodyW[4] = { 6, 8, 10, 12 };
const int EternalGame::_bodyH[4] = { 8, 10, 12, 14 };
const int EternalGame::_noseH[4] = { 3, 4, 5, 6 };
const int EternalGame::_upgradeCosts[3] = { 5, 12, 22 };

static const int GROUND_Y = 56;

// ==================== CONSTRUCTOR / SETUP ====================

EternalGame::EternalGame() {
    _pauseLabels[0] = "Resume";
    _pauseLabels[1] = "Restart";
    _pauseLabels[2] = "Menu";
}

void EternalGame::setup() {
    _state = ETState::MENU;
    _menuSelection = 0;
    _highScore = readHighScore(EEPROM_ET_SCORE_ADDR);
    if (_highScore > 999999) {
        _highScore = 0;
        EEPROM.put(EEPROM_ET_SCORE_ADDR, (uint32_t)0);
    }
}

// ==================== MAIN LOOP ====================

GameResult EternalGame::loop(InputState& input) {
    unsigned long now = millis();
    float dt = (now - _lastFrameMs) / 1000.0f;
    if (dt > 0.1f) dt = 0.1f;
    _lastFrameMs = now;

    switch (_state) {
        case ETState::MENU: {
            if (input.up() && _menuSelection > 0) _menuSelection--;
            if (input.down() && _menuSelection < 1) _menuSelection++;
            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                if (_menuSelection == 0) startGame();
                else return GameResult::EXIT_TO_MENU;
            }
            drawMenu();
            return GameResult::CONTINUE;
        }

        case ETState::PLAYING: {
            if (input.btn2.consumePress()) {
                _state = ETState::PAUSED;
                _pauseSelection = 0;
                _pauseEnteredTime = now;
                _pauseMoveTime = 0;
                drawPause();
                return GameResult::CONTINUE;
            }

            applyPhysics(input);
            updateCamera();
            if (now - _lastSpawnMs > 500) {
                spawnObjects();
                _lastSpawnMs = now;
            }
            updateEnemies();
            updateCoins();
            checkCollisions();

            // Grounded idle triggers upgrade screen
            if (_grounded && now - _groundedSince > 1500 && _state == ETState::PLAYING) {
                _state = ETState::UPGRADE;
                _upgradeSelection = 4; // Launch is default
                _upgradeMoveTime = now;
                _fuel = _maxFuel;
                _hp = _maxHp;
                _thrusting = false;
            }

            if (_hp <= 0) {
                _state = ETState::GAME_OVER;
                uint32_t alt = (uint32_t)_altitude;
                writeHighScore(EEPROM_ET_SCORE_ADDR, alt);
                _highScore = readHighScore(EEPROM_ET_SCORE_ADDR);
                _gameOverTime = now;
                soundManager.beep(200);
            }

            drawPlaying();
            return GameResult::CONTINUE;
        }

        case ETState::UPGRADE: {
            if (now - _upgradeMoveTime > 200) {
                if (input.up() && _upgradeSelection > 0) {
                    _upgradeSelection--;
                    _upgradeMoveTime = now;
                }
                if (input.down() && _upgradeSelection < 4) {
                    _upgradeSelection++;
                    _upgradeMoveTime = now;
                }
            }
            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                bool bought = false;
                switch (_upgradeSelection) {
                    case 0: // Fuel Tank
                        if (_fuelLevel < 2 && _coinCount >= (uint32_t)upgradeCost(_fuelLevel)) {
                            _coinCount -= upgradeCost(_fuelLevel);
                            _fuelLevel++;
                            bought = true;
                        }
                        break;
                    case 1: // Thrusters
                        if (_thrustLevel < 2 && _coinCount >= (uint32_t)upgradeCost(_thrustLevel)) {
                            _coinCount -= upgradeCost(_thrustLevel);
                            _thrustLevel++;
                            bought = true;
                        }
                        break;
                    case 2: // Hull
                        if (_hullLevel < 2 && _coinCount >= (uint32_t)upgradeCost(_hullLevel)) {
                            _coinCount -= upgradeCost(_hullLevel);
                            _hullLevel++;
                            bought = true;
                        }
                        break;
                    case 3: // New Rocket
                        if (_rocketType < 3 && _coinCount >= (uint32_t)_rocketCosts[_rocketType + 1]) {
                            _coinCount -= _rocketCosts[_rocketType + 1];
                            _rocketType++;
                            _fuelLevel = 0;
                            _thrustLevel = 0;
                            _hullLevel = 0;
                            bought = true;
                        }
                        break;
                    case 4: // Launch
                        recalcStats();
                        _altitude = 0;
                        _vy = 0;
                        _vx = 0;
                        _x = 64;
                        _fuel = _maxFuel;
                        _hp = _maxHp;
                        _grounded = true;
                        _groundedSince = now;
                        _scrollY = 0;
                        _thrusting = false;
                        _invulnUntil = 0;
                        // Reset pools
                        for (int i = 0; i < MAX_ENEMIES; i++) _enemies[i].alive = false;
                        for (int i = 0; i < MAX_COINS; i++) _coins[i].alive = false;
                        for (int i = 0; i < MAX_PARTICLES; i++) _particles[i].life = 0;
                        _nextCoinAlt = 20;
                        _nextEnemyAlt = 60;
                        _state = ETState::PLAYING;
                        _lastFrameMs = now;
                        break;
                }
                if (bought) {
                    recalcStats();
                    soundManager.beep(30);
                }
            }
            if (input.btn2.consumePress()) {
                recalcStats();
                _altitude = 0;
                _vy = 0;
                _vx = 0;
                _x = 64;
                _fuel = _maxFuel;
                _hp = _maxHp;
                _grounded = true;
                _groundedSince = now;
                _scrollY = 0;
                _thrusting = false;
                _invulnUntil = 0;
                for (int i = 0; i < MAX_ENEMIES; i++) _enemies[i].alive = false;
                for (int i = 0; i < MAX_COINS; i++) _coins[i].alive = false;
                for (int i = 0; i < MAX_PARTICLES; i++) _particles[i].life = 0;
                _nextCoinAlt = 20;
                _nextEnemyAlt = 60;
                _state = ETState::PLAYING;
                _lastFrameMs = now;
            }
            drawUpgrade();
            return GameResult::CONTINUE;
        }

        case ETState::PAUSED: {
            if (now - _pauseMoveTime > 180) {
                if (input.up() && _pauseSelection > 0) {
                    _pauseSelection--;
                    _pauseMoveTime = now;
                }
                if (input.down() && _pauseSelection < PAUSE_OPTIONS - 1) {
                    _pauseSelection++;
                    _pauseMoveTime = now;
                }
            }
            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                if (_pauseSelection == 0) {
                    _state = ETState::PLAYING;
                    _lastFrameMs = now;
                } else if (_pauseSelection == 1) startGame();
                else {
                    _state = ETState::MENU;
                    _menuSelection = 0;
                }
            }
            if (input.btn2.consumePress() && now - _pauseEnteredTime > 300) {
                _state = ETState::PLAYING;
                _lastFrameMs = now;
            }
            drawPause();
            return GameResult::CONTINUE;
        }

        case ETState::GAME_OVER: {
            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                _state = ETState::MENU;
                _menuSelection = 0;
            } else if (input.btn2.consumePress()) {
                return GameResult::EXIT_TO_MENU;
            }
            drawGameOver();
            return GameResult::CONTINUE;
        }
    }
    return GameResult::CONTINUE;
}

// ==================== GAME START ====================

void EternalGame::startGame() {
    _state = ETState::PLAYING;
    _rocketType = 0;
    _fuelLevel = 0;
    _thrustLevel = 0;
    _hullLevel = 0;
    _coinCount = 0;
    recalcStats();

    _x = 64;
    _altitude = 0;
    _vx = 0;
    _vy = 0;
    _fuel = _maxFuel;
    _hp = _maxHp;
    _grounded = true;
    _groundedSince = millis();
    _thrusting = false;
    _scrollY = 0;
    _invulnUntil = 0;

    for (int i = 0; i < MAX_ENEMIES; i++) _enemies[i].alive = false;
    for (int i = 0; i < MAX_COINS; i++) _coins[i].alive = false;
    for (int i = 0; i < MAX_PARTICLES; i++) _particles[i].life = 0;

    _nextCoinAlt = 20;
    _nextEnemyAlt = 60;
    _lastSpawnMs = 0;
    _lastFrameMs = millis();
}

void EternalGame::recalcStats() {
    _maxFuel = _baseMaxFuel[_rocketType] + _fuelLevel * 80.0f;
    _thrustPower = _baseThrust[_rocketType] + _thrustLevel * 0.06f;
    _maxHp = _baseMaxHp[_rocketType] + _hullLevel * 2;
    _termVelUp = 1.8f + _thrustLevel * 0.4f;
}

// ==================== PHYSICS ====================

void EternalGame::applyPhysics(InputState& input) {
    bool wantThrust = input.btn1.held() || input.joyButton.held();

    if (_grounded) {
        if (wantThrust && _fuel > 0) {
            _grounded = false;
            _groundedSince = 0;
            _vy = _thrustPower;
            _fuel -= 0.5f;
            _thrusting = true;
        } else {
            _thrusting = false;
            return;
        }
    }

    // Horizontal movement (clunky)
    if (input.left())  _vx -= 0.12f;
    if (input.right()) _vx += 0.12f;
    _vx *= 0.90f;
    if (_vx > 1.8f) _vx = 1.8f;
    if (_vx < -1.8f) _vx = -1.8f;

    // Thrust
    _thrusting = false;
    if (wantThrust && _fuel > 0) {
        _vy += _thrustPower;
        _fuel -= 0.5f;
        if (_fuel < 0) _fuel = 0;
        _thrusting = true;
    }

    // Gravity
    _vy -= 0.08f;

    // Terminal velocity
    if (_vy > _termVelUp) _vy = _termVelUp;
    if (_vy < -3.0f) _vy = -3.0f;

    // Integrate
    _altitude += _vy;
    _x += _vx;

    // Wrap X
    if (_x < -4) _x = 132;
    if (_x > 132) _x = -4;

    // Ground collision
    if (_altitude <= 0) {
        _altitude = 0;
        _vy = 0;
        if (!_grounded) {
            _grounded = true;
            _groundedSince = millis();
            _fuel = _maxFuel; // refuel on landing
        }
    }

    // Invulnerability timer tick
    if (millis() > _invulnUntil) _invulnUntil = 0;

    // Thrust particles
    if (_thrusting) {
        int psy = (int)playerScreenY() + _bodyH[_rocketType] + _noseH[_rocketType];
        int psx = (int)_x;
        spawnParticles(psx + random(-2, 3), psy + random(0, 3), 1);
    }
}

// ==================== CAMERA ====================

void EternalGame::updateCamera() {
    _scrollY = _altitude - 30.0f;
    if (_scrollY < 0) _scrollY = 0;
}

float EternalGame::worldToScreenY(float worldAlt) const {
    return (float)GROUND_Y - worldAlt + _scrollY;
}

float EternalGame::playerScreenY() const {
    float a = _altitude < 30.0f ? _altitude : 30.0f;
    return (float)GROUND_Y - a;
}

// ==================== SPAWNING ====================

void EternalGame::spawnObjects() {
    // Spawn coins ahead of player
    if (_altitude + 40 > _nextCoinAlt) {
        for (int i = 0; i < MAX_COINS; i++) {
            if (!_coins[i].alive) {
                _coins[i].x = random(5, 123);
                _coins[i].altitude = _altitude + random(25, 90);
                _coins[i].alive = true;
                _coins[i].animTimer = 0;
                _nextCoinAlt = _coins[i].altitude + random(15, 50);
                break;
            }
        }
    }

    // Spawn enemies ahead
    if (_altitude + 60 > _nextEnemyAlt) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!_enemies[i].alive) {
                _enemies[i].x = random(5, 123);
                _enemies[i].altitude = _altitude + random(40, 110);
                _enemies[i].alive = true;
                _enemies[i].animTimer = 0;
                if (_altitude < 150) {
                    _enemies[i].type = 0; // bird only
                    _enemies[i].vx = (random(0, 2) == 0) ? -0.5f : 0.5f;
                } else if (_altitude < 400) {
                    _enemies[i].type = random(0, 2); // bird or UFO
                    _enemies[i].vx = (_enemies[i].type == 1) ? 0.3f : ((random(0, 2) == 0) ? -0.6f : 0.6f);
                } else {
                    _enemies[i].type = random(1, 3); // UFO or satellite
                    _enemies[i].vx = (_enemies[i].type == 2) ? 1.0f : 0.4f;
                }
                _nextEnemyAlt = _enemies[i].altitude + random(50, 140);
                break;
            }
        }
    }
}

// ==================== ENEMY UPDATES ====================

void EternalGame::updateEnemies() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!_enemies[i].alive) continue;
        ETEnemy& e = _enemies[i];

        e.animTimer++;

        // Movement by type
        switch (e.type) {
            case 0: // Bird — horizontal back-and-forth with slight bob
                e.x += e.vx;
                if (e.x < 5 || e.x > 123) e.vx = -e.vx;
                break;
            case 1: // UFO — gentle sine wave
                e.x += sin(e.animTimer * 0.04f) * 0.8f;
                e.altitude += cos(e.animTimer * 0.03f) * 0.2f;
                if (e.x < 0) e.x = 0;
                if (e.x > 128) e.x = 128;
                break;
            case 2: // Satellite — fast horizontal, moves right
                e.x += e.vx;
                if (e.x > 140) {
                    e.alive = false;
                }
                break;
        }

        // Remove if too far below
        if (e.altitude < _altitude - 80) {
            e.alive = false;
        }
    }
}

// ==================== COIN UPDATES ====================

void EternalGame::updateCoins() {
    for (int i = 0; i < MAX_COINS; i++) {
        if (!_coins[i].alive) continue;
        _coins[i].animTimer++;
        // Coins slowly drift downward relative to world
        _coins[i].altitude -= 0.03f;
        // Remove if too far below
        if (_coins[i].altitude < _altitude - 80) {
            _coins[i].alive = false;
        }
    }
}

// ==================== COLLISIONS ====================

void EternalGame::checkCollisions() {
    int bw = _bodyW[_rocketType];
    int bh = _bodyH[_rocketType] + _noseH[_rocketType];
    int psy = (int)playerScreenY();
    int psx = (int)_x;

    // Player hitbox (slightly forgiving)
    int pl = psx - bw / 3;
    int pr = psx + bw / 3;
    int pt = psy + bh / 4;
    int pb = psy + bh;

    // Enemy collisions
    if (_invulnUntil == 0) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!_enemies[i].alive) continue;
            int esy = (int)worldToScreenY(_enemies[i].altitude);
            int esx = (int)_enemies[i].x;
            if (esy < -10 || esy > 70) continue;

            int el = esx - 4, er = esx + 4;
            int et = esy - 3, eb = esy + 4;

            if (pl < er && pr > el && pt < eb && pb > et) {
                _hp--;
                _invulnUntil = millis() + 1500;
                spawnParticles(psx, (psy + pb) / 2, 8);
                soundManager.beep(60);
                _enemies[i].alive = false;
                if (_hp <= 0) return;
            }
        }
    }

    // Coin collisions
    for (int i = 0; i < MAX_COINS; i++) {
        if (!_coins[i].alive) continue;
        int csy = (int)worldToScreenY(_coins[i].altitude);
        int csx = (int)_coins[i].x;
        if (csy < -10 || csy > 70) continue;

        int cl = csx - 3, cr = csx + 3;
        int ct = csy - 3, cb = csy + 3;

        if (pl < cr && pr > cl && pt < cb && pb > ct) {
            _coinCount++;
            _coins[i].alive = false;
            spawnParticles(csx, csy, 3);
            soundManager.beep(10);
        }
    }

    // Update particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (_particles[i].life > 0) {
            _particles[i].x += _particles[i].vx;
            _particles[i].y += _particles[i].vy;
            _particles[i].vx *= 0.94f;
            _particles[i].vy *= 0.94f;
            _particles[i].life--;
        }
    }
}

void EternalGame::spawnParticles(float x, float y, int count) {
    for (int i = 0; i < MAX_PARTICLES && count > 0; i++) {
        if (_particles[i].life == 0) {
            float angle = random(0, 628) / 100.0f;
            float speed = random(20, 80) / 100.0f;
            _particles[i].x = x;
            _particles[i].y = y;
            _particles[i].vx = cos(angle) * speed;
            _particles[i].vy = sin(angle) * speed;
            _particles[i].life = random(8, 18);
            count--;
        }
    }
}

// ==================== DRAW MENU ====================

void EternalGame::drawMenu() {
    u8g2.clearBuffer();

    u8g2.drawXBMP(56, 2, 16, 16, ICON_ETERNAL);

    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[32];
    snprintf(buf, sizeof(buf), "Best: %lu", _highScore);
    drawCenteredStr(26, buf);

    drawMenuOption(42, "Start Game", _menuSelection == 0);
    drawMenuOption(54, "Return to Menu", _menuSelection == 1);

    u8g2.sendBuffer();
}

// ==================== DRAW GAME (no sendBuffer) ====================

void EternalGame::drawGame() {
    u8g2.clearBuffer();

    // Background
    drawBackground();

    // Ground line
    float groundScreenY = (float)GROUND_Y + _scrollY;
    if (groundScreenY < 64 && groundScreenY >= 0) {
        u8g2.drawHLine(0, (int)groundScreenY, 128);
        // Ground fill
        for (int gy = (int)groundScreenY + 1; gy < 64; gy++) {
            u8g2.drawHLine(0, gy, 128);
        }
    }

    // Coins
    for (int i = 0; i < MAX_COINS; i++) {
        if (!_coins[i].alive) continue;
        int sy = (int)worldToScreenY(_coins[i].altitude);
        if (sy < -5 || sy > 64) continue;
        drawCoinSprite((int)_coins[i].x, sy, _coins[i].animTimer);
    }

    // Enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!_enemies[i].alive) continue;
        int sy = (int)worldToScreenY(_enemies[i].altitude);
        if (sy < -10 || sy > 64) continue;
        drawEnemySprite((int)_enemies[i].x, sy, _enemies[i].type, _enemies[i].animTimer);
    }

    // Rocket
    int psy = (int)playerScreenY();
    int psx = (int)_x;
    // Invulnerability blink
    if (_invulnUntil > 0 && ((millis() / 80) % 2) == 0) {
        // skip drawing rocket (blink effect)
    } else {
        drawRocket(psx, psy);
    }

    // Particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (_particles[i].life > 0) {
            u8g2.drawPixel((int)_particles[i].x, (int)_particles[i].y);
        }
    }

    // HUD
    drawHUD();
}

void EternalGame::drawPlaying() {
    drawGame();
    u8g2.sendBuffer();
}

void EternalGame::drawPause() {
    drawGame();

    u8g2.setDrawColor(0);
    u8g2.drawBox(0, 16, 128, 48);
    u8g2.setDrawColor(1);
    u8g2.drawFrame(0, 16, 128, 48);

    u8g2.setFont(u8g2_font_6x10_tf);
    drawCenteredStr(28, "PAUSED");
    for (int i = 0; i < PAUSE_OPTIONS; i++) {
        drawMenuOption(40 + i * 10, _pauseLabels[i], _pauseSelection == i);
    }
    u8g2.sendBuffer();
}

// ==================== DRAW UPGRADE ====================

void EternalGame::drawUpgrade() {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_6x10_tf);
    drawCenteredStr(9, "ETERNAL");

    u8g2.setFont(u8g2_font_5x7_tf);
    char buf[32];

    // Coins
    snprintf(buf, sizeof(buf), "Coins: %lu", _coinCount);
    u8g2.drawStr(4, 20, buf);

    // Divider
    u8g2.drawHLine(0, 23, 128);

    // Rocket name
    snprintf(buf, sizeof(buf), "%s", _rocketNames[_rocketType]);
    u8g2.drawStr(4, 31, buf);

    // Upgrade options
    const char* labels[5];
    char optBuf[5][32];

    if (_fuelLevel < 2) {
        snprintf(optBuf[0], sizeof(optBuf[0]), "Fuel Lv%d -> %d  %dc",
            _fuelLevel, _fuelLevel + 1, upgradeCost(_fuelLevel));
    } else {
        snprintf(optBuf[0], sizeof(optBuf[0]), "Fuel Tank: MAX");
    }

    if (_thrustLevel < 2) {
        snprintf(optBuf[1], sizeof(optBuf[1]), "Thrust Lv%d -> %d  %dc",
            _thrustLevel, _thrustLevel + 1, upgradeCost(_thrustLevel));
    } else {
        snprintf(optBuf[1], sizeof(optBuf[1]), "Thrusters: MAX");
    }

    if (_hullLevel < 2) {
        snprintf(optBuf[2], sizeof(optBuf[2]), "Hull Lv%d -> %d  %dc",
            _hullLevel, _hullLevel + 1, upgradeCost(_hullLevel));
    } else {
        snprintf(optBuf[2], sizeof(optBuf[2]), "Hull: MAX");
    }

    if (_rocketType < 3) {
        snprintf(optBuf[3], sizeof(optBuf[3]), "Buy: %s  %dc",
            _rocketNames[_rocketType + 1], _rocketCosts[_rocketType + 1]);
    } else {
        snprintf(optBuf[3], sizeof(optBuf[3]), "Rocket: MAXED");
    }

    snprintf(optBuf[4], sizeof(optBuf[4]), "LAUNCH");

    for (int i = 0; i < 5; i++) {
        int y = 38 + i * 6;
        bool sel = (i == _upgradeSelection);
        if (sel) {
            u8g2.drawStr(3, y, ">");
        }
        u8g2.drawStr(10, y, optBuf[i]);
    }

    u8g2.sendBuffer();
}

// ==================== DRAW GAME OVER ====================

void EternalGame::drawGameOver() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13B_tf);
    drawCenteredStr(14, "GAME OVER");

    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[32];
    snprintf(buf, sizeof(buf), "Altitude: %d", (int)_altitude);
    drawCenteredStr(30, buf);

    bool isNew = (uint32_t)_altitude > _highScore && _altitude > 0;
    snprintf(buf, sizeof(buf), "Best: %lu%s",
        isNew ? (uint32_t)_altitude : _highScore, isNew ? " NEW!" : "");
    drawCenteredStr(42, buf);

    drawCenteredStr(56, "Btn1: Menu  Btn2: Home");
    u8g2.sendBuffer();
}

// ==================== DRAW HELPERS ====================

void EternalGame::drawBackground() {
    // Stars at high altitude
    if (_scrollY > 80) {
        int bandLow = max(0, (int)(_scrollY - 80) / 25);
        int bandHigh = (int)(_scrollY + 64) / 25;
        for (int band = bandLow; band <= bandHigh; band++) {
            int count = band > 8 ? 8 : (band > 3 ? 4 + (band - 3) : band);
            for (int s = 0; s < count; s++) {
                int h = band * 47281 + s * 17321 + 9876;
                int sx = (h & 0x7F);
                float salt = ((h >> 7) & 0x1F) * 0.8f;
                int sy = (int)worldToScreenY(band * 25.0f + salt);
                if (sy >= 1 && sy < 64) {
                    u8g2.drawPixel(sx, sy);
                }
            }
        }
    }

    // Clouds at low altitude
    if (_scrollY < 300) {
        // Deterministic cloud positions based on altitude bands
        const int cloudCount = 6;
        for (int c = 0; c < cloudCount; c++) {
            int band = c + 1;
            float alt = band * 40.0f;
            int h = c * 34567 + band * 8912;
            int cx = 10 + (h % 108);
            int sy = (int)worldToScreenY(alt);
            if (sy > -10 && sy < 70) {
                drawCloud(cx, sy, 2 + (c % 3));
            }
        }
    }
}

void EternalGame::drawCloud(int sx, int sy, uint8_t size) {
    int r = 3 + size;
    u8g2.drawDisc(sx, sy, r);
    u8g2.drawDisc(sx - r, sy + 1, r - 1);
    u8g2.drawDisc(sx + r, sy + 1, r - 1);
}

void EternalGame::drawHUD() {
    u8g2.setFont(u8g2_font_4x6_tf);
    char buf[16];

    // Altitude
    snprintf(buf, sizeof(buf), "%dm", (int)_altitude);
    u8g2.drawStr(2, 6, buf);

    // Coins
    snprintf(buf, sizeof(buf), "C:%lu", _coinCount);
    u8g2.drawStr(96, 6, buf);

    // Fuel bar (right side)
    int fuelBarH = 24;
    int fuelBarX = 124;
    int fuelBarY = 15;
    u8g2.drawFrame(fuelBarX, fuelBarY, 3, fuelBarH);
    int fuelH = (int)(_fuel * fuelBarH / _maxFuel);
    if (fuelH > 0) {
        u8g2.drawBox(fuelBarX + 1, fuelBarY + fuelBarH - fuelH, 1, fuelH);
    }

    // HP hearts
    for (int i = 0; i < _maxHp; i++) {
        int hx = 2 + i * 7;
        int hy = 10;
        if (i < _hp) {
            u8g2.drawBox(hx, hy, 4, 4);
        } else {
            u8g2.drawFrame(hx, hy, 4, 4);
        }
    }
}

// ==================== ROCKET DRAWING ====================

void EternalGame::drawRocket(int cx, int topY) {
    int bw = _bodyW[_rocketType];
    int bh = _bodyH[_rocketType];
    int nh = _noseH[_rocketType];
    int halfW = bw / 2;

    // Nose cone — tapering upward
    for (int r = 0; r < nh; r++) {
        int nw = 2 + (bw - 2) * (r + 1) / nh;
        int nhw = nw / 2;
        u8g2.drawHLine(cx - nhw, topY + r, nw);
    }

    int bodyTop = topY + nh;
    int bodyBot = bodyTop + bh;

    // Body
    u8g2.drawBox(cx - halfW + 1, bodyTop, bw - 2, bh);
    u8g2.drawFrame(cx - halfW, bodyTop, bw, bh);

    // Hull armor lines
    if (_hullLevel >= 1) {
        u8g2.drawFrame(cx - halfW + 1, bodyTop + 1, bw - 2, bh - 2);
    }
    if (_hullLevel >= 2) {
        u8g2.drawVLine(cx - halfW - 1, bodyTop + 2, bh - 4);
        u8g2.drawVLine(cx + halfW + 1, bodyTop + 2, bh - 4);
    }

    // Windows (black dots on white body)
    u8g2.setDrawColor(0);
    int winY = bodyTop + bh / 3;
    if (_rocketType == 0) {
        u8g2.drawPixel(cx, winY);
    } else if (_rocketType == 1) {
        u8g2.drawPixel(cx - 1, winY);
        u8g2.drawPixel(cx + 2, winY);
    } else if (_rocketType == 2) {
        u8g2.drawPixel(cx - 2, winY);
        u8g2.drawPixel(cx, winY);
        u8g2.drawPixel(cx + 3, winY);
    } else {
        u8g2.drawPixel(cx - 3, winY);
        u8g2.drawPixel(cx - 1, winY);
        u8g2.drawPixel(cx + 2, winY);
        u8g2.drawPixel(cx + 4, winY);
        u8g2.drawPixel(cx - 2, winY + 3);
        u8g2.drawPixel(cx + 3, winY + 3);
    }
    u8g2.setDrawColor(1);

    // Side fuel tanks
    if (_fuelLevel >= 1) {
        int th = 3 + _fuelLevel * 2;
        int ty = bodyTop + bh / 4;
        u8g2.drawFrame(cx - halfW - 2, ty, 2, th);
        u8g2.drawFrame(cx + halfW + 1, ty, 2, th);
    }
    if (_fuelLevel >= 2) {
        int th = 4 + _fuelLevel * 2;
        int ty = bodyTop + bh / 5;
        u8g2.drawBox(cx - halfW - 2, ty, 2, th);
        u8g2.drawBox(cx + halfW + 1, ty, 2, th);
    }

    // Fins
    int finY = bodyBot - 2;
    u8g2.drawHLine(cx - halfW - 1, finY, 2);
    u8g2.drawHLine(cx + halfW, finY, 2);
    if (_rocketType >= 2) {
        u8g2.drawHLine(cx - halfW - 2, finY + 1, 2);
        u8g2.drawHLine(cx + halfW + 1, finY + 1, 2);
    }

    // Flame
    if (_thrusting) {
        int flameH = 2 + _thrustLevel * 2;
        if (_rocketType >= 2) flameH += 1;
        if (_rocketType >= 3) flameH += 1;
        drawFlame(cx, bodyBot, flameH);
    } else if (_grounded) {
        // tiny idle exhaust
        u8g2.drawPixel(cx, bodyBot);
    }
}

void EternalGame::drawFlame(int cx, int y, int h) {
    // Teardrop flame shape
    for (int r = 0; r < h; r++) {
        float t = (float)r / (float)h;
        int w = (int)((1.0f - t * 0.7f) * 2.5f) + 1;
        if (w > 4) w = 4;
        int hw = w / 2;
        for (int px = cx - hw; px <= cx + hw; px++) {
            u8g2.drawPixel(px, y + r);
        }
    }
}

// ==================== ENEMY DRAWING ====================

void EternalGame::drawEnemySprite(int sx, int sy, uint8_t type, uint8_t timer) {
    switch (type) {
        case 0: { // Bird
            int wing = ((timer / 6) % 2) == 0 ? -1 : 1;
            u8g2.drawPixel(sx, sy - 1);
            u8g2.drawPixel(sx - 1, sy);
            u8g2.drawPixel(sx + 1, sy);
            u8g2.drawHLine(sx - 2, sy + 1, 5);
            u8g2.drawPixel(sx - 2 + wing, sy - 1);
            u8g2.drawPixel(sx + 2 - wing, sy - 1);
            break;
        }
        case 1: { // UFO
            u8g2.drawDisc(sx, sy, 3);
            u8g2.setDrawColor(0);
            u8g2.drawHLine(sx - 1, sy, 3);
            u8g2.setDrawColor(1);
            u8g2.drawPixel(sx, sy - 3);
            u8g2.drawPixel(sx - 2, sy);
            u8g2.drawPixel(sx + 2, sy);
            break;
        }
        case 2: { // Satellite
            u8g2.drawBox(sx - 1, sy - 1, 3, 3);
            u8g2.drawHLine(sx - 4, sy, 3);
            u8g2.drawHLine(sx + 2, sy, 3);
            u8g2.drawVLine(sx, sy - 2, 2);
            break;
        }
    }
}

// ==================== COIN DRAWING ====================

void EternalGame::drawCoinSprite(int sx, int sy, uint8_t timer) {
    bool blink = ((timer / 10) % 2) == 0;
    if (blink) {
        u8g2.drawDisc(sx, sy, 2);
        u8g2.setDrawColor(0);
        u8g2.drawPixel(sx, sy);
        u8g2.setDrawColor(1);
    } else {
        u8g2.drawFrame(sx - 2, sy - 2, 4, 4);
        u8g2.drawPixel(sx, sy);
    }
}

// ==================== UPGRADE HELPERS ====================

int EternalGame::upgradeCost(int currentLevel) const {
    if (currentLevel < 0 || currentLevel >= 3) return 0;
    return _upgradeCosts[currentLevel];
}
