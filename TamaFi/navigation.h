#pragma once

#include <Arduino.h>
#include "pet_logic.h"
#include "input.h"

// ============ Screen enum ============

enum Screen {
    SCREEN_BOOT,
    SCREEN_HATCH,
    SCREEN_HOME,
    SCREEN_MENU,
    SCREEN_PET_STATUS,
    SCREEN_SYSINFO,
    SCREEN_SETTINGS,
    SCREEN_GAMEOVER
};

// ============ Navigation state ============

extern Screen   currentScreen;

// Hatch state
extern bool     hasHatchedOnce;
extern bool     hatchTriggered;

// Menu indices
extern int      mainMenuIndex;
extern int      settingsMenuIndex;

// ============ User settings defaults ============
#define DEFAULT_SOUND_VOLUME         0
#define DEFAULT_TFT_BRIGHTNESS_INDEX 2
#define DEFAULT_AUTO_SLEEP_MS        0       // 0=Off, 30000, 60000, 120000
#define DEFAULT_AUTO_SAVE_MS         60000

// ============ User settings ============

extern uint8_t  soundVolume;        // 0 = Off, 1-3 = volume level
extern uint8_t  tftBrightnessIndex;
extern uint32_t autoSleepMs;        // 0=Off, 30000, 60000, 120000
extern uint16_t autoSaveMs;
extern uint8_t  petSkin;            // 0=Gorgon, 1=Golem

// ============ API ============

// Initialize navigation state.
void navInit();

// Process a single input event (call after inputPoll/inputConsumeEvent).
// petState is needed for petSendCommand.
void navHandleInput(InputButton event, PetState &petState);

// Set current screen programmatically (e.g. from event handler on PET_EVT_DEATH).
void navSetScreen(Screen screen);

// Push current screen to stack and navigate to newScreen. OK on newScreen will call navGoBack().
void navPushScreen(Screen newScreen);

// Pop stack and return to previous screen. Used for OK on STATUS/SYSINFO.
void navGoBack();

// Execute the selected main menu item (single source of truth for menu actions).
// Called from both physical button OK handler and LVGL touch click callback.
void navMainMenuExecute(int idx);
