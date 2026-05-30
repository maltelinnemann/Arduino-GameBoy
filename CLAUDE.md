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
| Button 1 | 32 | External pull-down (shoot/jump/confirm) |
| Button 2 | 30 | External pull-down (pause/back) |
| Buzzer | 40 | Active buzzer (digital HIGH=on) |

## Architecture

**Game interface** (`game_base.h`): Every game is a class inheriting from `Game` with three methods — `setup()`, `loop(InputState&)`, `getName()`. `loop()` returns a `GameResult` enum: `CONTINUE` (still running), `GAME_SELECTED` (homescreen picked a game), or `EXIT_TO_MENU` (return home).

**App state machine** (`GameBoy.ino`): Two states — `HOMESCREEN` and `PLAYING_GAME`. The main loop reads inputs every iteration, runs game logic at 30 FPS (33ms frame timer), and dispatches to either `homescreen.loop()` or `currentGame->loop()`. Sound updates happen every iteration (not just on frames).

**Input system** (`hardware.h`): `DebouncedButton` provides edge detection with `consumePress()` (read + clear latched edge — always use this in game logic, never `pressed()`). `held()` for continuous actions. Debounce is 30ms. `InputState` wraps all three buttons plus joystick analog values with directional helpers (`left()`/`right()`/`up()`/`down()` — 300/700 thresholds). **Note: joystick axes are physically inverted** — `left()` reads `joyX > 700` and `right()` reads `joyX < 300` (same inversion for Y). This is a hardware wiring quirk; don't "fix" it.

**Button interrupts** (`hardware.cpp`): Joy SW (pin 2) uses INT4 (external interrupt). BTN1/BTN2 (on PORTC) use PCINT2 pin-change interrupt on PCI2 vector. A background pin-change ISR sets `irqFired` flags that `DebouncedButton::update()` polls — no debouncing happens in ISR context. PCINT2 ISR must mask the specific pins (PC7/PC5) since all PORTC changes share the same vector.

**EEPROM** (`hardware.cpp`): High scores stored as `uint32_t` at addresses 2 (Infinity), 6 (Phallus), 10 (Godly), 14 (Origin), 18 (Eternal). Magic number `0xAB01` at address 0 validates initialized EEPROM. `writeHighScore()` only writes if the new score is higher. When adding a new EEPROM address after the magic number was already written in a previous firmware version, add a sanity check in `setup()` to reset garbage values (e.g., `if (_highScore > 9999) _highScore = 0;`).

**Display** (`hardware.h`): Single global `u8g2` instance (full framebuffer mode, 1KB RAM). All drawing uses U8g2 primitives or `drawXBMP()` for PROGMEM bitmaps (NOT `drawXBM()` or `drawBitmap()` — those expect RAM pointers and will crash/glitch). The u8g2 object is declared `extern` in the header for all files to use. Screen is **128x64 pixels**.

**Sound** (`sound.h`): `SoundManager` is non-blocking — call `beep(ms)` to start, `update()` every loop iteration to turn off when the timer expires. Works with active buzzers (just digital HIGH/LOW, no PWM needed). `isPlaying()` returns whether a beep is currently active.

## Common Game Patterns

**Object pools**: Use fixed-size arrays instead of dynamic allocation. Phallus uses a 5-bullet pool (`DJBullet`), Godly uses a 20-bullet pool (`VSBullet` with `playerOwned`), Eternal uses 6-enemy / 10-coin / 20-particle pools. Each pool entry has an `alive` flag; iterate+skip dead entries, spawn by finding the first dead slot.

**Invulnerability frames**: After taking damage, set `_invulnUntil = millis() + DURATION` and check `millis() < _invulnUntil` before applying damage. Used in Infinity, Eternal, and Godly.

**Game-over input guard**: After entering GAME_OVER, record `_gameOverTime = millis()` and ignore btn2 (exit) for the first ~500ms to prevent accidental exits from a button held during gameplay. Btn1 (return to menu) needs no guard.

## Critical Drawing Pattern: Avoid Pause Flicker

Each game that has a pause overlay **must** split its draw methods into two layers:

```cpp
void drawGame() {       // Clears buffer + draws game scene. Does NOT call sendBuffer().
    u8g2.clearBuffer();
    // ... all game drawing ...
}

void drawPlaying() {    // Calls drawGame() + sendBuffer().
    drawGame();
    u8g2.sendBuffer();
}

void drawPause() {      // Calls drawGame() + overlay + its own sendBuffer().
    drawGame();          // NOT drawPlaying() — that would send an unpaused frame.
    // ... overlay with setDrawColor(0) fill + setDrawColor(1) text ...
    u8g2.sendBuffer();
}
```

Calling `drawPlaying()` from `drawPause()` causes visible flicker because the unpaused game frame flashes on screen before the overlay renders.

## Performance: Avoid `drawLine()` in Obstacles

`drawLine()` runs Bresenham on every call and causes visible lag when multiple line-drawing obstacles are on screen. Use `drawBox()`, `drawDisc()`, `drawPixel()`, and `drawHLine()`/`drawVLine()` instead. The vulva obstacle originally used 6 `drawLine()` calls and had to be replaced with box+disc primitives.

## Button Conventions

- **Btn1 / JoyButton**: Confirm / select / primary action (jump, shoot)
- **Btn2**: Pause toggle / back to previous screen
- In pause menus, always debounce selection changes (180ms delay via `_pauseMoveTime`) so the user can reliably select each option
- On the home screen, debounce game switching at 220ms via `_lastMoveTime`
- When entering pause, set `_pauseMoveTime = 0` to allow immediate first movement
- Btn2 to exit pause should ignore the first 300ms (`_pauseEnteredTime` check) to prevent accidental double-toggle

## Game State Machine

