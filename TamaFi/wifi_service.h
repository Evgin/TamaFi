#pragma once

#include <Arduino.h>
#include "pet_logic.h"   // WifiStats, WifiNetworkInfo, MAX_WIFI_LIST

// Initialize WiFi in STA mode.
void wifiInit();

// Start async WiFi scan.
void wifiStartScan();

// Check if scan completed. Returns true when done (results in wifiStats/wifiList).
bool wifiCheckScanDone();

// --- State (readable by UI and orchestrator) ---

extern WifiStats       wifiStats;
extern WifiNetworkInfo wifiList[MAX_WIFI_LIST];
extern int             wifiListCount;
extern bool            wifiScanInProgress;
extern unsigned long   lastWifiScanTime;
