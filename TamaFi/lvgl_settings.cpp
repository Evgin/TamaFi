/**
 * lvgl_settings.cpp - Settings screen widgets and picker/msgbox modals.
 * LVGL infrastructure (lv_init, driver, timer) lives in lvgl_core.cpp.
 */

#define LV_CONF_INCLUDE_SIMPLE
#include "lvgl_settings.h"
#include "lvgl_menu_common.h"
#include "device_config.h"
#include "ui_menu.h"
#include "navigation.h"
#include "sound.h"
#include "persistence.h"
#include "pet_logic.h"

#include <lvgl.h>

extern PetState petState;

static const int SETTINGS_COUNT = 9;
static const int PICKER_MAX_OPTS = 8;

static lv_obj_t*  settingsScreen = nullptr;
static lv_obj_t*  settingsList = nullptr;
static lv_obj_t*  listButtons[SETTINGS_COUNT];
static LvglHeader s_header;
static bool built = false;

// ============ Picker modal state ============

static lv_obj_t* s_pickerOverlay = nullptr;
static lv_obj_t* s_pickerBtns[PICKER_MAX_OPTS];
static int s_pickerOptCount = 0;
static int s_pickerSettingIdx = -1;
static int s_pickerOptIdx = 0;

// ============ Msgbox state ============

static bool s_msgboxActive = false;

// ============ Labels / row text ============

static const char* LABELS[] = {
    "Brightness", "Sound", "Skin", "Auto sleep", "Auto save",
    "Time scale", "Reset pet", "Reset all", "Back"
};

static void buildListRowText(int index, char* buf, size_t bufSize) {
    const char* val = uiMenuGetSettingsValue(index);
    if (val && val[0]) {
        snprintf(buf, bufSize, "%s    %s", LABELS[index], val);
    } else {
        snprintf(buf, bufSize, "%s", LABELS[index]);
    }
}

// ============ Picker internal ============

