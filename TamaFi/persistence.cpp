#include "persistence.h"
#include "navigation.h"       // soundVolume, tftBrightnessIndex, hasHatchedOnce, petSkin, currentScreen
#include "sound.h"            // soundSetVolume
#include "battery.h"          // batteryGetInfo
#include "HWCDC.h"
#include <Preferences.h>

extern HWCDC USBSerial;

static Preferences prefs;
static Screen savedScreen = SCREEN_BOOT;

void persistenceInit() {
    prefs.begin("tamafi2", false);
}

void saveState(const PetState &pet) {
    prefs.putInt("hunger",  pet.pet.hunger);
    prefs.putInt("happy",   pet.pet.happiness);
    prefs.putInt("health",  pet.pet.health);

    prefs.putULong("ageMin", pet.pet.ageMinutes);
    prefs.putULong("ageHr",  pet.pet.ageHours);
    prefs.putULong("ageDay", pet.pet.ageDays);

    prefs.putUChar("stage",  (uint8_t)pet.stage);
    prefs.putBool("hatched", hasHatchedOnce);

    prefs.putUChar("sndVol", soundVolume);
    prefs.putUChar("tftBri", tftBrightnessIndex);
    prefs.putUChar("petSkin", petSkin);

    prefs.putUChar("tCur", pet.traitCuriosity);
    prefs.putUChar("tAct", pet.traitActivity);
    prefs.putUChar("tStr", pet.traitStress);

    prefs.putULong("sleepMs", autoSleepMs);
    prefs.putUShort("saveMs", (uint16_t)(autoSaveMs / 1000));  // сохраняем в секундах
    prefs.putUChar("screen", (uint8_t)currentScreen);
    petSaveTimeScale(prefs);
}

Screen persistenceGetSavedScreen() {
    return savedScreen;
}

void loadState(PetState &pet) {
    int h = prefs.getInt("hunger", -1);
    if (h == -1) {
        // First boot — use defaults
        hasHatchedOnce     = false;
        soundVolume        = DEFAULT_SOUND_VOLUME;
        tftBrightnessIndex = DEFAULT_TFT_BRIGHTNESS_INDEX;
        petSkin            = 0;
        autoSleepMs        = DEFAULT_AUTO_SLEEP_MS;
        autoSaveMs         = DEFAULT_AUTO_SAVE_MS;
        saveState(pet);
        return;
    }

    pet.pet.hunger     = prefs.getInt("hunger", 70);
    pet.pet.happiness  = prefs.getInt("happy",  70);
    pet.pet.health     = prefs.getInt("health", 70);

    pet.pet.ageMinutes = prefs.getULong("ageMin", 0);
    pet.pet.ageHours   = prefs.getULong("ageHr",  0);
    pet.pet.ageDays    = prefs.getULong("ageDay", 0);

    pet.stage          = (Stage)prefs.getUChar("stage", (uint8_t)STAGE_BABY);
    hasHatchedOnce     = prefs.getBool("hatched", false);

    soundVolume        = prefs.getUChar("sndVol", DEFAULT_SOUND_VOLUME);
    tftBrightnessIndex = prefs.getUChar("tftBri", DEFAULT_TFT_BRIGHTNESS_INDEX);
    petSkin            = prefs.getUChar("petSkin", 0);

    // Apply loaded volume level to hardware
    soundSetVolume(soundVolume);

    pet.traitCuriosity = prefs.getUChar("tCur", 70);
    pet.traitActivity  = prefs.getUChar("tAct", 60);
    pet.traitStress    = prefs.getUChar("tStr", 40);

    autoSleepMs        = prefs.getULong("sleepMs", DEFAULT_AUTO_SLEEP_MS);
    uint16_t saveSec   = prefs.getUShort("saveMs", (uint16_t)(DEFAULT_AUTO_SAVE_MS / 1000));
    autoSaveMs         = (uint16_t)(saveSec * 1000);

    petLoadTimeScale(prefs);
    savedScreen        = (Screen)prefs.getUChar("screen", (uint8_t)SCREEN_BOOT);
}

void persistenceSaveBatteryBeforeSleep(int percent, uint16_t voltageMv) {
    prefs.putInt("batPct", percent);
    prefs.putUShort("batMv", voltageMv);
}

void persistenceLogBatteryDeltaAfterWake() {
    if (!prefs.isKey("batPct")) return;  // не было сна — нечего сравнивать
    int savedPct = prefs.getInt("batPct", -999);
    if (savedPct == -999) return;
    uint16_t savedMv = prefs.getUShort("batMv", 0);
    const BatteryInfo& cur = batteryGetInfo();
    int deltaMv = (int)cur.voltage - (int)savedMv;
    if (savedPct >= 0 && cur.percent >= 0) {
        int deltaPct = cur.percent - savedPct;
        Serial.printf("[battery] sleep: saved %d%% (%u mV) -> current %d%% (%u mV), delta %+d%% (%+d mV)\n",
                      savedPct, savedMv, cur.percent, cur.voltage, deltaPct, deltaMv);
        USBSerial.printf("[battery] sleep: saved %d%% (%u mV) -> current %d%% (%u mV), delta %+d%% (%+d mV)\n",
                        savedPct, savedMv, cur.percent, cur.voltage, deltaPct, deltaMv);
    } else {
        Serial.printf("[battery] sleep: saved %d%% (%u mV) -> current %d%% (%u mV), delta voltage %+d mV\n",
                      savedPct, savedMv, cur.percent, cur.voltage, deltaMv);
        USBSerial.printf("[battery] sleep: saved %d%% (%u mV) -> current %d%% (%u mV), delta voltage %+d mV\n",
                        savedPct, savedMv, cur.percent, cur.voltage, deltaMv);
    }
}
