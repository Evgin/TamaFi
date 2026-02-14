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
