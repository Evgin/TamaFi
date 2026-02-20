#pragma once

#include <Arduino.h>
#include "pet_logic.h"   // WifiStats, WifiNetworkInfo, MAX_WIFI_LIST

// Initialize WiFi in STA mode.
void wifiInit();

// Connect to WiFi using WIFI_SSID / WIFI_PASSWORD from device_config. Non-blocking.
void wifiConnect();

// Returns true if connected to WiFi.
bool wifiConnected();

// Start NTP sync and write result to RTC. Call only when wifiConnected().
void wifiStartNtpSync();

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
