#include "navigation.h"
#include "lvgl_settings.h"
#include "sound.h"
#include "persistence.h"
#include "display_amoled.h"

// Forward declaration — defined in ui.cpp
void uiOnScreenChange(Screen newScreen);

// ============ State definitions (externs declared in navigation.h) ============

Screen   currentScreen       = SCREEN_BOOT;

#define SCREEN_STACK_SIZE 8
static Screen   screenStack[SCREEN_STACK_SIZE];
static int      screenStackDepth = 0;

bool     hasHatchedOnce      = false;
bool     hatchTriggered      = false;

int      mainMenuIndex       = 0;
int      settingsMenuIndex   = 0;

uint8_t  soundVolume         = DEFAULT_SOUND_VOLUME;
uint8_t  tftBrightnessIndex  = DEFAULT_TFT_BRIGHTNESS_INDEX;
uint32_t autoSleepMs         = DEFAULT_AUTO_SLEEP_MS;
uint16_t autoSaveMs          = DEFAULT_AUTO_SAVE_MS;
uint8_t  petSkin             = 0;       // 0=Gorgon, 1=Golem

// ============ Internal helpers ============

static void applyTftBrightness() {
    uint8_t val = (tftBrightnessIndex == 0) ? 60 :
                  (tftBrightnessIndex == 1) ? 150 : 255;
    setDisplayBrightness(val);
}

// ============ Public API ============

void navInit() {
    currentScreen      = SCREEN_BOOT;
    screenStackDepth   = 0;
    mainMenuIndex      = 0;
    settingsMenuIndex  = 0;

    if (hasHatchedOnce) {
        Screen restored = persistenceGetSavedScreen();
        if (restored != SCREEN_BOOT && restored != SCREEN_HATCH) {
            currentScreen = restored;
        }
    }
    applyTftBrightness();
}

void navSetScreen(Screen screen) {
    Screen prev = currentScreen;
    currentScreen = screen;
    Serial.printf("[nav] setScreen: %d -> %d\n", (int)prev, (int)screen);
    uiOnScreenChange(currentScreen);
}

void navPushScreen(Screen newScreen) {
    Screen prev = currentScreen;
    if (screenStackDepth < SCREEN_STACK_SIZE) {
        screenStack[screenStackDepth++] = currentScreen;
    }
    currentScreen = newScreen;
    Serial.printf("[nav] pushScreen: %d -> %d (stack=%d)\n", (int)prev, (int)newScreen, screenStackDepth);
    uiOnScreenChange(currentScreen);
}

void navGoBack() {
    if (screenStackDepth > 0) {
        Screen prev = currentScreen;
        currentScreen = screenStack[--screenStackDepth];
        Serial.printf("[nav] goBack: %d -> %d (stack=%d)\n", (int)prev, (int)currentScreen, screenStackDepth);
        uiOnScreenChange(currentScreen);
    } else {
        Serial.println("[nav] goBack: stack empty, fallback to MENU");
        navSetScreen(SCREEN_MENU);  // fallback
    }
}

void navHandleInput(InputButton e, PetState &petState) {
    if (e == INPUT_NONE) return;

    bool up   = (e == INPUT_UP);
    bool down = (e == INPUT_DOWN);
    bool ok   = (e == INPUT_OK);
    bool r1   = (e == INPUT_R1);
    bool r2   = (e == INPUT_R2);
    bool r3   = (e == INPUT_R3);

    // Beep feedback
    if (up || down) { sndBeep();   for (int i = 0; i < 4 && soundFeed(); i++) {} }
    if (ok)         { sndBeepOk(); for (int i = 0; i < 4 && soundFeed(); i++) {} }

    // ===== QUICK-ACCESS from HOME (R1) =====
    if (currentScreen == SCREEN_HOME) {
        if (r1) { sndClick(); navPushScreen(SCREEN_PET_STATUS); return; }
    }

    // ===== RETURN from quick-access pages (R1 = back) =====
    if (currentScreen == SCREEN_PET_STATUS) {
        if (r1) {
            sndClick();
            navGoBack();
            return;
        }
    }

    // ===== BOOT =====
    if (currentScreen == SCREEN_BOOT) {
        if (up || ok || down) {
            sndClick();
            navSetScreen(hasHatchedOnce ? SCREEN_HOME : SCREEN_HATCH);
        }
        return;
    }

    // ===== HATCH =====
    if (currentScreen == SCREEN_HATCH) {
        if (ok && !hasHatchedOnce) {
            sndClick();
            hatchTriggered = true;   // UI picks this up and runs animation
        }
        return;
    }

    // ===== HOME (action strip + OK -> menu or invoke) =====
    if (currentScreen == SCREEN_HOME) {
        if (up) {
            sndClick();
            actionStripMoveSelection(-1);
        }
        if (down) {
            sndClick();
            actionStripMoveSelection(1);
        }
        if (ok) {
            sndClick();
            if (actionStripGetSelected() >= 0) {
                actionStripInvokeSelected(&petState);
            } else {
                mainMenuIndex = 0;
                navSetScreen(SCREEN_MENU);
            }
        }
        return;
    }

    // ===== MAIN MENU =====
    if (currentScreen == SCREEN_MENU) {
        if (up)   { sndClick(); mainMenuIndex = (mainMenuIndex - 1 + 4) % 4; }
        if (down) { sndClick(); mainMenuIndex = (mainMenuIndex + 1) % 4; }
        if (ok)   { sndClick(); navMainMenuExecute(mainMenuIndex); }
        return;
    }

    // ===== Simple OK-back pages (return to previous screen from stack) =====
    if (currentScreen == SCREEN_PET_STATUS ||
        currentScreen == SCREEN_SYSINFO) {
        if (ok) {
            sndClick();
            navGoBack();
        }
        return;
    }

    // ===== SETTINGS =====
    if (currentScreen == SCREEN_SETTINGS) {
        if (lvglSettingsIsModalOpen()) {
            // Picker (or msgbox) is open — forward input to it instead of
            // changing settingsMenuIndex.
            lvglSettingsModalInput(e);
            return;
        }
        if (up)   { sndClick(); settingsMenuIndex = (settingsMenuIndex - 1 + 9) % 9; }
        if (down) { sndClick(); settingsMenuIndex = (settingsMenuIndex + 1) % 9; }
        if (ok) {
            sndClick();
            lvglSettingsHandleOk(settingsMenuIndex);
        }
        return;
    }

    // ===== GAME OVER =====
    if (currentScreen == SCREEN_GAMEOVER) {
        if (ok) {
            sndClick();
            petSendCommand(petState, PET_CMD_RESET_FULL);
            petFlushCommands(petState, millis());
            hasHatchedOnce = false;
            saveState(petState);
            navSetScreen(SCREEN_HATCH);
        }
        return;
    }
}

void navMainMenuExecute(int idx) {
    switch (idx) {
        case 0: navPushScreen(SCREEN_PET_STATUS); break;
        case 1: navPushScreen(SCREEN_SYSINFO);    break;
        case 2: navSetScreen(SCREEN_SETTINGS);    break;
        case 3: navSetScreen(SCREEN_HOME);        break;
    }
}
