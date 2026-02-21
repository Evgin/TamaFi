// ============================================================
// TamaFi — WiFi-fed Virtual Pet
// Orchestrator: setup/loop + event mapping
// ============================================================

#include <Arduino.h>
#include "HWCDC.h"

#define U8G2_FONT_SUPPORT
#include <U8g2lib.h>

#include "pet_logic.h"
#include "sound.h"
#include "wifi_service.h"
#include "time_service.h"
#include "persistence.h"
#include "navigation.h"
#include "input.h"
#include "device_config.h"
#include "display_amoled.h"
#include "device_sleep.h"
#include "ui.h"
#include "battery.h"

HWCDC USBSerial;
#define DBG(x) do { Serial.println(x); USBSerial.println(x); } while(0)

// ============ Pet state (owned by orchestrator) ============

PetState petState;

// ============ Timers ============

static unsigned long lastLogicTick    = 0;
static unsigned long lastSaveTime     = 0;
static unsigned long lastBatteryPoll  = 0;
static unsigned long lastFpsTime      = 0;
static unsigned long lastNtpSyncMs    = 0;
static bool          ntpSyncDoneOnce  = false;
static unsigned int  fpsFrameCount    = 0;
#if UI_DEBUG_TIMING
static unsigned long lastPreDrawMs    = 0;
static unsigned long lastDrawMs       = 0;
static unsigned long lastSoundMs      = 0;
static unsigned long lastInputMs      = 0;
static unsigned long lastOtherMs      = 0;
#endif

// ============ Event mapping: PetEvent -> sound / indicators ============

static void processPetEvents() {
    PetEvent evt;
    while ((evt = petPollEvent(petState)) != PET_EVT_NONE) {
        switch (evt) {
            case PET_EVT_GOOD_FEED:
                sndGoodFeed();
                setIndicatorState(INDICATOR_HAPPY);
                break;

            case PET_EVT_BAD_FEED:
                sndBadFeed();
                setIndicatorState(INDICATOR_SAD);
                break;

            case PET_EVT_DISCOVER:
                sndDiscover();
                setIndicatorState(INDICATOR_WIFI);
                break;

            case PET_EVT_EVOLUTION:
                sndDiscover();
                break;

            case PET_EVT_REST_START:
                sndRestStart();
                setIndicatorState(INDICATOR_REST);
                break;

            case PET_EVT_REST_END:
                sndRestEnd();
                setIndicatorState(INDICATOR_OFF);
                break;

            case PET_EVT_WIFI_REQUEST:
                wifiStartScan();
                setIndicatorState(INDICATOR_WIFI);
                break;

            case PET_EVT_DEATH:
                navSetScreen(SCREEN_GAMEOVER);
                setIndicatorState(INDICATOR_SAD);
                break;

            case PET_EVT_ACTIVITY_END:
                setIndicatorState(INDICATOR_OFF);
                break;

            default:
                break;
        }
    }
}

// ============ setup ============

void setup() {
    Serial.begin(115200);
    USBSerial.begin(115200);
    delay(500);
    DBG("[TamaFi] start");

    randomSeed(esp_random());

    // Hardware init
    inputInit();
    DBG(inputTouchInited() ? "[input] FT3168 OK" : "[input] FT3168 init fail");

    timeServiceInit();
    DBG(timeServiceAvailable() ? "[time] RTC OK" : "[time] RTC not found");

    displayAmoledInit();

#ifdef U8G2_WITH_UNICODE
    DBG("[icon] U8G2_WITH_UNICODE: defined");
#else
    DBG("[icon] U8G2_WITH_UNICODE: NOT defined");
#endif

    if (soundInit()) {
        DBG("[sound] ES8311 OK");
    } else {
        DBG("[sound] ES8311 fail, PWM fallback");
    }

    wifiInit();
    wifiConnect();

    // Battery / PMIC
    batteryInit();
    DBG(batteryGetInfo().available ? "[battery] AXP2101 OK" : "[battery] AXP2101 not found");

    // Pet state init
    unsigned long now = millis();
    petInit(petState, now);

    // Load saved state (overwrites petInit defaults if save exists)
    persistenceInit();
    loadState(petState);
    persistenceLogBatteryDeltaAfterWake();

    // Navigation init
    navInit();

    // Timers
    lastLogicTick = now;
    lastSaveTime  = now;

    // UI init
    uiInit();
    DBG("[icon] before uiOnScreenChange");
    uiOnScreenChange(currentScreen);
    DBG("[icon] after uiOnScreenChange");
}

