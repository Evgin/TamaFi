#include "display_amoled.h"
#include "device_config.h"

#include <pgmspace.h>
// Один зонтичный заголовок библиотеки подключает databus/Arduino_ESP32QSPI.h,
// display/Arduino_SH8601.h, canvas/Arduino_Canvas.h и т.д. — явные инклюды не нужны.
#include <Arduino_GFX_Library.h>

static bool actionStripVisible = false;

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
    static uint16_t rowBuf[LCD_W];
    const int stripStartY = CONTENT_H - ACTION_STRIP_H;   // 318
    const int stripW      = LCD_W * 3 / 4;                // 276
    const int stripX      = LCD_W - stripW;                // right-aligned
    for (int dy = 0; dy < CONTENT_H; dy++) {
      int sy = dy * CONTENT_LOGICAL_H / CONTENT_H;
      const uint16_t* srcRow = bitmap + (size_t)sy * CONTENT_LOGICAL_W;
      for (int dx = 0; dx < LCD_W; dx++) {
        int sx = dx * CONTENT_LOGICAL_W / CONTENT_SCALE_NUM;
        rowBuf[dx] = srcRow[sx];
      }
      // Overlay action strip — right-aligned, 3/4 screen width, only on HOME screen
      if (actionStripVisible && dy >= stripStartY) {
        for (int dx = stripX; dx < stripX + stripW; dx++) {
          rowBuf[dx] = TFT_RED;
        }
      }
      _output->draw16bitRGBBitmap(0, dy, rowBuf, LCD_W, 1);
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

// Один раз рисуем нижнюю панель с тач-контролами (фон + иконки UP/OK/DOWN)
static void drawControlBarStatic() {
  Arduino_GFX* gfx = getDisplayGfx();
  if (!gfx) return;

  const int thirdW = LCD_W / 3;

  // Общий чёрный фон полосы
  gfx->fillRect(0, CONTENT_H, LCD_W, CONTROL_H, TFT_BLACK);
  // Разделительная линия сверху сейчас не нужна
  //gfx->drawRect(0, CONTENT_H, LCD_W, 1, TFT_DARKGREY);

  // Координаты центров зон
  const int centerY = CONTENT_H + CONTROL_H / 2;
  const int upCenterX   = thirdW / 2;
  const int okCenterX   = thirdW + thirdW / 2;
  const int downCenterX = 2 * thirdW + (LCD_W - 2 * thirdW) / 2;

  // UP: треугольник вверх
  gfx->fillTriangle(
    upCenterX,           centerY - 10,       // верхняя вершина
    upCenterX - 8,       centerY + 6,
    upCenterX + 8,       centerY + 6,
    TFT_WHITE
  );

  // OK: кружок
  gfx->drawCircle(okCenterX, centerY, 8, TFT_WHITE);

  // DOWN: треугольник вниз
  gfx->fillTriangle(
    downCenterX,         centerY + 10,       // нижняя вершина
    downCenterX - 8,     centerY - 6,
    downCenterX + 8,     centerY - 6,
    TFT_WHITE
  );
}

void displayAmoledInit() {
  bus = new Arduino_ESP32QSPI(LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
  realGfx = new Arduino_SH8601(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, LCD_W, LCD_H);
  scalerGfx = new ScalerGFX(realGfx);
  contentCanvas = new Arduino_Canvas(CONTENT_LOGICAL_W, CONTENT_LOGICAL_H, scalerGfx);

  contentCanvas->begin();
  setDisplayBrightness(150);

  // Один раз нарисовать нижнюю панель с тач-контролами
  drawControlBarStatic();
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
}

void flushContentAndDrawControlBar() {
  contentCanvas->flush();
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
