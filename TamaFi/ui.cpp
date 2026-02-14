#include <Arduino.h>
#include <pgmspace.h>
#define U8G2_FONT_SUPPORT
#include "ui.h"
#include "ui_anim.h"
#include "sound.h"              // sndHatch (hatch animation)
#include "wifi_service.h"       // wifiStats, wifiList, wifiScanInProgress
#include "battery.h"            // batteryGetInfo
// Полное определение Arduino_GFX нужно для вызовов getContentCanvas()->...
#include <Arduino_GFX_Library.h>
#include <U8g2lib.h>

// Graphics headers
#include "StoneGolem.h"
#include "egg_hatch.h"
#include "effect.h"
#include "background.h"

int petPosX = 120;
int petPosY = 90;

static const int TFT_W = 240;
static const int TFT_H = 240;
static const int PET_W = 115;
static const int PET_H = 110;
static const int EFFECT_W = 100;
static const int EFFECT_H = 95;

// Hunting animation
static int huntFrame = 0;
static unsigned long lastHuntFrameTime = 0;
static const int HUNT_FRAME_DELAY = 300;

// Idle sprite sets per stage (placeholder: same for all)
static const uint16_t* BABY_IDLE_FRAMES[4]  = { idle_1, idle_2, idle_3, idle_4 };
static const uint16_t* TEEN_IDLE_FRAMES[4]  = { idle_1, idle_2, idle_3, idle_4 };
static const uint16_t* ADULT_IDLE_FRAMES[4] = { idle_1, idle_2, idle_3, idle_4 };
static const uint16_t* ELDER_IDLE_FRAMES[4] = { idle_1, idle_2, idle_3, idle_4 };

// Egg frames
static const uint16_t* EGG_FRAMES[5] = {
    egg_hatch_1, egg_hatch_2, egg_hatch_3, egg_hatch_4, egg_hatch_5
};

static const uint16_t* EGG_IDLE_FRAMES[4] = {
    egg_hatch_11, egg_hatch_21, egg_hatch_31, egg_hatch_41
};

static const uint16_t* HUNGER_FRAMES[4] = {
    hunger1, hunger2, hunger3, hunger4
};

static const uint16_t* DEAD_FRAMES[3] = {
    dead_1, dead_2, dead_3
};

// HUNTING animation loop
static const uint16_t* ATTACK_FRAMES[3] = {
    attack_0, attack_1, attack_2
};

// Sprite buffers
#define PET_BUF_SIZE   (115 * 110)
#define EFFECT_BUF_SIZE (100 * 95)
static uint16_t petBuffer[PET_BUF_SIZE];
static uint16_t effectBuffer[EFFECT_BUF_SIZE];
static void copyProgmemToPet(const uint16_t* src) {
    for (size_t i = 0; i < PET_BUF_SIZE; i++) petBuffer[i] = pgm_read_word(src + i);
}
static void copyProgmemToEffect(const uint16_t* src) {
    for (size_t i = 0; i < EFFECT_BUF_SIZE; i++) effectBuffer[i] = pgm_read_word(src + i);
}

// Local UI state
static int idleFrameUi = 0;
static unsigned long lastIdleFrameUi = 0;

static int eggIdleFrameUi = 0;
static unsigned long lastEggIdleTimeUi = 0;

static int hatchFrameUi = 0;
static unsigned long lastHatchFrameUi = 0;

static int deadFrameUi = 0;
static unsigned long lastDeadFrameUi = 0;

// Highlight animation states (init = row 0)
static int menuHighlightY        = 28;
static int menuHighlightTargetY  = 28;
static unsigned long lastMenuAnimTime = 0;

static int setHighlightY         = 28;
static int setHighlightTargetY   = 28;
static unsigned long lastSetAnim = 0;

static const int MAIN_MENU_COUNT = 4;

// Единый стиль меню (на основе настроек)
static const int MENU_BASE_Y   = 45;
static const int MENU_STEP     = 18;
static const int MENU_FONT_H   = 13;   // 6x13
static const int MENU_HL_H     = 18;   // высота подсветки

// Y координата текста строки (привязано к setCursor)
static int menuRowTextY(int rowIndex) {
    return MENU_BASE_Y + rowIndex * MENU_STEP - 4;
}

