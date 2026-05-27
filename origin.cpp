#include "origin.h"
#include "hardware.h"
#include "sound.h"
#include "bitmaps.h"

extern SoundManager soundManager;

OriginGame::OriginGame() {
    _pauseLabels[0] = "Resume";
    _pauseLabels[1] = "Restart";
    _pauseLabels[2] = "Menu";
}

void OriginGame::setup() {
    _state = OGState::MENU;
    _menuSelection = 0;
    _menuEnteredTime = millis();
    _highScore = readHighScore(EEPROM_OG_SCORE_ADDR);
    if (_highScore > 999999 || (_highScore > 0 && _highScore < 3)) {
        _highScore = 0;
        EEPROM.put(EEPROM_OG_SCORE_ADDR, (uint32_t)0);
    }
}

// ==================== MAIN LOOP ====================

GameResult OriginGame::loop(InputState& input) {
    switch (_state) {
        case OGState::MENU: {
            if (input.up() && _menuSelection > 0) _menuSelection--;
            if (input.down() && _menuSelection < 1) _menuSelection++;
            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                if (_menuSelection == 0) startGame();
                else return GameResult::EXIT_TO_MENU;
            }
            drawMenu();
            return GameResult::CONTINUE;
        }

        case OGState::PLAYING: {
            if (input.btn2.consumePress()) {
                _state = OGState::PAUSED;
                _pauseSelection = 0;
                _pauseEnteredTime = millis();
                _pauseMoveTime = 0;
                return GameResult::CONTINUE;
            }

            // Toggle boost with Btn1
            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                _boost = !_boost;
            }

            updateDirection(input);

            // Move snake on timer
            int interval = _boost ? _moveIntervalMs / 3 : _moveIntervalMs;
            if (millis() - _lastMoveMs >= (unsigned long)interval) {
                moveSnake();
                _lastMoveMs = millis();
            }

            drawPlaying();
            return GameResult::CONTINUE;
        }

        case OGState::PAUSED: {
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
                if (_pauseSelection == 0) _state = OGState::PLAYING;
                else if (_pauseSelection == 1) startGame();
                else {
                    _state = OGState::MENU;
                    _menuSelection = 0;
                    _menuEnteredTime = millis();
                }
            }
            if (input.btn2.consumePress() && millis() - _pauseEnteredTime > 300) {
                _state = OGState::PLAYING;
            }
            drawPause();
            return GameResult::CONTINUE;
        }

        case OGState::GAME_OVER: {
            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                _state = OGState::MENU;
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

void OriginGame::startGame() {
    _state = OGState::PLAYING;
    _length = 3;
    _dirX = 1; _dirY = 0;
    _nextDirX = 1; _nextDirY = 0;
    _boost = false;
    _foodEaten = 0;
    _moveIntervalMs = 180;
    _lastMoveMs = millis();

    // Place snake horizontally at center of grid
    for (int i = 0; i < _length; i++) {
        _snake[i].x = 6 - i;
        _snake[i].y = 3;
    }

    spawnFood();
}

void OriginGame::spawnFood() {
    int attempts = 0;
    do {
        _foodX = random(0, 16);
        _foodY = random(0, 8);
        attempts++;
    } while (isOccupied(_foodX, _foodY) && attempts < 200);
}

bool OriginGame::isOccupied(int8_t x, int8_t y) {
    for (int i = 0; i < _length; i++) {
        if (_snake[i].x == x && _snake[i].y == y) return true;
    }
    return false;
}

// ==================== DIRECTION ====================

void OriginGame::updateDirection(InputState& input) {
    int8_t ndx = _nextDirX;
    int8_t ndy = _nextDirY;

    if (input.left())  { ndx = -1; ndy = 0; }
    if (input.right()) { ndx = 1;  ndy = 0; }
    if (input.up())    { ndx = 0;  ndy = -1; }
    if (input.down())  { ndx = 0;  ndy = 1; }

    // Prevent reversing into yourself
    if (ndx != -_dirX || ndy != -_dirY) {
        _nextDirX = ndx;
        _nextDirY = ndy;
    }
}

// ==================== MOVEMENT ====================

void OriginGame::moveSnake() {
    _dirX = _nextDirX;
    _dirY = _nextDirY;

    int8_t newX = _snake[0].x + _dirX;
    int8_t newY = _snake[0].y + _dirY;

    // Wall collision
    if (newX < 0 || newX >= 16 || newY < 0 || newY >= 8) {
        _state = OGState::GAME_OVER;
        if (_foodEaten > 0) {
            writeHighScore(EEPROM_OG_SCORE_ADDR, (uint32_t)(_foodEaten + 3));
            _highScore = readHighScore(EEPROM_OG_SCORE_ADDR);
        }
        _gameOverTime = millis();
        soundManager.beep(200);
        return;
    }

    // Determine if we're eating before checking self-collision
    bool eating = (newX == _foodX && newY == _foodY);

    // Self collision: skip tail segment since it will move away
    // But if eating, the tail stays, so check all segments
    int checkEnd = eating ? _length : _length - 1;
    for (int i = 0; i < checkEnd; i++) {
        if (_snake[i].x == newX && _snake[i].y == newY) {
            _state = OGState::GAME_OVER;
            if (_foodEaten > 0) {
                writeHighScore(EEPROM_OG_SCORE_ADDR, (uint32_t)(_foodEaten + 3));
                _highScore = readHighScore(EEPROM_OG_SCORE_ADDR);
            }
            _gameOverTime = millis();
            soundManager.beep(200);
            return;
        }
    }

    // Shift body: grow if eating, otherwise normal shift
    if (eating && _length < MAX_LENGTH) {
        for (int i = _length; i > 0; i--) {
            _snake[i] = _snake[i - 1];
        }
        _length++;
    } else {
        for (int i = _length - 1; i > 0; i--) {
            _snake[i] = _snake[i - 1];
        }
    }

    _snake[0].x = newX;
    _snake[0].y = newY;

    if (eating) {
        _foodEaten++;
        soundManager.beep(30);
        _moveIntervalMs = max(60, 180 - _foodEaten * 3);
        spawnFood();
    }
}

// ==================== DRAW ====================

void OriginGame::drawMenu() {
    u8g2.clearBuffer();

    u8g2.drawXBMP(56, 2, 16, 16, ICON_ORIGIN);

    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[32];
    snprintf(buf, sizeof(buf), "Best: %lu", _highScore);
    drawCenteredStr(26, buf);

    drawMenuOption(42, "Start Game", _menuSelection == 0);
    drawMenuOption(54, "Return to Menu", _menuSelection == 1);

    u8g2.sendBuffer();
}

void OriginGame::drawGame() {
    u8g2.clearBuffer();

    // Draw food (blinking dot with cross shape)
    if ((millis() / 400) % 2 == 0) {
        int fx = _foodX * 8 + 4;
        int fy = _foodY * 8 + 4;
        u8g2.drawDisc(fx, fy, 3);
        u8g2.setDrawColor(0);
        u8g2.drawDisc(fx, fy, 1);
        u8g2.setDrawColor(1);
    } else {
        int fx = _foodX * 8 + 4;
        int fy = _foodY * 8 + 4;
        u8g2.drawPixel(fx, fy);
        u8g2.drawPixel(fx - 2, fy);
        u8g2.drawPixel(fx + 2, fy);
        u8g2.drawPixel(fx, fy - 2);
        u8g2.drawPixel(fx, fy + 2);
    }

    // Draw snake body (from tail to head so head draws on top)
    for (int i = _length - 1; i >= 0; i--) {
        int sx = _snake[i].x * 8;
        int sy = _snake[i].y * 8;

        if (i == 0) {
            // Head: filled box with eyes
            u8g2.drawBox(sx + 1, sy + 1, 6, 6);
            // Eyes based on direction
            u8g2.setDrawColor(0);
            if (_dirX == 1) {
                u8g2.drawPixel(sx + 5, sy + 2);
                u8g2.drawPixel(sx + 5, sy + 5);
            } else if (_dirX == -1) {
                u8g2.drawPixel(sx + 2, sy + 2);
                u8g2.drawPixel(sx + 2, sy + 5);
            } else if (_dirY == -1) {
                u8g2.drawPixel(sx + 2, sy + 2);
                u8g2.drawPixel(sx + 5, sy + 2);
            } else {
                u8g2.drawPixel(sx + 2, sy + 5);
                u8g2.drawPixel(sx + 5, sy + 5);
            }
            u8g2.setDrawColor(1);
        } else {
            // Body: smaller box with slight gap between segments
            u8g2.drawBox(sx + 2, sy + 2, 4, 4);
        }
    }

    // Score and boost indicator
    u8g2.setFont(u8g2_font_4x6_tf);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", _foodEaten + 3);
    u8g2.drawStr(2, 62, buf);

    if (_boost) {
        u8g2.setFont(u8g2_font_4x6_tf);
        u8g2.drawStr(110, 62, "FAST");
    }

    // Grid border
    u8g2.drawFrame(0, 0, 128, 64);
}

void OriginGame::drawPlaying() {
    drawGame();
    u8g2.sendBuffer();
}

void OriginGame::drawPause() {
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

void OriginGame::drawGameOver() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13B_tf);
    drawCenteredStr(14, "GAME OVER");

    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[32];
    snprintf(buf, sizeof(buf), "Length: %d", _foodEaten + 3);
    drawCenteredStr(30, buf);

    uint32_t totalLen = (uint32_t)(_foodEaten + 3);
    bool isNew = totalLen > _highScore && _foodEaten > 0;
    snprintf(buf, sizeof(buf), "Best: %lu%s", isNew ? totalLen : _highScore, isNew ? " NEW!" : "");
    drawCenteredStr(42, buf);

    drawCenteredStr(56, "Btn1: Menu  Btn2: Home");
    u8g2.sendBuffer();
}
