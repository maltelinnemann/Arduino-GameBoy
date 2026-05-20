#include "space_invaders.h"
#include "hardware.h"
#include "sound.h"
#include "bitmaps.h"

extern SoundManager soundManager;

// ==================== LIFECYCLE ====================

SpaceInvadersGame::SpaceInvadersGame() {
    _pauseLabels[0] = "Resume";
    _pauseLabels[1] = "Restart";
    _pauseLabels[2] = "Menu";
}

void SpaceInvadersGame::setup() {
    _state = SIState::MENU;
    _menuSelection = 0;
    _highScore = readHighScore(EEPROM_SI_SCORE_ADDR);
}

GameResult SpaceInvadersGame::loop(InputState& input) {
    switch (_state) {
        case SIState::MENU: {
            if (input.up() && _menuSelection > 0) {
                _menuSelection--;
            }
            if (input.down() && _menuSelection < 1) {
                _menuSelection++;
            }
            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                if (_menuSelection == 0) {
                    startGame();
                } else {
                    return GameResult::EXIT_TO_MENU;
                }
            }
            drawMenu();
            return GameResult::CONTINUE;
        }

        case SIState::PLAYING: {
            if (input.btn2.consumePress()) {
                _state = SIState::PAUSED;
                _pauseSelection = 0;
                _pauseEnteredTime = millis();
                return GameResult::CONTINUE;
            }

            // Movement
            if (input.left())  _playerX -= 2.0f;
            if (input.right()) _playerX += 2.0f;
            if (_playerX < 0) _playerX = 0;
            if (_playerX > 116) _playerX = 116;

            // Shooting (hold for continuous, 250ms cooldown)
            if (input.btn1.held() && millis() - _lastShotMs >= 250) {
                firePlayerBullet();
                _lastShotMs = millis();
            }

            // Spawn enemies
            if (millis() - _lastSpawnMs >= (unsigned long)_spawnIntervalMs &&
                _enemiesSpawned < _maxEnemiesWave) {
                spawnEnemy();
                _lastSpawnMs = millis();
            }

            // Check if wave complete
            if (_enemiesSpawned >= _maxEnemiesWave && _aliveCount == 0) {
                spawnWave();
            }

            updateBullets();
            updateEnemies();
            updatePowerUp();
            checkCollisions();

            // Passive score
            _score++;

            // Invulnerability blink
            if (millis() < _invulnerableUntil) {
                _invulnVisible = (millis() / 100) % 2;
            } else {
                _invulnVisible = true;
            }

            drawPlaying();
            return GameResult::CONTINUE;
        }

        case SIState::PAUSED: {
            // Ignore btn2 for first 300ms to prevent accidental double-toggle
            if (input.up() && _pauseSelection > 0) _pauseSelection--;
            if (input.down() && _pauseSelection < PAUSE_OPTIONS - 1) _pauseSelection++;
            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                if (_pauseSelection == 0) {
                    _state = SIState::PLAYING;
                } else if (_pauseSelection == 1) {
                    startGame();
                } else {
                    return GameResult::EXIT_TO_MENU;
                }
            }
            if (input.btn2.consumePress() && millis() - _pauseEnteredTime > 300) {
                _state = SIState::PLAYING;
            }
            drawPause();
            return GameResult::CONTINUE;
        }

        case SIState::GAME_OVER: {
            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                startGame();
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

void SpaceInvadersGame::startGame() {
    _state = SIState::PLAYING;
    _score = 0;
    _lives = 3;
    _playerX = 58;
    _playerY = 54;
    _invulnerableUntil = 0;
    _invulnVisible = true;
    _lastShotMs = 0;
    _wave = 0;

    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) _playerBullets[i].active = false;
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) _enemyBullets[i].active = false;
    for (int i = 0; i < MAX_ENEMIES; i++) _enemies[i].alive = false;
    _powerUp.active = false;
    _aliveCount = 0;

    spawnWave();
}

// ==================== WAVES ====================

