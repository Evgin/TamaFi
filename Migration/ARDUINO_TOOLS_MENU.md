# Обзор меню Tools в Arduino IDE для TamaFi

Документ описывает пункты меню **Tools** (Инструменты) в Arduino IDE при сборке проекта TamaFi для платы **Waveshare ESP32-S3-Touch-AMOLED-1.8** (MiiBestOD / Spotpear).

**Источники:** фактическое меню Arduino IDE, [LOGGING.md](LOGGING.md), [device_config.h](../TamaFi/device_config.h).

---

## 1. Общие действия IDE

| Пункт | Описание |
|-------|----------|
| **Auto Format** | Автоформатирование кода |
| **Archive Sketch** | Архивирование текущего скетча |
| **Manage Libraries...** | Управление библиотеками |
| **Serial Monitor** | Последовательный монитор (115200 baud) |
| **Serial Plotter** | Последовательный плоттер |

---

## 2. Прошивка и сертификаты

| Пункт | Описание |
|-------|----------|
| **Firmware Updater** | Обновление прошивки |
| **Upload SSL Root Certificates** | Загрузка корневых SSL-сертификатов |

---

## 3. Плата и порт

| Пункт | Текущее / Рекомендуемое | Описание |
|-------|-------------------------|----------|
| **Board** | **Waveshare ESP32-S3-Touch-AMOLED-1.8** | Прямой выбор целевой платы. |
| **Port** | `/dev/cu.usbmodem*` (macOS) или `COM*` (Windows) | Native USB ESP32-S3. При двух портах — пробовать оба. |
| **Reload Board Data** | — | Перезагрузить данные платы |
| **Get Board Info** | — | Получить информацию о плате |

---

## 4. Настройки платы (Waveshare ESP32-S3-Touch-AMOLED-1.8)

### 4.1. USB и загрузка

| Пункт | Рекомендуемое | Описание |
|-------|---------------|----------|
| **USB CDC On Boot** | **Enabled** | **Обязательно для TamaFi.** Код использует `USBSerial` (HWCDC) для логов. При Disabled Serial Monitor не покажет вывод. |
| **USB DFU On Boot** | Disabled | Режим DFU при загрузке. |
| **USB Firmware MSC On Boot** | Disabled | Режим Mass Storage при загрузке. |
| **USB Mode** | **Hardware CDC and JTAG** | CDC для Serial + JTAG для отладки. |
| **Upload Mode** | UART0 / Hardware CDC | Режим загрузки. |
| **Upload Speed** | 460800 (или 921600) | Скорость загрузки. |

### 4.2. Процессор и память

| Пункт | Рекомендуемое | Описание |
|-------|---------------|----------|
| **CPU Frequency** | 240MHz (WiFi) | Максимальная частота. |
| **Flash Mode** | QIO 80MHz | Режим доступа к Flash. |
| **Partition Scheme** | 16M Flash (3MB APP/9.9MB FATFS) | Схема разделов. На плате 16MB Flash. |
| **PSRAM** | **Enabled (OPI PSRAM / Quad 8MB)** | **Обязательно для TamaFi.** На плате 8MB PSRAM. Нужно для canvas 368×368, скинов, LVGL. При Disabled — нехватка памяти. |

### 4.3. Ядра и отладка

| Пункт | Рекомендуемое | Описание |
|-------|---------------|----------|
| **Arduino Runs On** | Core 1 | Ядро для `setup()`/`loop()`. |
| **Events Run On** | Core 1 | Ядро для событий (WiFi, BLE). |
| **Core Debug Level** | None | Уровень отладки ядра. |

### 4.4. Прочее

| Пункт | Рекомендуемое | Описание |
|-------|---------------|----------|
| **Erase All Flash Before Sketch Upload** | Disabled | Стирать всю Flash перед загрузкой. Обычно не нужно. |

---

## 5. Программатор

| Пункт | Описание |
|-------|----------|
| **Programmer** | Esptool (по умолчанию) |
| **Burn Bootloader** | Записать загрузчик |

---

## 6. Важно: что поправить для TamaFi

По скриншоту меню видно два критичных отличия от рекомендуемых настроек:

| Пункт | Сейчас | Нужно для TamaFi |
|-------|--------|------------------|
| **USB CDC On Boot** | Disabled | **Enabled** — иначе логов в Serial Monitor не будет |
| **PSRAM** | Disabled | **Enabled** (OPI PSRAM или Quad 8MB) — иначе нехватка памяти для canvas, LVGL, скинов |

Остальные параметры (Partition Scheme 16M, Flash Mode QIO 80MHz, CPU 240MHz, USB Mode Hardware CDC and JTAG) выглядят корректно.

---

## 7. Резюме: рекомендуемый набор

```
Board:              Waveshare ESP32-S3-Touch-AMOLED-1.8
Port:               [ваш COM-порт]
USB CDC On Boot:     Enabled          ← обязательно для логов
USB Mode:            Hardware CDC and JTAG
PSRAM:              Enabled (OPI/Quad 8MB)  ← обязательно
Partition Scheme:   16M Flash (3MB APP/9.9MB FATFS)
Flash Mode:         QIO 80MHz
CPU Frequency:      240MHz (WiFi)
```

---

## 8. Ссылки

- [Waveshare Wiki - ESP32-S3-Touch-AMOLED-1.8](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.8)
- [LOGGING.md](LOGGING.md) — логирование и порты в TamaFi

---

*Документ обновлён по фактическому меню Tools в Arduino IDE.*
