// display_amoled.cpp - force rebuild
#include "display_amoled.h"
#include "device_config.h"
#include "debug_icon.h"

#if UI_DEBUG_TIMING
static unsigned long lastFlushMs = 0;
#endif
#include "navigation.h"
#include "pet_logic.h"

#include <pgmspace.h>
#define U8G2_FONT_SUPPORT
// Один зонтичный заголовок библиотеки подключает databus/Arduino_ESP32QSPI.h,
// display/Arduino_SH8601.h, canvas/Arduino_Canvas.h и т.д. — явные инклюды не нужны.
#include <Arduino_GFX_Library.h>
#include <U8g2lib.h>

extern PetState petState;

static bool actionStripVisible = false;
static bool actionStripDrawn = false;        // true = strip уже нарисована, не перезаписывать контентом
static int actionStripSelectedIndex = -1;
static bool actionStripNeedsRedraw = true;

#define ACTION_STRIP_BORDER_COLOR TFT_WHITE  // visible on green game background

typedef void (*ActionStripCallback)(PetState*);

enum ActionStripIcon { ICON_MENU, ICON_FORK, ICON_CROSS };

struct ActionStripButton {
    ActionStripIcon icon;
    ActionStripCallback onSelect;
};

static void feedCallback(PetState* state) {
    if (state) petSendCommand(*state, PET_CMD_FEED);
}

static void medicineCallback(PetState* state) {
    if (state) petSendCommand(*state, PET_CMD_MEDICINE);
}

static void menuCallback(PetState* state) {
    (void)state;
    mainMenuIndex = 0;
    navSetScreen(SCREEN_MENU);
}

static ActionStripButton actionStripButtons[] = {
    { ICON_MENU, menuCallback },   // [0] = справа
    { ICON_FORK, feedCallback },
    { ICON_CROSS, medicineCallback },
};
static const int actionStripButtonCount = sizeof(actionStripButtons) / sizeof(actionStripButtons[0]);

// Scale 240x240 canvas to 368x368 and draw to output (Canvas calls draw16bitRGBBitmap on flush)
class ScalerGFX : public Arduino_GFX {
 public:
  ScalerGFX(Arduino_GFX* output)
      : Arduino_GFX(LCD_W, LCD_H), _output(output) {}

  bool begin(int32_t speed = 0) override { return _output->begin(speed); }
  void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override {
    _output->writePixelPreclipped(x, y, color);
  }

  void draw16bitRGBBitmap(int16_t x, int16_t y, uint16_t* bitmap, int16_t w, int16_t h) override {
    if (w != CONTENT_LOGICAL_W || h != CONTENT_LOGICAL_H || x != 0 || y != 0) {
      _output->draw16bitRGBBitmap(x, y, bitmap, w, h);
      return;
    }
    // Scale 240x240 -> 368x368 (nearest neighbor)
    const int BATCH = 32;
    static uint16_t batchBuf[LCD_W * BATCH];
    const int stripStartY = CONTENT_H - ACTION_STRIP_H;
    const int stripX = LCD_W - (LCD_W * 3 / 4);  // 92 — иконки справа от этого
    const int ageAreaTop = CONTENT_H - 23, ageAreaBottom = CONTENT_H - 9;

    for (int dyStart = 0; dyStart < CONTENT_H; dyStart += BATCH) {
      int dyEnd = (dyStart + BATCH < CONTENT_H) ? dyStart + BATCH : CONTENT_H;
      int batchRows = dyEnd - dyStart;

      if (actionStripVisible && actionStripDrawn && dyStart >= stripStartY) {
        // Область возраста (x < stripX) не пересекается с иконками — перерисовываем
        if (dyEnd > ageAreaTop && dyStart < ageAreaBottom) {
          const int ay0 = (dyStart > ageAreaTop) ? dyStart : ageAreaTop;
          const int ay1 = (dyEnd < ageAreaBottom) ? dyEnd : ageAreaBottom;
          const int ageRows = ay1 - ay0;
          static uint16_t ageBuf[92 * 14];  // stripX=92
          int idx = 0;
          for (int dy = ay0; dy < ay1; dy++) {
            int sy = dy * CONTENT_LOGICAL_H / CONTENT_H;
            const uint16_t* srcRow = bitmap + (size_t)sy * CONTENT_LOGICAL_W;
            for (int dx = 0; dx < stripX; dx++) {
              int sx = dx * CONTENT_LOGICAL_W / CONTENT_SCALE_NUM;
              ageBuf[idx++] = srcRow[sx];
            }
          }
          _output->draw16bitRGBBitmap(0, ay0, ageBuf, stripX, ageRows);
        }
        continue;
      }
      if (actionStripVisible && actionStripDrawn && dyEnd > stripStartY) {
        if (dyStart >= stripStartY) continue;
        dyEnd = stripStartY;
        batchRows = dyEnd - dyStart;
      }

      int writeIdx = 0;
      for (int dy = dyStart; dy < dyEnd; dy++) {
        int sy = dy * CONTENT_LOGICAL_H / CONTENT_H;
        const uint16_t* srcRow = bitmap + (size_t)sy * CONTENT_LOGICAL_W;
        for (int dx = 0; dx < LCD_W; dx++) {
          int sx = dx * CONTENT_LOGICAL_W / CONTENT_SCALE_NUM;
          batchBuf[writeIdx++] = srcRow[sx];
        }
      }
      _output->draw16bitRGBBitmap(0, dyStart, batchBuf, LCD_W, batchRows);
    }
  }

