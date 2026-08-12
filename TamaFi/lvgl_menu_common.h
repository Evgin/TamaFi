#pragma once

#include <lvgl.h>

// Shared style constants for all LVGL menu screens.
// Colors: black bg, light gray items, black text.
#define LVGL_MENU_BG_BLACK        0x000000
#define LVGL_MENU_ITEM_GRAY       0xb0b0b0
#define LVGL_MENU_ITEM_SELECTED   0x909090
#define LVGL_MENU_TEXT_BLACK      0x000000

// Header bar descriptor: holds widget references for fast sync.
struct LvglHeader {
    lv_obj_t* container = nullptr;
    lv_obj_t* moodLabel = nullptr;
    lv_obj_t* timeLabel = nullptr;
    lv_obj_t* batLabel  = nullptr;
};

// Apply standard list item style (bg + text color) to an lv_list button.
void lvglApplyListItemStyle(lv_obj_t* obj);

// Build the 36px header bar as a child of parent screen.
// Returns populated LvglHeader. Call once per screen in Build().
LvglHeader lvglBuildHeader(lv_obj_t* parent);

// Update header labels with current pet mood, time, and battery.
// Call in each screen's Sync() function.
void lvglSyncHeader(const LvglHeader& h);

// Mood enum → display string. Shared between header and Pet Status screen.
const char* lvglMoodStr(int mood);
