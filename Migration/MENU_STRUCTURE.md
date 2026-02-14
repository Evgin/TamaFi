# Структура меню и навигация TamaFi

## Карта экранов

```
BOOT ──(любая кнопка)──┬──(hasHatchedOnce)──> HOME
                       └──(первый запуск)───> HATCH ──(OK → анимация)──> HOME

HOME ──(OK)──> MENU
HOME ──(R1)──> Pet Status     (быстрый доступ)

MENU (Main Menu)
 ├─ 0. Pet Status      ──(OK)──> назад в MENU
 ├─ 1. System Info     ──(OK)──> назад в MENU
 ├─ 2. Settings        ──(OK на пункте / Back)──> назад в MENU
 └─ 3. Back            ──(OK)──> HOME

GAMEOVER ──(OK)──> полный сброс ──> HATCH
```

## Экраны

### BOOT
- Приветственный экран "TamaFi v2"
- Любая кнопка (UP / OK / DOWN) → HOME или HATCH (если питомец ещё не вылуплялся)

### HATCH
- Анимация яйца (idle-покачивание)
- OK → запуск анимации вылупления → переход на HOME
- Показывается только при первом запуске или после полного сброса

### HOME
- Основной экран: спрайт питомца, бары hunger/happiness/health, mood, stage
- Отображение текущей активности (Hunting, Discovering, Resting, Idle)
- Эффект голода (overlay) при успешной/неудачной охоте
- **Управление:**
  - OK → Main Menu
  - R1 → Pet Status (быстрый доступ)

### MENU (Main Menu)
- 4 пункта, навигация UP/DOWN, выбор OK
- Пункты: Pet Status, System Info, Settings, Back

### Pet Status
- Stage, Age (дни/часы/минуты)
- Hunger %, Happiness %, Health %
- Mood
- Personality: Curiosity, Activity, Stress
- **Навигация:** OK → назад в MENU; R1 → HOME (из быстрого доступа)

### System Info
- Firmware version, MCU
- Heap Free (KB)
- Uptime (HH:MM:SS)
- WiFi Scan status
- Battery: заряд % (напряжение mV), Charging, USB
- WiFi Environment: Networks, Strong, Hidden, Open, WPA, Avg RSSI
- **Навигация:** OK → назад в MENU

### Settings
- 8 пунктов, навигация UP/DOWN, выбор/переключение OK:
  - **Brightness** — Low / Mid / High (циклически)
  - **Sound** — 3 / 2 / 1 / Off (4 уровня громкости, циклически 3→2→1→Off→3)
  - **Pet** — Golem / Dragon / Robot / Other (выбор скина питомца, циклически)
  - **Auto Sleep** — On / Off (toggle)
  - **Auto Save** — 15s / 30s / 60s (циклически)
  - **Reset Pet** — сброс hunger/happiness/health (stage и возраст сохраняются)
  - **Reset All** — полный сброс (stats + age + stage + traits) → HATCH
  - **Back** → MENU

### Game Over
- Анимация смерти питомца
- OK → полный сброс → HATCH

## Ввод

| Кнопка | Источник | Действие |
|--------|----------|----------|
| UP     | Touch (верхняя зона) / BOOT btn | Навигация вверх в меню |
| OK     | Touch (центральная зона) | Выбор / подтверждение |
| DOWN   | Touch (нижняя зона) | Навигация вниз в меню |
| R1     | Touch (правая зона 1) / PWR btn | Быстрый доступ: Pet Status / возврат HOME |

## Звуковая обратная связь

- UP / DOWN → `sndBeep()` (короткий тон)
- OK → `sndBeepOk()` (подтверждающий тон)
- Переходы между экранами → `sndClick()` (щелчок)
- Hatch → `sndHatch()` (мелодия вылупления)

## Громкость звука

4 уровня, переключаются в Settings → Sound:
- **3** — полная громкость (ES8311 vol=72, amplitude=2500)
- **2** — средняя (ES8311 vol=50, amplitude=1700)
- **1** — тихая (ES8311 vol=30, amplitude=900)
- **Off** — звук выключен (mute)

Уровень сохраняется в NVS (`sndVol`).

## Скин питомца

Переключается в Settings → Pet. Доступные варианты:
- Golem (по умолчанию)
- Dragon
- Robot
- Other

Выбор сохраняется в NVS (`petSkin`). Подключение ассетов под скины — в будущем.
