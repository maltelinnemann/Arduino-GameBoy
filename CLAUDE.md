# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build/Upload

- Open `GameBoy.ino` in Arduino IDE
- Board: **Arduino Mega 2560**
- Required library: **U8g2** (install via Library Manager)
- If the OLED uses SH1106 instead of SSD1309, swap the constructor in `hardware.h` line 17 and `hardware.cpp` line 6 from `U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI` to `U8G2_SH1106_128X64_VCOMH0_F_4W_HW_SPI`

## Hardware Pin Map

| Component | Pin | Notes |
|-----------|-----|-------|
| OLED CS | 10 | SPI |
| OLED DC | 9 | SPI |
| OLED RST | 8 | SPI |
| OLED MOSI | 51 | Hardware SPI |
| OLED SCK | 52 | Hardware SPI |
| Joystick VRX | A0 | Analog |
| Joystick VRY | A1 | Analog |
| Joystick SW | 2 | INPUT_PULLUP |
| Button 1 | 12 | External pull-down (shoot/jump) |
| Button 2 | 13 | External pull-down (pause) |
| Buzzer | 40 | Active buzzer (digital HIGH=on) |

## Architecture

**Game interface** (`game_base.h`): Every game is a class inheriting from `Game` with three methods — `setup()`, `loop(InputState&)`, `getName()`. `loop()` returns a `GameResult` enum: `CONTINUE` (still running), `GAME_SELECTED` (homescreen picked a game), or `EXIT_TO_MENU` (return home).

**App state machine** (`GameBoy.ino`): Two states — `HOMESCREEN` and `PLAYING_GAME`. The main loop reads inputs every iteration, runs game logic at 30 FPS (33ms frame timer), and dispatches to either `homescreen.loop()` or `currentGame->loop()`. Sound updates happen every iteration (not just on frames).

**Input system** (`hardware.h`): `DebouncedButton` provides edge detection (`pressed()`/`released()`) and continuous hold (`held()`) with 20ms debounce. `InputState` wraps all three buttons plus joystick analog values with directional helpers (`left()`/`right()`/`up()`/`down()` — 300/700 thresholds).

**EEPROM** (`hardware.cpp`): High scores stored at addresses 2 (Space Invaders) and 6 (Dino Jump). Magic number at address 0 validates initialized EEPROM. Use `writeHighScore()` which only writes if the new score is higher.

**Display** (`hardware.h`): Single global `u8g2` instance (full framebuffer mode, 1KB RAM). All drawing uses U8g2 primitives or `drawXBMP()` for PROGMEM bitmaps. The u8g2 object is declared `extern` in the header for all files to use.

**Sound** (`sound.h`): `SoundManager` is non-blocking — call `beep(ms)` to start, `update()` every loop iteration to turn off when the timer expires. Works with active buzzers (just digital HIGH/LOW, no PWM needed).

## Adding a New Game

1. Create `new_game.h` and `new_game.cpp`
2. Inherit from `Game`, implement `setup()`, `loop(InputState&)`, `getName()`
3. Use the same state machine pattern as existing games: `MENU → PLAYING → PAUSED → GAME_OVER`
4. In `GameBoy.ino`: add `#include "new_game.h"`, declare instance, call `homescreen.addGame(&instance)` and `instance.setup()` in `setup()`
5. For high scores: add a new EEPROM address in `hardware.h`

## Arduino IDE Constraints

- Only `.ino` files get auto-prototyped by the IDE preprocessor. `.cpp` files must have all function declarations in their corresponding `.h` files and `#include` their own header.
- All `.h` and `.cpp` files in the sketch folder compile automatically.
- Global instances defined in `.cpp` files need `extern` declarations in headers to be shared.
- Bitmap data lives in `PROGMEM` (flash) via `bitmaps.h` — use `u8g2.drawXBMP()` to render them, not `drawBitmap()` which expects RAM pointers.
