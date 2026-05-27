#include "vampire_survivors.h"
#include "hardware.h"
#include "sound.h"
#include "bitmaps.h"

extern SoundManager soundManager;

const VSUpgrade VampireSurvivorsGame::UPGRADE_POOL[UPGRADE_POOL_SIZE] = {
    {0, "Vitality",   "Max HP +1"},
    {1, "Rapid Fire", "Fire rate +20%"},
    {2, "Multi Shot", "+1 bullet"},
    {3, "Swiftness",  "Move speed +10%"},
    {4, "Power Up",   "Damage +1"},
    {5, "Heal",       "Restore 2 HP"},
    {6, "Spread",     "+1 bullet & fan"},
    {7, "Rear Shot",  "Shoot behind"},
};

VampireSurvivorsGame::VampireSurvivorsGame() {
    _pauseLabels[0] = "Resume";
    _pauseLabels[1] = "Restart";
    _pauseLabels[2] = "Menu";
}

void VampireSurvivorsGame::setup() {
    _state = VSState::MENU;
    _menuSelection = 0;
    _menuEnteredTime = millis();
    _highScore = readHighScore(EEPROM_VS_SCORE_ADDR);
    if (_highScore > 9999) _highScore = 0;
}

// ==================== MAIN LOOP ====================

GameResult VampireSurvivorsGame::loop(InputState& input) {
    switch (_state) {
        case VSState::MENU: {
            if (input.up() && _menuSelection > 0) _menuSelection--;
            if (input.down() && _menuSelection < 1) _menuSelection++;
            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                if (_menuSelection == 0) startGame();
                else return GameResult::EXIT_TO_MENU;
            }
            drawMenu();
            return GameResult::CONTINUE;
        }

        case VSState::PLAYING: {
            if (input.btn2.consumePress()) {
                _state = VSState::PAUSED;
                _pauseSelection = 0;
                _pauseEnteredTime = millis();
                _pauseMoveTime = 0;
                return GameResult::CONTINUE;
            }

            updatePlayer(input);
            updateEnemies();
            updateBullets();

            // Auto-shoot at nearest enemy
            if (millis() - _player.lastShotMs >= (unsigned long)_player.fireRateMs) {
                float dx, dy;
                int nearest = findNearestEnemy(dx, dy);
                if (nearest >= 0) {
                    firePlayerBullets(dx, dy);
                    _player.lastShotMs = millis();
                }
            }

            // Spawn enemies
            if (_enemiesSpawned < _enemiesToSpawn &&
                millis() - _lastSpawnMs >= (unsigned long)_spawnIntervalMs) {
                spawnEnemy();
                _lastSpawnMs = millis();
            }

            // Room cleared?
            if (_enemiesSpawned >= _enemiesToSpawn && _aliveCount == 0) {
                _room++;
                pickUpgrades();
                _state = VSState::UPGRADE_SELECT;
                _upgradeSelection = 0;
                _upgradeMoveTime = 0;
                soundManager.beep(80);
            }

            checkCollisions();
            drawPlaying();
            return GameResult::CONTINUE;
        }

        case VSState::UPGRADE_SELECT: {
            unsigned long now = millis();
            if (now - _upgradeMoveTime > 180) {
                if (input.up() && _upgradeSelection > 0) {
                    _upgradeSelection--;
                    _upgradeMoveTime = now;
                }
                if (input.down() && _upgradeSelection < UPGRADE_OPTIONS - 1) {
                    _upgradeSelection++;
                    _upgradeMoveTime = now;
                }
            }
            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                applyUpgrade(_upgrades[_upgradeSelection].id);
                startRoom();
                _state = VSState::PLAYING;
            }
            drawUpgradeSelect();
            return GameResult::CONTINUE;
        }

        case VSState::PAUSED: {
            unsigned long now = millis();
            if (now - _pauseMoveTime > 180) {
                if (input.up() && _pauseSelection > 0) {
                    _pauseSelection--;
                    _pauseMoveTime = now;
                }
                if (input.down() && _pauseSelection < PAUSE_OPTIONS - 1) {
                    _pauseSelection++;
                    _pauseMoveTime = now;
                }
            }
            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                if (_pauseSelection == 0) _state = VSState::PLAYING;
                else if (_pauseSelection == 1) startGame();
                else {
                    _state = VSState::MENU;
                    _menuSelection = 0;
                    _menuEnteredTime = millis();
                }
            }
            if (input.btn2.consumePress() && millis() - _pauseEnteredTime > 300) {
                _state = VSState::PLAYING;
            }
            drawPause();
            return GameResult::CONTINUE;
        }

        case VSState::GAME_OVER: {
            if (input.btn1.consumePress() || input.joyButton.consumePress()) {
                _state = VSState::MENU;
                _menuSelection = 0;
                _menuEnteredTime = millis();
            } else if (input.btn2.consumePress()) {
                return GameResult::EXIT_TO_MENU;
            }
            drawGameOver();
            return GameResult::CONTINUE;
        }
    }
    return GameResult::CONTINUE;
}