All games follow this state pattern (Eternal adds an `UPGRADE` state between PLAYING and PAUSED):

```
MENU → PLAYING → PAUSED → (back to PLAYING or MENU)
                 → GAME_OVER → MENU (Btn1) or EXIT_TO_MENU (Btn2)
```

- **MENU**: `consumePress()` on btn1/joyButton selects; up/down navigates options. Show 16x16 icon via `u8g2.drawXBMP()`, high score, and menu options.
- **PLAYING**: Game logic runs. Btn2 enters PAUSED.
- **PAUSED**: Debounced up/down for Resume/Restart/Menu. Btn2 resumes after 300ms guard. "Menu" goes to the game's own menu state (not `EXIT_TO_MENU`).
- **GAME_OVER**: Btn1 returns to the game's MENU. Btn2 returns `EXIT_TO_MENU` (home screen). Show score/room, best, and button labels.

## Home Screen

Horizontal card layout with smooth slide animation. Cards are 50x34px, each showing a 16x16 PROGMEM icon and game name. Left/right on joystick switches games with a 220ms debounce. `_scrollPos` lerps toward `_selectedIndex` at `10.0f * dt`. Selected card uses a double outline frame; unselected cards are filled white. `addGame()` takes a game pointer and a 16x16 icon pointer.

## Existing Games

### Infinity (`space_invaders.h/cpp`)
Vertical shooter. Player ship at bottom, enemies spawn from top. 3 enemy types (basic, single-shot, multi-shot) rendered via PROGMEM bitmaps. Waves scale enemy count and speed. Lives system with hearts UI and invulnerability frames. Power-up drops (+1 life). Btn1 held for continuous fire (250ms cooldown).

### Phallus (`dino_jump.h/cpp`)
Side-scrolling runner. Player auto-runs at PLAYER_X=20, jumps with btn1. 4 obstacle types drawn with fast primitives (no drawLine): butt, shaft+balls, breasts, flying. `GROUND_Y=48` with ground line at `GROUND_Y+14=62`. Obstacles sit at `GROUND_Y+14-h`. Speed ramps every 3s. Flying enemies (type 3) bob with `sin()` and always shoot. Obstacle bullets use a 5-bullet pool (`DJBullet` struct with vx/vy). Collision hitbox is slightly smaller than visual for fairness.

### Godly (`vampire_survivors.h/cpp`)
Room-based roguelike. Player moves with joystick, auto-aims at nearest enemy. Each room spawns `5 + room*3` enemies from screen edges. 4 enemy types (basic, fast, tank, shooter). After clearing a room, 3 random upgrades are offered from a pool of 8 (Vitality, Rapid Fire, Multi Shot, Swiftness, Power Up, Heal, Spread, Rear Shot). High score = best room reached (not cumulative points). Bullets use a 20-bullet pool (`VSBullet` with playerOwned flag). `findNearestEnemy()` returns index or -1; always check `>= 0` (index 0 is a valid enemy).

### Origin (`origin.h/cpp`)
Classic snake game on a 16x8 grid. Snake grows when eating food (`_length` capped at `MAX_LENGTH=100`). Direction changed via joystick — must differ from current direction to prevent reversal. Speed increases as food is eaten (`_moveIntervalMs` decreases). Boost mode with btn1 held doubles movement speed. `OGState` enum: MENU → PLAYING → PAUSED → GAME_OVER.

### Eternal (`eternal.h/cpp`)
Vertical rocket-launch game. Player controls a rocket climbing upward through enemy-filled sky, collecting coins. Upgrade system with 4 rocket types (costs coins) and 3 stat trees (fuel, thrust, hull) each with 3 levels. Physics-based movement with gravity, thrust, terminal velocity, and fuel consumption. Camera scrolls upward following the player. 3 enemy types (bird, UFO, satellite) rendered with sprite primitives. Particle effects for rocket flame and collisions. Object pools: 6 enemies, 10 coins, 20 particles. `ETState`: MENU → PLAYING → UPGRADE → PAUSED → GAME_OVER. Invulnerability frames after taking damage. High score = max altitude reached.

## Adding a New Game

1. Create `new_game.h` and `new_game.cpp`
2. Inherit from `Game`, implement `setup()`, `loop(InputState&)`, `getName()`
3. Follow the state machine pattern above (MENU/PLAYING/PAUSED/GAME_OVER + optional extra states)
4. Split draw methods into `drawGame()` (no sendBuffer) + `drawPlaying()` + `drawPause()`
5. Create a 16x16 PROGMEM icon in `bitmaps.h`
6. Add a new `EEPROM_XX_SCORE_ADDR` in `hardware.h` with matching init in `hardware.cpp`
7. In `GameBoy.ino`: add `#include`, declare instance, call `homescreen.addGame(&instance, ICON_XXX)` and `instance.setup()` in `setup()`
8. Add sanity check for garbage EEPROM values in the game's `setup()`

## Arduino IDE Constraints

- Only `.ino` files get auto-prototyped by the IDE preprocessor. `.cpp` files must have all function declarations in their corresponding `.h` files and `#include` their own header.
- All `.h` and `.cpp` files in the sketch folder compile automatically.
- Global instances defined in `.cpp` files need `extern` declarations in headers to be shared.
- Bitmap data lives in `PROGMEM` (flash) via `bitmaps.h` — use `u8g2.drawXBMP()` to render them, not `drawBitmap()` which expects RAM pointers.
- Arduino macros `min()`/`max()` only work with integers. Use manual comparisons for floats.
- `abs()` is integer-only on AVR. Use manual `x < 0 ? -x : x` for floats.
- `constrain()` is integer-only. Clamp floats manually.
- `sin()`, `sqrt()` are available via `<math.h>` (included by Arduino.h). The Mega 2560 handles them at 16MHz but use sparingly.
