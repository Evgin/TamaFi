/**
 * lvgl_main_menu.cpp - Main menu screen widgets.
 * 4 items: Status, System, Settings, Back.
 */

#define LV_CONF_INCLUDE_SIMPLE
#include "lvgl_main_menu.h"
#include "lvgl_menu_common.h"
#include "device_config.h"
#include "navigation.h"
#include "sound.h"

#include <lvgl.h>

static const int MAIN_MENU_COUNT = 4;

static lv_obj_t*  mainMenuScreen = nullptr;
static lv_obj_t*  mainMenuList = nullptr;
static lv_obj_t*  listButtons[MAIN_MENU_COUNT];
static LvglHeader s_header;
static bool built = false;

static const char* ITEMS[] = { "Status", "System", "Settings", "Back" };

static void mainMenuBtnClickCb(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(btn);
    if (idx < 0 || idx >= MAIN_MENU_COUNT) return;
    sndClick();
    mainMenuIndex = idx;
    navMainMenuExecute(idx);
}

void lvglMainMenuBuild() {
    if (built) return;

    mainMenuScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(mainMenuScreen, lv_color_hex(LVGL_MENU_BG_BLACK), 0);
    lv_obj_set_style_pad_all(mainMenuScreen, 0, 0);

    s_header = lvglBuildHeader(mainMenuScreen);

    mainMenuList = lv_list_create(mainMenuScreen);
    lv_obj_set_size(mainMenuList, LV_PCT(100), CONTENT_H - LVGL_HEADER_H);
    lv_obj_align(mainMenuList, LV_ALIGN_TOP_MID, 0, LVGL_HEADER_H);

    for (int i = 0; i < MAIN_MENU_COUNT; i++) {
        listButtons[i] = lv_list_add_btn(mainMenuList, nullptr, ITEMS[i]);
        if (!listButtons[i]) {
            built = false;
            return;
        }
        lv_obj_set_user_data(listButtons[i], (void*)(intptr_t)i);
        lv_obj_add_event_cb(listButtons[i], mainMenuBtnClickCb, LV_EVENT_CLICKED, nullptr);
        lvglApplyListItemStyle(listButtons[i]);
    }

    built = true;
}

void lvglMainMenuShow(int selectedIndex) {
    if (!mainMenuScreen) {
        lvglMainMenuBuild();
        if (!mainMenuScreen) return;
    }
    lv_scr_load(mainMenuScreen);
    lvglMainMenuSync(selectedIndex);
}

void lvglMainMenuSync(int selectedIndex) {
    if (!mainMenuList) return;
    lvglSyncHeader(s_header);
    for (int i = 0; i < MAIN_MENU_COUNT; i++) {
        if (i == selectedIndex) {
            lv_obj_add_state(listButtons[i], LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(listButtons[i], LV_STATE_CHECKED);
        }
    }
}
