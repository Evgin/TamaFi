#include <Arduino.h>
#include <pgmspace.h>
#include "ui.h"
#include "ui_anim.h"
// Полное определение Arduino_GFX нужно для вызовов getContentCanvas()->...
#include <Arduino_GFX_Library.h>

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
static const int HUNT_FRAME_DELAY = 300;   // adjust speed

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

// Sprite buffers (replacement for petSprite/effectSprite)
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

// Highlight animation states
static int menuHighlightY        = 30;
static int menuHighlightTargetY  = 30;
static unsigned long lastMenuAnimTime = 0;

static int ctlHighlightY         = 30;
static int ctlHighlightTargetY   = 30;
static unsigned long lastCtlAnim = 0;

static int setHighlightY         = 30;
static int setHighlightTargetY   = 30;
static unsigned long lastSetAnim = 0;

static const int MAIN_MENU_COUNT = 7;

// ---------------------------------------------------------------------------
// UNIVERSAL HIGHLIGHT ALIGNMENT
// ---------------------------------------------------------------------------
static int calcHighlightY(int rowIndex, int rowHeight, int topOffset) {
    return topOffset + rowIndex * rowHeight - 5;  // centered under text
}

static const char* moodTextLocal(Mood m) {
    switch (m) {
        case MOOD_HUNGRY:  return "HUNGRY";
        case MOOD_HAPPY:   return "HAPPY";
        case MOOD_CURIOUS: return "CURIOUS";
        case MOOD_BORED:   return "BORED";
        case MOOD_SICK:    return "SICK";
        case MOOD_EXCITED: return "EXCITED";
        case MOOD_CALM:    return "CALM";
    }
    return "?";
}

static const char* stageTextLocal(Stage s) {
    switch (s) {
        case STAGE_BABY:  return "BABY";
        case STAGE_TEEN:  return "TEEN";
        case STAGE_ADULT: return "ADULT";
        case STAGE_ELDER: return "ELDER";
    }
    return "?";
}

static const char* activityTextLocal(Activity a) {
    switch (a) {
        case ACT_HUNT:     return "HUNTING WIFI...";
        case ACT_DISCOVER: return "DISCOVERING...";
        case ACT_REST:     return "RESTING...";
        default:           return "";
    }
}

static void drawHeader(const char* title) {
    getContentCanvas()->fillRect(0, 0, TFT_W, 18, TFT_BLACK);
    getContentCanvas()->drawLine(0, 18, TFT_W, 18, TFT_CYAN);
    getContentCanvas()->drawLine(0, 19, TFT_W, 19, TFT_MAGENTA);

    getContentCanvas()->fillRect(5, 6, 6, 6, TFT_WHITE);
    getContentCanvas()->fillRect(6, 7, 4, 4, TFT_BLACK);

    getContentCanvas()->setTextColor(TFT_WHITE);
    getContentCanvas()->setCursor(18, 5);
    getContentCanvas()->print(title);
}

static void drawBar(int x, int y, int w, int h, int value, uint16_t color) {
    getContentCanvas()->drawRect(x, y, w, h, TFT_WHITE);
    int fillWidth = (w - 2) * value / 100;
    getContentCanvas()->fillRect(x + 1, y + 1, fillWidth, h - 2, color);
}

static void drawBubble(int x, int y, bool selected) {
    if (selected) {
        getContentCanvas()->fillCircle(x, y, 4, TFT_WHITE);
        getContentCanvas()->fillCircle(x, y, 2, TFT_BLACK);
    } else {
        getContentCanvas()->drawCircle(x, y, 4, TFT_WHITE);
    }
}

