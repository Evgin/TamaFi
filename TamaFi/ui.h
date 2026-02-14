#pragma once

#include <Arduino.h>
#include "pet_logic.h"         // Pet, Stage, Mood, Activity, RestPhase, PetState, WifiStats
#include "navigation.h"        // Screen, currentScreen, settings externs
#include "display_amoled.h"

// ============ UI API ============

void uiInit();                                      // Call in setup()
void uiOnScreenChange(Screen newScreen);            // Call whenever currentScreen changes
void uiDrawScreen(Screen screen,
                  int mainMenuIndex,
                  int settingsMenuIndex);           // Call every loop after logic/buttons

// ============ Shared UI state (defined in ui.cpp, read by navigation for hatch) ============

// Pet position on screen
extern int petPosX;
extern int petPosY;

// ============ Externs for pet state (defined in TamaFi.ino) ============

extern PetState petState;
