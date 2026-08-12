#pragma once

#include <Arduino.h>

// Virtual buttons (same semantics as original 6 GPIO buttons)
enum InputButton {
  INPUT_NONE = 0,
  INPUT_UP,
  INPUT_OK,
  INPUT_DOWN,
  INPUT_R1,
  INPUT_R2,
  INPUT_R3,
  INPUT_BOOT       // Hardware BOOT button (GPIO0) — sleep/wake toggle
};

// Call once from setup()
void inputInit();

// Call every loop(); then use inputConsumeEvent() to get edge events
void inputPoll();

// Returns next button event (edge: just pressed) or INPUT_NONE. One event per press.
InputButton inputConsumeEvent();

// true, если тач (FT3168) успешно инициализирован
bool inputTouchInited();

// Timestamp (millis) последнего касания/кнопки. Для детекции бездействия.
unsigned long inputLastActiveMs();

// Принудительный сброс таймера бездействия (например, при пробуждении из сна).
void inputResetActivity();

// For LVGL indev: touch state in content area (y < CONTROL_STRIP_TOP).
// Returns true if touch is active in content area; fills physical coords (0..LCD_W, 0..LCD_H).
// LVGL should use (x, y - LVGL_HEADER_H) for its coordinate system.
bool inputGetTouchState(int16_t* outX, int16_t* outY);
