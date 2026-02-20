#include <Arduino.h>
#include <pgmspace.h>
#define U8G2_FONT_SUPPORT
#include "device_config.h"
#include "ui.h"
#include "ui_common.h"
#include "ui_menu.h"
#include "ui_info.h"
#include "ui_anim.h"
#include "sound.h"              // sndHatch (hatch animation)
#include "wifi_service.h"       // wifiStats, wifiList, wifiScanInProgress
#include "battery.h"            // batteryGetInfo
#include "time_service.h"      // timeServiceGetRealTime
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

#if UI_DRAW_DEBUG
static void drawGameBackground(int x, int y, int w, int h, const uint16_t* bitmap, int stride) {
    draw16bitBitmapToContentProgmem(x, y, w, h, bitmap, stride);
}
#else
static void drawGameBackground(int x, int y, int w, int h, const uint16_t* /*bitmap*/, int /*stride*/) {
    getContentCanvas()->fillRect(x, y, w, h, 0x03E0);  // RGB565 dark green (grass)
}
#endif

// Local UI state
static int idleFrameUi = 0;
static unsigned long lastIdleFrameUi = 0;

static int eggIdleFrameUi = 0;
static unsigned long lastEggIdleTimeUi = 0;

static int hatchFrameUi = 0;
static unsigned long lastHatchFrameUi = 0;

static int deadFrameUi = 0;
static unsigned long lastDeadFrameUi = 0;

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
        case ACT_NONE:     return "Отдых";
        case ACT_HUNT:     return "Охота";
        case ACT_DISCOVER: return "Поиск";
        case ACT_REST:     return "Отдых";
        default:           return "";
    }
}