 private:
  Arduino_GFX* _output;
};

static Arduino_DataBus* bus = nullptr;
static Arduino_GFX* realGfx = nullptr;
static ScalerGFX* scalerGfx = nullptr;
static Arduino_Canvas* contentCanvas = nullptr;
static IndicatorState indicatorState = INDICATOR_OFF;
static bool sleeping = false;

// Рисуем нижнюю панель с тач-контролами. menuMode: true = UP/DOWN, false = LEFT/RIGHT.
static void drawControlBar(bool menuMode) {
  Arduino_GFX* gfx = getDisplayGfx();
  if (!gfx) return;

  const int thirdW = LCD_W / 3;
  const int centerY = CONTENT_H + CONTROL_H / 2;
  const int leftCenterX  = thirdW / 2;
  const int okCenterX    = thirdW + thirdW / 2;
  const int rightCenterX = 2 * thirdW + (LCD_W - 2 * thirdW) / 2;

  gfx->fillRect(0, CONTENT_H, LCD_W, CONTROL_H, TFT_BLACK);

  if (menuMode) {
    // UP: треугольник вверх
    gfx->fillTriangle(
      leftCenterX,        centerY - 10,
      leftCenterX - 8,    centerY + 6,
      leftCenterX + 8,    centerY + 6,
      TFT_WHITE
    );
    // DOWN: треугольник вниз
    gfx->fillTriangle(
      rightCenterX,       centerY + 10,
      rightCenterX - 8,   centerY - 6,
      rightCenterX + 8,   centerY - 6,
      TFT_WHITE
    );
  } else {
    // LEFT: треугольник влево
    gfx->fillTriangle(
      leftCenterX - 10,   centerY,
      leftCenterX + 6,    centerY - 8,
      leftCenterX + 6,    centerY + 8,
      TFT_WHITE
    );
    // RIGHT: треугольник вправо
    gfx->fillTriangle(
      rightCenterX + 10,  centerY,
      rightCenterX - 6,   centerY - 8,
      rightCenterX - 6,   centerY + 8,
      TFT_WHITE
    );
  }

  // OK: кружок (всегда по центру)
  gfx->drawCircle(okCenterX, centerY, 8, TFT_WHITE);
}

void displayControlBarSetScreen(int screen) {
  bool menuMode = (screen != SCREEN_HOME);
  drawControlBar(menuMode);
}

