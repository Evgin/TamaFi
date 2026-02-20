#include "display_amoled.h"
#include "device_config.h"

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

static bool actionStripVisible = false;
static int actionStripSelectedIndex = -1;
static bool actionStripNeedsRedraw = true;   // true = draw strip area; false = skip (preserve)

#define ACTION_STRIP_BORDER_COLOR TFT_WHITE  // visible on green game background

typedef void (*ActionStripCallback)(PetState*);

struct ActionStripButton {
    char label;
    ActionStripCallback onSelect;
};

static void feedCallback(PetState* state) {
    if (state) petSendCommand(*state, PET_CMD_FEED);
}

static void medicineCallback(PetState* state) {
    if (state) petSendCommand(*state, PET_CMD_MEDICINE);
}

static void statusCallback(PetState* state) {
    (void)state;
    navPushScreen(SCREEN_PET_STATUS);
}

static ActionStripButton actionStripButtons[] = {
    { 'S', statusCallback },
    { 'F', feedCallback },
    { 'M', medicineCallback },
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
    // Batch rows to reduce QSPI transaction count (368/32 ≈ 12 транзакций вместо ~23)
    const int BATCH = 32;
    static uint16_t batchBuf[LCD_W * BATCH];
    const int stripStartY = CONTENT_H - ACTION_STRIP_H;

    for (int dyStart = 0; dyStart < CONTENT_H; dyStart += BATCH) {
      int dyEnd = (dyStart + BATCH < CONTENT_H) ? dyStart + BATCH : CONTENT_H;
      int batchRows = dyEnd - dyStart;

      // Skip strip area when visible and no redraw needed
      if (actionStripVisible && !actionStripNeedsRedraw && dyStart >= stripStartY) {
        continue;
      }
      if (actionStripVisible && !actionStripNeedsRedraw && dyEnd > stripStartY) {
        // Partial batch: only draw rows before stripStartY
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
  if (visible) actionStripNeedsRedraw = true;
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

  Arduino_GFX* gfx = realGfx;
  if (!gfx) return;

  actionStripNeedsRedraw = false;

  const int stripStartY = CONTENT_H - ACTION_STRIP_H;
  const int stripW      = LCD_W * 3 / 4;
  const int stripX      = LCD_W - stripW;
  const int n           = actionStripButtonCount;
  const int btnY        = stripStartY + (ACTION_STRIP_H - ACTION_STRIP_BTN_SIZE) / 2;

  gfx->setUTF8Print(true);
  gfx->setTextColor(TFT_WHITE);

  for (int i = 0; i < n; i++) {
    int x = stripX + stripW - ACTION_STRIP_RIGHT_PADDING
          - (n - i) * ACTION_STRIP_BTN_SIZE
          - (n - 1 - i) * ACTION_STRIP_BTN_GAP;

    bool selected = (i == actionStripSelectedIndex);

    // Frame: larger square first (when selected), then button on top
    const int T = 5;  // frame thickness
    if (selected) {
      gfx->fillRect(x - T, btnY - T, ACTION_STRIP_BTN_SIZE + 2 * T, ACTION_STRIP_BTN_SIZE + 2 * T, ACTION_STRIP_BORDER_COLOR);
    }
    gfx->fillRect(x, btnY, ACTION_STRIP_BTN_SIZE, ACTION_STRIP_BTN_SIZE, TFT_DARKGREY);

    // Label: u8g2_font_10x20_tf (~10x20 px)
    gfx->setFont(u8g2_font_10x20_tf);
    const int charW = 10, charH = 20;
    int cx = x + (ACTION_STRIP_BTN_SIZE - charW) / 2;
    int cy = btnY + (ACTION_STRIP_BTN_SIZE - charH) / 2 + charH - 1;  // baseline
    gfx->setCursor(cx, cy);
    gfx->print(actionStripButtons[i].label);
  }

  gfx->setFont();
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
    drawActionStripIcons();
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
