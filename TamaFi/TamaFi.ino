// ============================================================
// TamaFi — WiFi-fed Virtual Pet
// Orchestrator: setup/loop + event mapping
// ============================================================

#include <Arduino.h>
#include "HWCDC.h"

#include "pet_logic.h"
#include "sound.h"
#include "wifi_service.h"
#include "persistence.h"
#include "navigation.h"
#include "input.h"
#include "display_amoled.h"
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

    displayAmoledInit();

    if (soundInit()) {
        DBG("[sound] ES8311 OK");
    } else {
        DBG("[sound] ES8311 fail, PWM fallback");
    }

    wifiInit();

    // Battery / PMIC
    batteryInit();
    DBG(batteryGetInfo().available ? "[battery] AXP2101 OK" : "[battery] AXP2101 not found");

    // Pet state init
    unsigned long now = millis();
    petInit(petState, now);

    // Load saved state (overwrites petInit defaults if save exists)
    persistenceInit();
    loadState(petState);

    // Navigation init
    navInit();

    // Timers
    lastLogicTick = now;
    lastSaveTime  = now;

    // UI init
    uiInit();
    uiOnScreenChange(currentScreen);
}

// ============ loop ============

void loop() {
    unsigned long now = millis();

    // 1. Sound: feed I2S buffer + advance sequencer
    for (int i = 0; i < 4 && soundFeed(); i++) {}
    sndUpdate();
    stopBuzzerIfNeeded();

    // 2. Input: poll touch/buttons
    inputPoll();
    InputButton event = inputConsumeEvent();

    // 3. AutoSleep: BOOT toggles sleep; touch ignored while asleep; idle timeout
    if (event == INPUT_BOOT) {
        if (displayIsAsleep()) {
            // Пробуждение по BOOT
            displayWake(tftBrightnessIndex);
            soundSetVolume(soundVolume);       // восстановить звук
            inputResetActivity();
        } else {
            // Принудительный сон по BOOT
            soundStopAll();
            soundSetVolume(0);
            displaySleep();
        }
        event = INPUT_NONE;   // BOOT не передаётся в навигацию
    }
    else if (displayIsAsleep()) {
        // Во сне: игнорировать все события кроме BOOT (тач не будит)
        event = INPUT_NONE;
    }
    else if (autoSleepMs > 0 && (millis() - inputLastActiveMs() >= autoSleepMs)) {
        // Таймаут бездействия -> сон
        soundStopAll();
        soundSetVolume(0);
        displaySleep();
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

    // 6. Check WiFi scan completion -> inject into pet
    if (wifiCheckScanDone()) {
        petInjectWifiResult(petState, wifiStats, now);
    }

    // 7. Process pet events -> sound / indicators
    processPetEvents();

    // 8. Battery poll (~5 s)
    if (now - lastBatteryPoll >= 5000) {
        lastBatteryPoll = now;
        batteryUpdate();
    }

    // 9. Autosave
    if (now - lastSaveTime >= autoSaveMs) {
        lastSaveTime = now;
        saveState(petState);
    }

    // 10. Draw UI (skip when display is asleep — save CPU)
    if (!displayIsAsleep()) {
        uiDrawScreen(currentScreen, mainMenuIndex, settingsMenuIndex);
    }
}
