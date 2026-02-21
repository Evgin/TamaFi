# Логгирование в TamaFi

Правила и устройство системы логирования. Использовать как справочник при добавлении отладочного вывода.

---

## 1. Макрос DBG (основной)

**Файл:** `TamaFi/TamaFi.ino`

```cpp
#define DBG(x) do { Serial.println(x); USBSerial.println(x); } while(0)
```

**Назначение:** основной способ вывода отладочных сообщений.

**Поведение:** вывод идёт в оба порта — `Serial` и `USBSerial` (HWCDC). Это нужно, потому что на ESP32-S3 могут быть два COM-порта (USB CDC и USB-UART), и неизвестно, к какому подключён монитор.

**Использование:**
```cpp
DBG("[TamaFi] start");
DBG(inputTouchInited() ? "[input] FT3168 OK" : "[input] FT3168 init fail");
DBG("[FPS] " + String(fps, 1) + " | ...");
```

**Ограничение:** `DBG` принимает один аргумент. Для форматированного вывода использовать `Serial.printf` / `USBSerial.printf` напрямую (см. п. 4).

---

## 2. ICON_DBG / ICON_DBG_F (отключены)

**Файл:** `TamaFi/debug_icon.h`

```cpp
#define ICON_DBG(x) do {} while(0)
#define ICON_DBG_F(fmt, ...) do {} while(0)
```

**Назначение:** логирование, связанное с иконками и отрисовкой (display_amoled, ui).

**Статус:** отключены. Комментарий в коде: «Debug output disabled (was causing freezes)». Не включать без необходимости — ранее вызывали подвисания.

**Использование (если включить):**
```cpp
ICON_DBG("[icon] drawActionStripIcons: drawing");
ICON_DBG_F("[icon] setActionStripVisible=%d", visible ? 1 : 0);
```

---

## 3. UI_DEBUG_TIMING (флаг)

**Файл:** `TamaFi/device_config.h`

```cpp
#define UI_DEBUG_TIMING 0  // 1 = print flush/loop ms with FPS (for bottleneck analysis)
```

**Назначение:** замеры производительности — FPS, время sound/input/draw/flush в loop.

**При `UI_DEBUG_TIMING == 1`:**
- В `TamaFi.ino` loop: замеры и вывод `DBG("[FPS] ...")` раз в ~1 с
- В `display_amoled.cpp`: замер flush
- Дополнительные переменные и ветки кода под тайминги

**Рекомендация:** держать `0` в обычной сборке. Включать только при анализе узких мест.

---

## 4. Прямой вывод (Serial / USBSerial)

**Когда использовать:** для форматированного вывода (`printf`), когда `DBG` не подходит.

**Правило:** выводить в **оба** порта, чтобы лог был виден независимо от выбранного COM-порта.

**Пример (persistence.cpp — лог батареи после сна):**
```cpp
#include "HWCDC.h"
extern HWCDC USBSerial;

Serial.printf("[battery] sleep: saved %d%% (%u mV) -> current %d%% (%u mV), delta %+d%% (%+d mV)\n",
              savedPct, savedMv, cur.percent, cur.voltage, deltaPct, deltaMv);
USBSerial.printf("[battery] sleep: saved %d%% (%u mV) -> current %d%% (%u mV), delta %+d%% (%+d mV)\n",
                 savedPct, savedMv, cur.percent, cur.voltage, deltaPct, deltaMv);
```

---

## 5. Порты и настройки Arduino IDE

| Порт       | Назначение                          |
|-----------|--------------------------------------|
| Serial    | UART0 или USB CDC (зависит от платы) |
| USBSerial | HWCDC, native USB ESP32-S3           |

**Рекомендуемые настройки:**
- **Tools → USB CDC On Boot → Enabled**
- **Tools → USB Mode → Hardware CDC and JTAG** (если доступно)
- **Serial Monitor:** 115200 baud

Если логов нет — попробовать оба COM-порта в Serial Monitor.

---

## 6. Префиксы сообщений

Использовать короткие префиксы для фильтрации:

| Префикс    | Область                    |
|------------|----------------------------|
| `[TamaFi]` | общий старт/состояние      |
| `[input]`  | тач, кнопки                |
| `[time]`   | RTC, время                 |
| `[sound]`  | ES8311, PWM, звук          |
| `[battery]`| AXP2101, заряд, сон        |
| `[icon]`   | иконки, action strip       |
| `[FPS]`    | производительность (при UI_DEBUG_TIMING) |

---

## 7. Специальные логи

### Лог батареи после Deep Sleep

**Файл:** `TamaFi/persistence.cpp` — `persistenceLogBatteryDeltaAfterWake()`

**Условие:** вызывается после `loadState()`, только если в NVS есть ключ `batPct` (устройство хотя бы раз уходило в сон).

**Формат:**
```
[battery] sleep: saved 85% (4100 mV) -> current 84% (4080 mV), delta -1% (-20 mV)
```

---

## 8. Чего избегать

- Не использовать только `Serial` или только `USBSerial` — всегда оба
- Не включать `ICON_DBG` без проверки на подвисания
- Не оставлять `UI_DEBUG_TIMING=1` в релизной сборке (лишняя нагрузка)
- Не вызывать `printf`/`println` в критичных по времени участках (например, внутри draw-цикла)
