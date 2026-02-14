#include "persistence.h"
#include "navigation.h"       // soundVolume, tftBrightnessIndex, hasHatchedOnce, petSkin
#include "sound.h"            // soundSetVolume
#include <Preferences.h>

static Preferences prefs;

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
}

void loadState(PetState &pet) {
    int h = prefs.getInt("hunger", -1);
    if (h == -1) {
        // First boot — use defaults already in petState
        hasHatchedOnce     = false;
        soundVolume        = 3;
        tftBrightnessIndex = 1;
        petSkin            = 0;
        autoSleepMs        = 60000;
        autoSaveMs         = 30000;
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

    soundVolume        = prefs.getUChar("sndVol", 3);
    tftBrightnessIndex = prefs.getUChar("tftBri", 1);
    petSkin            = prefs.getUChar("petSkin", 0);

    // Apply loaded volume level to hardware
    soundSetVolume(soundVolume);

    pet.traitCuriosity = prefs.getUChar("tCur", 70);
    pet.traitActivity  = prefs.getUChar("tAct", 60);
    pet.traitStress    = prefs.getUChar("tStr", 40);

    autoSleepMs        = prefs.getULong("sleepMs", 60000);
    uint16_t saveSec   = prefs.getUShort("saveMs", 30);
    autoSaveMs         = (uint16_t)(saveSec * 1000);
}
