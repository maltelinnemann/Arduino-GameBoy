#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <EEPROM.h>

// ==================== PIN DEFINITIONS ====================
// Display SPI (hardware SPI uses 51/MOSI, 52/SCK on Mega)
#define PIN_OLED_CS   10
#define PIN_OLED_DC    9
#define PIN_OLED_RST   8

// Joystick
#define PIN_JOY_X     A0
#define PIN_JOY_Y     A1
#define PIN_JOY_SW     2

// External pull-down buttons
#define PIN_BTN1      32
#define PIN_BTN2      30

// Active buzzer
#define PIN_BUZZER    40

// ==================== DISPLAY ====================
extern U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2;

// ==================== INPUT ====================
struct DebouncedButton {
    int pin;
    bool state;          // true = pressed (normalized for active-low/high)
    bool lastDebState;
    bool rawState;       // normalized raw reading
    unsigned long lastChangeMs;
    bool activeLow;      // true when INPUT_PULLUP (pressed reads LOW)
    bool pressedEdge;    // latched: true if pressed since last consume
    volatile bool irqFired; // set by ISR on any pin change

    void init(int p, int mode);
    void update();
    bool pressed() const;      // instantaneous edge (avoid using across frames)
    bool released() const;
    bool held() const;
    bool consumePress();       // read + clear latched edge — use this in game logic
};

struct InputState {
    int joyX;
    int joyY;

    DebouncedButton btn1;
    DebouncedButton btn2;
    DebouncedButton joyButton;

    bool left() const  { return joyX > 700; }
    bool right() const { return joyX < 300; }
    bool up() const    { return joyY > 700; }
    bool down() const  { return joyY < 300; }
};

// ==================== EEPROM HIGH SCORES ====================
#define EEPROM_MAGIC          0xAB01
#define EEPROM_MAGIC_ADDR     0
#define EEPROM_SI_SCORE_ADDR  2
#define EEPROM_DJ_SCORE_ADDR  6
#define EEPROM_VS_SCORE_ADDR  10
#define EEPROM_OG_SCORE_ADDR  14
#define EEPROM_ET_SCORE_ADDR  18

void initEEPROM();
uint32_t readHighScore(int addr);
void writeHighScore(int addr, uint32_t score);

// ==================== INTERRUPTS ====================
extern DebouncedButton* _btn1Ptr;
extern DebouncedButton* _btn2Ptr;
extern DebouncedButton* _joyBtnPtr;
void enableButtonInterrupts(int joyPin, int btn1Pin, int btn2Pin);

// ==================== INIT ====================
void initHardware();
void readInputs(InputState& input);

// ==================== DRAW HELPERS ====================
void drawCenteredStr(int y, const char* s);
void drawMenuOption(int y, const char* text, bool selected);

#endif