// ==================== GAME START ====================

void VampireSurvivorsGame::startGame() {
    _state = VSState::PLAYING;
    _player.x = 60;
    _player.y = 30;
    _player.hp = 5;
    _player.maxHp = 5;
    _player.moveSpeed = 1.5f;
    _player.fireRateMs = 500;
    _player.bulletCount = 1;
    _player.damage = 1;
    _player.spreadShot = false;
    _player.backShot = false;
    _player.lastShotMs = 0;
    _player.invulnerableUntil = 0;
    _player.facingDir = 0;

    for (int i = 0; i < MAX_ENEMIES; i++) _enemies[i].alive = false;
    for (int i = 0; i < MAX_BULLETS; i++) _bullets[i].active = false;

    _room = 0;
    startRoom();
}

void VampireSurvivorsGame::startRoom() {
    _enemiesToSpawn = 5 + _room * 3;
    _enemiesSpawned = 0;
    _aliveCount = 0;
    _spawnIntervalMs = max(200, 700 - _room * 40);
    _lastSpawnMs = millis();
}

// ==================== ENEMIES ====================

void VampireSurvivorsGame::spawnEnemy() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!_enemies[i].alive) {
            VSEnemy& e = _enemies[i];
            int edge = random(0, 4);
            switch (edge) {
                case 0: e.x = random(0, 120); e.y = -6; break;
                case 1: e.x = random(0, 120); e.y = 70; break;
                case 2: e.x = -6; e.y = random(0, 56); break;
                case 3: e.x = 134; e.y = random(0, 56); break;
            }
            int r = random(0, 100);
            if (_room <= 2) {
                e.type = (r < 75) ? 0 : 1;
                e.hp = 1;
            } else if (_room <= 5) {
                if (r < 50)      { e.type = 0; e.hp = 1; }
                else if (r < 80) { e.type = 1; e.hp = 1; }
                else             { e.type = 2; e.hp = 3; }
            } else {
                if (r < 35)      { e.type = 0; e.hp = 1; }
                else if (r < 60) { e.type = 1; e.hp = 1; }
                else if (r < 82) { e.type = 2; e.hp = 3; }
                else             { e.type = 3; e.hp = 2; }
            }
            e.alive = true;
            e.lastShotMs = millis() + random(1500, 3500);
            e.lastMoveMs = millis();
            _enemiesSpawned++;
            _aliveCount++;
            return;
        }
    }
}

