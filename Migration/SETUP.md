# Подготовка окружения (Фаза 0)

## Демо и пины

- Скачать демо: [ESP32-S3-Touch-AMOLED-1.8 Demo](https://files.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.8/ESP32-S3-Touch-AMOLED-1.8-Demo.zip). Собрать и прошить — убедиться, что дисплей и тач работают.
- Пины для нашей платы уже выписаны в [TamaFi/device_config.h](../TamaFi/device_config.h) (источник: [Waveshare GitHub Mylibrary/pin_config.h](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8/blob/main/examples/Arduino-v3.3.5/libraries/Mylibrary/pin_config.h)).

## Библиотеки (Arduino IDE / PlatformIO)

Установить:

- **GFX_Library_for_Arduino** — дисплей (Arduino_SH8601, Arduino_ESP32QSPI). Можно взять из демо или [GitHub](https://github.com/moononournation/GFX_Library_for_Arduino).
- **Arduino_DriveBus** — тач FT3168 (I2C).
- **Arduino_DataBus** — часто идёт в составе GFX или отдельно.
- **ESP32_IO_Expander** (TCA9554) — кнопка PWR (EXIO4). Альтернатива: **Adafruit_XCA9554** из демо Waveshare.

В проекте: плата **ESP32-S3** (USB CDC при необходимости). Зависимости от TFT_eSPI и Adafruit_NeoPixel для целевой сборки удалены.

## Результат

Рабочий демо на устройстве, пины зафиксированы в `device_config.h`, библиотеки установлены.
