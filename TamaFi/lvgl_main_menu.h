#pragma once

#include <Arduino.h>

// LVGL Main menu (Меню): 4 items — Status, System, Settings, Back.
// Navigation via UP/DOWN/OK (physical buttons) or touch.

void lvglMainMenuBuild();
void lvglMainMenuShow(int selectedIndex);
void lvglMainMenuSync(int selectedIndex);
