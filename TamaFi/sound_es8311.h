#pragma once

#include <stdint.h>
#include <stdbool.h>

// Инициализация ES8311 + I2S (PA pin, I2S pins, кодек). Вызывать после Wire.begin().
// Возвращает true при успехе.
bool soundEs8311Init(void);

// Остановить вывод тона (тишина).
void soundEs8311Stop(void);

// Задать тон для неблокирующего воспроизведения: частота Hz, длительность ms.
// Реальное воспроизведение идёт в soundEs8311Feed(), вызываемом из loop().
void soundEs8311SetTone(int freqHz, int durationMs);

// Подать порцию данных в I2S (вызывать из loop). Возвращает true, пока идёт воспроизведение.
bool soundEs8311Feed(void);

// Идёт ли сейчас воспроизведение тона (нужно для пошаговых звуков).
bool soundEs8311IsPlaying(void);

// Звук через ES8311 доступен (инициализация прошла успешно).
bool soundEs8311Available(void);

// Установить громкость ES8311 (0-100, аппаратный регистр).
void soundEs8311SetHwVolume(int volume);

// Установить амплитуду тона (программная громкость генератора).
void soundEs8311SetAmplitude(int amplitude);
