#include "device_sleep.h"
#include "device_config.h"
#include "display_amoled.h"
#include "sound.h"
#include "persistence.h"
#include "pet_logic.h"
#include <esp_sleep.h>
#include <Arduino.h>

void deviceEnterSleep(PetState &petState) {
    displaySleep();
    soundStopAll();
    soundSetVolume(0);

    // Применить отложенные команды перед сохранением
    petFlushCommands(petState, millis());
    saveState(petState);

    // Ждём отпускания BOOT, иначе проснёмся сразу
    while (digitalRead(BOOT_BTN_PIN) == LOW) { delay(10); }

    // ext0: один GPIO, level 0 = LOW (BOOT нажат). gpio_wakeup только для Light Sleep.
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BOOT_BTN_PIN, 0);
    esp_deep_sleep_start();   // не возвращается — перезагрузка
}