void displayAmoledInit() {
  bus = new Arduino_ESP32QSPI(LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
  realGfx = new Arduino_SH8601(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, LCD_W, LCD_H);
  scalerGfx = new ScalerGFX(realGfx);
  contentCanvas = new Arduino_Canvas(CONTENT_LOGICAL_W, CONTENT_LOGICAL_H, scalerGfx);

  contentCanvas->begin();
  setDisplayBrightness(150);

  drawControlBar(true);  // по умолчанию UP/DOWN (меню)
}

Arduino_GFX* getContentCanvas() { return contentCanvas; }
Arduino_GFX* getDisplayGfx() { return realGfx; }

void setDisplayBrightness(uint8_t value) {
  if (realGfx) static_cast<Arduino_SH8601*>(realGfx)->setBrightness(value);
}

void displaySleep() {
  if (!sleeping) {
    setDisplayBrightness(0);
    sleeping = true;
  }
}

void displayWake(uint8_t brightnessIndex) {
  if (sleeping) {
    uint8_t val = (brightnessIndex == 0) ? 60 :
                  (brightnessIndex == 1) ? 150 : 255;
    setDisplayBrightness(val);
    sleeping = false;
  }
}

bool displayIsAsleep() { return sleeping; }

void setIndicatorState(IndicatorState s) {
  indicatorState = s;
}

void setActionStripVisible(bool visible) {
  actionStripVisible = visible;
  if (!visible) {
    actionStripDrawn = false;  // при следующем показе — полная перерисовка
  } else {
    actionStripDrawn = false;  // при первом показе нужно нарисовать контент в области полосы
    actionStripSetSelected(0);  // по умолчанию — самая правая (индекс 0 = правый край)
    actionStripNeedsRedraw = true;
  }
  ICON_DBG_F("[icon] setActionStripVisible=%d", visible ? 1 : 0);
}

void actionStripSetSelected(int index) {
  int prev = actionStripSelectedIndex;
  if (index < 0) {
    actionStripSelectedIndex = -1;
  } else if (index >= actionStripButtonCount) {
    actionStripSelectedIndex = actionStripButtonCount - 1;
  } else {
    actionStripSelectedIndex = index;
  }
  if (prev != actionStripSelectedIndex) actionStripNeedsRedraw = true;
}

int actionStripGetSelected() {
  return actionStripSelectedIndex;
}

void actionStripMoveSelection(int delta) {
  // i=0 справа, i=n-1 слева: UP (delta=-1) = влево = +1 по индексу, DOWN (+1) = вправо = -1
  delta = -delta;
  int s = actionStripSelectedIndex;
  if (delta < 0) {
    actionStripSetSelected(s <= 0 ? actionStripButtonCount - 1 : s - 1);
  } else if (delta > 0) {
    actionStripSetSelected(s < 0 ? 0 : (s + 1) % actionStripButtonCount);
  }
}

void actionStripInvokeSelected(PetState* petState) {
  if (actionStripSelectedIndex >= 0 && actionStripSelectedIndex < actionStripButtonCount) {
    actionStripButtons[actionStripSelectedIndex].onSelect(petState);
  }
}

// Draw action strip buttons (on physical display, after content flush)
static void drawActionStripIcons() {
  if (!actionStripVisible) return;

  actionStripDrawn = true;  // полоса нарисована — при следующем flush не перезаписывать контентом
  ICON_DBG("[icon] drawActionStripIcons: drawing");

  Arduino_GFX* gfx = realGfx;
  if (!gfx) return;

  actionStripNeedsRedraw = false;

  const int stripStartY = CONTENT_H - ACTION_STRIP_H;
  const int stripW      = LCD_W * 3 / 4;
  const int stripX      = LCD_W - stripW;
  const int n           = actionStripButtonCount;

  const int iconH = 32;
  const int frameSize = 56;   // белый квадрат выбора (64 − 4×2)
  const int graySize = 48;    // серая область
  const int btnSize = frameSize;
  const int btnY    = stripStartY + (ACTION_STRIP_H - btnSize) / 2;
  const int grayInset = (frameSize - graySize) / 2;  // 2 px отступ от белого

  for (int i = 0; i < n; i++) {
    // i=0 справа, i=n-1 слева (индекс 0 = последняя в массиве = самая правая)
    int x = stripX + stripW - ACTION_STRIP_RIGHT_PADDING
          - (i + 1) * btnSize
          - i * ACTION_STRIP_BTN_GAP;

    bool selected = (i == actionStripSelectedIndex);

    // Белый и серый квадраты временно отключены
    // if (selected) { gfx->fillRect(...); gfx->fillRect(...); }

#if 1  // иконки включены
    // Icons: all Streamline (48+ in each font), 2x scale
    static const struct { const uint8_t* font; char ch; } iconCfg[] = {
      { u8g2_font_streamline_interface_essential_home_menu_t, '\x33' },
      { u8g2_font_streamline_food_drink_t,                  '\x3E' },
      { u8g2_font_streamline_health_beauty_t,               '\x44' },
    };
    gfx->setUTF8Print(false);
    gfx->setTextColor(selected ? TFT_BLACK : TFT_WHITE);
    gfx->setTextSize(2);
    const int iconBaselineY = btnY + (btnSize + iconH) / 2 + 5;
    const int iconX = x + (btnSize - iconH) / 2 - 4;
    gfx->setCursor(iconX, iconBaselineY);
    int idx = (int)actionStripButtons[i].icon;
    gfx->setFont(iconCfg[idx].font);
    gfx->print(iconCfg[idx].ch);
#endif
  }
  gfx->setFont();
  gfx->setTextSize(1);
  gfx->setUTF8Print(false);
}

void flushContentAndDrawControlBar() {
#if UI_DEBUG_TIMING
  unsigned long t0 = millis();
#endif
  contentCanvas->flush();
#if UI_DEBUG_TIMING
  lastFlushMs = millis() - t0;
#endif
  // Draw action strip only when needed (like control bar — drawn once, not every frame)
  if (actionStripNeedsRedraw) {
    ICON_DBG_F("[icon] flush: needsRedraw=1 visible=%d", actionStripVisible);
    drawActionStripIcons();
  }
  // Возраст над панелью UP/OK/DOWN, на траве (низ контента)
  if (currentScreen == SCREEN_HOME) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%3luд %2luч %2luм",
             (unsigned long)petState.pet.ageDays,
             (unsigned long)petState.pet.ageHours,
             (unsigned long)petState.pet.ageMinutes);
    Arduino_GFX* gfx = realGfx;
    if (gfx) {
      gfx->setFont(u8g2_font_6x13_t_cyrillic);
      gfx->setTextColor(TFT_BLACK);  // на траве
      gfx->setUTF8Print(true);
      gfx->setCursor(12, CONTENT_H - 10);  // над панелью, на траве
      gfx->print(buf);
      gfx->setFont();
      gfx->setUTF8Print(false);
    }
  }
}

