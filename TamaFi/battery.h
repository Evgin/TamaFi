#pragma once

#include <Arduino.h>

struct BatteryInfo {
    bool     available;         // AXP2101 found on I2C
    bool     batteryConnected;  // battery physically present
    int      percent;           // 0-100, -1 if unknown
    uint16_t voltage;           // mV
    bool     charging;          // charge in progress
    bool     usbConnected;      // USB power present
};

// Initialize AXP2101 PMIC. Call after inputInit() (Wire already up).
// Performs first batteryUpdate() internally.
void batteryInit();

// Poll AXP2101 for current battery state. Call every ~5 s from loop().
void batteryUpdate();

// Read-only access to latest battery data.
const BatteryInfo& batteryGetInfo();
