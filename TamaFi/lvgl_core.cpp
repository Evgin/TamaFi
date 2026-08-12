/**
 * lvgl_core.cpp - LVGL infrastructure: display driver, touch input, tick timer.
 * Owns lv_init and the single lv_timer_handler call per frame.
 */

#define LV_CONF_INCLUDE_SIMPLE
#include "lvgl_core.h"
#include "device_config.h"
#include "display_amoled.h"
#include "input.h"
#include "navigation.h"
#include "lvgl_main_menu.h"
#include "lvgl_settings.h"
#include "lvgl_pet_status.h"
#include "lvgl_sysinfo.h"

#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <esp_timer.h>
#include <Arduino.h>

static const int LVGL_SCR_W = CONTENT_H;
static const int LVGL_SCR_H = CONTENT_H;  // full content area; header is an LVGL widget

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[LVGL_SCR_W * 20];
static lv_disp_drv_t disp_drv;
static esp_timer_handle_t tickTimer = nullptr;
static const int TICK_PERIOD_MS = 2;

static void lvglTickCb(void* arg) {
    (void)arg;
    lv_tick_inc(TICK_PERIOD_MS);
}

static void indevReadCb(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    (void)drv;
    int16_t x, y;
    // Only forward touches inside the LVGL content area (y < CONTENT_H).
    // The control strip below is handled separately by navigation.cpp.
    if (inputGetTouchState(&x, &y) && y >= 0 && y < CONTENT_H) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void myDispFlush(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
    (void)drv;
    Arduino_GFX* gfx = getDisplayGfx();
    if (!gfx) {
        lv_disp_flush_ready(drv);
        return;
    }
    uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
    uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)color_p, (int16_t)w, (int16_t)h);
    lv_disp_flush_ready(drv);
}

void lvglCoreInit() {
    lv_init();

    lv_disp_draw_buf_init(&draw_buf, buf1, nullptr, sizeof(buf1) / sizeof(lv_color_t));

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LVGL_SCR_W;
    disp_drv.ver_res = LVGL_SCR_H;
    disp_drv.flush_cb = myDispFlush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = indevReadCb;
    lv_indev_drv_register(&indev_drv);

    const esp_timer_create_args_t args = {
        .callback = &lvglTickCb,
        .name = "lvgl_tick"
    };
    esp_timer_create(&args, &tickTimer);
    esp_timer_start_periodic(tickTimer, TICK_PERIOD_MS * 1000);

    Serial.println("[lvgl] core init OK");
}

void lvglCoreTick(Screen s) {
    static Screen prevScreen = SCREEN_BOOT;
    bool screenJustChanged = (s != prevScreen);
    prevScreen = s;

    switch (s) {
        case SCREEN_MENU:       lvglMainMenuSync(mainMenuIndex);   break;
        case SCREEN_SETTINGS:   lvglSettingsSync(settingsMenuIndex); break;
        case SCREEN_PET_STATUS: lvglPetStatusSync();               break;
        case SCREEN_SYSINFO:    lvglSysInfoSync();                 break;
        default: return;
    }

    // On first frame after transitioning to a LVGL screen, force full redraw.
    // LVGL may skip invalidation if it thinks the screen is already active
    // (lv_scr_load returns early when old_scr == new_scr), but meanwhile the
    // game canvas has overwritten the display pixels.
    if (screenJustChanged) {
        lv_obj_invalidate(lv_scr_act());
    }

    lv_timer_handler();
}
