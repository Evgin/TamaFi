#include "input.h"
#include "device_config.h"

#include <Wire.h>
#include <memory>

#include "Arduino_DriveBus_Library.h"

#define TCA9554_INPUT  0x00
#define TCA9554_OUTPUT 0x01
#define TCA9554_CONFIG 0x03   // TI TCA9554: 0=Input, 1=Output, 2=Polarity, 3=Configuration

static InputButton pendingEvent = INPUT_NONE;
static bool lastBoot = true;
static bool lastPwr = true;
static bool lastTouchActive = false;
static int lastTouchX = 0, lastTouchY = 0;
static unsigned long lastI2CPollMs = 0;
static const unsigned long I2C_POLL_INTERVAL_MS = 20;
static unsigned long lastActiveMs = 0;  // для детекции бездействия (AutoSleep)

// Двойной тап по центральной зоне (OK) => виртуальная кнопка R1
static unsigned long touchStartMs = 0;
static InputButton touchStartButton = INPUT_NONE;
static bool pendingOkTap = false;
static unsigned long pendingOkTapTime = 0;
static const unsigned long DOUBLE_OK_MS = 200;

// FT3168 через библиотеку (как в примере 02_Drawing_board)
static std::shared_ptr<Arduino_IIC_DriveBus> touchBus;
static std::unique_ptr<Arduino_FT3x68> touchFT3168;
static bool touchInited = false;

void Arduino_IIC_Touch_Interrupt(void) {
  if (touchFT3168) touchFT3168->IIC_Interrupt_Flag = true;
}

static const int CONTROL_STRIP_TOP = CONTENT_H;

static InputButton mapTouchToButton(int x, int y) {
  if (y < CONTROL_STRIP_TOP) return INPUT_NONE;
  const int thirdW = LCD_W / 3;
  if (x < thirdW)            return INPUT_UP;
  if (x < 2 * thirdW)        return INPUT_OK;
  return INPUT_DOWN;
}

// Как в 02_Drawing_board: по флагу прерывания читаем X,Y (не по количеству пальцев)
static bool readTouchOnInterrupt(int16_t& outX, int16_t& outY) {
  if (!touchInited || !touchFT3168) return false;
  if (!touchFT3168->IIC_Interrupt_Flag) return false;

  touchFT3168->IIC_Interrupt_Flag = false;
  int32_t x = (int32_t)touchFT3168->IIC_Read_Device_Value(Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
  int32_t y = (int32_t)touchFT3168->IIC_Read_Device_Value(Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);
  if (x < 0 || y < 0) return false;

  if (x > LCD_W || y > LCD_H) {
    x = (long)x * LCD_W / 4096;
    y = (long)y * LCD_H / 4096;
  }
  // Без инверсии Y: на этой панели низ экрана = высокий Y, полоса 368..448 внизу
  if (y < 0) y = 0;

  outX = (int16_t)x;
  outY = (int16_t)y;
  return true;
}

static bool pwrPresent = true;  // после первого NACK перестаём опрашивать PWR (меньше спама в логе)
static bool readPwr() {
  if (!pwrPresent) return false;
  Wire.beginTransmission(TCA9554_I2C_ADDR);
  Wire.write(TCA9554_INPUT);
  if (Wire.endTransmission(false) != 0) { pwrPresent = false; return false; }
  if (Wire.requestFrom((uint8_t)TCA9554_I2C_ADDR, (uint8_t)1) != 1) { pwrPresent = false; return false; }
  uint8_t b = Wire.read();
  return (b >> PWR_EXIO_BIT) & 1;
}

// Как в примере 04: расширитель TCA9554 пины 0,1,2 — LOW, пауза, HIGH (питание/сброс тача)
static void expanderInitForTouch() {
  Wire.beginTransmission(TCA9554_I2C_ADDR);
  Wire.write(TCA9554_CONFIG);
  Wire.write((uint8_t)0xF8);  // пины 0,1,2 = output
  if (Wire.endTransmission() != 0) return;
  Wire.beginTransmission(TCA9554_I2C_ADDR);
  Wire.write(TCA9554_OUTPUT);
  Wire.write((uint8_t)0x00);   // 0,1,2 LOW
  if (Wire.endTransmission() != 0) return;
  delay(20);
  Wire.beginTransmission(TCA9554_I2C_ADDR);
  Wire.write(TCA9554_OUTPUT);
  Wire.write((uint8_t)0x07);   // 0,1,2 HIGH
  if (Wire.endTransmission() != 0) return;
}

void inputInit() {
  Wire.begin(IIC_SDA, IIC_SCL);
  pinMode(BOOT_BTN_PIN, INPUT_PULLUP);
  lastBoot = (digitalRead(BOOT_BTN_PIN) == HIGH);
  lastPwr = !readPwr();

  lastActiveMs = millis();

  expanderInitForTouch();
  delay(50);

  touchBus = std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);
  touchFT3168 = std::make_unique<Arduino_FT3x68>(touchBus, FT3168_DEVICE_ADDRESS,
                                               DRIVEBUS_DEFAULT_VALUE, TP_INT, Arduino_IIC_Touch_Interrupt);
  if (touchFT3168->begin()) {
    touchFT3168->IIC_Write_Device_State(Arduino_IIC_Touch::Device::TOUCH_POWER_MODE,
                                        Arduino_IIC_Touch::Device_Mode::TOUCH_POWER_MONITOR);
    touchInited = true;
  }
}

