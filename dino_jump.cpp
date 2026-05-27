#include "dino_jump.h"
#include "hardware.h"
#include "sound.h"
#include "bitmaps.h"

extern SoundManager soundManager;

DinoJumpGame::DinoJumpGame() {
    _pauseLabels[0] = "Resume";
    _pauseLabels[1] = "Restart";
    _pauseLabels[2] = "Menu";
}

void DinoJumpGame::setup() {
    _state = DJState::MENU;
    _menuSelection = 0;
    _menuEnteredTime = millis();
    _highScore = readHighScore(EEPROM_DJ_SCORE_ADDR);
}

GameResult DinoJumpGame::loop(InputState& input) {
    switch (_state) {
        case DJState::MENU: {
            if ((input.up() || input.left()) && _menuSelection > 0) _menuSelection--;
            if ((input.down() || input.right()) && _menuSelection < 1) _menuSelection++;
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

        case DJState::PLAYING: {
            if (input.btn2.consumePress()) {
                _state = DJState::PAUSED;
                _pauseSelection = 0;
                _pauseEnteredTime = millis();
                _pauseMoveTime = 0;
                return GameResult::CONTINUE;
            }

            updatePlayer(input);
            updateObstacles();
            updateDJBullets();

            if (millis() - _lastSpawnMs >= (unsigned long)_spawnIntervalMs) {
                spawnObstacle();
                _lastSpawnMs = millis();
            }

            if (millis() - _lastSpeedIncreaseMs >= 3000) {
                _speed += 0.15f;
                _spawnIntervalMs -= 20;
                if (_spawnIntervalMs < 350) _spawnIntervalMs = 350;
                _lastSpeedIncreaseMs = millis();
            }

            checkCollisions();

            _score++;
            drawPlaying();
            return GameResult::CONTINUE;
        }

        case DJState::PAUSED: {
            unsigned long now = millis();
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
                    _state = DJState::PLAYING;
                } else if (_pauseSelection == 1) {
                    startGame();
                } else {
                    _state = DJState::MENU;
                    _menuSelection = 0;
                    _menuEnteredTime = millis();
                }
            }
            if (input.btn2.consumePress() && millis() - _pauseEnteredTime > 300) {
                _state = DJState::PLAYING;
            }
            drawPause();
            return GameResult::CONTINUE;
        }

        case DJState::GAME_OVER: {
            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                _state = DJState::MENU;
                _menuSelection = 0;
                _menuEnteredTime = millis();
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

void DinoJumpGame::startGame() {
    _state = DJState::PLAYING;
    _player.x = PLAYER_X;
    _player.y = GROUND_Y;
    _player.vy = 0;
    _player.onGround = true;
    _player.animFrame = 0;
    _player.lastAnimTime = millis();

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        _obstacles[i].active = false;
        _obstacles[i].shoots = false;
    }
    for (int i = 0; i < MAX_DJ_BULLETS; i++) _enemyBullets[i].active = false;

    _speed = 2.2f;
    _spawnIntervalMs = 1400;
    _lastSpawnMs = millis();
    _lastSpeedIncreaseMs = millis();
    _score = 0;
}

// ==================== PLAYER ====================

void DinoJumpGame::updatePlayer(InputState& input) {
    if (_player.onGround && (input.btn1.consumePress() || input.joyButton.consumePress())) {
        _player.vy = JUMP_VEL;
        _player.onGround = false;
        soundManager.beep(10);
    }

    _player.vy += GRAVITY;
    _player.y += _player.vy;

    if (_player.y >= GROUND_Y) {
        _player.y = GROUND_Y;
        _player.vy = 0;
        _player.onGround = true;
    }

    if (millis() - _player.lastAnimTime > 150) {
        _player.animFrame = (_player.animFrame + 1) % 4;
        _player.lastAnimTime = millis();
    }
}

// ==================== OBSTACLES ====================

void DinoJumpGame::spawnObstacle() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!_obstacles[i].active) {
            Obstacle& o = _obstacles[i];
            o.type = random(0, 4);
            o.x = 130;
            switch (o.type) {
                case 0: o.w = 8;  o.h = 8;  o.y = GROUND_Y + 14 - o.h; break; // crate
                case 1: o.w = 6;  o.h = 14; o.y = GROUND_Y + 14 - o.h; break; // pillar
                case 2: o.w = 12; o.h = 8;  o.y = GROUND_Y + 14 - o.h; break; // double barrier
                case 3: o.w = 10; o.h = 8;  o.y = random(10, 38);     break; // spinner drone
            }
            o.active = true;

            // Shooting: flying always shoots, others chance increases with speed
            int shootChance = (_speed > 4.0f) ? 40 : (_speed > 3.0f ? 20 : 0);
            o.shoots = (o.type == 3) || (random(0, 100) < shootChance);
            o.lastShotMs = millis() + ((o.type == 3) ? random(500, 1200) : random(800, 2500));

            // Randomize next spawn interval for variety
            _spawnIntervalMs = max(350, (int)(1400 - (_speed - 2.2f) * 200) + random(-300, 150));
            return;
        }
    }
}

