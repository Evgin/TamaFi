#pragma once

#include "pet_logic.h"
#include "navigation.h"   // Screen

// Open NVS namespace. Call once in setup().
void persistenceInit();

// Save pet state + user settings to NVS.
void saveState(const PetState &pet);

// Load pet state + user settings from NVS.
// On first boot (no saved data), writes defaults.
void loadState(PetState &pet);

// Returns screen loaded in loadState (for navInit restore after Deep Sleep wake).
Screen persistenceGetSavedScreen();

// Save battery level before Deep Sleep. Call from deviceEnterSleep.
void persistenceSaveBatteryBeforeSleep(int percent, uint16_t voltageMv);

// Log delta between saved battery (before sleep) and current. Call after persistenceInit + batteryInit.
void persistenceLogBatteryDeltaAfterWake();
