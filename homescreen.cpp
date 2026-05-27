#include "homescreen.h"
#include "hardware.h"

Homescreen::Homescreen() {
    _numGames = 0;
    _selectedIndex = 0;
    _scrollPos = 0;
    _lastMoveTime = 0;
    _lastFrameTime = 0;
    _state = HSState::BOOT_ANIMATION;
    _stateStartMs = 0;
    _animPhaseMs = 0;
    _animPhase = 0;
    _revealChars = 0;
    _idleBlink = false;
    for (int i = 0; i < HOMESCREEN_MAX_GAMES; i++) {
        _entries[i].game = nullptr;
        _entries[i].icon = nullptr;
    }
    for (int i = 0; i < MAX_PARTICLES; i++) {
        _particles[i].life = 0;
    }
}

void Homescreen::addGame(Game* game, const uint8_t* icon) {
    if (_numGames < HOMESCREEN_MAX_GAMES) {
        _entries[_numGames].game = game;
        _entries[_numGames].icon = icon;
        _numGames++;
    }
}

void Homescreen::setup() {
    _selectedIndex = 0;
    _scrollPos = 0;
    _lastMoveTime = 0;
    _lastFrameTime = millis();
    _state = HSState::BOOT_ANIMATION;
    _stateStartMs = millis();
    _animPhaseMs = millis();
    _animPhase = 0;
    _revealChars = 0;
    _idleBlink = false;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        _particles[i].life = 0;
    }
}

GameResult Homescreen::loop(InputState& input) {
    unsigned long now = millis();
    float dt = (now - _lastFrameTime) / 1000.0f;
    if (dt > 0.1f) dt = 0.1f;
    _lastFrameTime = now;

    switch (_state) {
        case HSState::BOOT_ANIMATION: {
            unsigned long elapsed = now - _stateStartMs;

            // Phase transitions
            if (_animPhase == 0 && elapsed > 300) {
                _animPhase = 1;
                _animPhaseMs = now;
                spawnParticles(64, 32, 20);
            }
            if (_animPhase == 1 && elapsed > 800) {
                _animPhase = 2;
                _animPhaseMs = now;
                _revealChars = 0;
                spawnParticles(64, 20, 25);
            }
            if (_animPhase == 2 && elapsed > 1500) {
                _animPhase = 3;
                _animPhaseMs = now;
                spawnParticles(64, 32, 30);
            }
            if (_animPhase == 3 && elapsed > 1900) {
                _state = HSState::IDLE;
                _stateStartMs = now;
                _idleBlink = true;
            }

            updateParticles();
            drawBootAnimation();
            return GameResult::CONTINUE;
        }

        case HSState::IDLE: {
            unsigned long elapsed = now - _stateStartMs;
            // Blink the "press any button" text every 500ms
            _idleBlink = ((elapsed / 500) % 2) == 0;

            // Any button press enters selection
            if (input.btn1.consumePress() || input.btn2.consumePress() ||
                input.joyButton.consumePress() ||
                input.left() || input.right() || input.up() || input.down()) {
                _state = HSState::SELECTING;
                _stateStartMs = now;
                // Consume the directional input so it doesn't immediately switch games
                _lastMoveTime = now;
                _lastFrameTime = now;
            }

            updateParticles();
            drawIdle();
            return GameResult::CONTINUE;
        }

        case HSState::SELECTING: {
            // Normal game selection logic
            if (input.left() || input.right()) {
                if (now - _lastMoveTime > 220) {
                    if (input.right() && _selectedIndex < _numGames - 1) {
                        _selectedIndex++;
                        _lastMoveTime = now;
                    }
                    if (input.left() && _selectedIndex > 0) {
                        _selectedIndex--;
                        _lastMoveTime = now;
                    }
                }
            } else {
                _lastMoveTime = 0;
            }

            float target = (float)_selectedIndex;
            float diff = target - _scrollPos;
            if (diff < 0.005f && diff > -0.005f) {
                _scrollPos = target;
            } else {
                _scrollPos += diff * 10.0f * dt;
            }

            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                return GameResult::GAME_SELECTED;
            }

            drawSelection();
            return GameResult::CONTINUE;
        }
    }

    return GameResult::CONTINUE;
}