static void drawBar(int x, int y, int w, int h, int value, uint16_t color) {
    getContentCanvas()->drawRect(x, y, w, h, TFT_WHITE);
    int fillWidth = (w - 2) * value / 100;
    getContentCanvas()->fillRect(x + 1, y + 1, fillWidth, h - 2, color);
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

    drawGameBackground(0, 18, TFT_W, TFT_H - 18, backgroundImage2, 240);
    unsigned long now = millis();

    // 1) Idle egg animation until OK pressed
    if (!hasHatchedOnce && !hatchTriggered) {
        if (now - lastEggIdleTimeUi >= EGG_IDLE_DELAY) {
            lastEggIdleTimeUi = now;
            eggIdleFrameUi = (eggIdleFrameUi + 1) % 4;
        }

#if UI_DRAW_DEBUG
        copyProgmemToPet(EGG_IDLE_FRAMES[eggIdleFrameUi]);
        drawSpriteToContent(70, 80, PET_W, PET_H, petBuffer, TFT_WHITE);
#endif

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

#if UI_DRAW_DEBUG
        copyProgmemToPet(EGG_FRAMES[hatchFrameUi]);
        drawSpriteToContent(70, 80, PET_W, PET_H, petBuffer, TFT_WHITE);
#endif

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
    drawHeader(activityTextLocal(petState.activity));

    drawGameBackground(0, 18, TFT_W, TFT_H - 18, backgroundImage, 240);

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

#if UI_DRAW_DEBUG
        copyProgmemToPet(EGG_FRAMES[frameIdx]);
        drawSpriteToContent(petPosX, petPosY, PET_W, PET_H, petBuffer, TFT_WHITE);
#endif

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

#if UI_DRAW_DEBUG
        copyProgmemToPet(ATTACK_FRAMES[huntFrame]);
        drawSpriteToContent(petPosX, petPosY, PET_W, PET_H, petBuffer, TFT_WHITE);
#endif

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

#if UI_DRAW_DEBUG
    const uint16_t** idleSet = currentIdleSet();
    copyProgmemToPet(idleSet[idleFrameUi]);
    drawSpriteToContent(petPosX, petPosY, PET_W, PET_H, petBuffer, TFT_WHITE);
#endif

    // =============================
    //         ALWAYS DRAW STATS
    // =============================
    drawStatsBlock();

    // =============================
    //     HUNGER EFFECT OVERLAY
    // =============================
#if UI_DRAW_DEBUG
    if (petState.hungerEffectActive) {
        copyProgmemToEffect(HUNGER_FRAMES[petState.hungerEffectFrame]);
        drawSpriteToContent(120, 90, EFFECT_W, EFFECT_H, effectBuffer, TFT_WHITE);
    }
#endif

    flushContentAndDrawControlBar();
}


// ---------------------------------------------------------------------------
// PET STATUS
// ---------------------------------------------------------------------------
static void screenPetStatus() {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("Статус питомца");
    getContentCanvas()->setFont(u8g2_font_6x13_t_cyrillic);

    static char buf[32];
    snprintf(buf, sizeof(buf), "%dд %dч %dм",
             (int)petState.pet.ageDays, (int)petState.pet.ageHours, (int)petState.pet.ageMinutes);

    int y = INFO_START_Y;
    y = drawInfoRow(y, "Стадия: ", stageTextLocal(petState.stage));
    y = drawInfoRow(y, "Возраст: ", buf);
    snprintf(buf, sizeof(buf), "%d%%", petState.pet.hunger);
    y = drawInfoRow(y, "Голод:    ", buf);
    snprintf(buf, sizeof(buf), "%d%%", petState.pet.happiness);
    y = drawInfoRow(y, "Счастье:  ", buf);
    snprintf(buf, sizeof(buf), "%d%%", petState.pet.health);
    y = drawInfoRow(y, "Здоровье: ", buf);
    y = drawInfoRow(y, "Настр.: ", moodTextLocal(petState.mood));
    y = drawInfoRow(y, "Характер:", nullptr);
    snprintf(buf, sizeof(buf), "%d", (int)petState.traitCuriosity);
    y = drawInfoRow(y, "Любопытство: ", buf, TFT_WHITE, INFO_INDENT);
    snprintf(buf, sizeof(buf), "%d", (int)petState.traitActivity);
    y = drawInfoRow(y, "Активность:  ", buf);
    snprintf(buf, sizeof(buf), "%d", (int)petState.traitStress);
    drawInfoRow(y, "Стресс:      ", buf);

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

    static char buf[32];
    int y = INFO_START_Y;
    y = drawInfoRow(y, "Прошивка: ", "2.0");
    y = drawInfoRow(y, "MCU: ", "ESP32-S3");
    snprintf(buf, sizeof(buf), "%d КБ", ESP.getFreeHeap() / 1024);
    y = drawInfoRow(y, "Память: ", buf);
    struct tm t;
    if (timeServiceGetRealTime(&t)) {
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
    } else {
        snprintf(buf, sizeof(buf), "--:--:--");
    }
    y = drawInfoRow(y, "Время: ", buf);
    unsigned long uptimeS = millis() / 1000;
    unsigned long uh = uptimeS / 3600;
    unsigned long um = (uptimeS % 3600) / 60;
    unsigned long us = uptimeS % 60;
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", uh, um, us);
    y = drawInfoRow(y, "Uptime: ", buf);
    y = drawInfoRow(y, "WiFi: ", wifiScanInProgress ? "Скан..." : "Ожидание");

    const BatteryInfo &bat = batteryGetInfo();
    y = drawInfoSectionHeader(y, "--- Батарея ---");
    if (bat.available) {
        if (bat.batteryConnected) {
            snprintf(buf, sizeof(buf), "%d%% (%d мВ)", bat.percent, bat.voltage);
            y = drawInfoRow(y, "Заряд: ", buf);
        } else {
            y = drawInfoRow(y, "Батарея: ", "нет");
        }
        y = drawInfoRow(y, "Зарядка: ", bat.charging ? "Да" : "Нет");
        y = drawInfoRow(y, "USB: ", bat.usbConnected ? "Подключён" : "---");
    } else {
        drawInfoRow(y, "Батарея: ", "нет PMIC", TFT_DARKGREY);
        y += INFO_STEP;
    }

    y = drawInfoSectionHeader(y, "--- WiFi ---");
    static char buf2[16];
    snprintf(buf, sizeof(buf), "%d", wifiStats.netCount);
    snprintf(buf2, sizeof(buf2), "%d", wifiStats.openCount);
    y = drawInfoRow2Col(y, "Сетей: ", buf, "Откр: ", buf2);
    snprintf(buf, sizeof(buf), "%d", wifiStats.strongCount);
    snprintf(buf2, sizeof(buf2), "%d", wifiStats.wpaCount);
    y = drawInfoRow2Col(y, "Сильных: ", buf, "WPA: ", buf2);
    snprintf(buf, sizeof(buf), "%d", wifiStats.hiddenCount);
    snprintf(buf2, sizeof(buf2), "%d", wifiStats.avgRSSI);
    drawInfoRow2Col(y, "Скрытых: ", buf, "RSSI: ", buf2);

    getContentCanvas()->setFont();
    flushContentAndDrawControlBar();
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

    drawGameBackground(0, 18, TFT_W, TFT_H - 18, backgroundImage, 240);

#if UI_DRAW_DEBUG
    copyProgmemToPet(DEAD_FRAMES[deadFrameUi]);
    drawSpriteToContent(petPosX, petPosY, PET_W, PET_H, petBuffer, TFT_WHITE);
#endif

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
    uiMenuOnScreenChange(newScreen);
    if (newScreen == SCREEN_HATCH) {
        eggIdleFrameUi = hatchFrameUi = 0;
    }
    setActionStripVisible(newScreen == SCREEN_HOME);
    if (newScreen == SCREEN_HOME) {
        actionStripSetSelected(0);
    }
    displayControlBarSetScreen((int)newScreen);
}

void uiDrawScreen(Screen screen,
                  int mainMenuIdx,
                  int settingsIdx)
{
    uiMenuUpdateHighlightTarget(screen, mainMenuIdx, settingsIdx);

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