void SpaceInvadersGame::spawnWave() {
    _wave++;
    _enemiesSpawned = 0;
    _maxEnemiesWave = 5 + _wave * 2;
    _spawnIntervalMs = getWaveSpawnInterval();
    _lastSpawnMs = millis();
}

int SpaceInvadersGame::getWaveSpawnInterval() const {
    int interval = 800 - _wave * 30;
    if (interval < 250) interval = 250;
    return interval;
}

float SpaceInvadersGame::getSpeedMultiplier() const {
    return 1.0f + _wave * 0.08f;
}

void SpaceInvadersGame::spawnEnemy() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!_enemies[i].alive) {
            Enemy& e = _enemies[i];
            e.x = random(2, 118);
            e.y = -8;
            e.alive = true;

            float mult = getSpeedMultiplier();
            float baseSpeed = 0.4f + random(0, 30) / 100.0f;

            // Determine type based on wave
            int r = random(0, 100);
            if (_wave <= 2) {
                e.type = EnemyType::BASIC;
            } else if (_wave <= 5) {
                e.type = (r < 30) ? EnemyType::SINGLE_SHOT : EnemyType::BASIC;
            } else if (_wave <= 9) {
                if (r < 50) e.type = EnemyType::BASIC;
                else if (r < 85) e.type = EnemyType::SINGLE_SHOT;
                else e.type = EnemyType::MULTI_SHOT;
            } else {
                if (r < 30) e.type = EnemyType::BASIC;
                else if (r < 70) e.type = EnemyType::SINGLE_SHOT;
                else e.type = EnemyType::MULTI_SHOT;
            }

            e.speedY = baseSpeed * mult;
            e.speedX = (e.type == EnemyType::BASIC) ? (random(-100, 101) / 100.0f * 0.3f) : 0;
            e.w = (e.type == EnemyType::MULTI_SHOT) ? 10 : 8;
            e.h = 8;

            if (e.type == EnemyType::SINGLE_SHOT || e.type == EnemyType::MULTI_SHOT) {
                e.nextFireMs = millis() + random(1500, 4000);
            } else {
                e.nextFireMs = 0;
            }

            _enemiesSpawned++;
            _aliveCount++;
            return;
        }
    }
}

// ==================== BULLETS ====================

void SpaceInvadersGame::firePlayerBullet() {
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (!_playerBullets[i].active) {
            _playerBullets[i].x = _playerX + 4;
            _playerBullets[i].y = _playerY - 2;
            _playerBullets[i].speedY = -3.0f;
            _playerBullets[i].active = true;
            soundManager.beep(15);
            return;
        }
    }
}

void SpaceInvadersGame::fireEnemyBullet(int idx) {
    Enemy& e = _enemies[idx];
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!_enemyBullets[i].active) {
            _enemyBullets[i].x = e.x + e.w / 2.0f;
            _enemyBullets[i].y = e.y + e.h;
            _enemyBullets[i].speedY = 2.0f;
            _enemyBullets[i].active = true;
            return;
        }
    }
}

void SpaceInvadersGame::updateBullets() {
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (_playerBullets[i].active) {
            _playerBullets[i].y += _playerBullets[i].speedY;
            if (_playerBullets[i].y < -4) _playerBullets[i].active = false;
        }
    }
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (_enemyBullets[i].active) {
            _enemyBullets[i].y += _enemyBullets[i].speedY;
            if (_enemyBullets[i].y > 68) _enemyBullets[i].active = false;
        }
    }
}

// ==================== ENEMIES ====================

