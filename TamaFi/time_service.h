#pragma once

#include <Arduino.h>
#include <time.h>

// ============ Time service (RTC PCF85063 + NTP) ============

// Initialize RTC. Call after Wire.begin() (e.g. after inputInit).
void timeServiceInit();

// Returns true if RTC is available and time is valid.
bool timeServiceAvailable();

// Get real time (from RTC or system). Returns true if time is valid.
bool timeServiceGetRealTime(struct tm* out);

// Write time from system (time()) to RTC. Call after NTP sync.
void timeServiceSetFromSystem();