// ============ loop ============

void loop() {
    unsigned long now = millis();
#if UI_DEBUG_TIMING
    unsigned long loopStart = now;
#endif

    // 1. Sound: feed I2S buffer + advance sequencer
#if UI_DEBUG_TIMING
    unsigned long t0 = millis();
#endif
    for (int i = 0; i < 2 && soundFeed(); i++) {}  // 2 вызова — меньше блокировки, i2s.write() блокирующий
    sndUpdate();
    stopBuzzerIfNeeded();
#if UI_DEBUG_TIMING
    lastSoundMs = millis() - t0;
#endif

    // 2. Input: poll touch/buttons
#if UI_DEBUG_TIMING
    t0 = millis();
#endif
    inputPoll();
#if UI_DEBUG_TIMING
    lastInputMs = millis() - t0;
#endif
    InputButton event = inputConsumeEvent();

    // 3. AutoSleep: Deep Sleep по таймауту или BOOT
    if (event == INPUT_BOOT) {
        deviceEnterSleep(petState);
        event = INPUT_NONE;
    }
    else if (autoSleepMs > 0 && (millis() - inputLastActiveMs() >= autoSleepMs)) {
        deviceEnterSleep(petState);
    }

    // 4. Navigation: handle input
    if (event != INPUT_NONE) {
        navHandleInput(event, petState);
    }

    // 5. Pet logic tick (~100 ms)
    if (now - lastLogicTick >= 100) {
        lastLogicTick = now;

        if (currentScreen != SCREEN_BOOT && currentScreen != SCREEN_HATCH) {
            bool allowAutonomous = (currentScreen == SCREEN_HOME);
            petTick(petState, now, allowAutonomous);
        }
    }

    // 6. NTP sync: at boot (first time connected) and every hour
    if (wifiConnected()) {
        if (!ntpSyncDoneOnce) {
            wifiStartNtpSync();
            ntpSyncDoneOnce = true;
            lastNtpSyncMs = now;
        } else if (now - lastNtpSyncMs >= 3600000) {  // 1 hour
            wifiStartNtpSync();
            lastNtpSyncMs = now;
        }
    }

    // 7. Check WiFi scan completion -> inject into pet
    if (wifiCheckScanDone()) {
        petInjectWifiResult(petState, wifiStats, now);
    }

    // 8. Process pet events -> sound / indicators
    processPetEvents();

    // 9. Battery poll (~5 s)
    if (now - lastBatteryPoll >= 5000) {
        lastBatteryPoll = now;
        batteryUpdate();
    }

    // 10. Autosave
    if (now - lastSaveTime >= autoSaveMs) {
        lastSaveTime = now;
        saveState(petState);
    }

    // 11. Draw UI (skip when display is asleep — save CPU)
    if (!displayIsAsleep()) {
#if UI_DEBUG_TIMING
        unsigned long tBeforeDraw = millis();
#endif
        uiDrawScreen(currentScreen, mainMenuIndex, settingsMenuIndex);
#if UI_DEBUG_TIMING
        unsigned long tAfterDraw = millis();
        lastPreDrawMs = tBeforeDraw - loopStart;
        lastDrawMs = tAfterDraw - tBeforeDraw;
        fpsFrameCount++;
        if (lastFpsTime == 0) {
            lastFpsTime = now;
        } else if (now - lastFpsTime >= 1000) {
            float fps = 1000.0f * fpsFrameCount / (now - lastFpsTime);
            float avgFrameMs = (float)(now - lastFpsTime) / fpsFrameCount;
            lastOtherMs = (lastSoundMs + lastInputMs <= lastPreDrawMs) ? (lastPreDrawMs - lastSoundMs - lastInputMs) : 0;
            DBG("[FPS] " + String(fps, 1) + " | sound=" + String(lastSoundMs) + "ms | input=" + String(lastInputMs) + "ms | other=" + String(lastOtherMs) + "ms | draw=" + String(lastDrawMs) + "ms | flush=" + String(getLastFlushMs()) + "ms | frame=" + String(avgFrameMs, 0) + "ms");
            fpsFrameCount = 0;
            lastFpsTime = now;
        }
#endif
    } else {
#if UI_DEBUG_TIMING
        if (lastFpsTime > 0 && now - lastFpsTime >= 1000) {
            lastFpsTime = 0;
            fpsFrameCount = 0;
        }
#endif
    }
}
