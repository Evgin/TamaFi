#pragma once

#include <Arduino.h>

// LVGL Pet Status screen: read-only info rows with scroll.
// Navigation: OK = back (handled by navHandleInput).

void lvglPetStatusBuild();
void lvglPetStatusShow();
void lvglPetStatusSync();