unsigned long getLastFlushMs() {
#if UI_DEBUG_TIMING
  return lastFlushMs;
#else
  return 0;
#endif
}

void drawSpriteToContent(int x, int y, int w, int h, const uint16_t* buffer, uint16_t transparentColor) {
  Arduino_GFX* c = getContentCanvas();
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      uint16_t c16 = buffer[j * w + i];
      if (c16 != transparentColor)
        c->drawPixel(x + i, y + j, c16);
    }
  }
}

void draw16bitBitmapToContent(int x, int y, int w, int h, const uint16_t* bitmap) {
  getContentCanvas()->draw16bitRGBBitmap(x, y, (uint16_t*)bitmap, w, h);
}

void draw16bitBitmapToContentProgmem(int x, int y, int w, int h, const uint16_t* bitmap, int srcStride) {
  Arduino_GFX* c = getContentCanvas();
  const int rowBufSize = 240;
  static uint16_t rowBuf[rowBufSize];
  // stride = ширина строки в исходной картинке. Для 240x240 используем w; для 340x240 в .h указано 340.
  int stride = (srcStride > 0) ? srcStride : w;
  for (int row = 0; row < h; row++) {
    const uint16_t* rowSrc = (const uint16_t*)((const uint8_t*)bitmap + (size_t)row * stride * 2);
    for (int col = 0; col < w && col < rowBufSize; col++)
      rowBuf[col] = pgm_read_word(rowSrc + col);
    c->draw16bitRGBBitmap(x, y + row, rowBuf, w, 1);
  }
}
