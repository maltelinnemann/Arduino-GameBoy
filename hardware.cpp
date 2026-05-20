#include "hardware.h"
#include <SPI.h>

U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(
    U8G2_R0,
    PIN_OLED_CS,
    PIN_OLED_DC,
    PIN_OLED_RST
);

// ==================== DEBOUNCED BUTTON ====================

void DebouncedButton::init(int p, int mode) {
    pin = p;
    pinMode(pin, mode);
    activeLow = (mode == INPUT_PULLUP);

    // Read initial state to avoid spurious edge on first update
    bool reading = digitalRead(pin);
    if (activeLow) reading = !reading;
    rawState = reading;
    state = reading;
    lastDebState = reading;
    lastChangeMs = millis();
    pressedEdge = false;
}

void DebouncedButton::update() {
    const unsigned long DEBOUNCE_MS = 30;
    lastDebState = state;

    bool reading = digitalRead(pin);
    if (activeLow) reading = !reading;  // normalize: true always means pressed

    if (reading != rawState) {
        rawState = reading;
        lastChangeMs = millis();
    }
    if (millis() - lastChangeMs > DEBOUNCE_MS) {
        bool prevState = state;
        state = rawState;
        if (state && !prevState) {
            pressedEdge = true;  // latch the press edge
        }
    }
}

bool DebouncedButton::consumePress() {
    bool result = pressedEdge;
    pressedEdge = false;
    return result;
}

bool DebouncedButton::pressed() const  { return state && !lastDebState; }
bool DebouncedButton::released() const { return !state && lastDebState; }
bool DebouncedButton::held() const     { return state; }

// ==================== HARDWARE INIT ====================

void initHardware() {
    u8g2.begin();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setContrast(255);
    initEEPROM();

    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);
}

void readInputs(InputState& input) {
    input.joyX = analogRead(PIN_JOY_X);
    input.joyY = analogRead(PIN_JOY_Y);
    input.btn1.update();
    input.btn2.update();
    input.joyButton.update();
}

// ==================== EEPROM ====================

void initEEPROM() {
    uint16_t magic;
    EEPROM.get(EEPROM_MAGIC_ADDR, magic);
    if (magic != EEPROM_MAGIC) {
        EEPROM.put(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
        uint32_t zero = 0;
        EEPROM.put(EEPROM_SI_SCORE_ADDR, zero);
        EEPROM.put(EEPROM_DJ_SCORE_ADDR, zero);
    }
}

uint32_t readHighScore(int addr) {
    uint32_t score;
    EEPROM.get(addr, score);
    return score;
}

void writeHighScore(int addr, uint32_t score) {
    uint32_t current = readHighScore(addr);
    if (score > current) {
        EEPROM.put(addr, score);
    }
}

// ==================== DRAW HELPERS ====================

void drawCenteredStr(int y, const char* s) {
    int x = (128 - u8g2.getStrWidth(s)) / 2;
    u8g2.drawStr(x, y, s);
}

void drawMenuOption(int y, const char* text, bool selected) {
    if (selected) {
        u8g2.drawStr(10, y, ">");
    }
    u8g2.drawStr(22, y, text);
}
