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

// Basic enemy 8x8
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

// Heart icon 6x6
const uint8_t HEART[] PROGMEM = {
    0b01100110,
    0b11111111,
    0b11111111,
    0b01111110,
    0b00111100,
    0b00011000,
};

// ==================== 16x16 GAME ICONS ====================

// Space Invaders icon 16x16
const uint8_t ICON_SPACE_INVADERS[] PROGMEM = {
    0b00000011, 0b11000000,
    0b00001111, 0b11110000,
    0b00011111, 0b11111000,
    0b00011001, 0b10011000,
    0b00011111, 0b11111000,
    0b00111111, 0b11111100,
    0b00111001, 0b10011100,
    0b01111111, 0b11111110,
    0b01101111, 0b11110110,
    0b01000111, 0b11100010,
    0b00000011, 0b11000000,
    0b00001000, 0b00010000,
    0b00000100, 0b00100000,
    0b00000011, 0b11000000,
    0b00000000, 0b00000000,
    0b00000000, 0b00000000,
};

// Dino Jump icon 16x16
const uint8_t ICON_DINO_JUMP[] PROGMEM = {
    0b00000111, 0b11100000,
    0b00000111, 0b11110000,
    0b00000111, 0b11110000,
    0b00000111, 0b11100000,
    0b00000111, 0b10000000,
    0b00000111, 0b00000000,
    0b00000011, 0b00000000,
    0b00000011, 0b11000000,
    0b00000010, 0b01000000,
    0b00000010, 0b01000000,
    0b00000010, 0b01000000,
    0b00000010, 0b01000000,
    0b00000110, 0b01100000,
    0b00000100, 0b00100000,
    0b00000000, 0b00000000,
    0b00000000, 0b00000000,
};

// Vampire Survivors icon 16x16 (bat/skull)
const uint8_t ICON_VAMPIRE_SURVIVORS[] PROGMEM = {
    0b00000011, 0b11000000,
    0b00000111, 0b11100000,
    0b00001111, 0b11110000,
    0b00011111, 0b11111000,
    0b00011101, 0b10111000,
    0b00111111, 0b11111100,
    0b00111111, 0b11111100,
    0b01111001, 0b10011110,
    0b01110001, 0b10001110,
    0b00110000, 0b00001100,
    0b00011000, 0b00011000,
    0b00001100, 0b00110000,
    0b00000111, 0b11100000,
    0b00000011, 0b11000000,
    0b00000001, 0b10000000,
    0b00000000, 0b00000000,
};

// Player character for Vampire Survivors 8x8
const uint8_t VS_PLAYER[] PROGMEM = {
    0b00011000,
    0b00111100,
    0b01111110,
    0b01111110,
    0b01111110,
    0b00111100,
    0b00011000,
    0b00000000,
};

// Basic enemy for Vampire Survivors 8x8 (skull)
const uint8_t VS_ENEMY_BASIC[] PROGMEM = {
    0b00111100,
    0b01111110,
    0b11111111,
    0b10111101,
    0b10111101,
    0b11111111,
    0b01000010,
    0b00111100,
};

// Fast enemy for Vampire Survivors 8x8 (bat)
const uint8_t VS_ENEMY_FAST[] PROGMEM = {
    0b10000001,
    0b11000011,
    0b01100110,
    0b01111110,
    0b01111110,
    0b01100110,
    0b11000011,
    0b10000001,
};

// Tank enemy for Vampire Survivors 10x10
const uint8_t VS_ENEMY_TANK[] PROGMEM = {
    0b00111100, 0b00000000,
    0b01111110, 0b00000000,
    0b11111111, 0b00000000,
    0b11111111, 0b00000000,
    0b11111111, 0b00000000,
    0b11111111, 0b00000000,
    0b11111111, 0b00000000,
    0b01111110, 0b00000000,
    0b00111100, 0b00000000,
    0b00000000, 0b00000000,
};