Game* Homescreen::getSelectedGame() const {
    if (_selectedIndex >= 0 && _selectedIndex < _numGames) {
        return _entries[_selectedIndex].game;
    }
    return nullptr;
}

// ==================== PARTICLES ====================

void Homescreen::spawnParticles(int cx, int cy, int count) {
    for (int i = 0; i < MAX_PARTICLES && count > 0; i++) {
        if (_particles[i].life == 0) {
            float angle = random(0, 628) / 100.0f; // 0 to 2*PI
            float speed = random(30, 150) / 100.0f;
            _particles[i].x = cx;
            _particles[i].y = cy;
            _particles[i].vx = cos(angle) * speed;
            _particles[i].vy = sin(angle) * speed;
            _particles[i].life = random(15, 35);
            count--;
        }
    }
}

void Homescreen::updateParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (_particles[i].life > 0) {
            _particles[i].x += _particles[i].vx;
            _particles[i].y += _particles[i].vy;
            _particles[i].vx *= 0.96f;
            _particles[i].vy *= 0.96f;
            _particles[i].life--;
        }
    }
}

// ==================== DRAW ====================

void Homescreen::drawBootAnimation() {
    u8g2.clearBuffer();

    unsigned long elapsed = millis() - _stateStartMs;

    switch (_animPhase) {
        case 0: {
            // Expanding concentric rings from center
            float t = elapsed / 300.0f;
            if (t > 1.0f) t = 1.0f;
            // Ease out
            float e = 1.0f - (1.0f - t) * (1.0f - t);

            for (int r = 0; r < 6; r++) {
                float delay = r * 0.12f;
                float rt = (t - delay) * 1.2f;
                if (rt < 0) rt = 0;
                if (rt > 1.0f) rt = 1.0f;
                float re = 1.0f - (1.0f - rt) * (1.0f - rt);
                int radius = (int)(re * 50.0f);
                if (radius > 0 && radius < 50) {
                    u8g2.drawFrame(64 - radius, 32 - radius * 64 / 128, radius * 2, radius * 2 * 64 / 128);
                }
            }
            break;
        }
        case 1: {
            // Rings fading + particles
            float t = (elapsed - 300) / 500.0f;
            if (t > 1.0f) t = 1.0f;
            for (int r = 0; r < 4; r++) {
                int radius = 30 + r * 12 + (int)(t * 20.0f);
                if (radius < 60) {
                    u8g2.drawFrame(64 - radius, 32 - radius / 2, radius * 2, radius);
                }
            }

            // Particles
            for (int i = 0; i < MAX_PARTICLES; i++) {
                if (_particles[i].life > 0) {
                    u8g2.drawPixel((int)_particles[i].x, (int)_particles[i].y);
                }
            }
            break;
        }
        case 2: {
            // "LINNEMANN" text reveal, one char at a time
            float t2 = (elapsed - 800) / 700.0f;
            if (t2 > 1.0f) t2 = 1.0f;
            int totalChars = 9; // L I N N E M A N N
            int shown = (int)(t2 * totalChars) + 1;
            if (shown > totalChars) shown = totalChars;

            u8g2.setFont(u8g2_font_8x13B_tf);
            const char* title = "LINNEMANN";
            char buf[8];
            for (int c = 0; c < shown; c++) {
                buf[c] = title[c];
                buf[c + 1] = '\0';
            }
            if (shown > 0) {
                int strW = u8g2.getStrWidth(buf);
                u8g2.drawStr(64 - strW / 2, 22, buf);
            }

            // Particles
            for (int i = 0; i < MAX_PARTICLES; i++) {
                if (_particles[i].life > 0) {
                    u8g2.drawPixel((int)_particles[i].x, (int)_particles[i].y);
                }
            }
            break;
        }
        case 3: {
            // Full title + zoom out
            float t3 = (elapsed - 1500) / 400.0f;
            if (t3 > 1.0f) t3 = 1.0f;

            u8g2.setFont(u8g2_font_8x13B_tf);
            const char* title = "LINNEMANN";
            int strW = u8g2.getStrWidth(title);
            u8g2.drawStr(64 - strW / 2, 22, title);

            // Particles
            for (int i = 0; i < MAX_PARTICLES; i++) {
                if (_particles[i].life > 0) {
                    u8g2.drawPixel((int)_particles[i].x, (int)_particles[i].y);
                }
            }

            // Scanline sweep transition
            int sweepY = (int)(t3 * 70.0f);
            u8g2.drawHLine(0, sweepY, 128);
            u8g2.drawHLine(0, sweepY - 1, 128);
            break;
        }
    }

    u8g2.sendBuffer();
}