static void drawMenuIcon(int iconIndex, int x, int y) {
    switch (iconIndex) {
        case 0: getContentCanvas()->drawRect(x, y+3, 5, 4, TFT_WHITE); getContentCanvas()->fillRect(x+1,y+4,3,2,TFT_WHITE); break;
        case 1: getContentCanvas()->drawLine(x+2,y+8,x+5,y+2,TFT_WHITE); getContentCanvas()->drawLine(x+8,y+8,x+5,y+2,TFT_WHITE); getContentCanvas()->fillRect(x+4,y+8,2,3,TFT_WHITE); break;
        case 2: getContentCanvas()->drawRect(x+1,y+2,8,6,TFT_WHITE); getContentCanvas()->drawPixel(x,y+3,TFT_WHITE); getContentCanvas()->drawPixel(x+9,y+3,TFT_WHITE); break;
        case 3: getContentCanvas()->drawLine(x+1,y+3,x+9,y+3,TFT_WHITE); getContentCanvas()->fillRect(x+3,y+2,3,3,TFT_WHITE); break;
        case 4: getContentCanvas()->drawCircle(x+5,y+5,3,TFT_WHITE); break;
        case 5: getContentCanvas()->fillRect(x+4,y+2,2,2,TFT_WHITE); break;
        case 6: getContentCanvas()->drawLine(x+8,y+4,x+2,y+4,TFT_WHITE); getContentCanvas()->drawLine(x+2,y+4,x+4,y+2,TFT_WHITE); break;
    }
}

static void animateSelector(int &pos, int &target, unsigned long &lastTick) {
    // Без плавной анимации: курсор сразу прыгает к целевой позиции
    (void)lastTick;
    pos = target;
}