void DinoJumpGame::updateObstacles() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (_obstacles[i].active) {
            _obstacles[i].x -= _speed;
            if (_obstacles[i].x + _obstacles[i].w < -15) {
                _obstacles[i].active = false;
            }
            // Shooting obstacles fire toward player
            int shootRange = (_obstacles[i].type == 3) ? 120 : 110;
            if (_obstacles[i].shoots && _obstacles[i].x < shootRange && _obstacles[i].x > 10) {
                if (millis() >= _obstacles[i].lastShotMs) {
                    fireObstacleBullet(_obstacles[i]);
                    _obstacles[i].lastShotMs = millis() + random(1500, 3500);
                }
            }
        }
    }
}

// ==================== OBSTACLE BULLETS ====================

void DinoJumpGame::fireObstacleBullet(const Obstacle& obs) {
    for (int i = 0; i < MAX_DJ_BULLETS; i++) {
        if (!_enemyBullets[i].active) {
            _enemyBullets[i].x = obs.x;
            _enemyBullets[i].y = obs.y + obs.h / 2;
            _enemyBullets[i].vx = -1.5f - _speed * 0.3f;
            _enemyBullets[i].vy = (random(0, 100) < 50) ? 0.3f : -0.3f;
            _enemyBullets[i].active = true;
            return;
        }
    }
}

void DinoJumpGame::updateDJBullets() {
    for (int i = 0; i < MAX_DJ_BULLETS; i++) {
        if (_enemyBullets[i].active) {
            _enemyBullets[i].x += _enemyBullets[i].vx;
            _enemyBullets[i].y += _enemyBullets[i].vy;
            if (_enemyBullets[i].x < -5 || _enemyBullets[i].y < 0 || _enemyBullets[i].y > 63) {
                _enemyBullets[i].active = false;
            }
        }
    }
}

// ==================== COLLISION ====================

void DinoJumpGame::checkCollisions() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (_obstacles[i].active && collidesWith(_obstacles[i])) {
            _state = DJState::GAME_OVER;
            saveHighScore();
            _gameOverTime = millis();
            soundManager.beep(150);
            return;
        }
    }
    // Bullet collisions
    for (int i = 0; i < MAX_DJ_BULLETS; i++) {
        if (!_enemyBullets[i].active) continue;
        DJBullet& b = _enemyBullets[i];
        float px = PLAYER_X - 2;
        float py = _player.y + 2;
        if (b.x >= px && b.x <= px + 8 && b.y >= py && b.y <= py + 12) {
            b.active = false;
            _state = DJState::GAME_OVER;
            saveHighScore();
            _gameOverTime = millis();
            soundManager.beep(150);
            return;
        }
    }
}

bool DinoJumpGame::collidesWith(const Obstacle& obs) const {
    float px = PLAYER_X - 2;
    float py = _player.y + 4;
    float pw = 7;
    float ph = 10;

    return (px < obs.x + obs.w - 1 && px + pw > obs.x + 1 &&
            py < obs.y + obs.h && py + ph > obs.y);
}

// ==================== HIGH SCORE ====================

void DinoJumpGame::saveHighScore() {
    if (_score > _highScore) {
        writeHighScore(EEPROM_DJ_SCORE_ADDR, _score);
        _highScore = _score;
    }
}

// ==================== DRAW ====================

void DinoJumpGame::drawMenu() {
    u8g2.clearBuffer();

    // Icon at top center
    u8g2.drawXBMP(56, 2, 16, 16, ICON_DINO_JUMP);

    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[32];
    snprintf(buf, sizeof(buf), "Best: %lu", _highScore);
    drawCenteredStr(26, buf);

    drawMenuOption(42, "Start Game", _menuSelection == 0);
    drawMenuOption(54, "Return to Menu", _menuSelection == 1);

    u8g2.sendBuffer();
}

void DinoJumpGame::drawGame() {
    u8g2.clearBuffer();

    drawGround();

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (_obstacles[i].active) {
            drawObstacle(_obstacles[i]);
        }
    }

    drawDJBullets();

    drawRunner(PLAYER_X, _player.y);

    drawScore();
}

void DinoJumpGame::drawPlaying() {
    drawGame();
    u8g2.sendBuffer();
}

