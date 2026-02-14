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

// ============ User settings ============

extern uint8_t  soundVolume;        // 0 = Off, 1-3 = volume level
extern uint8_t  tftBrightnessIndex;
extern uint32_t autoSleepMs;        // 0=Off, 30000, 60000, 120000
extern uint16_t autoSaveMs;
extern uint8_t  petSkin;            // 0=Golem, 1=Dragon, 2=Robot, 3=Other

// ============ API ============

// Initialize navigation state.
void navInit();

// Process a single input event (call after inputPoll/inputConsumeEvent).
// petState is needed for petSendCommand.
void navHandleInput(InputButton event, PetState &petState);

// Set current screen programmatically (e.g. from event handler on PET_EVT_DEATH).
void navSetScreen(Screen screen);
