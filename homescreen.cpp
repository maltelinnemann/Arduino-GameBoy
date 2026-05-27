#include "homescreen.h"
#include "hardware.h"

Homescreen::Homescreen() {
    _numGames = 0;
    _selectedIndex = 0;
    _scrollPos = 0;
    _lastMoveTime = 0;
    _lastFrameTime = 0;
    for (int i = 0; i < HOMESCREEN_MAX_GAMES; i++) {
        _entries[i].game = nullptr;
        _entries[i].icon = nullptr;
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
}

GameResult Homescreen::loop(InputState& input) {
    unsigned long now = millis();
    float dt = (now - _lastFrameTime) / 1000.0f;
    if (dt > 0.1f) dt = 0.1f;
    _lastFrameTime = now;

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

    // Smooth scroll toward selected
    float target = (float)_selectedIndex;
    float diff = target - _scrollPos;
    if (diff < 0.005f && diff > -0.005f) {
        _scrollPos = target;
    } else {
        _scrollPos += diff * 10.0f * dt;
    }

    if (input.joyButton.consumePress() || input.btn1.consumePress()) {
        return GameResult::GAME_SELECTED;
    }

    draw();
    return GameResult::CONTINUE;
}

Game* Homescreen::getSelectedGame() const {
    if (_selectedIndex >= 0 && _selectedIndex < _numGames) {
        return _entries[_selectedIndex].game;
    }
    return nullptr;
}

void Homescreen::draw() {
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
        const int CARD_SPACING = 85;

        for (int i = 0; i < _numGames; i++) {
            int centerX = 64 + (int)((i - _scrollPos) * CARD_SPACING);

            // Only draw cards that are at least partially visible
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

    // Clamp to screen bounds for clean edges
    if (x < -10 || x > 138) return;

    int dist = idx - _selectedIndex;
    if (dist < 0) dist = -dist;

    // Card frame
    if (dist == 0) {
        // Selected: clean double outline, no fill
        u8g2.setDrawColor(1);
        u8g2.drawFrame(x, CARD_Y, CARD_W, CARD_H);
        u8g2.drawFrame(x + 1, CARD_Y + 1, CARD_W - 2, CARD_H - 2);
    } else {
        // Unselected: filled white rectangle
        u8g2.setDrawColor(1);
        u8g2.drawBox(x, CARD_Y, CARD_W, CARD_H);
        u8g2.setDrawColor(0);
        u8g2.drawFrame(x + 1, CARD_Y + 1, CARD_W - 2, CARD_H - 2);
    }

    // Icon (16x16 centered in card)
    if (_entries[idx].icon) {
        int iconX = centerX - 8;
        int iconY = CARD_Y + 4;
        u8g2.setDrawColor(1);
        u8g2.drawXBMP(iconX, iconY, 16, 16, _entries[idx].icon);
    }

    // Game name below icon
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.setDrawColor(dist == 0 ? 1 : 0);
    const char* name = _entries[idx].game->getName();
    int nameW = u8g2.getStrWidth(name);
    u8g2.drawStr(centerX - nameW / 2, CARD_Y + 28, name);

    // Arrow indicators for navigation
    if (dist == 0) {
        u8g2.setDrawColor(1);
        u8g2.setFont(u8g2_font_6x10_tf);
        if (idx > 0) {
            u8g2.drawStr(0, 38, "<");
        }
        if (idx < _numGames - 1) {
            u8g2.drawStr(122, 38, ">");
        }
    }

    u8g2.setDrawColor(1);
}