void SpaceInvadersGame::updateEnemies() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!_enemies[i].alive) continue;
        Enemy& e = _enemies[i];

        e.y += e.speedY;
        e.x += e.speedX;

        if (e.x < 0) { e.x = 0; e.speedX = -e.speedX; }
        if (e.x > 128 - e.w) { e.x = 128 - e.w; e.speedX = -e.speedX; }

        if (e.y > 70) {
            e.alive = false;
            _aliveCount--;
        }

        // Enemy shooting
        if ((e.type == EnemyType::SINGLE_SHOT || e.type == EnemyType::MULTI_SHOT) &&
            e.alive && e.y > 0 && e.y < 55) {
            if (millis() >= e.nextFireMs) {
                if (e.type == EnemyType::SINGLE_SHOT) {
                    fireEnemyBullet(i);
                } else {
                    // Multi-shot: 3 bullets in spread
                    fireEnemyBullet(i);
                    for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                        if (!_enemyBullets[j].active) {
                            _enemyBullets[j].x = e.x - 3;
                            _enemyBullets[j].y = e.y + e.h;
                            _enemyBullets[j].speedY = 2.0f;
                            _enemyBullets[j].active = true;
                            break;
                        }
                    }
                    for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                        if (!_enemyBullets[j].active) {
                            _enemyBullets[j].x = e.x + e.w + 1;
                            _enemyBullets[j].y = e.y + e.h;
                            _enemyBullets[j].speedY = 2.0f;
                            _enemyBullets[j].active = true;
                            break;
                        }
                    }
                }
                e.nextFireMs = millis() + random(2000, 5000);
            }
        }
    }
}

// ==================== POWER UP ====================

void SpaceInvadersGame::spawnPowerUp(float x, float y) {
    if (random(0, 100) < 5 && !_powerUp.active) {
        _powerUp.x = x;
        _powerUp.y = y;
        _powerUp.speedY = 0.5f;
        _powerUp.active = true;
        _powerUp.spawnTime = millis();
    }
}

void SpaceInvadersGame::updatePowerUp() {
    if (_powerUp.active) {
        _powerUp.y += _powerUp.speedY;
        if (_powerUp.y > 68) _powerUp.active = false;
        if (millis() - _powerUp.spawnTime > 8000) _powerUp.active = false;
    }
}

// ==================== COLLISIONS ====================

void SpaceInvadersGame::checkCollisions() {
    // Player bullets vs enemies
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (!_playerBullets[i].active) continue;
        Bullet& pb = _playerBullets[i];

        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!_enemies[j].alive) continue;
            Enemy& e = _enemies[j];

            if (pb.x >= e.x && pb.x <= e.x + e.w &&
                pb.y >= e.y && pb.y <= e.y + e.h) {
                pb.active = false;
                e.alive = false;
                _aliveCount--;

                switch (e.type) {
                    case EnemyType::BASIC:        _score += 10; break;
                    case EnemyType::SINGLE_SHOT: _score += 20; break;
                    case EnemyType::MULTI_SHOT:  _score += 35; break;
                }

                soundManager.beep(20);
                spawnPowerUp(e.x, e.y);
                break;
            }
        }
    }

    // Enemy bullets vs player
    if (millis() >= _invulnerableUntil) {
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            if (!_enemyBullets[i].active) continue;
            Bullet& eb = _enemyBullets[i];
            if (eb.x >= _playerX && eb.x <= _playerX + 12 &&
                eb.y >= _playerY && eb.y <= _playerY + 8) {
                eb.active = false;
                _lives--;
                _invulnerableUntil = millis() + 1500;
                soundManager.beep(100);
                if (_lives <= 0) {
                    _state = SIState::GAME_OVER;
                    writeHighScore(EEPROM_SI_SCORE_ADDR, _score);
                    _highScore = readHighScore(EEPROM_SI_SCORE_ADDR);
                    _gameOverTime = millis();
                }
                return;
            }
        }

        // Enemies vs player
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!_enemies[i].alive) continue;
            Enemy& e = _enemies[i];
            if (_playerX + 10 > e.x && _playerX + 2 < e.x + e.w &&
                _playerY + 6 > e.y && _playerY < e.y + e.h) {
                e.alive = false;
                _aliveCount--;
                _lives--;
                _invulnerableUntil = millis() + 1500;
                soundManager.beep(100);
                if (_lives <= 0) {
                    _state = SIState::GAME_OVER;
                    writeHighScore(EEPROM_SI_SCORE_ADDR, _score);
                    _highScore = readHighScore(EEPROM_SI_SCORE_ADDR);
                    _gameOverTime = millis();
                }
                return;
            }
        }
    }

    // PowerUp vs player
    if (_powerUp.active) {
        if (_playerX + 12 > _powerUp.x && _playerX < _powerUp.x + 8 &&
            _playerY + 8 > _powerUp.y && _playerY < _powerUp.y + 8) {
            _powerUp.active = false;
            _lives++;
            soundManager.beep(80);
        }
    }
}