void VampireSurvivorsGame::updateEnemies() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!_enemies[i].alive) continue;
        VSEnemy& e = _enemies[i];

        float dx = _player.x - e.x;
        float dy = _player.y - e.y;
        float dist = sqrt(dx * dx + dy * dy);
        if (dist < 0.5f) dist = 0.5f;

        float speed;
        switch (e.type) {
            case 0: speed = 0.4f + _room * 0.03f; break;
            case 1: speed = 0.9f + _room * 0.04f; break;
            case 2: speed = 0.25f + _room * 0.02f; break;
            case 3: speed = 0.35f; break;
            default: speed = 0.4f; break;
        }

        // Shooter: maintain distance
        if (e.type == 3) {
            if (dist < 28) {
                e.vx = -dx / dist * speed;
                e.vy = -dy / dist * speed;
            } else if (dist > 48) {
                e.vx = dx / dist * speed;
                e.vy = dy / dist * speed;
            } else {
                e.vx *= 0.9f;
                e.vy *= 0.9f;
            }
        } else {
            e.vx = dx / dist * speed;
            e.vy = dy / dist * speed;
        }

        // Fast enemy wobble
        if (e.type == 1 && millis() - e.lastMoveMs > 400) {
            e.vx += random(-25, 26) / 100.0f;
            e.vy += random(-25, 26) / 100.0f;
            e.lastMoveMs = millis();
        }

        e.x += e.vx;
        e.y += e.vy;

        if (e.x < 0) e.x = 0;
        if (e.x > 120) e.x = 120;
        if (e.y < 0) e.y = 0;
        if (e.y > 56) e.y = 56;

        // Shooter fires
        if (e.type == 3 && millis() >= e.lastShotMs) {
            fireEnemyBullet(e);
            e.lastShotMs = millis() + random(1800, 4000);
        }
    }
}

// ==================== BULLETS ====================

int VampireSurvivorsGame::findNearestEnemy(float& outDx, float& outDy) {
    float best = 999999;
    int bestIdx = -1;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!_enemies[i].alive) continue;
        float dx = _enemies[i].x - _player.x;
        float dy = _enemies[i].y - _player.y;
        float d = dx * dx + dy * dy;
        if (d < best) {
            best = d;
            bestIdx = i;
            outDx = dx;
            outDy = dy;
        }
    }
    return bestIdx;
}

void VampireSurvivorsGame::firePlayerBullets(float dx, float dy) {
    float len = sqrt(dx * dx + dy * dy);
    if (len < 0.01f) { dx = 1; dy = 0; len = 1; }
    dx /= len;
    dy /= len;
    float perpX = -dy;
    float perpY = dx;
    float bulletSpeed = 3.0f;
    int count = _player.bulletCount;

    for (int dir = 0; dir < (_player.backShot ? 2 : 1); dir++) {
        float sdx = (dir == 0) ? dx : -dx;
        float sdy = (dir == 0) ? dy : -dy;

        for (int b = 0; b < count; b++) {
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!_bullets[i].active) {
                    float bdx = sdx;
                    float bdy = sdy;
                    if (_player.spreadShot) {
                        float t = (b - (count - 1) / 2.0f) * 0.35f;
                        bdx = sdx + perpX * t;
                        bdy = sdy + perpY * t;
                        float blen = sqrt(bdx * bdx + bdy * bdy);
                        if (blen > 0.01f) { bdx /= blen; bdy /= blen; }
                    }
                    _bullets[i].x = _player.x + 4;
                    _bullets[i].y = _player.y + 4;
                    _bullets[i].vx = bdx * bulletSpeed;
                    _bullets[i].vy = bdy * bulletSpeed;
                    _bullets[i].active = true;
                    _bullets[i].playerOwned = true;
                    _bullets[i].damage = _player.damage;
                    break;
                }
            }
        }
    }
}

void VampireSurvivorsGame::fireEnemyBullet(const VSEnemy& e) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!_bullets[i].active) {
            float dx = _player.x - e.x;
            float dy = _player.y - e.y;
            float len = sqrt(dx * dx + dy * dy);
            if (len < 0.5f) len = 0.5f;
            _bullets[i].x = e.x + 4;
            _bullets[i].y = e.y + 4;
            _bullets[i].vx = dx / len * 1.5f;
            _bullets[i].vy = dy / len * 1.5f;
            _bullets[i].active = true;
            _bullets[i].playerOwned = false;
            _bullets[i].damage = 1;
            return;
        }
    }
}

