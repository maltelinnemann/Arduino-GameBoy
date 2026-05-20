#ifndef BITMAPS_H
#define BITMAPS_H

#include <Arduino.h>
#include <U8g2lib.h>

// Player ship 12x8
const uint8_t PLAYER_SHIP[] PROGMEM = {
    0b00000111, 0b00000000,
    0b00001111, 0b10000000,
    0b00011111, 0b11000000,
    0b00111111, 0b11100000,
    0b01111111, 0b11110000,
    0b00111111, 0b11100000,
    0b00011111, 0b11000000,
    0b00001111, 0b10000000,
};

// Basic enemy (non-shooting) 8x8
const uint8_t ENEMY_BASIC[] PROGMEM = {
    0b00100100,
    0b01011010,
    0b01111110,
    0b11111111,
    0b11111111,
    0b01100110,
    0b01100110,
    0b00100100,
};

// Single-shot enemy 8x8
const uint8_t ENEMY_SINGLE[] PROGMEM = {
    0b00011000,
    0b01111110,
    0b01111110,
    0b11111111,
    0b11111111,
    0b10100101,
    0b01011010,
    0b00100100,
};

// Multi-shot enemy 10x8
const uint8_t ENEMY_MULTI[] PROGMEM = {
    0b00011000, 0b00000000,
    0b01111110, 0b00000000,
    0b11111111, 0b00000000,
    0b11111111, 0b00000000,
    0b11111111, 0b00000000,
    0b10111101, 0b00000000,
    0b01100110, 0b00000000,
    0b00100100, 0b00000000,
};

// Extra life power-up 8x8
const uint8_t POWERUP_LIFE[] PROGMEM = {
    0b00000000,
    0b01101100,
    0b11111110,
    0b11111110,
    0b01111100,
    0b00111000,
    0b00010000,
    0b00000000,
};

// Heart icon for lives display 6x6
const uint8_t HEART[] PROGMEM = {
    0b01100110,
    0b11111111,
    0b11111111,
    0b01111110,
    0b00111100,
    0b00011000,
};

inline void drawBitmap(int x, int y, const uint8_t* bitmap, uint8_t w, uint8_t h) {
    u8g2.drawXBMP(x, y, w, h, bitmap);
}

#endif