static void syncPickerSelection() {
    for (int i = 0; i < s_pickerOptCount; i++) {
        if (i == s_pickerOptIdx) {
            lv_obj_add_state(s_pickerBtns[i], LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(s_pickerBtns[i], LV_STATE_CHECKED);
        }
    }
}

// Close picker and apply the currently selected option.
// Safe to call from main loop (not from within LVGL event handler).
static void closePicker() {
    if (!s_pickerOverlay) return;
    uiMenuApplySetting(s_pickerSettingIdx, s_pickerOptIdx);
    lv_obj_del(s_pickerOverlay);
    s_pickerOverlay = nullptr;
    s_pickerOptCount = 0;
    s_pickerSettingIdx = -1;
    lvglSettingsSync(settingsMenuIndex);
}

// Timer-deferred close: used when closing from inside an LVGL touch callback
// (cannot lv_obj_del a parent while processing a child event).
static void pickerCloseTimerCb(lv_timer_t* timer) {
    int optIdx = (int)(intptr_t)timer->user_data;
    s_pickerOptIdx = optIdx;
    closePicker();
    lv_timer_del(timer);
}

static void pickerOptionClickCb(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    uint32_t ud = (uint32_t)(intptr_t)lv_obj_get_user_data(btn);
    int optIdx = (int)(ud & 0xFFFF);
    lv_timer_create(pickerCloseTimerCb, 10, (void*)(intptr_t)optIdx);
}

static void showPickerModal(int settingIndex) {
    if (s_pickerOverlay) return;  // already open — prevent stacking

    int n = uiMenuGetSettingOptionsCount(settingIndex);
    if (n <= 0 || n > PICKER_MAX_OPTS) return;

    s_pickerSettingIdx = settingIndex;
    s_pickerOptCount = n;

    // Pre-select current value so the user sees what is currently active.
    const char* curVal = uiMenuGetSettingsValue(settingIndex);
    s_pickerOptIdx = 0;
    for (int i = 0; i < n; i++) {
        const char* lbl = uiMenuGetSettingOptionLabel(settingIndex, i);
        if (lbl && curVal && strcmp(lbl, curVal) == 0) {
            s_pickerOptIdx = i;
            break;
        }
    }

    s_pickerOverlay = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(s_pickerOverlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_pickerOverlay, LV_OPA_80, 0);
    lv_obj_set_size(s_pickerOverlay, CONTENT_H, CONTENT_H - LVGL_HEADER_H);
    lv_obj_set_pos(s_pickerOverlay, 0, LVGL_HEADER_H);  // start below header

    lv_obj_t* list = lv_list_create(s_pickerOverlay);
    lv_obj_center(list);
    lv_obj_set_size(list, LV_PCT(80), LV_PCT(80));

    for (int i = 0; i < n; i++) {
        const char* lbl = uiMenuGetSettingOptionLabel(settingIndex, i);
        if (!lbl) continue;
        s_pickerBtns[i] = lv_list_add_btn(list, nullptr, lbl);
        uint32_t ud = ((uint32_t)settingIndex << 16) | (uint32_t)i;
        lv_obj_set_user_data(s_pickerBtns[i], (void*)(intptr_t)ud);
        lv_obj_add_event_cb(s_pickerBtns[i], pickerOptionClickCb, LV_EVENT_CLICKED, nullptr);
        lvglApplyListItemStyle(s_pickerBtns[i]);
    }

    syncPickerSelection();
}

// ============ Public modal state / input ============

bool lvglSettingsIsModalOpen() {
    return s_pickerOverlay != nullptr || s_msgboxActive;
}

void lvglSettingsModalInput(InputButton btn) {
    if (s_pickerOverlay) {
        if (btn == INPUT_UP)   { s_pickerOptIdx = (s_pickerOptIdx - 1 + s_pickerOptCount) % s_pickerOptCount; syncPickerSelection(); }
        if (btn == INPUT_DOWN) { s_pickerOptIdx = (s_pickerOptIdx + 1) % s_pickerOptCount;                    syncPickerSelection(); }
        if (btn == INPUT_OK)   { closePicker(); }
    }
    // Msgbox: physical button navigation not supported; touch Yes/No to respond.
}

// ============ Reset msgbox ============

static void msgboxCloseTimerCb(lv_timer_t* timer) {
    lv_obj_t* msgbox = (lv_obj_t*)timer->user_data;
    if (msgbox) lv_obj_del(msgbox);
    s_msgboxActive = false;
    lv_timer_del(timer);
}

static void msgboxBtnCb(lv_event_t* e) {
    lv_obj_t* msgbox = (lv_obj_t*)lv_event_get_user_data(e);
    lv_obj_t* btns_obj = lv_event_get_target(e);
    uint16_t sel = lv_btnmatrix_get_selected_btn(btns_obj);
    if (sel == 0) {  // Yes
        int action = (int)(intptr_t)lv_obj_get_user_data(msgbox);
        if (action == 6) {
            petSendCommand(petState, PET_CMD_RESET);
        } else if (action == 7) {
            petSendCommand(petState, PET_CMD_RESET_FULL);
            petFlushCommands(petState, millis());
            hasHatchedOnce = false;
            saveState(petState);
            navSetScreen(SCREEN_HATCH);
        }
    }
    lv_timer_create(msgboxCloseTimerCb, 10, msgbox);
}

static void showResetMsgbox(int index) {
    if (s_msgboxActive) return;  // prevent stacking
    s_msgboxActive = true;
    const char* title = (index == 6) ? "Reset pet?" : "Reset all?";
    static const char* btns[] = {"Yes", "No", ""};
    lv_obj_t* msgbox = lv_msgbox_create(NULL, title, "", btns, false);
    lv_obj_set_user_data(msgbox, (void*)(intptr_t)index);
    lv_obj_t* btns_obj = lv_msgbox_get_btns(msgbox);
    if (btns_obj) {
        lv_obj_add_event_cb(btns_obj, msgboxBtnCb, LV_EVENT_VALUE_CHANGED, msgbox);
    }
}

// ============ Settings screen build / show / sync ============

static void listBtnClickCb(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(btn);
    if (idx >= 0 && idx < SETTINGS_COUNT) {
        lvglSettingsHandleOk(idx);
    }
}

void lvglSettingsBuild() {
    if (built) return;

    settingsScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(settingsScreen, lv_color_hex(LVGL_MENU_BG_BLACK), 0);
    lv_obj_set_style_pad_all(settingsScreen, 0, 0);

    s_header = lvglBuildHeader(settingsScreen);

    settingsList = lv_list_create(settingsScreen);
    lv_obj_set_size(settingsList, LV_PCT(100), CONTENT_H - LVGL_HEADER_H);
    lv_obj_align(settingsList, LV_ALIGN_TOP_MID, 0, LVGL_HEADER_H);

    static char rowBuf[64];
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        buildListRowText(i, rowBuf, sizeof(rowBuf));
        listButtons[i] = lv_list_add_btn(settingsList, nullptr, rowBuf);
        if (!listButtons[i]) {
            Serial.printf("[lvgl] ERROR: lv_list_add_btn[%d] failed (OOM?)\n", i);
            built = false;
            return;
        }
        lv_obj_set_user_data(listButtons[i], (void*)(intptr_t)i);
        lv_obj_add_event_cb(listButtons[i], listBtnClickCb, LV_EVENT_CLICKED, nullptr);
        lvglApplyListItemStyle(listButtons[i]);
    }

    built = true;
    Serial.println("[lvgl] settings build OK");
}

void lvglSettingsShow(int selectedIndex) {
    if (!settingsScreen) return;
    lv_scr_load(settingsScreen);
    lvglSettingsSync(selectedIndex);
}

void lvglSettingsSync(int selectedIndex) {
    if (!settingsList) return;
    lvglSyncHeader(s_header);

    static char rowBuf[64];
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        buildListRowText(i, rowBuf, sizeof(rowBuf));
        lv_obj_t* label = lv_obj_get_child(listButtons[i], 0);
        if (label) {
            lv_label_set_text_fmt(label, "%s", rowBuf);
        }
        if (i == selectedIndex) {
            lv_obj_add_state(listButtons[i], LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(listButtons[i], LV_STATE_CHECKED);
        }
    }
}

// ============ Public OK handler ============

void lvglSettingsHandleOk(int index) {
    if (index < 0 || index > 8) return;
    if (index <= 5) {
        showPickerModal(index);
    } else if (index == 6 || index == 7) {
        showResetMsgbox(index);
    } else {
        navSetScreen(SCREEN_MENU);
    }
}
