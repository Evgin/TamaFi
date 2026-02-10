#pragma once

#include <Arduino.h>

// Forward declare so we don't need GFX includes in every consumer
class Arduino_GFX;

// Init display (QSPI + canvas). Call once from setup().
void displayAmoledInit();

// 240x240 content canvas — draw here; then call flushContentAndDrawControlBar().
Arduino_GFX* getContentCanvas();

// Real display (368x448) — for control bar.
Arduino_GFX* getDisplayGfx();
// Яркость 0–255 (обёртка над setBrightness SH8601).
void setDisplayBrightness(uint8_t value);

// Indicator state for control bar (replaces LED feedback)
enum IndicatorState { INDICATOR_OFF, INDICATOR_HAPPY, INDICATOR_SAD, INDICATOR_WIFI, INDICATOR_REST };
void setIndicatorState(IndicatorState s);

// Scale content 240x240 -> (0,0)-(368,368), then draw control bar (0,368)-(368,448).
void flushContentAndDrawControlBar();

// Draw sprite with transparency onto content canvas (replacement for pushToSprite).
void drawSpriteToContent(int x, int y, int w, int h, const uint16_t* buffer, uint16_t transparentColor);

// Draw full bitmap from RAM to content canvas (e.g. background).
void draw16bitBitmapToContent(int x, int y, int w, int h, const uint16_t* bitmap);
// Draw from PROGMEM (flash). srcStride = source row width (0 = use w).
void draw16bitBitmapToContentProgmem(int x, int y, int w, int h, const uint16_t* bitmap, int srcStride = 0);

// RGB565 color constants (same as TFT_eSPI)
#define TFT_BLACK       0x0000
#define TFT_WHITE       0xFFFF
#define TFT_RED         0xF800
#define TFT_GREEN       0x07E0
#define TFT_BLUE        0x001F
#define TFT_CYAN        0x07FF
#define TFT_MAGENTA     0xF81F
#define TFT_YELLOW      0xFFE0
#define TFT_DARKGREY    0x7BEF
