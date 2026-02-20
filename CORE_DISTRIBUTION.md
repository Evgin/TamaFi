# Распределение функционала по ядрам ESP32-S3

Документ описывает текущее и возможное распределение задач по ядрам в проекте TamaFi.

## Железо

**ESP32-S3** — двухъядерный процессор (Xtensa LX7, до 240 МГц). Используется **FreeRTOS**.

| Ядро | Название | Типичное назначение |
|------|----------|---------------------|
| **Core 0** | PRO_CPU | WiFi, Bluetooth, системные задачи ESP-IDF |
| **Core 1** | APP_CPU | Приложение, Arduino `setup()` и `loop()` |

---

## Текущее распределение

### Core 0 (PRO_CPU)

| Компонент | Владелец | Описание |
|-----------|----------|----------|
| WiFi stack | ESP-IDF | Стек протоколов, управление радио |
| WiFi scan | ESP-IDF | `WiFi.scanNetworks(true)` — асинхронное сканирование, внутренние задачи |
| Bluetooth | ESP-IDF | (если включён) |

**Примечание:** Явного кода TamaFi на Core 0 нет. Используются только системные задачи ESP-IDF.

### Core 1 (APP_CPU)

Вся логика приложения выполняется в `loop()` на Core 1. Порядок выполнения в каждом проходе:

| № | Функционал | Файл/функция | Периодичность |
|---|------------|--------------|---------------|
| 1 | Звук: заполнение I2S буфера, сиквенсер | `soundFeed()`, `sndUpdate()`, `stopBuzzerIfNeeded()` | Каждый loop |
| 2 | Опрос тач-экрана (FT3168) и кнопок (BOOT) | `inputPoll()` | Каждый loop (тач — раз в 20 мс) |
| 3 | AutoSleep: BOOT, таймаут бездействия | inline в loop | По событиям |
| 4 | Навигация: обработка ввода | `navHandleInput()` | По событиям |
| 5 | Логика питомца | `petTick()` | ~100 мс |
| 6 | Проверка завершения WiFi-скана | `wifiCheckScanDone()`, `petInjectWifiResult()` | По завершении скана |
| 7 | Обработка событий питомца | `processPetEvents()` | По событиям |
| 8 | Опрос батареи | `batteryUpdate()` | ~5 с |
| 9 | Автосохранение | `saveState()` | по `autoSaveMs` |
| 10 | Отрисовка UI | `uiDrawScreen()` | Каждый loop (если не спит) |

### Сводка по компонентам

| Компонент | Ядро | Примечание |
|-----------|------|------------|
| Звук (ES8311 I2S) | Core 1 | `soundFeed()` → `i2s.write()` блокирующий |
| Тач FT3168 | Core 1 | `inputPoll()` → I2C, интервал 20 мс |
| Кнопки BOOT, PWR | Core 1 | GPIO, TCA9554 (I2C) |
| Логика питомца | Core 1 | `petTick()` |
| Навигация | Core 1 | `navHandleInput()` |
| Отрисовка UI | Core 1 | `uiDrawScreen()` → QSPI |
| Батарея (AXP2101) | Core 1 | I2C, раз в 5 с |
| Сохранение (NVS) | Core 1 | Preferences |
| WiFi scan | Core 0 | Асинхронно, внутренние задачи ESP-IDF |

---

## Возможные улучшения

### 1. Звук на Core 0 (рекомендуется)

**Идея:** Вынести `soundFeed()` и `sndUpdate()` в отдельную FreeRTOS-задачу на Core 0.

**Плюсы:**
- Устраняет блокировку `i2s.write()` в основном loop
- Звук — лёгкая задача (заполнение буферов), мало конфликтует с WiFi
- Уже отмечено в session-log как потенциальное улучшение

**Минусы:**
- Нужна синхронизация: очередь команд (sndGoodFeed, sndDiscover и т.д.) или shared state
- Mutex при доступе к общим данным

### 2. UI на отдельном ядре — не рекомендуется

**Почему:** Core 0 занят WiFi. Опыт LVGL-сообщества: display на Core 0 приводит к периодическим просадкам UI (до ~1000 мс на кадр) из‑за конкуренции с WiFi.

**Альтернатива:** UI в отдельной задаче на Core 1 — даёт структурирование кода, но не даёт реального параллелизма (оба потока на одном ядре).

### 3. Принятые в сообществе практики

- **Core 0:** WiFi/BT (система) + лёгкие фоновые задачи (звук, опрос датчиков)
- **Core 1:** Основная логика приложения, UI, ввод
- **Не ставить** тяжёлые CPU-задачи на Core 0 рядом с WiFi

---

## Проверка ядра в коде

Для отладки можно использовать:

```cpp
#include "esp_task_wdt.h"

void loop() {
    Serial.print("loop on core ");
    Serial.println(xPortGetCoreID());  // Должно быть 1
}
```

---

## Ссылки

- [ESP-IDF FreeRTOS SMP](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/freertos-smp.html)
- [Arduino ESP32 Dual Core](https://randomnerdtutorials.com/esp32-dual-core-arduino-ide/)
- LVGL Forum: [ESP32 display on core 0 — slow updates](https://forum.lvgl.io/t/esp32-xtaskcreatepinnedtocore-to-core-0-with-arduino-occasionally-slow-ui-updates/5120)
