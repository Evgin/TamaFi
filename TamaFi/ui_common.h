#pragma once

#include <Arduino.h>

// Общие элементы UI: шапка, батарея, анимация подсветки

void drawHeader(const char* title);

// Draw header directly on display (368x18) for LVGL menu screens (variant A).
// Call before lv_timer_handler when on MENU/SETTINGS/PET_STATUS/SYSINFO.
void drawHeaderToDisplay(const char* title);

void animateSelector(int& pos, int& target, unsigned long& lastTick);
