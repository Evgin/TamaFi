#include "ui_menu.h"
#include "navigation.h"
#include "pet_logic.h"
#include "sound.h"
#include "display_amoled.h"
#include <cstdio>

static const char* petSkinText(uint8_t skin) {
    switch (skin) {
        case 0: return "Gorgon";
        case 1: return "Golem";
    }
    return "?";
}

const char* uiMenuGetSettingsValue(int index) {
    static char buf[12];
    switch (index) {
        case 0: return tftBrightnessIndex==0?"Low":tftBrightnessIndex==1?"Mid":"High";
        case 1: return soundVolume==0?"Off":soundVolume==1?"1":soundVolume==2?"2":"3";
        case 2: return petSkinText(petSkin);
        case 3: return autoSleepMs==0?"Off":autoSleepMs==30000?"30s":autoSleepMs==60000?"60s":"120s";
        case 4: snprintf(buf, sizeof(buf), "%lus", (unsigned long)(autoSaveMs/1000)); return buf;
        case 5: return petGetTimeScaleLabel();
        default: return nullptr;
    }
}

void uiMenuOnScreenChange(Screen /*newScreen*/) {
    // LVGL handles menus; no canvas highlight state needed
}

void uiMenuUpdateHighlightTarget(Screen /*screen*/, int /*mainMenuIdx*/, int /*settingsIdx*/) {
    // LVGL handles menus; no canvas highlight state needed
}

// Option labels for modal picker (indices 0-5)
static const char* OPT_BRIGHTNESS[] = { "Low", "Mid", "High" };
static const char* OPT_SOUND[] = { "Off", "1", "2", "3" };
static const char* OPT_SKIN[] = { "Gorgon", "Golem" };
static const char* OPT_AUTOSLEEP[] = { "Off", "30s", "60s", "120s" };
static const char* OPT_AUTOSAVE[] = { "15s", "30s", "60s" };
static const char* OPT_TIMESCALE[] = { "x1", "x10", "x60", "x100" };
static const uint32_t AUTOSLEEP_VALS[] = { 0, 30000, 60000, 120000 };
static const uint16_t AUTOSAVE_VALS[] = { 15000, 30000, 60000 };
static const uint8_t TIMESCALE_VALS[] = { 1, 10, 60, 100 };

int uiMenuGetSettingOptionsCount(int index) {
    switch (index) {
        case 0: return 3;
        case 1: return 4;
        case 2: return 2;
        case 3: return 4;
        case 4: return 3;
        case 5: return 4;
        default: return 0;
    }
}

const char* uiMenuGetSettingOptionLabel(int index, int optionIndex) {
    switch (index) {
        case 0: return (optionIndex >= 0 && optionIndex < 3) ? OPT_BRIGHTNESS[optionIndex] : nullptr;
        case 1: return (optionIndex >= 0 && optionIndex < 4) ? OPT_SOUND[optionIndex] : nullptr;
        case 2: return (optionIndex >= 0 && optionIndex < 2) ? OPT_SKIN[optionIndex] : nullptr;
        case 3: return (optionIndex >= 0 && optionIndex < 4) ? OPT_AUTOSLEEP[optionIndex] : nullptr;
        case 4: return (optionIndex >= 0 && optionIndex < 3) ? OPT_AUTOSAVE[optionIndex] : nullptr;
        case 5: return (optionIndex >= 0 && optionIndex < 4) ? OPT_TIMESCALE[optionIndex] : nullptr;
        default: return nullptr;
    }
}

static void applyTftBrightness() {
    uint8_t val = (tftBrightnessIndex == 0) ? 60 :
                  (tftBrightnessIndex == 1) ? 150 : 255;
    setDisplayBrightness(val);
}

void uiMenuApplySetting(int index, int optionIndex) {
    switch (index) {
        case 0:
            tftBrightnessIndex = (optionIndex >= 0 && optionIndex < 3) ? (uint8_t)optionIndex : 0;
            applyTftBrightness();
            break;
        case 1:
            soundVolume = (optionIndex >= 0 && optionIndex < 4) ? (uint8_t)optionIndex : 0;
            soundSetVolume(soundVolume);
            if (soundVolume == 0) soundStopAll();
            else sndBeepOk();
            break;
        case 2:
            petSkin = (optionIndex >= 0 && optionIndex < 2) ? (uint8_t)optionIndex : 0;
            break;
        case 3:
            autoSleepMs = (optionIndex >= 0 && optionIndex < 4) ? AUTOSLEEP_VALS[optionIndex] : 0;
            break;
        case 4:
            autoSaveMs = (optionIndex >= 0 && optionIndex < 3) ? AUTOSAVE_VALS[optionIndex] : 60000;
            break;
        case 5:
            petSetTimeScale((optionIndex >= 0 && optionIndex < 4) ? TIMESCALE_VALS[optionIndex] : 1);
            break;
        default:
            break;
    }
}
