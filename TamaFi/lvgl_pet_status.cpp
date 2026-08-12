/**
 * lvgl_pet_status.cpp - Pet Status screen widgets.
 * Read-only text rows with scroll.
 */

#define LV_CONF_INCLUDE_SIMPLE
#include "lvgl_pet_status.h"
#include "lvgl_menu_common.h"
#include "device_config.h"
#include "navigation.h"
#include "pet_logic.h"

#include <lvgl.h>

extern PetState petState;

static const int PET_STATUS_ROWS = 9;

static lv_obj_t*  screen = nullptr;
static lv_obj_t*  list = nullptr;
static lv_obj_t*  rowLabels[PET_STATUS_ROWS];
static LvglHeader s_header;
static bool built = false;

static const char* stageStr(Stage s) {
    switch (s) {
        case STAGE_BABY:  return "Baby";
        case STAGE_TEEN:  return "Teen";
        case STAGE_ADULT: return "Adult";
        case STAGE_ELDER: return "Elder";
    }
    return "?";
}

static void buildRows(char buf[PET_STATUS_ROWS][64]) {
    snprintf(buf[0], 64, "Stage: %s", stageStr(petState.stage));
    snprintf(buf[1], 64, "Age: %dd %dh %dm",
             (int)petState.pet.ageDays, (int)petState.pet.ageHours, (int)petState.pet.ageMinutes);
    snprintf(buf[2], 64, "Hunger: %d%%", petState.pet.hunger);
    snprintf(buf[3], 64, "Happiness: %d%%", petState.pet.happiness);
    snprintf(buf[4], 64, "Health: %d%%", petState.pet.health);
    snprintf(buf[5], 64, "Mood: %s", lvglMoodStr((int)petState.mood));
    snprintf(buf[6], 64, "Curiosity: %d", (int)petState.traitCuriosity);
    snprintf(buf[7], 64, "Activity: %d", (int)petState.traitActivity);
    snprintf(buf[8], 64, "Stress: %d", (int)petState.traitStress);
}

void lvglPetStatusBuild() {
    if (built) return;

    char rows[PET_STATUS_ROWS][64];
    buildRows(rows);

    screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen, lv_color_hex(LVGL_MENU_BG_BLACK), 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    s_header = lvglBuildHeader(screen);

    list = lv_list_create(screen);
    lv_obj_set_size(list, LV_PCT(100), CONTENT_H - LVGL_HEADER_H);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, LVGL_HEADER_H);

    for (int i = 0; i < PET_STATUS_ROWS; i++) {
        rowLabels[i] = lv_list_add_text(list, rows[i]);
        if (!rowLabels[i]) return;
        lvglApplyListItemStyle(rowLabels[i]);
    }

    built = true;
}

void lvglPetStatusShow() {
    if (!screen) {
        lvglPetStatusBuild();
        if (!screen) return;
    }
    lv_scr_load(screen);
    lvglPetStatusSync();
}

void lvglPetStatusSync() {
    if (!list) return;
    lvglSyncHeader(s_header);
    char rows[PET_STATUS_ROWS][64];
    buildRows(rows);
    for (int i = 0; i < PET_STATUS_ROWS; i++) {
        if (rowLabels[i]) {
            lv_label_set_text(rowLabels[i], rows[i]);
        }
    }
}
