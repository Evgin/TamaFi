#pragma once

#include "pet_logic.h"

// Переход в Deep Sleep: сохранение в NVS, выключение устройства.
// Пробуждение только по BOOT (GPIO0). Не возвращается — перезагрузка при wake.
void deviceEnterSleep(PetState &petState);
