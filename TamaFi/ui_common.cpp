#define U8G2_FONT_SUPPORT
#include "ui_common.h"
#include "display_amoled.h"
#include "battery.h"
#include "device_config.h"
#include <Arduino_GFX_Library.h>
#include <U8g2lib.h>

static const int TFT_W = CONTENT_LOGICAL_W;
static const int TFT_H = CONTENT_LOGICAL_H;

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
    c->fillRect(25, 6, 6, 6, TFT_WHITE);
    c->fillRect(26, 7, 4, 4, TFT_BLACK);

    c->setFont(u8g2_font_6x13_t_cyrillic);
    c->setTextColor(TFT_WHITE);
    c->setCursor(38, 12);
    c->print(title);
    c->setFont();

    drawBatteryIndicator();
}

void animateSelector(int& pos, int& target, unsigned long& lastTick) {
    (void)lastTick;
    pos = target;
}
