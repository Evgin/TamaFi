#pragma once

#include <Arduino.h>
#include "navigation.h"

// Рендер меню и настроек. Вызывать при смене экрана и при отрисовке.

void uiMenuOnScreenChange(Screen newScreen);
void uiMenuUpdateHighlightTarget(Screen screen, int mainMenuIdx, int settingsIdx);

void screenMenu(int mainMenuIndex);
void screenSettings(int settingsMenuIndex);