// Y подсветки — верхняя граница совпадает с верхней границей текста
static int calcHighlightY(int rowIndex) {
    return menuRowTextY(rowIndex) - MENU_FONT_H;
}

// Универсальный рендер списка меню — объявление, определение после drawHeader/animateSelector
typedef const char* (*MenuGetValueFn)(int index);
static void drawMenuList(const char* title, const char* items[], int count, int selectedIndex,
                         int& highlightY, int& highlightTargetY, unsigned long& lastAnimTime,
                         MenuGetValueFn getValue = nullptr);

static const char* moodTextLocal(Mood m) {
    switch (m) {
        case MOOD_HUNGRY:  return "Голодный";
        case MOOD_HAPPY:   return "Счастлив";
        case MOOD_CURIOUS: return "Любопытный";
        case MOOD_BORED:   return "Скучает";
        case MOOD_SICK:    return "Болен";
        case MOOD_EXCITED: return "Возбуждён";
        case MOOD_CALM:    return "Спокоен";
    }
    return "?";
}

static const char* stageTextLocal(Stage s) {
    switch (s) {
        case STAGE_BABY:  return "Малыш";
        case STAGE_TEEN:  return "Подросток";
        case STAGE_ADULT: return "Взрослый";
        case STAGE_ELDER: return "Старец";
    }
    return "?";
}

static const char* activityTextLocal(Activity a) {
    switch (a) {
        case ACT_HUNT:     return "Охота на WiFi...";
        case ACT_DISCOVER: return "Исследование...";
        case ACT_REST:     return "Отдых...";
        default:           return "";
    }
}

static void drawBatteryIndicator() {
    const BatteryInfo &bat = batteryGetInfo();
    if (!bat.available || !bat.batteryConnected) return;

    int pct = constrain(bat.percent, 0, 100);

    // Color by level
    uint16_t color;
    if (pct > 50)      color = TFT_GREEN;
    else if (pct > 20) color = TFT_YELLOW;
    else                color = TFT_RED;

    // Battery icon: body 12x7 + tip 2x3
    int bx = 202, by = 5, bw = 12, bh = 7;

    getContentCanvas()->drawRect(bx, by, bw, bh, TFT_WHITE);           // body outline
    getContentCanvas()->fillRect(bx + bw, by + 2, 2, 3, TFT_WHITE);    // tip

    int fillW = (bw - 2) * pct / 100;
    if (fillW > 0) {
        getContentCanvas()->fillRect(bx + 1, by + 1, fillW, bh - 2, color);
    }

    // Percent text left of icon
    char buf[5];
    snprintf(buf, sizeof(buf), "%d%%", pct);
    int textW = strlen(buf) * 6;   // 6px per char at default font
    getContentCanvas()->setTextColor(bat.charging ? TFT_CYAN : TFT_WHITE);
    getContentCanvas()->setCursor(bx - textW - 2, by);
    getContentCanvas()->print(buf);
}

static void drawHeader(const char* title) {
    getContentCanvas()->fillRect(0, 0, TFT_W, 18, TFT_BLACK);
    getContentCanvas()->drawLine(0, 18, TFT_W, 18, TFT_CYAN);
    getContentCanvas()->drawLine(0, 19, TFT_W, 19, TFT_MAGENTA);

    getContentCanvas()->fillRect(25, 6, 6, 6, TFT_WHITE);
    getContentCanvas()->fillRect(26, 7, 4, 4, TFT_BLACK);

    getContentCanvas()->setFont(u8g2_font_6x13_t_cyrillic);
    getContentCanvas()->setTextColor(TFT_WHITE);
    getContentCanvas()->setCursor(38, 12);  // +7px down for 6x13 font
    getContentCanvas()->print(title);
    getContentCanvas()->setFont();  // reset to default 5x7

    drawBatteryIndicator();
}

static void drawBar(int x, int y, int w, int h, int value, uint16_t color) {
    getContentCanvas()->drawRect(x, y, w, h, TFT_WHITE);
    int fillWidth = (w - 2) * value / 100;
    getContentCanvas()->fillRect(x + 1, y + 1, fillWidth, h - 2, color);
}

