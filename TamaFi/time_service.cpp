#include "time_service.h"
#include "device_config.h"
#include <Wire.h>
#include <time.h>

// PCF85063 register addresses (NXP datasheet)
#define PCF85063_REG_CTRL1    0x00
#define PCF85063_REG_SECONDS  0x04
#define PCF85063_REG_MINUTES  0x05
#define PCF85063_REG_HOURS    0x06
#define PCF85063_REG_DAY      0x07
#define PCF85063_REG_WEEKDAY  0x08
#define PCF85063_REG_MONTH    0x09
#define PCF85063_REG_YEAR     0x0A

#define PCF85063_OS_BIT       0x80  // Oscillator stop flag in seconds reg

static bool rtcAvailable = false;

static uint8_t bcdToDec(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

static uint8_t decToBcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

static bool rtcReadRegisters(uint8_t startReg, uint8_t* buf, uint8_t len) {
    Wire.beginTransmission(PCF85063_I2C_ADDR);
    Wire.write(startReg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((uint8_t)PCF85063_I2C_ADDR, len) != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
}

static bool rtcWriteRegisters(uint8_t startReg, const uint8_t* buf, uint8_t len) {
    Wire.beginTransmission(PCF85063_I2C_ADDR);
    Wire.write(startReg);
    for (uint8_t i = 0; i < len; i++) Wire.write(buf[i]);
    return Wire.endTransmission() == 0;
}

void timeServiceInit() {
    rtcAvailable = false;
    uint8_t buf[7];
    if (!rtcReadRegisters(PCF85063_REG_SECONDS, buf, 7)) return;
    // Check oscillator stop flag - if set, RTC was stopped (e.g. power loss)
    // We still consider it "available" but time may be invalid
    rtcAvailable = true;
}

bool timeServiceAvailable() {
    return rtcAvailable;
}

bool timeServiceGetRealTime(struct tm* out) {
    if (!out || !rtcAvailable) return false;

    // First try system time (NTP may have synced it)
    time_t now = time(nullptr);
    if (now > 0) {
        struct tm* sys = localtime(&now);
        if (sys && sys->tm_year > 70) {  // year 1970+
            *out = *sys;
            return true;
        }
    }

    // Fallback to RTC
    uint8_t buf[7];
    if (!rtcReadRegisters(PCF85063_REG_SECONDS, buf, 7)) return false;

    out->tm_sec  = bcdToDec(buf[0] & 0x7F);
    out->tm_min  = bcdToDec(buf[1] & 0x7F);
    out->tm_hour = bcdToDec(buf[2] & 0x3F);  // 24h mode
    out->tm_mday = bcdToDec(buf[3] & 0x3F);
    out->tm_wday = buf[4] & 0x07;
    out->tm_mon  = bcdToDec(buf[5] & 0x1F) - 1;  // 0-11
    out->tm_year = bcdToDec(buf[6]) + 100;        // years since 1900

    // Check oscillator stop - time may be stale
    if (buf[0] & PCF85063_OS_BIT) {
        // RTC was stopped, time might be wrong
    }
    return true;
}

void timeServiceSetFromSystem() {
    if (!rtcAvailable) return;

    time_t now = time(nullptr);
    if (now <= 0) return;

    struct tm* t = localtime(&now);
    if (!t) return;

    uint8_t buf[7];
    buf[0] = decToBcd(t->tm_sec) & 0x7F;   // clear OS bit
    buf[1] = decToBcd(t->tm_min);
    buf[2] = decToBcd(t->tm_hour) & 0x3F;  // 24h
    buf[3] = decToBcd(t->tm_mday);
    buf[4] = t->tm_wday & 0x07;
    buf[5] = decToBcd(t->tm_mon + 1);
    buf[6] = decToBcd(t->tm_year % 100);

    rtcWriteRegisters(PCF85063_REG_SECONDS, buf, 7);
}
