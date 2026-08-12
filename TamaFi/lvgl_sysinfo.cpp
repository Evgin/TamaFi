/**
 * lvgl_sysinfo.cpp - System Info screen widgets.
 * Read-only text rows with scroll.
 */

#define LV_CONF_INCLUDE_SIMPLE
#include "lvgl_sysinfo.h"
#include "lvgl_menu_common.h"
#include "device_config.h"
#include "navigation.h"
#include "battery.h"
#include "time_service.h"
#include "wifi_service.h"

#include <lvgl.h>
#include <Arduino.h>

static const int ROW_COUNT = 18;

static lv_obj_t*  screen = nullptr;
static lv_obj_t*  list = nullptr;
static lv_obj_t*  rowLabels[ROW_COUNT];
static LvglHeader s_header;
static int s_rowCount = 0;
static bool built = false;

static int buildRows(char buf[ROW_COUNT][64]) {
    int i = 0;
    snprintf(buf[i++], 64, "Firmware: 2.0");
    snprintf(buf[i++], 64, "MCU: ESP32-S3");
    snprintf(buf[i++], 64, "Memory: %d KB", ESP.getFreeHeap() / 1024);

    struct tm t;
    if (timeServiceGetRealTime(&t)) {
        snprintf(buf[i++], 64, "Time: %02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
    } else {
        snprintf(buf[i++], 64, "Time: --:--:--");
    }

    unsigned long us = millis() / 1000;
    snprintf(buf[i++], 64, "Uptime: %02lu:%02lu:%02lu", us / 3600, (us % 3600) / 60, us % 60);
    snprintf(buf[i++], 64, "WiFi: %s", wifiScanInProgress ? "Scanning..." : "Idle");
    snprintf(buf[i++], 64, "--- Battery ---");

    const BatteryInfo& bat = batteryGetInfo();
    if (bat.available) {
        if (bat.batteryConnected) {
            snprintf(buf[i++], 64, "Charge: %d%% (%d mV)", bat.percent, bat.voltage);
        } else {
            snprintf(buf[i++], 64, "Battery: none");
        }
        snprintf(buf[i++], 64, "Charging: %s", bat.charging ? "Yes" : "No");
        snprintf(buf[i++], 64, "USB: %s", bat.usbConnected ? "Connected" : "---");
    } else {
        snprintf(buf[i++], 64, "Battery: no PMIC");
    }

    snprintf(buf[i++], 64, "--- WiFi ---");
    snprintf(buf[i++], 64, "Networks: %d  Open: %d", wifiStats.netCount, wifiStats.openCount);
    snprintf(buf[i++], 64, "Strong: %d  WPA: %d", wifiStats.strongCount, wifiStats.wpaCount);
    snprintf(buf[i++], 64, "Hidden: %d  RSSI: %d", wifiStats.hiddenCount, wifiStats.avgRSSI);

    return i;
}

void lvglSysInfoBuild() {
    if (built) return;

    char rows[ROW_COUNT][64];
    s_rowCount = buildRows(rows);

    screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen, lv_color_hex(LVGL_MENU_BG_BLACK), 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    s_header = lvglBuildHeader(screen);

    list = lv_list_create(screen);
    lv_obj_set_size(list, LV_PCT(100), CONTENT_H - LVGL_HEADER_H);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, LVGL_HEADER_H);

    for (int i = 0; i < s_rowCount; i++) {
        rowLabels[i] = lv_list_add_text(list, rows[i]);
        if (!rowLabels[i]) return;
        lvglApplyListItemStyle(rowLabels[i]);
    }

    built = true;
}

void lvglSysInfoShow() {
    if (!screen) {
        lvglSysInfoBuild();
        if (!screen) return;
    }
    lv_scr_load(screen);
    lvglSysInfoSync();
}

void lvglSysInfoSync() {
    if (!list) return;
    lvglSyncHeader(s_header);
    char rows[ROW_COUNT][64];
    int count = buildRows(rows);
    for (int i = 0; i < count && rowLabels[i]; i++) {
        lv_label_set_text(rowLabels[i], rows[i]);
    }
}