void VampireSurvivorsGame::updateBullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!_bullets[i].active) continue;
        _bullets[i].x += _bullets[i].vx;
        _bullets[i].y += _bullets[i].vy;
        if (_bullets[i].x < -5 || _bullets[i].x > 133 ||
            _bullets[i].y < -5 || _bullets[i].y > 69) {
            _bullets[i].active = false;
        }
    }
}

// ==================== PLAYER ====================

void VampireSurvivorsGame::updatePlayer(InputState& input) {
    float mx = 0, my = 0;
    if (input.left())  { mx = -_player.moveSpeed; _player.facingDir = 2; }
    if (input.right()) { mx = _player.moveSpeed;  _player.facingDir = 0; }
    if (input.up())    { my = -_player.moveSpeed; _player.facingDir = 1; }
    if (input.down())  { my = _player.moveSpeed;  _player.facingDir = 3; }

    // Diagonal normalization
    if (mx != 0 && my != 0) {
        mx *= 0.707f;
        my *= 0.707f;
    }

    _player.x += mx;
    _player.y += my;

    if (_player.x < 0) _player.x = 0;
    if (_player.x > 120) _player.x = 120;
    if (_player.y < 0) _player.y = 0;
    if (_player.y > 56) _player.y = 56;
}

// ==================== COLLISION ====================

void VampireSurvivorsGame::checkCollisions() {
    // Player bullets vs enemies
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!_bullets[i].active || !_bullets[i].playerOwned) continue;
        VSBullet& b = _bullets[i];

        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!_enemies[j].alive) continue;
            VSEnemy& e = _enemies[j];
            int ew = (e.type == 2) ? 10 : 8;
            int eh = (e.type == 2) ? 10 : 8;

            if (b.x >= e.x && b.x <= e.x + ew &&
                b.y >= e.y && b.y <= e.y + eh) {
                b.active = false;
                e.hp -= b.damage;
                if (e.hp <= 0) {
                    e.alive = false;
                    _aliveCount--;
                    soundManager.beep(12);
                }
                break;
            }
        }
    }

    // Enemy bullets vs player
    if (millis() >= _player.invulnerableUntil) {
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!_bullets[i].active || _bullets[i].playerOwned) continue;
            VSBullet& b = _bullets[i];
            if (b.x >= _player.x && b.x <= _player.x + 8 &&
                b.y >= _player.y && b.y <= _player.y + 8) {
                b.active = false;
                _player.hp--;
                _player.invulnerableUntil = millis() + 800;
                soundManager.beep(80);
                if (_player.hp <= 0) {
                    _state = VSState::GAME_OVER;
                    writeHighScore(EEPROM_VS_SCORE_ADDR, _room);
                    _highScore = readHighScore(EEPROM_VS_SCORE_ADDR);
                    _gameOverTime = millis();
                }
                return;
            }
        }

        // Enemy contact damage
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!_enemies[i].alive) continue;
            VSEnemy& e = _enemies[i];
            int ew = (e.type == 2) ? 10 : 8;
            int eh = (e.type == 2) ? 10 : 8;
            if (_player.x + 6 > e.x && _player.x + 2 < e.x + ew &&
                _player.y + 6 > e.y && _player.y + 2 < e.y + eh) {
                _player.hp--;
                _player.invulnerableUntil = millis() + 800;
                soundManager.beep(80);
                // Knockback
                float kdx = _player.x - e.x;
                float kdy = _player.y - e.y;
                float klen = sqrt(kdx * kdx + kdy * kdy);
                if (klen > 0.5f) {
                    _player.x += kdx / klen * 5;
                    _player.y += kdy / klen * 5;
                }
                if (_player.hp <= 0) {
                    _state = VSState::GAME_OVER;
                    writeHighScore(EEPROM_VS_SCORE_ADDR, _room);
                    _highScore = readHighScore(EEPROM_VS_SCORE_ADDR);
                    _gameOverTime = millis();
                }
                return;
            }
        }
    }
}

