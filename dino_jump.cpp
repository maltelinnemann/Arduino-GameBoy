#include "dino_jump.h"
#include "hardware.h"
#include "sound.h"

extern SoundManager soundManager;

DinoJumpGame::DinoJumpGame() {
    _pauseLabels[0] = "Resume";
    _pauseLabels[1] = "Restart";
    _pauseLabels[2] = "Menu";
}

void DinoJumpGame::setup() {
    _state = DJState::MENU;
    _menuSelection = 0;
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
                return GameResult::CONTINUE;
            }

            updatePlayer(input);
            updateObstacles();

            // Spawn obstacles
            if (millis() - _lastSpawnMs >= (unsigned long)_spawnIntervalMs) {
                spawnObstacle();
                _lastSpawnMs = millis();
            }

            // Speed ramp
            if (millis() - _lastSpeedIncreaseMs >= 3000) {
                _speed += 0.15f;
                _spawnIntervalMs -= 15;
                if (_spawnIntervalMs < 400) _spawnIntervalMs = 400;
                _lastSpeedIncreaseMs = millis();
            }

            checkCollisions();

            _score++;
            drawPlaying();
            return GameResult::CONTINUE;
        }

        case DJState::PAUSED: {
            if ((input.up() || input.left()) && _pauseSelection > 0) _pauseSelection--;
            if ((input.down() || input.right()) && _pauseSelection < PAUSE_OPTIONS - 1) _pauseSelection++;
            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                if (_pauseSelection == 0) {
                    _state = DJState::PLAYING;
                } else if (_pauseSelection == 1) {
                    startGame();
                } else {
                    return GameResult::EXIT_TO_MENU;
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

void DinoJumpGame::startGame() {
    _state = DJState::PLAYING;
    _player.x = PLAYER_X;
    _player.y = GROUND_Y;
    _player.vy = 0;
    _player.onGround = true;
    _player.animFrame = 0;
    _player.lastAnimTime = millis();

    for (int i = 0; i < MAX_OBSTACLES; i++) _obstacles[i].active = false;

    _speed = 2.5f;
    _spawnIntervalMs = 1200;
    _lastSpawnMs = millis();
    _lastSpeedIncreaseMs = millis();
    _score = 0;
}

// ==================== PLAYER ====================

void DinoJumpGame::updatePlayer(InputState& input) {
    if (_player.onGround && input.btn1.consumePress()) {
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

    // Animation
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
            o.type = random(0, 3);
            o.x = 130;
            switch (o.type) {
                case 0: o.w = 4; o.h = 8;  break; // small
                case 1: o.w = 4; o.h = 14; break; // tall
                case 2: o.w = 10; o.h = 8; break; // double
            }
            o.y = GROUND_Y + 8 - o.h;
            o.active = true;
            return;
        }
    }
}

void DinoJumpGame::updateObstacles() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (_obstacles[i].active) {
            _obstacles[i].x -= _speed;
            if (_obstacles[i].x + _obstacles[i].w < 0) {
                _obstacles[i].active = false;
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
}

bool DinoJumpGame::collidesWith(const Obstacle& obs) const {
    // Player hitbox slightly smaller than visual for fairness
    float px = PLAYER_X - 3;
    float py = _player.y + 3;
    float pw = 8;
    float ph = 12;

    return (px < obs.x + obs.w && px + pw > obs.x &&
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
    u8g2.setFont(u8g2_font_8x13B_tf);
    drawCenteredStr(16, "Dino Jump");

    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[32];
    snprintf(buf, sizeof(buf), "High Score: %lu", _highScore);
    drawCenteredStr(32, buf);

    drawMenuOption(48, "Start Game", _menuSelection == 0);
    drawMenuOption(58, "Return to Menu", _menuSelection == 1);

    // Draw small phallus preview
    drawPhallus(64, 20);

    u8g2.sendBuffer();
}

void DinoJumpGame::drawPlaying() {
    u8g2.clearBuffer();

    drawGround();

    // Obstacles
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (_obstacles[i].active) {
            drawObstacle(_obstacles[i]);
        }
    }

    // Player
    drawPhallus(PLAYER_X, _player.y);

    drawScore();
    u8g2.sendBuffer();
}

void DinoJumpGame::drawPhallus(float px, float py) {
    int cx = (int)px;
    int topY = (int)py;

    // Testes (two circles at base)
    u8g2.drawDisc(cx - 3, topY + 12, 3);
    u8g2.drawDisc(cx + 3, topY + 12, 3);

    // Shaft
    u8g2.drawBox(cx - 2, topY + 2, 5, 9);

    // Glans (wider tip)
    u8g2.drawDisc(cx, topY + 1, 4);

    // Eyes on glans (animating: blink or shift)
    if (_player.animFrame % 2 == 0) {
        u8g2.drawPixel(cx - 1, topY - 1);
        u8g2.drawPixel(cx + 1, topY - 1);
    }

    // Running animation: legs/wiggle when on ground
    if (_player.onGround) {
        if (_player.animFrame == 0 || _player.animFrame == 2) {
            u8g2.drawPixel(cx - 3, topY + 13);
            u8g2.drawPixel(cx + 3, topY + 13);
        } else {
            u8g2.drawPixel(cx - 4, topY + 14);
            u8g2.drawPixel(cx + 4, topY + 14);
        }
    }
}

void DinoJumpGame::drawGround() {
    u8g2.drawHLine(0, GROUND_Y + 16, 128);
    // Scrolling dots
    int offset = (millis() / 60) % 16;
    for (int i = 0; i < 128; i += 12) {
        int gx = (i + offset) % 128;
        u8g2.drawPixel(gx, (int)(GROUND_Y + 16 + 1));
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
    // Draw cactus-like shapes
    if (obs.type == 0 || obs.type == 1) {
        // Single cactus
        u8g2.drawBox(ox, oy, obs.w, obs.h);
        if (obs.type == 1) {
            // Branches on tall cactus
            u8g2.drawBox(ox - 2, oy + 4, 2, 3);
            u8g2.drawBox(ox + obs.w, oy + 4, 2, 3);
        }
    } else {
        // Double cactus
        u8g2.drawBox(ox, oy, 4, obs.h);
        u8g2.drawBox(ox + 6, oy, 4, obs.h);
    }
}

void DinoJumpGame::drawPause() {
    drawPlaying();
    // Overlay
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

void DinoJumpGame::drawGameOver() {
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
