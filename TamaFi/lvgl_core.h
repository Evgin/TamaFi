#pragma once

#include "navigation.h"  // for Screen enum

// LVGL core: one-time infrastructure init and per-frame tick orchestration.
// Call lvglCoreInit() in setup() before any lvglXxxBuild() call.
// Call lvglCoreTick(currentScreen) once per loop() iteration.

void lvglCoreInit();
void lvglCoreTick(Screen s);
