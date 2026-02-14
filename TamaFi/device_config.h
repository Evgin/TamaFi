#pragma once

// WAVESHARE_ESP32_S3_1.8_AMOLED (MiiBestOD/Spotpear ESP32-S3-Touch-AMOLED-1.8)
// Pins from Waveshare GitHub: examples/Arduino-v3.3.5/libraries/Mylibrary/pin_config.h

// ---------- Display (QSPI, SH8601) ----------
#define LCD_CS    12
#define LCD_SCLK  11
#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7

#define LCD_W       368
#define LCD_H       448
#define CONTENT_H   368   // content area height (square 368x368)
#define CONTROL_H   80    // bottom strip for virtual buttons / indicators
#define ACTION_STRIP_H  50  // action strip overlay height (drawn over bottom of content)

// Logical content size (game renders at this resolution, then scaled to CONTENT_H x CONTENT_H)
#define CONTENT_LOGICAL_W  240
#define CONTENT_LOGICAL_H  240
#define CONTENT_SCALE_NUM  368
#define CONTENT_SCALE_DEN  240

// ---------- Touch (I2C, FT3168) ----------
#define IIC_SDA 15
#define IIC_SCL 14
#define TP_INT  21

// ---------- Buttons ----------
#define BOOT_BTN_PIN  0   // GPIO0, LOW = pressed (не BOOT_PIN — конфликт с esp32-hal.h)
// PWR = EXIO4 via TCA9554 (I2C same as touch); HIGH = pressed
#define TCA9554_I2C_ADDR  0x20
#define PWR_EXIO_BIT      4

// ---------- Sound (ES8311 I2S на плате 1.8" AMOLED) ----------
#define BUZZER_PIN  46   // PA (power amplifier enable) — подать HIGH для выхода на динамик
#define BUZZER_CH   0
// I2S для ES8311 (Mylibrary/pin_config.h)
#define I2S_MCK_IO  16
#define I2S_BCK_IO  9
#define I2S_DI_IO   10
#define I2S_WS_IO   45
#define I2S_DO_IO   8

// ---------- Power Management (AXP2101 PMIC, I2C same bus as touch) ----------
#define AXP2101_I2C_ADDR  0x34

// ---------- Display brightness ----------
// Controlled via gfx->Display_Brightness(0..255), no separate PWM pin
