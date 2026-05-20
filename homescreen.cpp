#include "homescreen.h"
#include "hardware.h"

Homescreen::Homescreen() {
    _numGames = 0;
    _selectedIndex = 0;
    _lastMoveTime = 0;
    for (int i = 0; i < HOMESCREEN_MAX_GAMES; i++) {
        _games[i] = nullptr;
    }
}

void Homescreen::addGame(Game* game) {
    if (_numGames < HOMESCREEN_MAX_GAMES) {
        _games[_numGames++] = game;
    }
}

void Homescreen::setup() {
    _selectedIndex = 0;
    _lastMoveTime = 0;
}

GameResult Homescreen::loop(InputState& input) {
    unsigned long now = millis();

    if (input.up() || input.down()) {
        if (now - _lastMoveTime > 200) {
            if (input.up() && _selectedIndex > 0) {
                _selectedIndex--;
                _lastMoveTime = now;
            }
            if (input.down() && _selectedIndex < _numGames - 1) {
                _selectedIndex++;
                _lastMoveTime = now;
            }
        }
    } else {
        _lastMoveTime = 0;
    }

    if (input.joyButton.consumePress() || input.btn1.consumePress()) {
        return GameResult::GAME_SELECTED;
    }

    draw();
    return GameResult::CONTINUE;
}

Game* Homescreen::getSelectedGame() const {
    if (_selectedIndex >= 0 && _selectedIndex < _numGames) {
        return _games[_selectedIndex];
    }
    return nullptr;
}

void Homescreen::draw() {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_8x13B_tf);
    drawCenteredStr(14, "GAME BOY");

    u8g2.drawHLine(0, 20, 128);
    u8g2.drawHLine(0, 21, 128);

    u8g2.setFont(u8g2_font_6x10_tf);
    for (int i = 0; i < _numGames; i++) {
        int y = 36 + i * 14;
        drawMenuOption(y, _games[i]->getName(), i == _selectedIndex);
    }

    u8g2.sendBuffer();
}
