# Answers

## 1. Origin (Snake) Score Bug

**Problem:** Current score showed 14 but high score saved as 11.

**Root cause:** The high score was storing `_foodEaten` (number of food pellets eaten, starting at 0) while the game over screen displayed total snake length as `_foodEaten + 3` (initial snake is 3 segments long). So a snake that ate 11 food pellets showed "Length: 14" on screen, but only 11 was persisted to EEPROM.

**Fix:** High score now stores total snake length (`_foodEaten + 3`). A sanity check clears EEPROM values between 1-2 (impossible as total length since minimum is 3), handling migration from the old format.

## 2. Pause Button Not Always Triggering

**Problem:** Pressing Btn2 (pin 13) to pause sometimes didn't register.

**Root cause — two bugs in `hardware.cpp`:**

**Bug A: `update()` only read the pin inside the interrupt block.** The original polling code was replaced with interrupt-only reading. If the ISR didn't fire (or fired for the wrong button), the pin state was never updated. This meant that without a working interrupt, button presses were completely lost.

**Bug B: PCINT0 ISR flagged both buttons regardless of which changed.** Pins 12 (PB6) and 13 (PB7) share the same interrupt vector (PCINT0). The ISR set `irqFired = true` on both btn1 and btn2 whenever either pin changed. When `readInputs()` called `btn1.update()` first, it consumed the flag and read pin 12 — which hadn't changed. Then `btn2.update()` found `irqFired` already cleared and never read pin 13.

**Fix:**
- `update()` now always polls `digitalRead()` every call (restored original behavior). The interrupt flag is still cleared but serves only as a fast-path indicator, not the sole read mechanism.
- PCINT0 ISR now compares current `PINB` against a stored `_lastPinB` to detect which specific pin(s) changed, and only sets `irqFired` on the corresponding button(s).

**Pin interrupt capabilities:**
- Pin 2 (joystick SW): Uses INT4 — dedicated external interrupt, works reliably.
- Pin 12 (Btn1, PB6): Uses PCINT0 — pin change interrupt, shared with pin 13. Now correctly identifies the source pin.
- Pin 13 (Btn2, PB7): Uses PCINT0 — same vector as pin 12. Now correctly identified.

There is no hardware issue with the pins. Both PCINT and external interrupts work correctly on the Arduino Mega 2560 for these pins. The problems were entirely in software.
