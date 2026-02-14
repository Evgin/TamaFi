#include "battery.h"
#include "device_config.h"

#include <Wire.h>

#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"

static XPowersPMU power;
static BatteryInfo info = {};

void batteryInit() {
    info.available = power.begin(Wire, AXP2101_I2C_ADDR, IIC_SDA, IIC_SCL);
    if (!info.available) return;

    // Enable ADC channels needed for battery monitoring
    power.enableBattDetection();
    power.enableBattVoltageMeasure();
    power.enableVbusVoltageMeasure();
    power.enableSystemVoltageMeasure();

    // First reading right away
    batteryUpdate();
}

void batteryUpdate() {
    if (!info.available) return;

    info.batteryConnected = power.isBatteryConnect();
    info.charging         = power.isCharging();
    info.usbConnected     = power.isVbusIn();
    info.voltage          = power.getBattVoltage();

    if (info.batteryConnected) {
        info.percent = power.getBatteryPercent();
    } else {
        info.percent = -1;
    }
}

const BatteryInfo& batteryGetInfo() {
    return info;
}
