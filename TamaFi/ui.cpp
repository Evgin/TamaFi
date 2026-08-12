#include <Arduino.h>
#include <pgmspace.h>
#define U8G2_FONT_SUPPORT
#include "device_config.h"
#include "debug_icon.h"
#include "ui.h"
#include "ui_common.h"
#include "ui_menu.h"
#include "ui_info.h"
#include "lvgl_settings.h"
#include "lvgl_main_menu.h"
#include "lvgl_pet_status.h"
#include "lvgl_sysinfo.h"
#include "ui_anim.h"
#include "sound.h"              // sndHatch (hatch animation)
#include "wifi_service.h"       // wifiStats, wifiList, wifiScanInProgress
#include "battery.h"            // batteryGetInfo
#include "time_service.h"      // timeServiceGetRealTime
#include <Arduino_GFX_Library.h>
#include <U8g2lib.h>

// Graphics headers
#include "navigation.h"   // petSkin
#include "skin_assets.h"
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
        case MOOD_HUNGRY:  return "Hungry";
        case MOOD_HAPPY:   return "Happy";
        case MOOD_CURIOUS: return "Curious";
        case MOOD_BORED:   return "Bored";
        case MOOD_SICK:    return "Sick";
        case MOOD_EXCITED: return "Excited";
        case MOOD_CALM:    return "Calm";
    }
    return "?";
}

static const char* stageTextLocal(Stage s) {
    switch (s) {
        case STAGE_BABY:  return "Baby";
        case STAGE_TEEN:  return "Teen";
        case STAGE_ADULT: return "Adult";
        case STAGE_ELDER: return "Elder";
    }
    return "?";
}

static const char* activityTextLocal(Activity a) {
    switch (a) {
        case ACT_NONE:     return "Rest";
        case ACT_HUNT:     return "Hunt";
        case ACT_DISCOVER: return "Search";
        case ACT_REST:     return "Rest";
        default:           return "";
    }
}

static void drawBar(int x, int y, int w, int h, int value, uint16_t color) {
    getContentCanvas()->drawRect(x, y, w, h, TFT_WHITE);
    int fillWidth = (w - 2) * value / 100;
    getContentCanvas()->fillRect(x + 1, y + 1, fillWidth, h - 2, color);
}

// Вычисляет rect отрисовки питомца. applyScale=true только для Gorgon idle (спрайт-лист).
static void getPetDrawRect(int baseX, int baseY, int* outX, int* outY, int* outW, int* outH, bool applyScale) {
    if (!applyScale) {
        *outX = baseX;
        *outY = baseY;
        *outW = PET_W;
        *outH = PET_H;
        return;
    }
    int scale = skinGetPetDisplayScale(petSkin);
    if (scale <= 0) scale = 100;
    int w = PET_W * scale / 100;
    int h = PET_H * scale / 100;
    *outX = baseX - (w - PET_W) / 2;
    *outY = baseY - (h - PET_H) / 2;
    *outW = w;
    *outH = h;
}

// Draw pet frame. useLegacySize=true for attack/dead/egg (Golem assets 115x110).
static void drawPetFrame(int x, int y, int dstW, int dstH, const uint16_t* frame, bool useLegacySize) {
    int srcW, srcH;
    if (useLegacySize)
        skinGetPetFrameSizeForLegacy(petSkin, &srcW, &srcH);
    else
        skinGetPetFrameSize(petSkin, &srcW, &srcH);
    if (srcW <= 0 || srcH <= 0) {
        if (dstW == PET_W && dstH == PET_H) {
            copyProgmemToPet(frame);
            drawSpriteToContent(x, y, PET_W, PET_H, petBuffer, TFT_WHITE);
        } else {
            drawSpriteToContentScaled(x, y, dstW, dstH, frame, 115, 110, TFT_WHITE);
        }
    } else {
        drawSpriteToContentScaled(x, y, dstW, dstH, frame, srcW, srcH, TFT_WHITE);
    }
}

// Draw effect frame (e.g. hunger overlay), with scaling if skin has non-standard size.
static void drawEffectFrame(int x, int y, const uint16_t* frame) {
    int srcW, srcH;
    skinGetEffectFrameSize(petSkin, &srcW, &srcH);
    if (srcW <= 0 || srcH <= 0) {
        copyProgmemToEffect(frame);
        drawSpriteToContent(x, y, EFFECT_W, EFFECT_H, effectBuffer, TFT_WHITE);
    } else {
        drawSpriteToContentScaled(x, y, EFFECT_W, EFFECT_H, frame, srcW, srcH, TFT_WHITE);
    }
}

// ---------------------------------------------------------------------------
// BOOT SCREEN
// ---------------------------------------------------------------------------
static void screenBoot() {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("TamaFi v2");

    getContentCanvas()->setFont(u8g2_font_6x13_tf);
    getContentCanvas()->setTextColor(TFT_WHITE);
    getContentCanvas()->setCursor(20, 60);
    getContentCanvas()->print("Virtual Pet");

    getContentCanvas()->setCursor(20, 100);
    getContentCanvas()->print("Press any button...");
    getContentCanvas()->setFont();

    flushContentAndDrawControlBar();
}

