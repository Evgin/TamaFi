#pragma once

#include <Arduino.h>

// Общие элементы UI: шапка, батарея, анимация подсветки

void drawHeader(const char* title);
void animateSelector(int& pos, int& target, unsigned long& lastTick);
