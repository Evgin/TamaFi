#pragma once

#include <Arduino.h>

// Единый стиль экранов label + value (Статус, Система)
// Использование: setFont(u8g2_font_6x13_t_cyrillic) перед вызовами

#define INFO_START_Y 28
#define INFO_STEP    16
#define INFO_INDENT  6

// Одна строка: label + value. Возвращает y + INFO_STEP.
int drawInfoRow(int y, const char* label, const char* value, uint16_t color = 0xFFFF, int indentX = 0);

// Заголовок секции (cyan). Возвращает y + INFO_STEP.
int drawInfoSectionHeader(int y, const char* text);

// Две колонки. Возвращает y + INFO_STEP.
int drawInfoRow2Col(int y, const char* label1, const char* val1, const char* label2, const char* val2);