void DinoJumpGame::drawRunner(float px, float py) {
    int cx = (int)px;
    int topY = (int)py;

    // Sleek aerodynamic body — arrow/diamond shape
    u8g2.drawBox(cx - 2, topY + 4, 5, 6);          // torso
    u8g2.drawPixel(cx, topY + 2);                    // pointed top
    u8g2.drawPixel(cx - 1, topY + 3);
    u8g2.drawPixel(cx + 1, topY + 3);

    // Head (small triangle facing right = direction of movement)
    u8g2.drawPixel(cx + 3, topY + 5);
    u8g2.drawPixel(cx + 4, topY + 6);
    u8g2.drawPixel(cx + 3, topY + 7);

    // Eye
    if (_player.animFrame % 2 == 0) {
        u8g2.drawPixel(cx + 2, topY + 5);
    }

    // Running legs (animated)
    if (_player.onGround) {
        if (_player.animFrame == 0) {
            u8g2.drawVLine(cx - 1, topY + 10, 4);    // left leg straight
            u8g2.drawVLine(cx + 1, topY + 10, 3);    // right leg back
            u8g2.drawPixel(cx + 2, topY + 12);
        } else if (_player.animFrame == 1) {
            u8g2.drawVLine(cx - 2, topY + 10, 3);    // left leg forward
            u8g2.drawPixel(cx - 3, topY + 12);
            u8g2.drawVLine(cx, topY + 10, 4);         // right leg straight
        } else if (_player.animFrame == 2) {
            u8g2.drawVLine(cx - 1, topY + 10, 4);
            u8g2.drawVLine(cx + 2, topY + 10, 3);
            u8g2.drawPixel(cx + 1, topY + 12);
        } else {
            u8g2.drawVLine(cx - 2, topY + 10, 3);
            u8g2.drawPixel(cx - 1, topY + 12);
            u8g2.drawVLine(cx, topY + 10, 4);
        }
    } else {
        // In air: legs tucked
        u8g2.drawPixel(cx - 1, topY + 11);
        u8g2.drawPixel(cx + 1, topY + 11);
    }
}

void DinoJumpGame::drawGround() {
    u8g2.drawHLine(0, GROUND_Y + 14, 128);
    int offset = (millis() / 60) % 16;
    for (int i = 0; i < 128; i += 12) {
        int gx = (i + offset) % 128;
        u8g2.drawPixel(gx, (int)(GROUND_Y + 15));
    }
}

void DinoJumpGame::drawScore() {
    u8g2.setFont(u8g2_font_5x7_tf);
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu", _score);
    int sx = 128 - u8g2.getStrWidth(buf) - 2;
    u8g2.drawStr(sx, 6, buf);
}

void DinoJumpGame::drawObstacle(const Obstacle& obs) {
    int ox = (int)obs.x;
    int oy = (int)obs.y;

    switch (obs.type) {
        case 0: { // Crate - simple block with cross
            u8g2.drawFrame(ox, oy, 8, 8);
            u8g2.drawHLine(ox + 2, oy + 4, 4);
            u8g2.drawVLine(ox + 4, oy + 2, 4);
            break;
        }
        case 1: { // Tall pillar
            u8g2.drawBox(ox + 1, oy + 3, 4, 9);
            u8g2.drawHLine(ox, oy + 2, 6);
            u8g2.drawHLine(ox, oy + 12, 6);
            break;
        }
        case 2: { // Double low barrier
            u8g2.drawBox(ox, oy + 2, 4, 6);
            u8g2.drawBox(ox + 8, oy + 2, 4, 6);
            u8g2.drawHLine(ox + 2, oy + 1, 8);
            // Spikes on top
            u8g2.drawPixel(ox + 1, oy + 1);
            u8g2.drawPixel(ox + 9, oy + 1);
            break;
        }
        case 3: { // Flying spinner drone
            int bob = (int)(sin(millis() / 120.0f + ox) * 2.5f);
            int fy = oy + bob;
            // Cross shape that spins visually via the bob
            u8g2.drawHLine(ox + 2, fy + 4, 6);
            u8g2.drawVLine(ox + 5, fy + 1, 6);
            u8g2.drawPixel(ox + 3, fy + 3);
            u8g2.drawPixel(ox + 7, fy + 3);
            u8g2.drawPixel(ox + 3, fy + 5);
            u8g2.drawPixel(ox + 7, fy + 5);
            break;
        }
    }

    // Shooting indicator: small spark above
    if (obs.shoots) {
        u8g2.drawPixel(ox + obs.w / 2, oy - 2);
        u8g2.drawPixel(ox + obs.w / 2 - 1, oy - 1);
        u8g2.drawPixel(ox + obs.w / 2 + 1, oy - 1);
    }
}

void DinoJumpGame::drawDJBullets() {
    for (int i = 0; i < MAX_DJ_BULLETS; i++) {
        if (_enemyBullets[i].active) {
            u8g2.drawBox((int)_enemyBullets[i].x, (int)_enemyBullets[i].y, 2, 2);
        }
    }
}

void DinoJumpGame::drawPause() {
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

void DinoJumpGame::drawGameOver() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13B_tf);
    drawCenteredStr(14, "GAME OVER");

    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[32];
    snprintf(buf, sizeof(buf), "Score: %lu", _score);
    drawCenteredStr(30, buf);

    bool isNew = _score >= _highScore && _score > 0;
    snprintf(buf, sizeof(buf), "Best: %lu%s", _highScore, isNew ? " NEW!" : "");
    drawCenteredStr(40, buf);

    drawCenteredStr(56, "Btn1: Menu  Btn2: Home");
    u8g2.sendBuffer();
}