// ==================== UPGRADES ====================

void VampireSurvivorsGame::pickUpgrades() {
    // Pick 3 unique random upgrades
    bool used[UPGRADE_POOL_SIZE];
    for (int i = 0; i < UPGRADE_POOL_SIZE; i++) used[i] = false;

    for (int i = 0; i < UPGRADE_OPTIONS; i++) {
        int idx;
        do {
            idx = random(0, UPGRADE_POOL_SIZE);
        } while (used[idx]);
        used[idx] = true;
        _upgrades[i] = UPGRADE_POOL[idx];
    }
}

void VampireSurvivorsGame::applyUpgrade(uint8_t id) {
    switch (id) {
        case 0: _player.maxHp += 1; _player.hp = min(_player.hp + 1, _player.maxHp); break;
        case 1: _player.fireRateMs = max(120, (int)(_player.fireRateMs * 0.8f)); break;
        case 2: _player.bulletCount = min(6, _player.bulletCount + 1); break;
        case 3: _player.moveSpeed += 0.2f; break;
        case 4: _player.damage += 1; break;
        case 5: _player.hp = min(_player.hp + 2, _player.maxHp); break;
        case 6: _player.spreadShot = true; _player.bulletCount = min(6, _player.bulletCount + 1); break;
        case 7: _player.backShot = true; break;
    }
}

// ==================== DRAW ====================

void VampireSurvivorsGame::drawMenu() {
    u8g2.clearBuffer();

    u8g2.drawXBMP(56, 2, 16, 16, ICON_VAMPIRE_SURVIVORS);

    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[32];
    snprintf(buf, sizeof(buf), "Best Room: %lu", _highScore);
    drawCenteredStr(26, buf);

    drawMenuOption(42, "Start Game", _menuSelection == 0);
    drawMenuOption(54, "Return to Menu", _menuSelection == 1);

    u8g2.sendBuffer();
}

void VampireSurvivorsGame::drawGame() {
    u8g2.clearBuffer();

    // Room background (subtle grid lines)
    for (int gx = 0; gx < 128; gx += 16) {
        u8g2.drawVLine(gx, 0, 64);
    }

    drawEnemies();
    drawBullets();

    // Player (blink if invulnerable)
    if (millis() >= _player.invulnerableUntil || (millis() / 80) % 2) {
        drawPlayer();
    }

    drawUI();
}

void VampireSurvivorsGame::drawPlaying() {
    drawGame();
    u8g2.sendBuffer();
}

void VampireSurvivorsGame::drawPlayer() {
    u8g2.drawXBMP((int)_player.x, (int)_player.y, 8, 8, VS_PLAYER);
}

void VampireSurvivorsGame::drawEnemies() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!_enemies[i].alive) continue;
        VSEnemy& e = _enemies[i];
        switch (e.type) {
            case 0:
                u8g2.drawXBMP((int)e.x, (int)e.y, 8, 8, VS_ENEMY_BASIC);
                break;
            case 1:
                u8g2.drawXBMP((int)e.x, (int)e.y, 8, 8, VS_ENEMY_FAST);
                break;
            case 2:
                u8g2.drawXBMP((int)e.x, (int)e.y, 10, 10, VS_ENEMY_TANK);
                break;
            case 3:
                u8g2.drawXBMP((int)e.x, (int)e.y, 8, 8, VS_ENEMY_SHOOTER);
                break;
        }
        // HP bar for tanks
        if (e.type == 2 && e.hp > 1) {
            u8g2.drawHLine((int)e.x, (int)e.y - 2, e.hp * 3);
        }
    }
}

