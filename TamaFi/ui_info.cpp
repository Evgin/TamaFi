#include "ui_info.h"
#include "display_amoled.h"
#include <Arduino_GFX_Library.h>

int drawInfoRow(int y, const char* label, const char* value, uint16_t color, int indentX) {
    auto* c = getContentCanvas();
    c->setCursor(10 + indentX, y);
    c->setTextColor(color);
    c->print(label);
    if (value) c->print(value);
    return y + INFO_STEP;
}

int drawInfoSectionHeader(int y, const char* text) {
    return drawInfoRow(y, text, nullptr, TFT_CYAN);
}

int drawInfoRow2Col(int y, const char* label1, const char* val1, const char* label2, const char* val2) {
    auto* c = getContentCanvas();
    c->setCursor(10, y);
    c->setTextColor(TFT_WHITE);
    c->print(label1);
    if (val1) c->print(val1);
    c->setCursor(120, y);
    c->print(label2);
    if (val2) c->print(val2);
    return y + INFO_STEP;
}
