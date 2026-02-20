#include "wifi_service.h"
#include "time_service.h"
#include "device_config.h"
#include <WiFi.h>
#include <time.h>

// --- State ---

WifiStats       wifiStats;
WifiNetworkInfo wifiList[MAX_WIFI_LIST];
int             wifiListCount      = 0;
bool            wifiScanInProgress = false;
unsigned long   lastWifiScanTime   = 0;

// NTP: Moscow UTC+3, no DST
#define NTP_GMT_OFFSET_SEC   (3 * 3600)
#define NTP_DAYLIGHT_OFFSET  0
#define NTP_SERVER1          "pool.ntp.org"
#define NTP_SERVER2          "time.nist.gov"

// ============ Public API ============

void wifiInit() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
}

void wifiConnect() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

bool wifiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void wifiStartNtpSync() {
    if (!wifiConnected()) return;

    configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET, NTP_SERVER1, NTP_SERVER2);

    // Wait for time sync (max 10 seconds)
    for (int i = 0; i < 100; i++) {
        time_t now = time(nullptr);
        if (now > 1700000000) {  // reasonable date (2023+)
            timeServiceSetFromSystem();
            return;
        }
        delay(100);
    }
}

void wifiStartScan() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    WiFi.scanNetworks(true);       // async
    wifiScanInProgress = true;
}

bool wifiCheckScanDone() {
    if (!wifiScanInProgress) return false;

    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) return false;

    wifiScanInProgress = false;
    lastWifiScanTime   = millis();

    if (n < 0) {
        wifiStats     = WifiStats();
        wifiListCount = 0;
        WiFi.scanDelete();
        return true;
    }

    WifiStats s;
    s.netCount    = n;
    s.strongCount = 0;
    s.hiddenCount = 0;
    s.openCount   = 0;
    s.wpaCount    = 0;
    int totalRSSI = 0;

    wifiListCount = 0;

    for (int i = 0; i < n; i++) {
        int rssi = WiFi.RSSI(i);
        totalRSSI += rssi;
        if (rssi > -60) s.strongCount++;

        String ssid    = WiFi.SSID(i);
        bool   isHidden = (ssid.length() == 0);
        if (isHidden) s.hiddenCount++;

        wifi_auth_mode_t enc = WiFi.encryptionType(i);
        bool isOpen = (enc == WIFI_AUTH_OPEN);
        if (isOpen) s.openCount++;
        else        s.wpaCount++;

        if (wifiListCount < MAX_WIFI_LIST) {
            WifiNetworkInfo &info = wifiList[wifiListCount++];
            info.ssid   = isHidden ? String("(hidden)") : ssid;
            info.rssi   = rssi;
            info.isOpen = isOpen;
        }
    }

    s.avgRSSI = (n > 0) ? (totalRSSI / n) : -100;
    wifiStats = s;

    WiFi.scanDelete();
    return true;
}
