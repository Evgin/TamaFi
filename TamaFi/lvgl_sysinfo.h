#pragma once

#include <Arduino.h>

// LVGL System Info screen: read-only info rows with scroll.
// Navigation: OK = back (handled by navHandleInput).

void lvglSysInfoBuild();
void lvglSysInfoShow();
void lvglSysInfoSync();