static const uint16_t** currentIdleSet() {
    switch (petStage) {
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

    getContentCanvas()->setTextColor(TFT_WHITE);
    getContentCanvas()->setCursor(20, 60);
    getContentCanvas()->print("WiFi-fed Virtual Pet");

    getContentCanvas()->setCursor(20, 100);
    getContentCanvas()->print("Press any button...");

    flushContentAndDrawControlBar();
}

// ---------------------------------------------------------------------------
// HATCH SCREEN (Idle egg → OK → hatch → home)
// ---------------------------------------------------------------------------
static void screenHatch() {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("Hatching...");

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

        //getContentCanvas()->setCursor(10, 200);
        //getContentCanvas()->setTextColor(TFT_WHITE);
        //getContentCanvas()->print("Press OK to hatch");

        flushContentAndDrawControlBar();
        return;
    }

    // 2) Triggered hatch animation
    if (!hasHatchedOnce && hatchTriggered) {
      if (hatchFrameUi == 0) {
        sndHatch();    // <<< PLAY RETRO HATCH SOUND HERE
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

        //getContentCanvas()->setCursor(10, 200);
        //getContentCanvas()->setTextColor(TFT_WHITE);
        //getContentCanvas()->print("Hatching...");

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

    drawBar(x, y,       w, h, pet.hunger,    TFT_RED);
    drawBar(x, y + 28,  w, h, pet.happiness, TFT_YELLOW);
    drawBar(x, y + 56,  w, h, pet.health,    TFT_GREEN);

    getContentCanvas()->setTextColor(TFT_BLACK);
    getContentCanvas()->setCursor(x + 3, y + 75);
    getContentCanvas()->print("Mood:  ");
    getContentCanvas()->print(moodTextLocal(currentMood));

    getContentCanvas()->setCursor(x + 3, y + 89);
    getContentCanvas()->print("Stage: ");
    getContentCanvas()->print(stageTextLocal(petStage));
}

static void screenHome() {
    getContentCanvas()->fillScreen(TFT_BLACK);

    // ===== TOP BAR MESSAGE =====
    if (currentActivity != ACT_NONE)
        drawHeader(activityTextLocal(currentActivity));
    else
        drawHeader("Idle");

    draw16bitBitmapToContentProgmem(0, 18, TFT_W, TFT_H - 18, backgroundImage, 240);

    unsigned long now = millis();

    // =============================
    //        REST ANIMATION
    // =============================
    if (currentActivity == ACT_REST && restPhase != REST_NONE) {

        int frameIdx = 0;

        if (restPhase == REST_ENTER) {
            // Going to sleep → 5 → 4 → 3 → 2 → 1
            frameIdx = 4 - constrain(restFrameIndex, 0, 4);
        }
        else if (restPhase == REST_DEEP) {
            // Deep sleeping → always frame 1 (egg_hatch_1)
            frameIdx = 0;
        }
        else if (restPhase == REST_WAKE) {
            // Waking up → 1 → 2 → 3 → 4 → 5
            frameIdx = constrain(restFrameIndex, 0, 4);
        }

        copyProgmemToPet(EGG_FRAMES[frameIdx]);
        drawSpriteToContent(petPosX, petPosY, PET_W, PET_H, petBuffer, TFT_WHITE);

        // --- Always draw stats ---
        drawStatsBlock();

        flushContentAndDrawControlBar();
        return;
    }

    // =============================
    //        HUNTING ANIMATION
    // =============================
    if (currentActivity == ACT_HUNT) {

        if (now - lastHuntFrameTime >= HUNT_FRAME_DELAY) {
            lastHuntFrameTime = now;
            huntFrame = (huntFrame + 1) % 3;   // attack_0 → attack_1 → attack_2
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
    if (currentMood == MOOD_EXCITED) idleSpeed = IDLE_FAST_DELAY;
    if (currentMood == MOOD_BORED || currentMood == MOOD_SICK) idleSpeed = IDLE_SLOW_DELAY;

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
    if (hungerEffectActive) {
        copyProgmemToEffect(HUNGER_FRAMES[hungerEffectFrame]);
        drawSpriteToContent(120, 90, EFFECT_W, EFFECT_H, effectBuffer, TFT_WHITE);
    }

    flushContentAndDrawControlBar();
}


// ---------------------------------------------------------------------------
// MAIN MENU
// ---------------------------------------------------------------------------
static void screenMenu(int mainMenuIndex) {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("Main Menu");

    // Размер шрифта по умолчанию
    getContentCanvas()->setTextSize(1);

    animateSelector(menuHighlightY, menuHighlightTargetY, lastMenuAnimTime);

    getContentCanvas()->fillRect(8, menuHighlightY, 224, 18, TFT_DARKGREY);
    getContentCanvas()->drawRect(8, menuHighlightY, 224, 18, TFT_CYAN);

    const char* items[] = {
        "Pet Status",
        "Environment",
        "System Info",
        "Controls",
        "Settings",
        "Diagnostics",
        "Back"
    };

    int baseY = 30;
    int step  = 20;

    for (int i = 0; i < MAIN_MENU_COUNT; i++) {
        int y = baseY + i * step;

        drawMenuIcon(i, 16, y - 2);

        getContentCanvas()->setCursor(40, y);
        getContentCanvas()->setTextColor(i == mainMenuIndex ? TFT_YELLOW : TFT_WHITE);
        getContentCanvas()->print(items[i]);
    }

    // Подсказка по управлению скрыта: нижняя панель с иконками уже поясняет зоны тача
    //getContentCanvas()->setCursor(10, 200);
    //getContentCanvas()->setTextColor(TFT_WHITE);
    //getContentCanvas()->print("UP/DOWN = move | OK = select");

    flushContentAndDrawControlBar();
}

// ---------------------------------------------------------------------------
// PET STATUS
// ---------------------------------------------------------------------------
static void screenPetStatus() {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("Pet Status");

    getContentCanvas()->setTextColor(TFT_WHITE);

    getContentCanvas()->setCursor(10, 26);
    getContentCanvas()->print("Stage: ");  getContentCanvas()->print(stageTextLocal(petStage));

    getContentCanvas()->setCursor(10,38);
    getContentCanvas()->print("Age:   ");
    getContentCanvas()->print(pet.ageDays); getContentCanvas()->print("d ");
    getContentCanvas()->print(pet.ageHours); getContentCanvas()->print("h ");
    getContentCanvas()->print(pet.ageMinutes); getContentCanvas()->print("m");


    getContentCanvas()->setCursor(10, 56);
    getContentCanvas()->print("Hunger: "); getContentCanvas()->print(pet.hunger); getContentCanvas()->print("%");

    getContentCanvas()->setCursor(10, 68);
    getContentCanvas()->print("Happy:  "); getContentCanvas()->print(pet.happiness); getContentCanvas()->print("%");

    getContentCanvas()->setCursor(10, 80);
    getContentCanvas()->print("Health: "); getContentCanvas()->print(pet.health); getContentCanvas()->print("%");

    getContentCanvas()->setCursor(10, 98);
    getContentCanvas()->print("Mood:   "); getContentCanvas()->print(moodTextLocal(currentMood));

    getContentCanvas()->setCursor(10, 116);
    getContentCanvas()->print("Personality:");

    getContentCanvas()->setCursor(16, 130);
    getContentCanvas()->print("Curiosity: "); getContentCanvas()->print((int)traitCuriosity);

    getContentCanvas()->setCursor(16, 142);
    getContentCanvas()->print("Activity : "); getContentCanvas()->print((int)traitActivity);

    getContentCanvas()->setCursor(16, 154);
    getContentCanvas()->print("Stress   : "); getContentCanvas()->print((int)traitStress);

    // Подсказка "OK = Back" скрыта
    //getContentCanvas()->setCursor(10, 200);
    //getContentCanvas()->print("OK = Back");

    flushContentAndDrawControlBar();
}

// ---------------------------------------------------------------------------
// ENVIRONMENT
// ---------------------------------------------------------------------------
static void screenEnvironment() {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("Environment");

    getContentCanvas()->setTextColor(TFT_WHITE);

    getContentCanvas()->setCursor(10, 30);
    getContentCanvas()->print("Networks : "); getContentCanvas()->print(wifiStats.netCount);

    getContentCanvas()->setCursor(10, 42);
    getContentCanvas()->print("Strong   : "); getContentCanvas()->print(wifiStats.strongCount);

    getContentCanvas()->setCursor(10, 54);
    getContentCanvas()->print("Hidden   : "); getContentCanvas()->print(wifiStats.hiddenCount);

    getContentCanvas()->setCursor(10, 66);
    getContentCanvas()->print("Open     : "); getContentCanvas()->print(wifiStats.openCount);

    getContentCanvas()->setCursor(10, 78);
    getContentCanvas()->print("WPA/etc  : "); getContentCanvas()->print(wifiStats.wpaCount);

    getContentCanvas()->setCursor(10, 94);
    getContentCanvas()->print("Avg RSSI : "); getContentCanvas()->print(wifiStats.avgRSSI);

    // Подсказка "OK = Back" скрыта
    //getContentCanvas()->setCursor(10, 200);
    //getContentCanvas()->print("OK = Back");

    flushContentAndDrawControlBar();
}

// ---------------------------------------------------------------------------
// SYSTEM INFO
// ---------------------------------------------------------------------------
static void screenSysInfo() {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("System Info");

    getContentCanvas()->setTextColor(TFT_WHITE);

    getContentCanvas()->setCursor(10, 30);
    getContentCanvas()->print("Firmware: 2.0");

    getContentCanvas()->setCursor(10, 42);
    getContentCanvas()->print("MCU:      ESP32");

    getContentCanvas()->setCursor(10, 54);
    getContentCanvas()->print("Heap Free: ");
    getContentCanvas()->print(ESP.getFreeHeap() / 1024); getContentCanvas()->print(" KB");

    unsigned long s = millis() / 1000;
    unsigned long m = s / 60;
    unsigned long h = m / 60;
    s %= 60; m %= 60;

    getContentCanvas()->setCursor(10, 72);
    getContentCanvas()->print("Uptime: ");
    getContentCanvas()->printf("%02lu:%02lu:%02lu", h, m, s);

    getContentCanvas()->setCursor(10, 90);
    getContentCanvas()->print("WiFi Scan: ");
    getContentCanvas()->print(wifiScanInProgress ? "Running" : "Idle");

    // Подсказка "OK = Back" скрыта
    //getContentCanvas()->setCursor(10, 200);
    //getContentCanvas()->print("OK = Back");

    flushContentAndDrawControlBar();
}

// ---------------------------------------------------------------------------
// CONTROLS MENU
// ---------------------------------------------------------------------------
static void screenControls(int controlsIndex) {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("Controls");

    animateSelector(ctlHighlightY, ctlHighlightTargetY, lastCtlAnim);

    getContentCanvas()->fillRect(8, ctlHighlightY, 224, 18, TFT_DARKGREY);
    getContentCanvas()->drawRect(8, ctlHighlightY, 224, 18, TFT_CYAN);

    const char* labels[] = {
        "Screen Brightness",
        "Sound",
        "Back"
    };

    getContentCanvas()->setTextColor(TFT_WHITE);
    getContentCanvas()->setTextSize(1);

    int baseY = 30;
    int step  = 20;

    for (int i = 0; i < 3; i++) {
        int y = baseY + i * step;

        drawBubble(14, y, i == controlsIndex);

        getContentCanvas()->setCursor(30, y - 4);
        getContentCanvas()->setTextColor(i == controlsIndex ? TFT_YELLOW : TFT_WHITE);
        getContentCanvas()->print(labels[i]);

        getContentCanvas()->setCursor(150, y - 4);
        getContentCanvas()->setTextColor(TFT_CYAN);

        switch (i) {
            case 0:
                getContentCanvas()->print(tftBrightnessIndex==0?"Low":tftBrightnessIndex==1?"Mid":"High");
                break;
            case 1:
                getContentCanvas()->print(soundEnabled?"On":"Off");
                break;
        }
    }

    // Подсказка "OK = Select/Back" скрыта
    //getContentCanvas()->setCursor(10, 200);
    //getContentCanvas()->print("OK = Select/Back");

    flushContentAndDrawControlBar();
}

// ---------------------------------------------------------------------------
// SETTINGS MENU
// ---------------------------------------------------------------------------
static void screenSettings(int settingsMenuIndex) {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("Settings");

    animateSelector(setHighlightY, setHighlightTargetY, lastSetAnim);

    getContentCanvas()->fillRect(8, setHighlightY, 224, 18, TFT_DARKGREY);
    getContentCanvas()->drawRect(8, setHighlightY, 224, 18, TFT_CYAN);

    const char* labels[] = {
        "Theme",
        "Auto Sleep",
        "Auto Save",
        "Reset Pet",
        "Reset All",
        "Back"
    };

    // Размер шрифта по умолчанию
    getContentCanvas()->setTextSize(1);

    int baseY = 30;
    int step  = 18;

    for (int i = 0; i < 6; i++) {
        int y = baseY + i * step;

        drawBubble(14, y, i == settingsMenuIndex);

        getContentCanvas()->setCursor(30, y - 4);
        getContentCanvas()->setTextColor(i == settingsMenuIndex ? TFT_YELLOW : TFT_WHITE);
        getContentCanvas()->print(labels[i]);

        getContentCanvas()->setCursor(150, y - 4);
        getContentCanvas()->setTextColor(TFT_CYAN);

        switch (i) {
            case 0: getContentCanvas()->print("Pixel"); break;
            case 1: getContentCanvas()->print(autoSleep?"On":"Off"); break;
            case 2: getContentCanvas()->print(autoSaveMs/1000); getContentCanvas()->print("s"); break;
        }
    }

    // Подсказка "OK = Select" скрыта
    //getContentCanvas()->setCursor(10, 200);
    //getContentCanvas()->print("OK = Select");

    flushContentAndDrawControlBar();
}

// ---------------------------------------------------------------------------
// DIAGNOSTICS
// ---------------------------------------------------------------------------
static void screenDiagnostics() {
    getContentCanvas()->fillScreen(TFT_BLACK);
    drawHeader("Diagnostics");

    getContentCanvas()->setTextColor(TFT_WHITE);

    getContentCanvas()->setCursor(10, 30);
    getContentCanvas()->print("Activity: ");
    getContentCanvas()->print(activityTextLocal(currentActivity));

    getContentCanvas()->setCursor(10, 42);
    getContentCanvas()->print("Mood: ");
    getContentCanvas()->print(moodTextLocal(currentMood));

    getContentCanvas()->setCursor(10, 54);
    getContentCanvas()->print("RestPhase: ");
    getContentCanvas()->print(restPhase==REST_ENTER?"ENTER":
             restPhase==REST_DEEP ?"DEEP":
             restPhase==REST_WAKE ?"WAKE":"NONE");

    getContentCanvas()->setCursor(10, 66);
    getContentCanvas()->print("WiFi Scan: ");
    getContentCanvas()->print(wifiScanInProgress?"Running":"Idle");

    // Подсказка "OK = Back" скрыта
    //getContentCanvas()->setCursor(10, 200);
    //getContentCanvas()->print("OK = Back");

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
        //deadFrameUi = (deadFrameUi + 1) % DEAD_FRAME_COUNT;
        deadFrameUi++;
        if (deadFrameUi > 2) deadFrameUi = 2;
    }

    draw16bitBitmapToContentProgmem(0, 18, TFT_W, TFT_H - 18, backgroundImage, 240);

    copyProgmemToPet(DEAD_FRAMES[deadFrameUi]);
    drawSpriteToContent(petPosX, petPosY, PET_W, PET_H, petBuffer, TFT_WHITE);

    //getContentCanvas()->setCursor(10, 200);
    //getContentCanvas()->setTextColor(TFT_WHITE);
    //getContentCanvas()->print("OK = Restart");

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
}

void uiOnScreenChange(Screen newScreen) {
    if (newScreen == SCREEN_MENU) {
        menuHighlightY = menuHighlightTargetY = calcHighlightY(mainMenuIndex, 20, 30);
    }
    if (newScreen == SCREEN_CONTROLS) {
        ctlHighlightY  = ctlHighlightTargetY = calcHighlightY(controlsIndex, 20, 30);
    }
    if (newScreen == SCREEN_SETTINGS) {
        setHighlightY  = setHighlightTargetY = calcHighlightY(settingsMenuIndex, 18, 30);
    }
    if (newScreen == SCREEN_HATCH) {
        eggIdleFrameUi = hatchFrameUi = 0;
    }
}

void uiDrawScreen(Screen screen,
                  int mainMenuIdx,
                  int controlsIdx,
                  int settingsIdx)
{
    if (screen == SCREEN_MENU) {
        menuHighlightTargetY = calcHighlightY(mainMenuIdx, 20, 30);
    }
    if (screen == SCREEN_CONTROLS) {
        ctlHighlightTargetY = calcHighlightY(controlsIdx, 20, 30) - 4;
    }
    if (screen == SCREEN_SETTINGS) {
        setHighlightTargetY = calcHighlightY(settingsMenuIndex, 18, 30) - 4;
    }

    switch (screen) {
        case SCREEN_BOOT:        screenBoot(); break;
        case SCREEN_HATCH:       screenHatch(); break;
        case SCREEN_HOME:        screenHome(); break;
        case SCREEN_MENU:        screenMenu(mainMenuIdx); break;
        case SCREEN_PET_STATUS:  screenPetStatus(); break;
        case SCREEN_ENVIRONMENT: screenEnvironment(); break;
        case SCREEN_SYSINFO:     screenSysInfo(); break;
        case SCREEN_CONTROLS:    screenControls(controlsIdx); break;
        case SCREEN_SETTINGS:    screenSettings(settingsIdx); break;
        case SCREEN_DIAGNOSTICS: screenDiagnostics(); break;
        case SCREEN_GAMEOVER:    screenGameOver(); break;
    }
}
