#include "lvgl_menu_common.h"
#include "device_config.h"
#include "time_service.h"
#include "battery.h"
#include "pet_logic.h"

#include <Arduino.h>

extern PetState petState;

// ============ Style helpers ============

void lvglApplyListItemStyle(lv_obj_t* obj) {
    if (!obj) return;
    lv_obj_set_style_bg_color(obj, lv_color_hex(LVGL_MENU_ITEM_GRAY), 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(LVGL_MENU_ITEM_SELECTED), LV_STATE_CHECKED);
    lv_obj_set_style_text_color(obj, lv_color_hex(LVGL_MENU_TEXT_BLACK), 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(LVGL_MENU_TEXT_BLACK), LV_STATE_CHECKED);
    // Increase button height by ~30% via vertical padding (default ~8px → 14px each side).
    lv_obj_set_style_pad_top(obj, 14, 0);
    lv_obj_set_style_pad_bottom(obj, 14, 0);
}

// ============ Mood string ============

const char* lvglMoodStr(int mood) {
    switch ((Mood)mood) {
        case MOOD_HUNGRY:  return "Hungry";
        case MOOD_HAPPY:   return "Happy";
        case MOOD_CURIOUS: return "Curious";
        case MOOD_BORED:   return "Bored";
        case MOOD_SICK:    return "Sick";
        case MOOD_EXCITED: return "Excited";
        case MOOD_CALM:    return "Calm";
    }
    return "---";
}

// ============ Header build ============

LvglHeader lvglBuildHeader(lv_obj_t* parent) {
    LvglHeader h;

    h.container = lv_obj_create(parent);
    lv_obj_set_size(h.container, LV_PCT(100), LVGL_HEADER_H);
    lv_obj_set_pos(h.container, 0, 0);
    lv_obj_set_style_bg_color(h.container, lv_color_hex(LVGL_MENU_BG_BLACK), 0);
    lv_obj_set_style_bg_opa(h.container, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(h.container, 0, 0);
    lv_obj_set_style_radius(h.container, 0, 0);
    // Bottom separator line (cyan)
    lv_obj_set_style_border_width(h.container, 1, 0);
    lv_obj_set_style_border_side(h.container, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(h.container, lv_color_hex(0x07FF), 0);
    lv_obj_clear_flag(h.container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(h.container, LV_OBJ_FLAG_CLICKABLE);

    // Mood (left) — offset from rounded screen corner
    h.moodLabel = lv_label_create(h.container);
    lv_label_set_text(h.moodLabel, "---");
    lv_obj_set_style_text_color(h.moodLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(h.moodLabel, LV_ALIGN_LEFT_MID, 30, 0);

    // Time (center)
    h.timeLabel = lv_label_create(h.container);
    lv_label_set_text(h.timeLabel, "--:--");
    lv_obj_set_style_text_color(h.timeLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(h.timeLabel, LV_ALIGN_CENTER, 0, 0);

    // Battery (right) — offset from rounded screen corner
    h.batLabel = lv_label_create(h.container);
    lv_label_set_text(h.batLabel, "---");
    lv_obj_set_style_text_color(h.batLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(h.batLabel, LV_ALIGN_RIGHT_MID, -30, 0);

    return h;
}

// ============ Header sync ============

void lvglSyncHeader(const LvglHeader& h) {
    if (!h.container) return;

    if (h.moodLabel) {
        lv_label_set_text(h.moodLabel, lvglMoodStr((int)petState.mood));
    }

    if (h.timeLabel) {
        struct tm t;
        char buf[6];
        if (timeServiceGetRealTime(&t)) {
            snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
        } else {
            snprintf(buf, sizeof(buf), "--:--");
        }
        lv_label_set_text(h.timeLabel, buf);
    }

    if (h.batLabel) {
        char buf[8];
        const BatteryInfo& bat = batteryGetInfo();
        if (bat.available && bat.batteryConnected) {
            snprintf(buf, sizeof(buf), "%d%%", bat.percent);
        } else {
            snprintf(buf, sizeof(buf), "---");
        }
        lv_label_set_text(h.batLabel, buf);
    }
}
