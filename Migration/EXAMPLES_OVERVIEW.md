# Обзор примеров (examples/) — Waveshare ESP32-S3-Touch-AMOLED-1.8

Примеры из демо платы лежат в каталоге `examples/` в корне проекта. Ниже — что в них используется и как это соотносится с TamaFi.

---

## Общее

- **Пины:** все примеры подключают `pin_config.h`. В проекте TamaFi тот же набор пинов задан в `TamaFi/device_config.h` и в `TamaFi/libraries/Mylibrary/pin_config.h` (из демо).
- **Дисплей:** везде одна и та же инициализация:
  - `Arduino_DataBus *bus = new Arduino_ESP32QSPI(LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);`
  - `Arduino_SH8601 *gfx = new Arduino_SH8601(bus, GFX_NOT_DEFINED, 0, LCD_WIDTH, LCD_HEIGHT);`
  - Яркость: `gfx->setBrightness(0..255)` (не `Display_Brightness`).
- **Библиотеки:** `Arduino_GFX_Library.h`, при работе с тачем — `Arduino_DriveBus_Library.h`, для расширителя — `Adafruit_XCA9554`.

---

## По примерам

| Пример | Дисплей | Тач | Расширитель | Прочее |
|--------|---------|-----|-------------|--------|
| **01_HelloWorld** | QSPI + SH8601 | — | — | Минимальный вывод текста |
| **02_Drawing_board** | + | FT3168 (Arduino_FT3x68) | XCA9554 @ 0x20 | Рисование по касаниям |
| **03_GFX_AsciiTable** | + | — | — | Таблица символов на весь экран |
| **04_GFX_FT3168_Image** | + | FT3168 | XCA9554 | Смена картинок по тачу |
| **05_GFX_PCF85063_simpleTime** | + | — | — | RTC PCF85063, часы |
| **06_GFX_ESPWiFiAnalyzer** | + | — | — | Анализ WiFi |
| **07_GFX_Clock** | + | — | — | Часы |
| **08–14, 16** | + | FT3168 | — | LVGL, виджеты, SD, Sqprj |
| **15_ES8311** | — | — | — | Аудиокодек ES8311 |

---

## Тач (FT3168)

- **Библиотека:** `Arduino_DriveBus_Library.h`, класс `Arduino_FT3x68`.
- **Создание:**  
  `std::unique_ptr<Arduino_IIC> FT3168(new Arduino_FT3x68(IIC_Bus, FT3168_DEVICE_ADDRESS, DRIVEBUS_DEFAULT_VALUE, TP_INT, Arduino_IIC_Touch_Interrupt));`  
  где `IIC_Bus = std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);`
- **Перед началом работы:** инициализация расширителя XCA9554 (пины 0,1,2 в OUT, затем HIGH), затем `FT3168->begin()`.
- **Режим:**  
  `FT3168->IIC_Write_Device_State(FT3168->Arduino_IIC_Touch::Device::TOUCH_POWER_MODE, FT3168->Arduino_IIC_Touch::Device_Mode::TOUCH_POWER_MONITOR);`
- **Чтение:**  
  `TOUCH_COORDINATE_X`, `TOUCH_COORDINATE_Y`, `TOUCH_FINGER_NUMBER` через `FT3168->IIC_Read_Device_Value(...)`.

В TamaFi тач реализован своим модулем `input.cpp` (сырой I2C по адресу 0x38). При необходимости можно перейти на Arduino_DriveBus и FT3x68 по образцу этих примеров.

---

## Расширитель (XCA9554)

- **Адрес I2C:** `0x20` (`expander.begin(0x20)`).
- В примерах используется для включения питания тача (пины 0,1,2 расширителя: LOW, пауза, затем HIGH). Кнопка PWR — EXIO4 того же чипа (в TamaFi читается в `input.cpp` через TCA9554).

---

## Внесённые в TamaFi изменения по примерам

1. **Конструктор SH8601:** как в примерах — 5 параметров:  
   `(bus, GFX_NOT_DEFINED, 0, LCD_W, LCD_H)` — убран лишний параметр `false` (IPS).
2. **Яркость:** в коде используется обёртка `setDisplayBrightness(val)`, внутри вызывается `Arduino_SH8601::setBrightness()` (как в примерах).
3. **Инклюды дисплея:** один зонтичный `Arduino_GFX_Library.h` (по аналогии с примерами).
