# Arduino GameBoy

A GameBoy-style handheld gaming device built on Arduino Mega 2560.

## Hardware

| Component | Pin |
|-----------|-----|
| OLED 2.42" CS | 10 |
| OLED DC | 9 |
| OLED RST | 8 |
| OLED MOSI | 51 (HW SPI) |
| OLED SCK | 52 (HW SPI) |
| Joystick VRX | A0 |
| Joystick VRY | A1 |
| Joystick SW | 2 |
| Button 1 | 32 (shoot/jump) |
| Button 2 | 30 (pause) |
| Buzzer | 40 |

## Build

1. Open `GameBoy.ino` in Arduino IDE
2. Select **Arduino Mega 2560** board
3. Install **U8g2** library via Library Manager
4. Upload

If the OLED uses SH1106 instead of SSD1309, swap the constructor in `hardware.h` and `hardware.cpp`.

## Games

- **Origin** — Classic snake game on a 16x8 grid with speed boost
- **Infinity** — Space shooter with waves of enemies (3 types), power-ups, lives, high score
- **Phallus** — Chrome dino-style endless runner with jump physics
- **Godly** — Room-based roguelike (Vampire Survivors style) with upgrade system
- **Eternal** — Vertical rocket-launch game with physics, coins, and upgrades

## Controls

- Joystick: navigate menus / move ship
- Button 1: select / shoot / jump
- Button 2: pause (in-game)
- Joystick press: select