void Homescreen::drawIdle() {
    u8g2.clearBuffer();

    unsigned long now = millis();
    float bob = sin(now / 400.0f) * 3.0f;

    // Draw all game icons in a row at center
    int totalW = _numGames * 22;
    int startX = 64 - totalW / 2;
    int iconY = 18 + (int)bob;

    for (int i = 0; i < _numGames; i++) {
        if (_entries[i].icon) {
            u8g2.drawXBMP(startX + i * 22, iconY, 16, 16, _entries[i].icon);
        }
    }

    // "PRESS ANY BUTTON" blinking
    if (_idleBlink) {
        u8g2.setFont(u8g2_font_5x7_tf);
        const char* msg = "PRESS ANY BUTTON";
        int msgW = u8g2.getStrWidth(msg);
        u8g2.drawStr(64 - msgW / 2, 50, msg);
    }

    // Ambient particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (_particles[i].life > 0) {
            u8g2.drawPixel((int)_particles[i].x, (int)_particles[i].y);
        }
    }

    // Occasionally spawn new ambient particles
    if (random(0, 100) < 15) {
        spawnParticles(random(20, 108), random(15, 50), 1);
    }
    updateParticles();

    u8g2.sendBuffer();
}

void Homescreen::drawSelection() {
    u8g2.clearBuffer();

    // Top decorative line
    u8g2.drawHLine(0, 0, 128);
    u8g2.drawHLine(0, 1, 128);

    // Title
    u8g2.setFont(u8g2_font_6x10_tf);
    drawCenteredStr(10, "SELECT GAME");

    u8g2.drawHLine(0, 14, 128);

    if (_numGames == 0) {
        u8g2.setFont(u8g2_font_6x10_tf);
        drawCenteredStr(40, "No games :(");
    } else {
        const int CARD_SPACING = 68;

        for (int i = 0; i < _numGames; i++) {
            int centerX = 64 + (int)((i - _scrollPos) * CARD_SPACING);

            if (centerX > -50 && centerX < 178) {
                drawCard(i, centerX);
            }
        }
    }

    // Bottom line
    u8g2.drawHLine(0, 62, 128);
    u8g2.drawHLine(0, 63, 128);

    u8g2.sendBuffer();
}

void Homescreen::drawCard(int idx, int centerX) {
    const int CARD_W = 50;
    const int CARD_H = 34;
    const int CARD_Y = 20;
    int x = centerX - CARD_W / 2;

    if (x < -30 || x > 138) return;

    // All cards: black background with white outline + white icon/text
    u8g2.setDrawColor(1);
    u8g2.drawFrame(x, CARD_Y, CARD_W, CARD_H);
    u8g2.drawFrame(x + 1, CARD_Y + 1, CARD_W - 2, CARD_H - 2);

    // Icon (16x16 centered in card)
    if (_entries[idx].icon) {
        int iconX = centerX - 8;
        int iconY = CARD_Y + 4;
        u8g2.setDrawColor(1);
        u8g2.drawXBMP(iconX, iconY, 16, 16, _entries[idx].icon);
    }

    // Game name below icon
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.setDrawColor(1);
    const char* name = _entries[idx].game->getName();
    int nameW = u8g2.getStrWidth(name);
    u8g2.drawStr(centerX - nameW / 2, CARD_Y + 28, name);

    // Arrow indicators for the selected card, positioned next to card borders
    int dist = idx - _selectedIndex;
    if (dist < 0) dist = -dist;
    if (dist == 0) {
        u8g2.setDrawColor(1);
        u8g2.setFont(u8g2_font_6x10_tf);
        if (idx > 0) {
            int leftArrowX = x - 12;
            if (leftArrowX < 0) leftArrowX = 0;
            u8g2.drawStr(leftArrowX, 38, "<");
        }
        if (idx < _numGames - 1) {
            int rightArrowX = x + CARD_W + 6;
            if (rightArrowX > 122) rightArrowX = 122;
            u8g2.drawStr(rightArrowX, 38, ">");
        }
    }

    u8g2.setDrawColor(1);
}
