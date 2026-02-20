#include "ui_menu.h"
#include "ui_common.h"
#include "display_amoled.h"
#include "navigation.h"
#include <Arduino_GFX_Library.h>
#define U8G2_FONT_SUPPORT
#include <U8g2lib.h>

static const int MENU_BASE_Y   = 45;
static const int MENU_STEP     = 18;
static const int MENU_FONT_H   = 13;
static const int MENU_HL_H     = 18;
static const int MAIN_MENU_COUNT = 4;

static int menuHighlightY        = 28;
static int menuHighlightTargetY  = 28;
static unsigned long lastMenuAnimTime = 0;

static int setHighlightY         = 28;
static int setHighlightTargetY   = 28;
static unsigned long lastSetAnim = 0;

static int menuRowTextY(int rowIndex) {
    return MENU_BASE_Y + rowIndex * MENU_STEP - 4;
}

static int calcHighlightY(int rowIndex) {
    return menuRowTextY(rowIndex) - MENU_FONT_H;
}

typedef const char* (*MenuGetValueFn)(int index);

static const char* petSkinText(uint8_t skin) {
    switch (skin) {
        case 0: return "Golem";
        case 1: return "Dragon";
        case 2: return "Robot";
        case 3: return "Other";
    }
    return "?";
}

static const char* settingsGetValue(int index) {
    static char buf[12];
    switch (index) {
        case 0: return tftBrightnessIndex==0?"Низ":tftBrightnessIndex==1?"Сред":"Выс";
        case 1: return soundVolume==0?"Выкл":soundVolume==1?"1":soundVolume==2?"2":"3";
        case 2: return petSkinText(petSkin);
        case 3: return autoSleepMs==0?"Выкл":autoSleepMs==30000?"30с":autoSleepMs==60000?"60с":"120с";
        case 4: snprintf(buf, sizeof(buf), "%luс", (unsigned long)(autoSaveMs/1000)); return buf;
        default: return nullptr;
    }
}

static void drawMenuList(const char* title,
                         const char* items[],
                         int count,
                         int selectedIndex,
                         int& highlightY,
                         int& highlightTargetY,
                         unsigned long& lastAnimTime,
                         MenuGetValueFn getValue)
{
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader(title);
    getContentCanvas()->setTextSize(1);

    animateSelector(highlightY, highlightTargetY, lastAnimTime);
    getContentCanvas()->fillRect(8, highlightY, 224, MENU_HL_H, TFT_DARKGREY);
    getContentCanvas()->drawRect(8, highlightY, 224, MENU_HL_H, TFT_CYAN);

    getContentCanvas()->setFont(u8g2_font_6x13_t_cyrillic);
    for (int i = 0; i < count; i++) {
        int textY = menuRowTextY(i);
        bool sel = (i == selectedIndex);

        getContentCanvas()->setCursor(14, textY);
        getContentCanvas()->setTextColor(sel ? TFT_YELLOW : TFT_WHITE);
        getContentCanvas()->print("> ");

        getContentCanvas()->setCursor(30, textY);
        getContentCanvas()->setTextColor(sel ? TFT_YELLOW : TFT_WHITE);
        getContentCanvas()->print(items[i]);

        if (getValue) {
            const char* val = getValue(i);
            if (val) {
                getContentCanvas()->setCursor(150, textY);
                getContentCanvas()->setTextColor(TFT_CYAN);
                getContentCanvas()->print(val);
            }
        }
    }
    getContentCanvas()->setFont();
    flushContentAndDrawControlBar();
}

void uiMenuOnScreenChange(Screen newScreen) {
    if (newScreen == SCREEN_MENU) {
        menuHighlightY = menuHighlightTargetY = calcHighlightY(mainMenuIndex);
    }
    if (newScreen == SCREEN_SETTINGS) {
        setHighlightY  = setHighlightTargetY = calcHighlightY(settingsMenuIndex);
    }
}

void uiMenuUpdateHighlightTarget(Screen screen, int mainMenuIdx, int settingsIdx) {
    if (screen == SCREEN_MENU) {
        menuHighlightTargetY = calcHighlightY(mainMenuIdx);
    }
    if (screen == SCREEN_SETTINGS) {
        setHighlightTargetY = calcHighlightY(settingsIdx);
    }
}

void screenMenu(int mainMenuIndex) {
    const char* items[] = { "Статус", "Система", "Настройки", "Назад" };
    drawMenuList("Меню", items, MAIN_MENU_COUNT, mainMenuIndex,
                 menuHighlightY, menuHighlightTargetY, lastMenuAnimTime, nullptr);
}

void screenSettings(int settingsMenuIndex) {
    const char* labels[] = {
        "Яркость", "Звук", "Скин", "Авто сон", "Авто сохр.",
        "Сброс питомца", "Сброс всего", "Назад"
    };
    drawMenuList("Настройки", labels, 8, settingsMenuIndex,
                 setHighlightY, setHighlightTargetY, lastSetAnim, settingsGetValue);
}
