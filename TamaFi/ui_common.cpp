#define U8G2_FONT_SUPPORT
#include "ui_common.h"
#include "display_amoled.h"
#include "battery.h"
#include "time_service.h"
#include "device_config.h"
#include <Arduino_GFX_Library.h>
#include <U8g2lib.h>

static const int TFT_W = CONTENT_LOGICAL_W;
static const int DISP_W = 368;   // display content width
static const int HEADER_H = 18;
static const int TFT_H = CONTENT_LOGICAL_H;

static void drawTimeInHeader() {
    struct tm t;
    char buf[8];
    if (timeServiceGetRealTime(&t)) {
        snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
    } else {
        snprintf(buf, sizeof(buf), "--:--");
    }
    auto* c = getContentCanvas();
    c->setTextColor(TFT_WHITE);
    int textW = strlen(buf) * 6;  // 6px per char
    int centerX = (TFT_W - textW) / 2;
    c->setCursor(centerX, 5);
    c->print(buf);
}

static void drawBatteryIndicator() {
    const BatteryInfo& bat = batteryGetInfo();
    if (!bat.available || !bat.batteryConnected) return;

    int pct = constrain(bat.percent, 0, 100);
    uint16_t color;
    if (pct > 50)      color = TFT_GREEN;
    else if (pct > 20) color = TFT_YELLOW;
    else               color = TFT_RED;

    int bx = 202, by = 5, bw = 12, bh = 7;
    auto* c = getContentCanvas();
    c->drawRect(bx, by, bw, bh, TFT_WHITE);
    c->fillRect(bx + bw, by + 2, 2, 3, TFT_WHITE);
    int fillW = (bw - 2) * pct / 100;
    if (fillW > 0) c->fillRect(bx + 1, by + 1, fillW, bh - 2, color);

    char buf[5];
    snprintf(buf, sizeof(buf), "%d%%", pct);
    int textW = strlen(buf) * 6;
    c->setTextColor(bat.charging ? TFT_CYAN : TFT_WHITE);
    c->setCursor(bx - textW - 2, by);
    c->print(buf);
}

void drawHeader(const char* title) {
    auto* c = getContentCanvas();
    c->fillRect(0, 0, TFT_W, 18, TFT_BLACK);
    c->drawLine(0, 18, TFT_W, 18, TFT_CYAN);
    c->drawLine(0, 19, TFT_W, 19, TFT_MAGENTA);

    c->setFont(u8g2_font_6x13_tf);
    c->setTextColor(TFT_WHITE);
    c->setCursor(40, 12);
    c->print(title);
    c->setFont();

    drawTimeInHeader();
    drawBatteryIndicator();
}

static void drawTimeInHeaderToDisplay(Arduino_GFX* gfx) {
    struct tm t;
    char buf[8];
    if (timeServiceGetRealTime(&t)) {
        snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
    } else {
        snprintf(buf, sizeof(buf), "--:--");
    }
    gfx->setTextColor(TFT_WHITE);
    int textW = strlen(buf) * 6;
    int centerX = (DISP_W - textW) / 2;
    gfx->setCursor(centerX, 5);
    gfx->print(buf);
}

static void drawBatteryIndicatorToDisplay(Arduino_GFX* gfx) {
    const BatteryInfo& bat = batteryGetInfo();
    if (!bat.available || !bat.batteryConnected) return;

    int pct = constrain(bat.percent, 0, 100);
    uint16_t color;
    if (pct > 50)      color = TFT_GREEN;
    else if (pct > 20) color = TFT_YELLOW;
    else               color = TFT_RED;

    int bx = 309, by = 5, bw = 12, bh = 7;  // scaled for 368px
    gfx->drawRect(bx, by, bw, bh, TFT_WHITE);
    gfx->fillRect(bx + bw, by + 2, 2, 3, TFT_WHITE);
    int fillW = (bw - 2) * pct / 100;
    if (fillW > 0) gfx->fillRect(bx + 1, by + 1, fillW, bh - 2, color);

    char buf[5];
    snprintf(buf, sizeof(buf), "%d%%", pct);
    int textW = strlen(buf) * 6;
    gfx->setTextColor(bat.charging ? TFT_CYAN : TFT_WHITE);
    gfx->setCursor(bx - textW - 2, by);
    gfx->print(buf);
}

void drawHeaderToDisplay(const char* title) {
    Arduino_GFX* gfx = getDisplayGfx();
    if (!gfx) return;

    gfx->fillRect(0, 0, DISP_W, HEADER_H, TFT_BLACK);
    gfx->drawLine(0, HEADER_H, DISP_W, HEADER_H, TFT_CYAN);

    gfx->setFont(u8g2_font_6x13_tf);
    gfx->setTextColor(TFT_WHITE);
    gfx->setCursor(61, 12);  // 40*368/240
    gfx->print(title);
    gfx->setFont();

    drawTimeInHeaderToDisplay(gfx);
    drawBatteryIndicatorToDisplay(gfx);
}

void animateSelector(int& pos, int& target, unsigned long& lastTick) {
    (void)lastTick;
    pos = target;
}
