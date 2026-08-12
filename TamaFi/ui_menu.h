#pragma once

#include <Arduino.h>
#include "navigation.h"

// Рендер меню и настроек. Вызывать при смене экрана и при отрисовке.

void uiMenuOnScreenChange(Screen newScreen);
void uiMenuUpdateHighlightTarget(Screen screen, int mainMenuIdx, int settingsIdx);

// For LVGL settings screen: get display value for settings item index.
const char* uiMenuGetSettingsValue(int index);

// For modal picker: settings with multiple options (indices 0-5).
// Returns option count, or 0 if not a picker setting.
int uiMenuGetSettingOptionsCount(int index);
// Returns label for option (e.g. "Low", "30s"). nullptr if invalid.
const char* uiMenuGetSettingOptionLabel(int index, int optionIndex);
// Apply selected option. Call after user picks in modal.
void uiMenuApplySetting(int index, int optionIndex);
