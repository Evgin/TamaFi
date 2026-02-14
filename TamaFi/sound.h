#pragma once

#include <Arduino.h>

// Initialize sound system: ES8311 (I2S) + PWM buzzer fallback.
// Returns true if ES8311 is available.
bool soundInit();

// Sequencer step — call every loop() to advance multi-tone sequences.
void sndUpdate();

// Feed I2S buffer — call every loop(), multiple times. Returns true while playing.
bool soundFeed();

// Stop PWM buzzer if tone duration expired.
void stopBuzzerIfNeeded();

// Stop all sound output immediately (buzzer + ES8311).
void soundStopAll();

// Apply volume level (0=mute, 1-3). Adjusts ES8311 HW volume + tone amplitude.
void soundSetVolume(uint8_t level);

// ---------- Sound effects (retro multi-tone sequences) ----------

void sndClick();
void sndGoodFeed();
void sndBadFeed();
void sndDiscover();
void sndRestStart();
void sndRestEnd();
void sndHatch();

// ---------- Simple beeps (button feedback) ----------

void sndBeep();
void sndBeepOk();
