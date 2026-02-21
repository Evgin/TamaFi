# Логика работы с графикой в TamaFi

Документ описывает архитектуру отрисовки, используемые библиотеки и порядок вывода на дисплей WAVESHARE_ESP32_S3_1.8_AMOLED (368×448 px).

---

## 1. Библиотеки

| Библиотека | Версия | Назначение |
|------------|--------|------------|
| **GFX_Library_for_Arduino** | v1.4.9 | Дисплей, canvas, примитивы (fillRect, drawPixel, draw16bitRGBBitmap и т.д.) |
| **U8g2** | — | Шрифты (кириллица, Streamline-иконки) |

Подключение:
```cpp
#include <Arduino_GFX_Library.h>
#define U8G2_FONT_SUPPORT
#include <U8g2lib.h>
```

---

## 2. Цепочка вывода

```
UI (ui.cpp, ui_common, ui_menu, ui_info)
    │
    ▼  getContentCanvas() → рисует в буфер 240×240
Arduino_Canvas (contentCanvas)
    │
    ▼  flush() → передаёт буфер в ScalerGFX
ScalerGFX (кастомный класс)
    │
    ▼  масштабирует 240×240 → 368×368, рисует на realGfx
Arduino_SH8601 (realGfx)
    │
    ▼  QSPI
Arduino_ESP32QSPI (bus)
    │
    ▼
Физический дисплей SH8601 (368×448)
```

---

## 3. Компоненты

### 3.1. Arduino_Canvas (contentCanvas)

- **Размер:** 240×240 px (CONTENT_LOGICAL_W × CONTENT_LOGICAL_H)
- **Роль:** off-screen буфер (framebuffer) для игрового контента
- **API:** наследует Arduino_GFX — fillRect, drawPixel, draw16bitRGBBitmap, setFont, print и т.д.

Вся игровая логика (питомец, фон, меню, статус) рисуется в content canvas в координатах 240×240. При вызове `flush()` буфер передаётся в ScalerGFX.

### 3.2. ScalerGFX

Кастомный класс, наследник Arduino_GFX. Выступает как «выход» для content canvas.

**Задачи:**
1. Масштабирование 240×240 → 368×368 (nearest neighbor)
2. Пакетная отрисовка (батчи по 32 строки) для снижения числа QSPI-транзакций
3. Пропуск области action strip при `actionStripDrawn` (чтобы не затирать иконки)
4. Частичная перерисовка области возраста (x < 92) при пропуске strip — фон из content, без fillRect

**Ключевой метод:** `draw16bitRGBBitmap(x, y, bitmap, w, h)` — вызывается из canvas при flush. Принимает полный буфер 240×240, масштабирует и выводит на `_output` (realGfx).

### 3.3. Arduino_SH8601 (realGfx)

Драйвер дисплея SH8601. Рисует напрямую на физический экран 368×448.

Используется для:
- приёма масштабированного контента от ScalerGFX;
- отрисовки control bar (UP/OK/DOWN);
- отрисовки action strip (иконки меню/еда/лекарство);
- отрисовки возраста питомца.

### 3.4. Arduino_ESP32QSPI (bus)

QSPI-шина для обмена с дисплеем. Пины: LCD_CS, LCD_SCLK, LCD_SDIO0..3.

---

## 4. Разметка экрана

```
┌─────────────────────────────────────┐  y=0
│ Шапка (заголовок, время, батарея)   │  ← content canvas
├─────────────────────────────────────┤
│                                     │
│  Игровая область (контент)          │
│  240×240 логически → 368×368 на     │
│  дисплее                            │
│                                     │
│  Action strip (иконки)              │  y=304..368
│  x=92..368, только на HOME          │  ← realGfx
│                                     │
├─────────────────────────────────────┤  y=368 (CONTENT_H)
│  Control bar (UP / OK / DOWN)        │  высота 80 px
│  ← realGfx                          │
└─────────────────────────────────────┘  y=448
```

---

## 5. Порядок отрисовки (flushContentAndDrawControlBar)

1. **contentCanvas->flush()** — буфер 240×240 передаётся в ScalerGFX, масштабируется и выводится на дисплей (с учётом пропуска strip и частичной перерисовки возраста).
2. **drawActionStripIcons()** — если `actionStripNeedsRedraw`, рисуются иконки action strip на realGfx.
3. **Возраст питомца** — если экран HOME, рисуется на realGfx в (12, CONTENT_H-10).

Control bar (UP/OK/DOWN) рисуется отдельно при смене экрана через `displayControlBarSetScreen()`.

---

## 6. Шрифты (U8g2)

- **u8g2_font_6x13_t_cyrillic** — меню, настройки, статус, заголовки, возраст (6×13 px)
- **Streamline** — иконки action strip (menu, feed, medicine), setTextSize(2) → ~48×48 px

Включение UTF-8: `setUTF8Print(true)` для кириллицы.

---

## 7. Спрайты и битмапы

- **drawSpriteToContent()** — пиксель-за-пикселем с прозрачностью (TFT_WHITE как transparent)
- **draw16bitBitmapToContent()** — полный bitmap из RAM
- **draw16bitBitmapToContentProgmem()** — bitmap из PROGMEM (flash), построчно

Спрайты питомца, эффектов и фона рисуются в content canvas в координатах 240×240.

---

## 8. Оптимизации

- **Пропуск strip:** при `actionStripDrawn` область y≥304 не перезаписывается контентом, чтобы не моргать иконками при смене выбора.
- **Частичная перерисовка возраста:** при пропуске strip левая часть (x<92) всё равно перерисовывается из content — возраст получает свежий фон без fillRect.
- **Батчи по 32 строки:** уменьшение числа вызовов draw16bitRGBBitmap при масштабировании.

---

## 9. Файлы

| Файл | Роль |
|------|------|
| `display_amoled.cpp` | Инициализация, ScalerGFX, flush, action strip, возраст |
| `display_amoled.h` | API, константы цветов |
| `device_config.h` | LCD_W, LCD_H, CONTENT_H, CONTROL_H, ACTION_STRIP_H и т.д. |
| `ui.cpp` | Игровые экраны, отрисовка в content canvas |
| `ui_common.cpp` | drawHeader, drawTimeInHeader, drawBatteryIndicator |
| `ui_menu.cpp` | Меню, настройки |
| `ui_info.cpp` | Статус, системная информация |