// Shooter enemy for Vampire Survivors 8x8
const uint8_t VS_ENEMY_SHOOTER[] PROGMEM = {
    0b00011000,
    0b00111100,
    0b01111110,
    0b11111111,
    0b11111111,
    0b01011010,
    0b00011000,
    0b00000000,
};

// ==================== UPGRADE ICONS 8x8 ====================

// Vitality: heart
const uint8_t ICON_UPG_VITALITY[] PROGMEM = {
    0b01101100,
    0b11111110,
    0b11111110,
    0b11111110,
    0b01111100,
    0b00111000,
    0b00010000,
    0b00000000,
};

// Rapid Fire: double arrow right
const uint8_t ICON_UPG_RAPIDFIRE[] PROGMEM = {
    0b00001000,
    0b00011000,
    0b00111000,
    0b01111111,
    0b01111111,
    0b00111000,
    0b00011000,
    0b00001000,
};

// Multi Shot: three bullets
const uint8_t ICON_UPG_MULTISHOT[] PROGMEM = {
    0b00000000,
    0b01000100,
    0b01000100,
    0b11111111,
    0b11111111,
    0b01000100,
    0b01000100,
    0b00000000,
};

// Swiftness: wing/boot
const uint8_t ICON_UPG_SWIFTNESS[] PROGMEM = {
    0b00001110,
    0b00011110,
    0b00111100,
    0b01111110,
    0b01111110,
    0b00111100,
    0b00011110,
    0b00001110,
};

// Power Up: star
const uint8_t ICON_UPG_POWER[] PROGMEM = {
    0b00001000,
    0b00101010,
    0b00011100,
    0b01111111,
    0b00011100,
    0b00101010,
    0b00001000,
    0b00000000,
};

// Heal: plus/cross
const uint8_t ICON_UPG_HEAL[] PROGMEM = {
    0b00001000,
    0b00001000,
    0b00001000,
    0b01111111,
    0b01111111,
    0b00001000,
    0b00001000,
    0b00001000,
};

// Spread: fan arrows
const uint8_t ICON_UPG_SPREAD[] PROGMEM = {
    0b01000001,
    0b01100011,
    0b00110110,
    0b00011100,
    0b00011100,
    0b00110110,
    0b01100011,
    0b01000001,
};

// Origin game icon 16x16 (spark/origin point with rays)
const uint8_t ICON_ORIGIN[] PROGMEM = {
    0b00000001, 0b10000000,
    0b00000001, 0b10000000,
    0b00000001, 0b10000000,
    0b00010001, 0b10001000,
    0b00001001, 0b10010000,
    0b00000111, 0b11100000,
    0b00000011, 0b11000000,
    0b01111111, 0b11111110,
    0b01111111, 0b11111110,
    0b00000011, 0b11000000,
    0b00000111, 0b11100000,
    0b00001001, 0b10010000,
    0b00010001, 0b10001000,
    0b00000001, 0b10000000,
    0b00000001, 0b10000000,
    0b00000001, 0b10000000,
};

// Rear Shot: arrow left
const uint8_t ICON_UPG_REARSHOT[] PROGMEM = {
    0b00000000,
    0b00000100,
    0b00001100,
    0b11111111,
    0b11111111,
    0b00001100,
    0b00000100,
    0b00000000,
};

// Icon lookup by upgrade ID
inline const uint8_t* getUpgradeIcon(uint8_t id) {
    switch (id) {
        case 0: return ICON_UPG_VITALITY;
        case 1: return ICON_UPG_RAPIDFIRE;
        case 2: return ICON_UPG_SWIFTNESS;
        case 3: return ICON_UPG_POWER;
        case 4: return ICON_UPG_HEAL;
        case 5: return ICON_UPG_SPREAD;
        case 6: return ICON_UPG_REARSHOT;
        default: return ICON_UPG_POWER;
    }
}

inline void drawBitmap(int x, int y, const uint8_t* bitmap, uint8_t w, uint8_t h) {
    u8g2.drawXBMP(x, y, w, h, bitmap);
}

#endif
