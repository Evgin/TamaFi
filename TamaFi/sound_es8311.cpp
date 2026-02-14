/*
 * Звук через ES8311 (I2S) по примеру 15_ES8311.
 */

#include "sound_es8311.h"
#include "device_config.h"
#include "es8311.h"
#include <Wire.h>
#include <math.h>

// ESP_I2S.h — из пакета платы (WAVESHARE_ESP32_S3_1.8_AMOLED / MiiBestOD)
#include "ESP_I2S.h"

#define I2S_SAMPLE_RATE  16000
#define I2S_NUM_CH       2
#define TONE_BUF_SAMPLES 1024    // больше буфер — меньше прерываний при занятом loop()
static int toneAmplitude = 2500; // программная амплитуда (управляется через soundSetVolume)

static I2SClass i2s;
static void* es8311_handle = nullptr;
static bool inited = false;

// Неблокирующий тон: частота, конец по времени, сколько сэмплов уже выведено, всего нужно.
static int toneFreqHz = 0;
static unsigned long toneEndMs = 0;
static uint32_t toneSamplesWritten = 0;
static uint32_t toneSamplesTotal = 0;
static float tonePhase = 0.f;

bool soundEs8311Init(void) {
  if (inited) return true;

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);

  i2s.setPins(I2S_BCK_IO, I2S_WS_IO, I2S_DO_IO, I2S_DI_IO, I2S_MCK_IO);
  if (!i2s.begin(I2S_MODE_STD, I2S_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
    return false;
  }

  es8311_handle = es8311_create(0, (uint16_t)0x18u);
  if (!es8311_handle) return false;

  const es8311_clock_config_t clk = {
    .mclk_inverted = false,
    .sclk_inverted = false,
    .mclk_from_mclk_pin = true,
    .mclk_frequency = (int)(I2S_SAMPLE_RATE * 256),
    .sample_frequency = I2S_SAMPLE_RATE
  };

  if (es8311_init(es8311_handle, &clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16) != ESP_OK) {
    es8311_delete(es8311_handle);
    es8311_handle = nullptr;
    return false;
  }
  if (es8311_sample_frequency_config(es8311_handle, clk.mclk_frequency, clk.sample_frequency) != ESP_OK) {
    es8311_delete(es8311_handle);
    es8311_handle = nullptr;
    return false;
  }
  if (es8311_microphone_config(es8311_handle, false) != ESP_OK) {
    es8311_delete(es8311_handle);
    es8311_handle = nullptr;
    return false;
  }
  // Громкость умеренная (чтобы не «орало» и не клипило)
  es8311_voice_volume_set(es8311_handle, 72, nullptr);
  es8311_voice_mute(es8311_handle, false);

  inited = true;
  return true;
}

void soundEs8311Stop(void) {
  toneEndMs = 0;
  toneSamplesTotal = 0;
  toneSamplesWritten = 0;
  if (inited) {
    static int16_t silence[TONE_BUF_SAMPLES * 2];
    for (int i = 0; i < TONE_BUF_SAMPLES * 2; i++) silence[i] = 0;
    for (int k = 0; k < 4; k++)
      i2s.write((const uint8_t*)silence, sizeof(silence));
  }
}

void soundEs8311SetTone(int freqHz, int durationMs) {
  if (freqHz <= 0 || durationMs <= 0) return;
  toneFreqHz = freqHz;
  toneEndMs = millis() + (unsigned long)durationMs;
  toneSamplesTotal = (uint32_t)((long)I2S_SAMPLE_RATE * durationMs / 1000) * I2S_NUM_CH;
  toneSamplesWritten = 0;
  tonePhase = 0.f;
}

bool soundEs8311Feed(void) {
  if (!inited || toneEndMs == 0 || toneSamplesWritten >= toneSamplesTotal) {
    return false;
  }
  if (millis() >= toneEndMs) {
    toneEndMs = 0;
    return false;
  }

  const float omega = 6.28318530718f * (float)toneFreqHz / (float)I2S_SAMPLE_RATE;
  int16_t buf[TONE_BUF_SAMPLES * 2];
  int toGen = (int)(toneSamplesTotal - toneSamplesWritten);
  if (toGen > TONE_BUF_SAMPLES * I2S_NUM_CH)
    toGen = TONE_BUF_SAMPLES * I2S_NUM_CH;
  const int numStereoSamples = toGen / 2;

  // Лёгкое сглаживание в начале/конце тона (убирает щелчки и резкость)
  const int fadeLen = (numStereoSamples < 64) ? (numStereoSamples / 2) : 32;
  for (int i = 0; i < numStereoSamples; i++) {
    float gain = 1.0f;
    if (i < fadeLen)
      gain = (float)i / (float)fadeLen;
    else if (i >= numStereoSamples - fadeLen)
      gain = (float)(numStereoSamples - 1 - i) / (float)fadeLen;
    int16_t s = (int16_t)(toneAmplitude * gain * sinf(tonePhase));
    tonePhase += omega;
    buf[i * 2] = s;
    buf[i * 2 + 1] = s;
  }
  size_t wrote = i2s.write((const uint8_t*)buf, (size_t)toGen * 2);
  if (wrote > 0)
    toneSamplesWritten += (uint32_t)(wrote / 2);
  if (toneSamplesWritten >= toneSamplesTotal || millis() >= toneEndMs)
    toneEndMs = 0;
  return (toneEndMs != 0);
}

bool soundEs8311IsPlaying(void) {
  return (toneEndMs != 0);
}

bool soundEs8311Available(void) {
  return inited;
}

void soundEs8311SetHwVolume(int volume) {
  if (inited && es8311_handle) {
    es8311_voice_volume_set(es8311_handle, volume, nullptr);
  }
}

void soundEs8311SetAmplitude(int amplitude) {
  toneAmplitude = amplitude;
}