// ---------------------------------------------------------------------------
// HATCH SCREEN (Idle egg -> OK -> hatch -> home)
// ---------------------------------------------------------------------------
static void screenHatch() {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("Hatching...");

    drawGameBackground(0, 18, TFT_W, TFT_H - 18, backgroundImage2, 240);
    unsigned long now = millis();

    // 1) Idle egg animation until OK pressed
    if (!hasHatchedOnce && !hatchTriggered) {
        if (now - lastEggIdleTimeUi >= EGG_IDLE_DELAY) {
            lastEggIdleTimeUi = now;
            eggIdleFrameUi = (eggIdleFrameUi + 1) % 4;
        }

#if UI_DRAW_DEBUG
        int px, py, pw, ph;
        getPetDrawRect(70, 80, &px, &py, &pw, &ph, false);
        drawPetFrame(px, py, pw, ph, skinGetEggIdleFrames(petSkin)[eggIdleFrameUi], true);
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
        int px, py, pw, ph;
        getPetDrawRect(70, 80, &px, &py, &pw, &ph, false);
        drawPetFrame(px, py, pw, ph, skinGetEggHatchFrames(petSkin)[hatchFrameUi], true);
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

    getContentCanvas()->setFont(u8g2_font_6x13_tf);
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
        int px, py, pw, ph;
        getPetDrawRect(petPosX, petPosY, &px, &py, &pw, &ph, false);
        drawPetFrame(px, py, pw, ph, skinGetEggHatchFrames(petSkin)[frameIdx], true);
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
        int px, py, pw, ph;
        getPetDrawRect(petPosX, petPosY, &px, &py, &pw, &ph, false);
        drawPetFrame(px, py, pw, ph, skinGetAttackFrames(petSkin)[huntFrame], true);
#endif

        drawStatsBlock();
        flushContentAndDrawControlBar();
        return;
    }

    // =============================
    //        IDLE ANIMATION
    // =============================
    int idleSpeed = (petSkin == SKIN_GORGON) ? IDLE_GORGON_DELAY : IDLE_BASE_DELAY;
    if (petState.mood == MOOD_EXCITED) idleSpeed = IDLE_FAST_DELAY;
    if (petState.mood == MOOD_BORED || petState.mood == MOOD_SICK) idleSpeed = IDLE_SLOW_DELAY;

    int idleFrameCount = 4;
    const uint16_t** idleSet = skinGetIdleFrames(petSkin, petState.stage, &idleFrameCount);
    if (now - lastIdleFrameUi >= (unsigned long)idleSpeed) {
        lastIdleFrameUi = now;
        idleFrameUi = (idleFrameUi + 1) % idleFrameCount;
    }

#if UI_DRAW_DEBUG
    if (skinUsesIdleSpriteSheet(petSkin)) {
        int px, py, pw, ph;
        getPetDrawRect(petPosX, petPosY, &px, &py, &pw, &ph, true);  // только Gorgon idle — масштаб 150%
        const uint16_t* sheet;
        int sheetW, sheetH, frameW, frameH;
        skinGetIdleSpriteSheet(petSkin, &sheet, &sheetW, &sheetH, &frameW, &frameH);
        drawSpriteSheetFrameToContentScaled(px, py, pw, ph,
            sheet, sheetW, sheetH, idleFrameUi, frameW, frameH, TFT_WHITE);
    } else {
        int px, py, pw, ph;
        getPetDrawRect(petPosX, petPosY, &px, &py, &pw, &ph, false);
        drawPetFrame(px, py, pw, ph, idleSet[idleFrameUi], false);
    }
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
        drawEffectFrame(120, 90, skinGetHungerFrames(petSkin)[petState.hungerEffectFrame]);
    }
#endif

    flushContentAndDrawControlBar();
}


// ---------------------------------------------------------------------------
// GAME OVER
// ---------------------------------------------------------------------------
static void screenGameOver() {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("Game Over");

    unsigned long now = millis();
    if (now - lastDeadFrameUi >= DEAD_DELAY) {
        lastDeadFrameUi = now;
        deadFrameUi++;
        if (deadFrameUi > 2) deadFrameUi = 2;
    }

    drawGameBackground(0, 18, TFT_W, TFT_H - 18, backgroundImage, 240);

#if UI_DRAW_DEBUG
    int px, py, pw, ph;
    getPetDrawRect(petPosX, petPosY, &px, &py, &pw, &ph, false);
    drawPetFrame(px, py, pw, ph, skinGetDeadFrames(petSkin)[deadFrameUi], true);
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
    static Screen s_prev = SCREEN_BOOT;
    Serial.printf("[ui] screenChange: %d -> %d\n", (int)s_prev, (int)newScreen);
    s_prev = newScreen;
    switch (newScreen) {
        case SCREEN_MENU:       lvglMainMenuShow(mainMenuIndex);    break;
        case SCREEN_PET_STATUS: lvglPetStatusShow();                break;
        case SCREEN_SYSINFO:    lvglSysInfoShow();                  break;
        case SCREEN_SETTINGS:   lvglSettingsShow(settingsMenuIndex); break;
        default: break;
    }
    uiMenuOnScreenChange(newScreen);
    if (newScreen == SCREEN_HATCH) {
        eggIdleFrameUi = hatchFrameUi = 0;
    }
    ICON_DBG_F("[icon] uiOnScreenChange HOME=%d", newScreen == SCREEN_HOME ? 1 : 0);
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
        case SCREEN_MENU:
        case SCREEN_PET_STATUS:
        case SCREEN_SYSINFO:
        case SCREEN_SETTINGS:
            // Rendered by lvglCoreTick() in loop()
            break;
        case SCREEN_GAMEOVER:    screenGameOver(); break;
    }
}
