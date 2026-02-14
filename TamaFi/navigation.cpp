#include "navigation.h"
#include "sound.h"
#include "persistence.h"
#include "display_amoled.h"    // setDisplayBrightness

// Forward declaration — defined in ui.cpp
void uiOnScreenChange(Screen newScreen);

// ============ State definitions (externs declared in navigation.h) ============

Screen   currentScreen       = SCREEN_BOOT;

bool     hasHatchedOnce      = false;
bool     hatchTriggered      = false;

int      mainMenuIndex       = 0;
int      settingsMenuIndex   = 0;

uint8_t  soundVolume         = 3;       // 0=Off, 1-3=volume level
uint8_t  tftBrightnessIndex  = 1;
uint32_t autoSleepMs         = 60000;  // 0=Off, 30000, 60000, 120000
uint16_t autoSaveMs          = 30000;
uint8_t  petSkin             = 0;       // 0=Golem, 1=Dragon, 2=Robot, 3=Other

// ============ Internal helpers ============

static void applyTftBrightness() {
    uint8_t val = (tftBrightnessIndex == 0) ? 60 :
                  (tftBrightnessIndex == 1) ? 150 : 255;
    setDisplayBrightness(val);
}

// ============ Public API ============

void navInit() {
    currentScreen      = SCREEN_BOOT;
    mainMenuIndex      = 0;
    settingsMenuIndex  = 0;
    applyTftBrightness();
}

void navSetScreen(Screen screen) {
    currentScreen = screen;
    uiOnScreenChange(currentScreen);
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
        if (r1) { sndClick(); navSetScreen(SCREEN_PET_STATUS); return; }
    }

    // ===== RETURN from quick-access pages =====
    if (currentScreen == SCREEN_PET_STATUS) {
        if (r1) {
            sndClick();
            navSetScreen(SCREEN_HOME);
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

    // ===== HOME (OK -> menu) =====
    if (currentScreen == SCREEN_HOME) {
        if (ok) {
            sndClick();
            mainMenuIndex = 0;
            navSetScreen(SCREEN_MENU);
        }
        return;
    }

    // ===== MAIN MENU =====
    if (currentScreen == SCREEN_MENU) {
        if (up)   { sndClick(); mainMenuIndex = (mainMenuIndex - 1 + 4) % 4; }
        if (down) { sndClick(); mainMenuIndex = (mainMenuIndex + 1) % 4; }
        if (ok) {
            sndClick();
            switch (mainMenuIndex) {
                case 0: navSetScreen(SCREEN_PET_STATUS); break;
                case 1: navSetScreen(SCREEN_SYSINFO);    break;
                case 2: navSetScreen(SCREEN_SETTINGS);   break;
                case 3: navSetScreen(SCREEN_HOME);       break;
            }
        }
        return;
    }

    // ===== Simple OK-back pages =====
    if (currentScreen == SCREEN_PET_STATUS ||
        currentScreen == SCREEN_SYSINFO) {
        if (ok) {
            sndClick();
            navSetScreen(SCREEN_MENU);
        }
        return;
    }

    // ===== SETTINGS =====
    if (currentScreen == SCREEN_SETTINGS) {
        if (up)   { sndClick(); settingsMenuIndex = (settingsMenuIndex - 1 + 8) % 8; }
        if (down) { sndClick(); settingsMenuIndex = (settingsMenuIndex + 1) % 8; }
        if (ok) {
            sndClick();
            switch (settingsMenuIndex) {
                case 0:  // Screen Brightness
                    tftBrightnessIndex = (tftBrightnessIndex + 1) % 3;
                    applyTftBrightness();
                    break;
                case 1:  // Sound (3->2->1->0->3)
                    soundVolume = (soundVolume == 0) ? 3 : soundVolume - 1;
                    soundSetVolume(soundVolume);
                    if (soundVolume == 0) {
                        soundStopAll();
                    } else {
                        sndBeepOk();  // воспроизвести бип на новом уровне
                    }
                    break;
                case 2:  // Pet skin (Golem->Dragon->Robot->Other->Golem)
                    petSkin = (petSkin + 1) % 4;
                    break;
                case 3:  // Auto Sleep (Off -> 30s -> 60s -> 120s -> Off)
                    if      (autoSleepMs == 0)      autoSleepMs = 30000;
                    else if (autoSleepMs == 30000)   autoSleepMs = 60000;
                    else if (autoSleepMs == 60000)   autoSleepMs = 120000;
                    else                             autoSleepMs = 0;
                    break;
                case 4:  // Auto Save
                    if      (autoSaveMs == 15000) autoSaveMs = 30000;
                    else if (autoSaveMs == 30000) autoSaveMs = 60000;
                    else                          autoSaveMs = 15000;
                    break;
                case 5:  // Reset Pet (stats only)
                    petSendCommand(petState, PET_CMD_RESET);
                    break;
                case 6:  // Reset All
                    petSendCommand(petState, PET_CMD_RESET_FULL);
                    petFlushCommands(petState, millis());
                    hasHatchedOnce = false;
                    saveState(petState);
                    navSetScreen(SCREEN_HATCH);
                    return;
                case 7:  // Back
                    navSetScreen(SCREEN_MENU);
                    break;
            }
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