bool inputTouchInited() { return touchInited; }
unsigned long inputLastActiveMs() { return lastActiveMs; }

void inputPoll() {
  if (pendingEvent != INPUT_NONE) return;

  unsigned long now = millis();

  // --- Опрос кнопки BOOT (GPIO0, LOW = pressed) ---
  bool bootNow = (digitalRead(BOOT_BTN_PIN) == LOW);
  if (bootNow && !lastBoot) {
    pendingEvent = INPUT_BOOT;
    lastActiveMs = now;
  }
  lastBoot = bootNow;
  if (pendingEvent != INPUT_NONE) return;   // BOOT сгенерировал событие — не читать тач

  if (now - lastI2CPollMs >= I2C_POLL_INTERVAL_MS) {
    lastI2CPollMs = now;
    // PWR не опрашиваем — кнопка не используется, TCA9554 может отсутствовать (NACK в логе)

    int16_t tx, ty;
    bool touchActive = readTouchOnInterrupt(tx, ty);
    if (touchActive) {
      lastActiveMs = now;            // любое касание в любой зоне = активность
      if (!lastTouchActive) {
        // Старт нового тапа — запоминаем кнопку и время
        InputButton b = mapTouchToButton(tx, ty);
        touchStartButton = b;
        touchStartMs = now;

        // Для UP/DOWN сразу генерим событие на нажатие
        if (b == INPUT_UP || b == INPUT_DOWN) {
          pendingEvent = b;
        }
      }
      lastTouchX = tx;
      lastTouchY = ty;
    } else {
      // Переход "палец был на экране -> убран"
      if (lastTouchActive && touchStartButton != INPUT_NONE) {
        if (touchStartButton == INPUT_OK) {
          // Логика одинарного/двойного тапа по OK:
          if (pendingOkTap && (now - pendingOkTapTime) <= DOUBLE_OK_MS) {
            // Второй тап в пределах окна — считаем двойным тапом
            pendingEvent = INPUT_R1;
            pendingOkTap = false;
          } else {
            // Первый тап: ждём возможный второй
            pendingOkTap = true;
            pendingOkTapTime = now;
          }
        }
        // Для UP/DOWN событие уже было сгенерировано на нажатие
        touchStartButton = INPUT_NONE;
      }
    }
    lastTouchActive = touchActive;

    // Если есть отложенный одиночный тап по OK и окно двойного тапа истекло —
    // генерируем обычный INPUT_OK (если ещё нет другого события).
    if (pendingOkTap && (now - pendingOkTapTime) > DOUBLE_OK_MS && pendingEvent == INPUT_NONE) {
      pendingEvent = INPUT_OK;
      pendingOkTap = false;
    }
  }
}

InputButton inputConsumeEvent() {
  InputButton e = pendingEvent;
  pendingEvent = INPUT_NONE;
  if (e != INPUT_NONE) lastActiveMs = millis();  // любое событие = активность
  return e;
}

void inputResetActivity() {
  lastActiveMs = millis();
}
