#include "hardware.h"
#include "game_base.h"
#include "sound.h"
#include "bitmaps.h"
#include "homescreen.h"
#include "space_invaders.h"
#include "dino_jump.h"
#include "vampire_survivors.h"
#include "origin.h"
#include "eternal.h"

// ==================== GLOBAL INSTANCES ====================
SoundManager soundManager;
InputState input;

Homescreen homescreen;
SpaceInvadersGame spaceInvaders;
DinoJumpGame dinoJump;
VampireSurvivorsGame vampireSurvivors;
OriginGame originGame;
EternalGame eternalGame;

enum class AppState : uint8_t {
    HOMESCREEN,
    PLAYING_GAME
};

AppState appState = AppState::HOMESCREEN;
Game* currentGame = nullptr;

unsigned long lastFrameMs = 0;
const unsigned long FRAME_INTERVAL_MS = 33;  // ~30 FPS

// ==================== SETUP ====================

void setup() {
    initHardware();
    soundManager.init(PIN_BUZZER);

    // Init input buttons
    input.btn1.init(PIN_BTN1, INPUT);
    input.btn2.init(PIN_BTN2, INPUT);
    input.joyButton.init(PIN_JOY_SW, INPUT_PULLUP);

    // Register button pointers for ISR access
    _btn1Ptr = &input.btn1;
    _btn2Ptr = &input.btn2;
    _joyBtnPtr = &input.joyButton;
    enableButtonInterrupts(PIN_JOY_SW, PIN_BTN1, PIN_BTN2);

    // Register games
    homescreen.addGame(&originGame, ICON_ORIGIN);
    homescreen.addGame(&spaceInvaders, ICON_SPACE_INVADERS);
    homescreen.addGame(&dinoJump, ICON_DINO_JUMP);
    homescreen.addGame(&vampireSurvivors, ICON_VAMPIRE_SURVIVORS);
    homescreen.addGame(&eternalGame, ICON_ETERNAL);

    homescreen.setup();
    spaceInvaders.setup();
    dinoJump.setup();
    vampireSurvivors.setup();
    originGame.setup();
    eternalGame.setup();

    // Seed random from unconnected analog pin + runtime
    randomSeed(analogRead(A2) ^ analogRead(A3) ^ micros());  

    soundManager.beep(30);
}

// ==================== MAIN LOOP ====================

void loop() {
    readInputs(input);

    unsigned long now = millis();
    if (now - lastFrameMs >= FRAME_INTERVAL_MS) {
        lastFrameMs = now;

        if (appState == AppState::HOMESCREEN) {
            GameResult result = homescreen.loop(input);
            if (result == GameResult::GAME_SELECTED) {
                currentGame = homescreen.getSelectedGame();
                if (currentGame) {
                    currentGame->setup();
                    appState = AppState::PLAYING_GAME;
                    soundManager.beep(50);
                }
            }
        } else {
            GameResult result = currentGame->loop(input);
            if (result == GameResult::EXIT_TO_MENU) {
                currentGame = nullptr;
                appState = AppState::HOMESCREEN;
                homescreen.setup();
                soundManager.beep(30);
            }
        }
    }

    soundManager.update();
}