void VampireSurvivorsGame::drawBullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!_bullets[i].active) continue;
        if (_bullets[i].playerOwned) {
            u8g2.drawPixel((int)_bullets[i].x, (int)_bullets[i].y);
            u8g2.drawPixel((int)_bullets[i].x + 1, (int)_bullets[i].y);
        } else {
            // Enemy bullet: small cross
            u8g2.drawPixel((int)_bullets[i].x, (int)_bullets[i].y);
            u8g2.drawPixel((int)_bullets[i].x, (int)_bullets[i].y + 1);
        }
    }
}

void VampireSurvivorsGame::drawUI() {
    // HP bar top-left
    u8g2.drawFrame(1, 1, 30, 5);
    int hpW = (_player.hp * 28) / max(1, _player.maxHp);
    if (hpW > 0) u8g2.drawBox(2, 2, hpW, 3);

    u8g2.setFont(u8g2_font_4x6_tf);
    char buf[16];
    snprintf(buf, sizeof(buf), "HP:%d/%d", _player.hp, _player.maxHp);
    u8g2.drawStr(33, 5, buf);

    // Room indicator
    u8g2.setFont(u8g2_font_5x7_tf);
    snprintf(buf, sizeof(buf), "Room %d", _room);
    u8g2.drawStr(85, 6, buf);

    // Alive count (debug-ish, bottom-right)
    snprintf(buf, sizeof(buf), "x%d", _aliveCount);
    u8g2.drawStr(110, 62, buf);
}

void VampireSurvivorsGame::drawUpgradeSelect() {
    u8g2.clearBuffer();

    u8g2.setDrawColor(0);
    u8g2.drawBox(0, 0, 128, 64);
    u8g2.setDrawColor(1);
    u8g2.drawFrame(0, 0, 128, 64);

    u8g2.setFont(u8g2_font_6x10_tf);
    drawCenteredStr(12, "CHOOSE UPGRADE");

    u8g2.setFont(u8g2_font_5x7_tf);
    for (int i = 0; i < UPGRADE_OPTIONS; i++) {
        int y = 22 + i * 14;
        bool sel = (i == _upgradeSelection);
        if (sel) {
            u8g2.drawBox(4, y - 2, 120, 13);
            u8g2.setDrawColor(0);
        }
        u8g2.drawStr(8, y + 5, _upgrades[i].name);
        u8g2.drawStr(72, y + 5, _upgrades[i].desc);
        if (sel) u8g2.setDrawColor(1);
    }

    u8g2.drawHLine(4, 18, 120);
    u8g2.sendBuffer();
}

void VampireSurvivorsGame::drawPause() {
    drawGame();

    u8g2.setDrawColor(0);
    u8g2.drawBox(0, 16, 128, 48);
    u8g2.setDrawColor(1);
    u8g2.drawFrame(0, 16, 128, 48);

    u8g2.setFont(u8g2_font_6x10_tf);
    drawCenteredStr(28, "PAUSED");
    for (int i = 0; i < PAUSE_OPTIONS; i++) {
        drawMenuOption(40 + i * 10, _pauseLabels[i], _pauseSelection == i);
    }
    u8g2.sendBuffer();
}

void VampireSurvivorsGame::drawGameOver() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13B_tf);
    drawCenteredStr(14, "YOU DIED");

    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[32];
    snprintf(buf, sizeof(buf), "Room reached: %d", _room);
    drawCenteredStr(30, buf);

    bool isNew = _room >= (int)_highScore && _room > 0;
    snprintf(buf, sizeof(buf), "Best: %lu%s", _highScore, isNew ? " NEW!" : "");
    drawCenteredStr(42, buf);

    drawCenteredStr(56, "Btn1: Menu  Btn2: Home");
    u8g2.sendBuffer();
}