// ==================== DRAW ====================

void SpaceInvadersGame::drawMenu() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13B_tf);
    drawCenteredStr(16, "Space Invaders");

    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[32];
    snprintf(buf, sizeof(buf), "High Score: %lu", _highScore);
    drawCenteredStr(32, buf);

    drawMenuOption(48, "Start Game", _menuSelection == 0);
    drawMenuOption(58, "Return to Menu", _menuSelection == 1);
    u8g2.sendBuffer();
}

void SpaceInvadersGame::drawPlaying() {
    u8g2.clearBuffer();

    // Enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!_enemies[i].alive) continue;
        Enemy& e = _enemies[i];
        switch (e.type) {
            case EnemyType::BASIC:
                drawBitmap((int)e.x, (int)e.y, ENEMY_BASIC, 8, 8);
                break;
            case EnemyType::SINGLE_SHOT:
                drawBitmap((int)e.x, (int)e.y, ENEMY_SINGLE, 8, 8);
                break;
            case EnemyType::MULTI_SHOT:
                drawBitmap((int)e.x, (int)e.y, ENEMY_MULTI, 10, 8);
                break;
        }
    }

    // Enemy bullets
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (_enemyBullets[i].active) {
            u8g2.drawBox((int)_enemyBullets[i].x, (int)_enemyBullets[i].y, 2, 3);
        }
    }

    // Player bullets
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (_playerBullets[i].active) {
            u8g2.drawBox((int)_playerBullets[i].x, (int)_playerBullets[i].y, 1, 3);
        }
    }

    // PowerUp
    if (_powerUp.active) {
        drawBitmap((int)_powerUp.x, (int)_powerUp.y, POWERUP_LIFE, 8, 8);
    }

    // Player ship (blink if invulnerable)
    if (_invulnVisible) {
        drawShip();
    }

    drawUI();
    u8g2.sendBuffer();
}

void SpaceInvadersGame::drawShip() {
    drawBitmap((int)_playerX, (int)_playerY, PLAYER_SHIP, 12, 8);
}

void SpaceInvadersGame::drawUI() {
    // Lives (hearts top-left)
    for (int i = 0; i < _lives && i < 5; i++) {
        drawBitmap(2 + i * 8, 1, HEART, 6, 6);
    }

    // Score (top-right)
    u8g2.setFont(u8g2_font_5x7_tf);
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu", _score);
    int sx = 128 - u8g2.getStrWidth(buf) - 2;
    u8g2.drawStr(sx, 6, buf);

    // Wave indicator
    snprintf(buf, sizeof(buf), "W%d", _wave);
    u8g2.drawStr(56, 6, buf);
}

void SpaceInvadersGame::drawPause() {
    drawPlaying(); // Show game state underneath
    // Dimmed overlay
    u8g2.setDrawColor(0);
    u8g2.drawBox(0, 18, 128, 46);
    u8g2.setDrawColor(1);
    u8g2.drawFrame(0, 18, 128, 46);

    u8g2.setFont(u8g2_font_6x10_tf);
    drawCenteredStr(30, "PAUSED");
    for (int i = 0; i < PAUSE_OPTIONS; i++) {
        drawMenuOption(42 + i * 10, _pauseLabels[i], _pauseSelection == i);
    }
    u8g2.sendBuffer();
}

void SpaceInvadersGame::drawGameOver() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13B_tf);
    drawCenteredStr(16, "GAME OVER");

    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[32];
    snprintf(buf, sizeof(buf), "Score: %lu", _score);
    drawCenteredStr(32, buf);

    bool isNew = _score >= _highScore && _score > 0;
    snprintf(buf, sizeof(buf), "Best: %lu%s", _highScore, isNew ? " NEW!" : "");
    drawCenteredStr(42, buf);

    drawCenteredStr(56, "Btn1: Retry  Btn2: Menu");
    u8g2.sendBuffer();
}
