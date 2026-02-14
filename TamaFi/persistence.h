#pragma once

#include "pet_logic.h"

// Open NVS namespace. Call once in setup().
void persistenceInit();

// Save pet state + user settings to NVS.
void saveState(const PetState &pet);

// Load pet state + user settings from NVS.
// On first boot (no saved data), writes defaults.
void loadState(PetState &pet);
