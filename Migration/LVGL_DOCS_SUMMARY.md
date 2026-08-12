# LVGL — справочник для TamaFi

> Документ для AI-ассистента: при работе с LVGL в проекте TamaFi обращаться к этому файлу и переходить по ссылкам в документацию LVGL.

**Документация LVGL:** https://docs.lvgl.io/master/

**Версия в проекте:** LVGL v8 API (см. [lv_api_map_v8.h](https://docs.lvgl.io/master/API/lv_api_map_v8_h.html))

---

## 1. Основные концепции

| Концепция | Описание |
|-----------|----------|
| **Display** | Физический дисплей, к нему подключается LVGL |
| **Screen** | Логический экран, контейнер виджетов. Создание: `lv_obj_create(NULL)` |
| **Widgets** | Кнопки, списки, слайдеры и т.п. |
| **Стили** | Цвет, шрифт, отступы и т.д. |
| **События** | Клик, скролл, long press и т.д. |

**Цикл инициализации:**
```
lv_init() → lv_tick_set_cb(millis) → создать display + indev → создать UI → в loop вызывать lv_timer_handler()
```

**Основы:** [Learn the Basics](https://docs.lvgl.io/master/getting_started/learn_the_basics.html)  
**Интеграция:** [Overview](https://docs.lvgl.io/master/integration/overview.html)  
**Arduino:** [Arduino Integration](https://docs.lvgl.io/master/integration/frameworks/arduino.html)

---

## 2. Виджеты (компоненты библиотеки)

Полный список: [All Widgets](https://docs.lvgl.io/master/widgets/index.html)

### 2.1 Base Widget (lv_obj)

Базовый виджет, от которого наследуются все остальные. Прямоугольник с поддержкой детей, позиции, размеров, layout, стилей, событий, флагов.

- **Документация:** [Base Widget (lv_obj)](https://docs.lvgl.io/master/widgets/base_widget.html)
- **API:** [lv_obj.h](https://docs.lvgl.io/master/API/core/lv_obj_h.html)

---

### 2.2 List (lv_list)

Вертикальный список с кнопками и текстом. Поддержка скролла.

- **Документация:** [List (lv_list)](https://docs.lvgl.io/master/widgets/list.html)
- **API:** [lv_list.h](https://docs.lvgl.io/master/API/widgets/list/lv_list_h.html)

**Части:** `LV_PART_MAIN`, `LV_PART_SCROLLBAR`

**Функции:**
- `lv_list_add_button(list, icon, text)` — добавить кнопку (v8: `lv_list_add_btn`)
- `lv_list_add_text(list, text)` — добавить текст
- `lv_list_get_button_text(list, btn)` — получить текст кнопки

**События:** от кнопок внутри списка (`LV_EVENT_CLICKED` и т.д.)

---

### 2.3 Button (lv_button)

Кнопка. По умолчанию: не скроллируется, в группе, размер по контенту.

- **Документация:** [Button (lv_button)](https://docs.lvgl.io/master/widgets/button.html)
- **API:** [lv_button.h](https://docs.lvgl.io/master/API/widgets/button/lv_button_h.html)

**Части:** `LV_PART_MAIN`

**События:** `LV_EVENT_CLICKED`, `LV_EVENT_VALUE_CHANGED` (для checkable)

---

### 2.4 Label (lv_label)

Текст. Поддержка переноса, длинных строк, символов.

- **Документация:** [Label (lv_label)](https://docs.lvgl.io/master/widgets/label.html)
- **API:** [lv_label.h](https://docs.lvgl.io/master/API/widgets/label/lv_label_h.html)

**Функции:**
- `lv_label_set_text(label, "text")`
- `lv_label_set_text_fmt(label, "%d", val)`
- `lv_label_set_long_mode(label, LV_LABEL_LONG_...)` — обрезка, скролл, перенос

---

### 2.5 Slider (lv_slider)

Слайдер с ползунком. Горизонтальный или вертикальный.

- **Документация:** [Slider (lv_slider)](https://docs.lvgl.io/master/widgets/slider.html)
- **API:** [lv_slider.h](https://docs.lvgl.io/master/API/widgets/slider/lv_slider_h.html)

**Части:** `LV_PART_MAIN`, `LV_PART_INDICATOR`, `LV_PART_KNOB`

**Функции:**
- `lv_slider_set_value(slider, val, LV_ANIM_ON/OFF)`
- `lv_slider_set_range(slider, min, max)`
- `lv_slider_set_orientation(slider, LV_SLIDER_ORIENTATION_...)`

**События:** `LV_EVENT_VALUE_CHANGED`, `LV_EVENT_RELEASED`

---

### 2.6 Drop-Down List (lv_dropdown)

Выпадающий список выбора.

- **Документация:** [Drop-Down List (lv_dropdown)](https://docs.lvgl.io/master/widgets/dropdown.html)
- **API:** [lv_dropdown.h](https://docs.lvgl.io/master/API/widgets/dropdown/lv_dropdown_h.html)

**Функции:**
- `lv_dropdown_set_options(dd, "Opt1\nOpt2\nOpt3")`
- `lv_dropdown_set_selected(dd, index)`
- `lv_dropdown_get_selected(dd)`
- `lv_dropdown_get_selected_str(dd, buf, size)`
- `lv_dropdown_set_dir(dd, LV_DIR_BOTTOM/TOP/LEFT/RIGHT)`

**События:** `LV_EVENT_VALUE_CHANGED`, `LV_EVENT_CANCEL`, `LV_EVENT_READY`

---

### 2.7 Switch (lv_switch)

Переключатель вкл/выкл.

- **Документация:** [Switch (lv_switch)](https://docs.lvgl.io/master/widgets/switch.html)

---

### 2.8 Checkbox (lv_checkbox)

Чекбокс.

- **Документация:** [Checkbox (lv_checkbox)](https://docs.lvgl.io/master/widgets/checkbox.html)

---

### 2.9 Bar (lv_bar)

Полоса (прогресс, индикатор). Без ползунка, в отличие от Slider.

- **Документация:** [Bar (lv_bar)](https://docs.lvgl.io/master/widgets/bar.html)

---

### 2.10 Table (lv_table)

Таблица ячеек.

- **Документация:** [Table (lv_table)](https://docs.lvgl.io/master/widgets/table.html)

---

### 2.11 Roller (lv_roller)

Вращающийся список (как колёсико выбора).

- **Документация:** [Roller (lv_roller)](https://docs.lvgl.io/master/widgets/roller.html)

---

### 2.12 Image (lv_image)

Изображение.

- **Документация:** [Image (lv_image)](https://docs.lvgl.io/master/widgets/image.html)

---

### 2.13 Canvas (lv_canvas)

Холст для рисования.

- **Документация:** [Canvas (lv_canvas)](https://docs.lvgl.io/master/widgets/canvas.html)

---

### 2.14 Остальные виджеты

| Виджет | Документация |
|--------|--------------|
| Arc | [Arc (lv_arc)](https://docs.lvgl.io/master/widgets/arc.html) |
| Button Matrix | [Button Matrix (lv_buttonmatrix)](https://docs.lvgl.io/master/widgets/buttonmatrix.html) |
| Calendar | [Calendar (lv_calendar)](https://docs.lvgl.io/master/widgets/calendar.html) |
| Chart | [Chart (lv_chart)](https://docs.lvgl.io/master/widgets/chart.html) |
| Image Button | [Image Button (lv_imagebutton)](https://docs.lvgl.io/master/widgets/imagebutton.html) |
| Keyboard | [Keyboard (lv_keyboard)](https://docs.lvgl.io/master/widgets/keyboard.html) |
| LED | [LED (lv_led)](https://docs.lvgl.io/master/widgets/led.html) |
| Line | [Line (lv_line)](https://docs.lvgl.io/master/widgets/line.html) |
| Menu | [Menu (lv_menu)](https://docs.lvgl.io/master/widgets/menu.html) |
| Message Box | [Message Box (lv_msgbox)](https://docs.lvgl.io/master/widgets/msgbox.html) |
| Scale | [Scale (lv_scale)](https://docs.lvgl.io/master/widgets/scale.html) |
| Spinbox | [Spinbox (lv_spinbox)](https://docs.lvgl.io/master/widgets/spinbox.html) |
| Spinner | [Spinner (lv_spinner)](https://docs.lvgl.io/master/widgets/spinner.html) |
| Tab View | [Tab View (lv_tabview)](https://docs.lvgl.io/master/widgets/tabview.html) |
| Text Area | [Text Area (lv_textarea)](https://docs.lvgl.io/master/widgets/textarea.html) |
| Tile View | [Tile View (lv_tileview)](https://docs.lvgl.io/master/widgets/tileview.html) |
| Window | [Window (lv_win)](https://docs.lvgl.io/master/widgets/win.html) |

---

## 3. Input devices (ввод)

### Touchpad / Mouse (Pointer)

- **Документация:** [Touchpad and Mouse](https://docs.lvgl.io/master/main-modules/indev/pointer.html)
- **Обзор:** [Input devices (lv_indev)](https://docs.lvgl.io/master/main-modules/indev/index.html)

**Тип:** `LV_INDEV_TYPE_POINTER`  
**Callback:** возвращает `point.x`, `point.y`, `state` (PRESSED/RELEASED)

**Параметры:**
- `lv_indev_set_long_press_time(indev, ms)`
- `lv_indev_set_scroll_limit(indev, px)`
- `lv_indev_set_scroll_throw(indev, percent)`
- `lv_obj_set_ext_click_area(widget, size)` — расширить зону клика

### Keypad / Encoder / Button

- [Keypad and Keyboard](https://docs.lvgl.io/master/main-modules/indev/keypad.html)
- [Encoder](https://docs.lvgl.io/master/main-modules/indev/encoder.html)
- [Hardware Button](https://docs.lvgl.io/master/main-modules/indev/button.html)
- [Groups](https://docs.lvgl.io/master/main-modules/indev/groups.html)

---

## 4. Скролл

- **Документация:** [Scrolling](https://docs.lvgl.io/master/common-widget-features/scrolling.html)

**Режимы скроллбара:** `LV_SCROLLBAR_MODE_OFF`, `ON`, `ACTIVE`, `AUTO`  
**Часть скроллбара:** `LV_PART_SCROLLBAR`  
**Состояние при скролле:** `LV_STATE_SCROLLED`

**Флаги:** `LV_OBJ_FLAG_SCROLL_MOMENTUM`, `LV_OBJ_FLAG_SCROLL_ELASTIC`, `LV_OBJ_FLAG_SNAPPABLE`

---

## 5. Стили и состояния

### Parts (части)

- `LV_PART_MAIN` — основной фон
- `LV_PART_SCROLLBAR` — скроллбар
- `LV_PART_INDICATOR` — индикатор (slider, bar)
- `LV_PART_KNOB` — ползунок (slider)
- `LV_PART_SELECTED` — выбранный элемент

**Документация:** [Parts and States](https://docs.lvgl.io/master/common-widget-features/parts_and_states.html)

### States (состояния)

- `LV_STATE_DEFAULT` — обычное
- `LV_STATE_PRESSED` — нажато
- `LV_STATE_CHECKED` — отмечено
- `LV_STATE_FOCUSED` — в фокусе
- `LV_STATE_DISABLED` — отключено
- `LV_STATE_SCROLLED` — при скролле

### Стили

- **Обзор:** [Styles Overview](https://docs.lvgl.io/master/common-widget-features/styles/overview.html)
- **Свойства:** [Style Properties](https://docs.lvgl.io/master/common-widget-features/styles/style-properties.html)

```c
static lv_style_t style;
lv_style_init(&style);
lv_style_set_bg_color(&style, lv_color_hex(0xb0b0b0));
lv_obj_add_style(obj, &style, LV_PART_MAIN | LV_STATE_DEFAULT);
```

---

## 6. События

- **Документация:** [Events](https://docs.lvgl.io/master/common-widget-features/events.html)

**Основные коды:**
- `LV_EVENT_CLICKED` — клик
- `LV_EVENT_VALUE_CHANGED` — изменение значения
- `LV_EVENT_PRESSED` / `LV_EVENT_RELEASED`
- `LV_EVENT_LONG_PRESSED` / `LV_EVENT_LONG_PRESSED_REPEAT`
- `LV_EVENT_SCROLL` / `LV_EVENT_SCROLL_BEGIN` / `LV_EVENT_SCROLL_END`

```c
lv_obj_add_event_cb(btn, my_cb, LV_EVENT_CLICKED, NULL);
lv_event_code_t code = lv_event_get_code(e);
lv_obj_t* target = lv_event_get_target_obj(e);
```

---

## 7. API v8 → v9 (совместимость)

Проект TamaFi использует **LVGL v8**. Карта совместимости:

- [lv_api_map_v8.h](https://docs.lvgl.io/master/API/lv_api_map_v8_h.html)

**Примеры отличий:**
- v8: `lv_list_add_btn` → v9: `lv_list_add_button`
- v8: `lv_scr_load` → v9: `lv_screen_load`
- v8: `lv_disp_drv_t`, `lv_disp_draw_buf_t` → v9: `lv_display_t`, `lv_display_set_buffers`

---

## 8. Полезные ссылки

| Раздел | URL |
|--------|-----|
| Главная | https://docs.lvgl.io/master/ |
| Getting Started | https://docs.lvgl.io/master/getting_started/index.html |
| Learn the Basics | https://docs.lvgl.io/master/getting_started/learn_the_basics.html |
| Integration Overview | https://docs.lvgl.io/master/integration/overview.html |
| Arduino | https://docs.lvgl.io/master/integration/frameworks/arduino.html |
| All Widgets | https://docs.lvgl.io/master/widgets/index.html |
| Common Widget Features | https://docs.lvgl.io/master/common-widget-features/index.html |
| Display (lv_display) | https://docs.lvgl.io/master/main-modules/display/index.html |
| Input devices | https://docs.lvgl.io/master/main-modules/indev/index.html |
| API Reference | https://docs.lvgl.io/master/API/index.html |