static void animateSelector(int &pos, int &target, unsigned long &lastTick) {
    (void)lastTick;
    pos = target;
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

static const uint16_t** currentIdleSet() {
    switch (petState.stage) {
        case STAGE_BABY:  return BABY_IDLE_FRAMES;
        case STAGE_TEEN:  return TEEN_IDLE_FRAMES;
        case STAGE_ADULT: return ADULT_IDLE_FRAMES;
        case STAGE_ELDER: return ELDER_IDLE_FRAMES;
    }
    return BABY_IDLE_FRAMES;
}

// ---------------------------------------------------------------------------
// BOOT SCREEN
// ---------------------------------------------------------------------------
static void screenBoot() {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("TamaFi v2");

    getContentCanvas()->setFont(u8g2_font_6x13_t_cyrillic);
    getContentCanvas()->setTextColor(TFT_WHITE);
    getContentCanvas()->setCursor(20, 60);
    getContentCanvas()->print("Виртуальный питомец");

    getContentCanvas()->setCursor(20, 100);
    getContentCanvas()->print("Нажми любую кнопку...");
    getContentCanvas()->setFont();

    flushContentAndDrawControlBar();
}

// ---------------------------------------------------------------------------
// HATCH SCREEN (Idle egg -> OK -> hatch -> home)
// ---------------------------------------------------------------------------
static void screenHatch() {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("Вылупление...");

    draw16bitBitmapToContentProgmem(0, 18, TFT_W, TFT_H - 18, backgroundImage2, 240);
    unsigned long now = millis();

    // 1) Idle egg animation until OK pressed
    if (!hasHatchedOnce && !hatchTriggered) {
        if (now - lastEggIdleTimeUi >= EGG_IDLE_DELAY) {
            lastEggIdleTimeUi = now;
            eggIdleFrameUi = (eggIdleFrameUi + 1) % 4;
        }

        copyProgmemToPet(EGG_IDLE_FRAMES[eggIdleFrameUi]);
        drawSpriteToContent(70, 80, PET_W, PET_H, petBuffer, TFT_WHITE);

        flushContentAndDrawControlBar();
        return;
    }

    // 2) Triggered hatch animation
    if (!hasHatchedOnce && hatchTriggered) {
        if (hatchFrameUi == 0) {
            sndHatch();
        }
        if (now - lastHatchFrameUi >= HATCH_DELAY) {
            lastHatchFrameUi = now;

            if (hatchFrameUi < 4) hatchFrameUi++;
            else {
                hasHatchedOnce = true;
                hatchTriggered = false;
                hatchFrameUi   = 0;
                currentScreen  = SCREEN_HOME;
                uiOnScreenChange(currentScreen);
                return;
            }
        }

        copyProgmemToPet(EGG_FRAMES[hatchFrameUi]);
        drawSpriteToContent(70, 80, PET_W, PET_H, petBuffer, TFT_WHITE);

        flushContentAndDrawControlBar();
        return;
    }

    // Safety fallback
    currentScreen = SCREEN_HOME;
    uiOnScreenChange(currentScreen);
}

// ---------------------------------------------------------------------------
// HOME SCREEN
// ---------------------------------------------------------------------------

static void drawStatsBlock() {
    int x = 20, y = 100, w = 80, h = 8;

    drawBar(x, y,       w, h, petState.pet.hunger,    TFT_RED);
    drawBar(x, y + 28,  w, h, petState.pet.happiness, TFT_YELLOW);
    drawBar(x, y + 56,  w, h, petState.pet.health,    TFT_GREEN);

    getContentCanvas()->setFont(u8g2_font_6x13_t_cyrillic);
    getContentCanvas()->setTextColor(TFT_BLACK);
    getContentCanvas()->setCursor(x + 3, y + 75);
    getContentCanvas()->print(moodTextLocal(petState.mood));

    getContentCanvas()->setCursor(x + 3, y + 89);
    getContentCanvas()->print(stageTextLocal(petState.stage));
    getContentCanvas()->setFont();  // reset to default
}

static void screenHome() {
    getContentCanvas()->fillScreen(TFT_BLACK);

    // ===== TOP BAR MESSAGE =====
    if (petState.activity != ACT_NONE)
        drawHeader(activityTextLocal(petState.activity));
    else
        drawHeader("Отдыхает");

    draw16bitBitmapToContentProgmem(0, 18, TFT_W, TFT_H - 18, backgroundImage, 240);

    unsigned long now = millis();

    // =============================
    //        REST ANIMATION
    // =============================
    if (petState.activity == ACT_REST && petState.restPhase != REST_NONE) {

        int frameIdx = 0;

        if (petState.restPhase == REST_ENTER) {
            frameIdx = 4 - constrain(petState.restFrameIndex, 0, 4);
        }
        else if (petState.restPhase == REST_DEEP) {
            frameIdx = 0;
        }
        else if (petState.restPhase == REST_WAKE) {
            frameIdx = constrain(petState.restFrameIndex, 0, 4);
        }

        copyProgmemToPet(EGG_FRAMES[frameIdx]);
        drawSpriteToContent(petPosX, petPosY, PET_W, PET_H, petBuffer, TFT_WHITE);

        drawStatsBlock();

        flushContentAndDrawControlBar();
        return;
    }

    // =============================
    //        HUNTING ANIMATION
    // =============================
    if (petState.activity == ACT_HUNT) {

        if (now - lastHuntFrameTime >= HUNT_FRAME_DELAY) {
            lastHuntFrameTime = now;
            huntFrame = (huntFrame + 1) % 3;
        }

        copyProgmemToPet(ATTACK_FRAMES[huntFrame]);
        drawSpriteToContent(petPosX, petPosY, PET_W, PET_H, petBuffer, TFT_WHITE);

        drawStatsBlock();

        flushContentAndDrawControlBar();
        return;
    }

    // =============================
    //        IDLE ANIMATION
    // =============================
    int idleSpeed = IDLE_BASE_DELAY;
    if (petState.mood == MOOD_EXCITED) idleSpeed = IDLE_FAST_DELAY;
    if (petState.mood == MOOD_BORED || petState.mood == MOOD_SICK) idleSpeed = IDLE_SLOW_DELAY;

    if (now - lastIdleFrameUi >= (unsigned long)idleSpeed) {
        lastIdleFrameUi = now;
        idleFrameUi = (idleFrameUi + 1) % 4;
    }

    const uint16_t** idleSet = currentIdleSet();
    copyProgmemToPet(idleSet[idleFrameUi]);
    drawSpriteToContent(petPosX, petPosY, PET_W, PET_H, petBuffer, TFT_WHITE);

    // =============================
    //         ALWAYS DRAW STATS
    // =============================
    drawStatsBlock();

    // =============================
    //     HUNGER EFFECT OVERLAY
    // =============================
    if (petState.hungerEffectActive) {
        copyProgmemToEffect(HUNGER_FRAMES[petState.hungerEffectFrame]);
        drawSpriteToContent(120, 90, EFFECT_W, EFFECT_H, effectBuffer, TFT_WHITE);
    }

    flushContentAndDrawControlBar();
}


// ---------------------------------------------------------------------------
// MAIN MENU
// ---------------------------------------------------------------------------
static void screenMenu(int mainMenuIndex) {
    const char* items[] = { "Статус", "Система", "Настройки", "Назад" };
    drawMenuList("Меню", items, MAIN_MENU_COUNT, mainMenuIndex,
                 menuHighlightY, menuHighlightTargetY, lastMenuAnimTime, nullptr);
}

// ---------------------------------------------------------------------------
// PET STATUS
// ---------------------------------------------------------------------------
static void screenPetStatus() {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("Статус питомца");

    getContentCanvas()->setFont(u8g2_font_6x13_t_cyrillic);
    getContentCanvas()->setTextColor(TFT_WHITE);

    getContentCanvas()->setCursor(10, 26);
    getContentCanvas()->print("Стадия: ");  getContentCanvas()->print(stageTextLocal(petState.stage));

    getContentCanvas()->setCursor(10, 40);
    getContentCanvas()->print("Возраст: ");
    getContentCanvas()->print(petState.pet.ageDays);    getContentCanvas()->print("д ");
    getContentCanvas()->print(petState.pet.ageHours);   getContentCanvas()->print("ч ");
    getContentCanvas()->print(petState.pet.ageMinutes); getContentCanvas()->print("м");

    getContentCanvas()->setCursor(10, 58);
    getContentCanvas()->print("Голод:    "); getContentCanvas()->print(petState.pet.hunger); getContentCanvas()->print("%");

    getContentCanvas()->setCursor(10, 72);
    getContentCanvas()->print("Счастье:  "); getContentCanvas()->print(petState.pet.happiness); getContentCanvas()->print("%");

    getContentCanvas()->setCursor(10, 86);
    getContentCanvas()->print("Здоровье: "); getContentCanvas()->print(petState.pet.health); getContentCanvas()->print("%");

    getContentCanvas()->setCursor(10, 104);
    getContentCanvas()->print("Настр.: "); getContentCanvas()->print(moodTextLocal(petState.mood));

    getContentCanvas()->setCursor(10, 122);
    getContentCanvas()->print("Характер:");

    getContentCanvas()->setCursor(16, 136);
    getContentCanvas()->print("Любопытство: "); getContentCanvas()->print((int)petState.traitCuriosity);

    getContentCanvas()->setCursor(16, 150);
    getContentCanvas()->print("Активность:  "); getContentCanvas()->print((int)petState.traitActivity);

    getContentCanvas()->setCursor(16, 164);
    getContentCanvas()->print("Стресс:      "); getContentCanvas()->print((int)petState.traitStress);
    getContentCanvas()->setFont();

    flushContentAndDrawControlBar();
}

// ---------------------------------------------------------------------------
// SYSTEM INFO
// ---------------------------------------------------------------------------
static void screenSysInfo() {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("Система");

    getContentCanvas()->setFont(u8g2_font_6x13_t_cyrillic);
    getContentCanvas()->setTextColor(TFT_WHITE);

    getContentCanvas()->setCursor(10, 30);
    getContentCanvas()->print("Прошивка: 2.0");

    getContentCanvas()->setCursor(10, 44);
    getContentCanvas()->print("MCU: ESP32-S3");

    getContentCanvas()->setCursor(10, 58);
    getContentCanvas()->print("Память: ");
    getContentCanvas()->print(ESP.getFreeHeap() / 1024); getContentCanvas()->print(" КБ");

    unsigned long s = millis() / 1000;
    unsigned long m = s / 60;
    unsigned long h = m / 60;
    s %= 60; m %= 60;

    getContentCanvas()->setCursor(10, 72);
    getContentCanvas()->print("Время: ");
    getContentCanvas()->printf("%02lu:%02lu:%02lu", h, m, s);

    getContentCanvas()->setCursor(10, 90);
    getContentCanvas()->print("WiFi: ");
    getContentCanvas()->print(wifiScanInProgress ? "Скан..." : "Ожидание");

    // --- Батарея ---
    const BatteryInfo &bat = batteryGetInfo();
    if (bat.available) {
        getContentCanvas()->setCursor(10, 112);
        getContentCanvas()->setTextColor(TFT_CYAN);
        getContentCanvas()->print("--- Батарея ---");

        getContentCanvas()->setCursor(10, 126);
        getContentCanvas()->setTextColor(TFT_WHITE);
        if (bat.batteryConnected) {
            getContentCanvas()->print("Заряд: ");
            getContentCanvas()->print(bat.percent);
            getContentCanvas()->print("% (");
            getContentCanvas()->print(bat.voltage);
            getContentCanvas()->print(" мВ)");
        } else {
            getContentCanvas()->print("Батарея: нет");
        }

        getContentCanvas()->setCursor(10, 140);
        getContentCanvas()->print("Зарядка: ");
        getContentCanvas()->print(bat.charging ? "Да" : "Нет");

        getContentCanvas()->setCursor(10, 154);
        getContentCanvas()->print("USB: ");
        getContentCanvas()->print(bat.usbConnected ? "Подключён" : "---");
    } else {
        getContentCanvas()->setCursor(10, 112);
        getContentCanvas()->setTextColor(TFT_DARKGREY);
        getContentCanvas()->print("Батарея: нет PMIC");
    }

    // --- WiFi ---
    int wifiY = bat.available ? 172 : 130;

    getContentCanvas()->setCursor(10, wifiY);
    getContentCanvas()->setTextColor(TFT_CYAN);
    getContentCanvas()->print("--- WiFi ---");

    getContentCanvas()->setTextColor(TFT_WHITE);

    getContentCanvas()->setCursor(10, wifiY + 14);
    getContentCanvas()->print("Сетей: "); getContentCanvas()->print(wifiStats.netCount);

    getContentCanvas()->setCursor(10, wifiY + 28);
    getContentCanvas()->print("Сильных: "); getContentCanvas()->print(wifiStats.strongCount);

    getContentCanvas()->setCursor(120, wifiY + 14);
    getContentCanvas()->print("Откр: "); getContentCanvas()->print(wifiStats.openCount);

    getContentCanvas()->setCursor(120, wifiY + 28);
    getContentCanvas()->print("WPA: "); getContentCanvas()->print(wifiStats.wpaCount);

    getContentCanvas()->setCursor(10, wifiY + 42);
    getContentCanvas()->print("Скрытых: "); getContentCanvas()->print(wifiStats.hiddenCount);

    getContentCanvas()->setCursor(120, wifiY + 42);
    getContentCanvas()->print("RSSI: "); getContentCanvas()->print(wifiStats.avgRSSI);

    getContentCanvas()->setFont();

    flushContentAndDrawControlBar();
}

// ---------------------------------------------------------------------------
// SETTINGS MENU
// ---------------------------------------------------------------------------
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

static void screenSettings(int settingsMenuIndex) {
    const char* labels[] = {
        "Яркость", "Звук", "Скин", "Авто сон", "Авто сохр.",
        "Сброс питомца", "Сброс всего", "Назад"
    };
    drawMenuList("Настройки", labels, 8, settingsMenuIndex,
                 setHighlightY, setHighlightTargetY, lastSetAnim, settingsGetValue);
}

// ---------------------------------------------------------------------------
// GAME OVER
// ---------------------------------------------------------------------------
static void screenGameOver() {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("Конец игры");

    unsigned long now = millis();
    if (now - lastDeadFrameUi >= DEAD_DELAY) {
        lastDeadFrameUi = now;
        deadFrameUi++;
        if (deadFrameUi > 2) deadFrameUi = 2;
    }

    draw16bitBitmapToContentProgmem(0, 18, TFT_W, TFT_H - 18, backgroundImage, 240);

    copyProgmemToPet(DEAD_FRAMES[deadFrameUi]);
    drawSpriteToContent(petPosX, petPosY, PET_W, PET_H, petBuffer, TFT_WHITE);

    flushContentAndDrawControlBar();
}

// ---------------------------------------------------------------------------
// PUBLIC UI API
// ---------------------------------------------------------------------------
void uiInit() {
    idleFrameUi = 0;
    lastIdleFrameUi = millis();

    eggIdleFrameUi = 0;
    lastEggIdleTimeUi = millis();

    hatchFrameUi = 0;
    lastHatchFrameUi = millis();

    deadFrameUi = 0;
    lastDeadFrameUi = millis();

    // Enable UTF-8 support for U8G2 fonts (cyrillic, symbols)
    getContentCanvas()->setUTF8Print(true);
}

void uiOnScreenChange(Screen newScreen) {
    if (newScreen == SCREEN_MENU) {
        menuHighlightY = menuHighlightTargetY = calcHighlightY(mainMenuIndex);
    }
    if (newScreen == SCREEN_SETTINGS) {
        setHighlightY  = setHighlightTargetY = calcHighlightY(settingsMenuIndex);
    }
    if (newScreen == SCREEN_HATCH) {
        eggIdleFrameUi = hatchFrameUi = 0;
    }

    // Action strip only on HOME screen
    setActionStripVisible(false);  // красная полоса отключена
}

void uiDrawScreen(Screen screen,
                  int mainMenuIdx,
                  int settingsIdx)
{
    if (screen == SCREEN_MENU) {
        menuHighlightTargetY = calcHighlightY(mainMenuIdx);
    }
    if (screen == SCREEN_SETTINGS) {
        setHighlightTargetY = calcHighlightY(settingsMenuIndex);
    }

    switch (screen) {
        case SCREEN_BOOT:        screenBoot(); break;
        case SCREEN_HATCH:       screenHatch(); break;
        case SCREEN_HOME:        screenHome(); break;
        case SCREEN_MENU:        screenMenu(mainMenuIdx); break;
        case SCREEN_PET_STATUS:  screenPetStatus(); break;
        case SCREEN_SYSINFO:     screenSysInfo(); break;
        case SCREEN_SETTINGS:    screenSettings(settingsIdx); break;
        case SCREEN_GAMEOVER:    screenGameOver(); break;
    }
}
