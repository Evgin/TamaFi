#pragma once

#include <Arduino.h>
#include "input.h"

// LVGL Settings screen.
// Renders 9-item settings list. Navigation via physical buttons or touch.

void lvglSettingsBuild();
void lvglSettingsShow(int selectedIndex);
void lvglSettingsSync(int selectedIndex);

// Called from navigation when OK pressed on Settings (physical button).
// Opens modal picker (0-5), msgbox (6-7), or navigates back (8).
void lvglSettingsHandleOk(int selectedIndex);

// Returns true when a picker or msgbox modal is currently open.
// Navigation should forward input to lvglSettingsModalInput instead of
// changing settingsMenuIndex.
bool lvglSettingsIsModalOpen();

// Forward physical button input to the open modal (picker navigation).
// Call only when lvglSettingsIsModalOpen() == true.
void lvglSettingsModalInput(InputButton btn);
