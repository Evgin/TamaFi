#include "sound.h"
#include "sound_es8311.h"
#include "device_config.h"
#include "navigation.h"          // for soundVolume extern

// ESP32 Arduino 3.x: ledcWriteTone(pin, freq); older: ledcWriteTone(channel, freq)
#if defined(ESP_ARDUINO_VERSION) && ESP_ARDUINO_VERSION >= 0x030000
  #define BUZZER_LEDC_TARGET  BUZZER_PIN
#else
  #define BUZZER_LEDC_TARGET  BUZZER_CH
#endif

// ============ PWM buzzer core ============

static unsigned long buzzerEndTime = 0;

void stopBuzzerIfNeeded() {
    if (buzzerEndTime == 0) return;
    if (millis() > buzzerEndTime) {
        ledcWriteTone(BUZZER_LEDC_TARGET, 0);
        buzzerEndTime = 0;
    }
}

static void buzzerPlay(int freq, int durMs) {
    if (soundVolume == 0) return;
    ledcWriteTone(BUZZER_LEDC_TARGET, freq);
    buzzerEndTime = millis() + durMs;
}

// ============ Ultra-Retro sequencer ============

struct RetroSound {
    const int *freqs;
    const int *times;
    int length;
};

static int  sndIndex = -1;
static int  sndStep  = 0;
static unsigned long sndNext = 0;

// --- Tone tables ---

static const int CLICK_FREQS[] = { 2100, 1600, 900 };
static const int CLICK_TIMES[] = {   20,   20,  20 };
static RetroSound SND_CLICK = { CLICK_FREQS, CLICK_TIMES, 3 };

static const int GOOD_FREQS[] = { 600, 900, 1200, 1500 };
static const int GOOD_TIMES[] = {  40,  40,   40,   60 };
static RetroSound SND_GOOD = { GOOD_FREQS, GOOD_TIMES, 4 };

static const int BAD_FREQS[] = { 900, 700, 500, 300 };
static const int BAD_TIMES[] = {  50,  50,  60,  80 };
static RetroSound SND_BAD = { BAD_FREQS, BAD_TIMES, 4 };

static const int DISC_FREQS[] = { 400, 650, 900, 1200, 1500 };
static const int DISC_TIMES[] = {  40,  40,  40,   40,   60 };
static RetroSound SND_DISC = { DISC_FREQS, DISC_TIMES, 5 };

static const int REST_START_FREQS[] = { 600, 400, 300 };
static const int REST_START_TIMES[] = {  60,  70,  90 };
static RetroSound SND_REST_START = { REST_START_FREQS, REST_START_TIMES, 3 };

static const int REST_END_FREQS[] = { 300, 500, 700 };
static const int REST_END_TIMES[] = {  60,  60,  80 };
static RetroSound SND_REST_END = { REST_END_FREQS, REST_END_TIMES, 3 };

static const int HATCH_FREQS[] = { 500, 800, 1200, 1600, 2000 };
static const int HATCH_TIMES[] = {  60,  60,   60,   80,  100 };
static RetroSound SND_HATCH = { HATCH_FREQS, HATCH_TIMES, 5 };

// --- Sequencer lookup ---

static const RetroSound* sndLookup(int idx) {
    switch (idx) {
        case 0: return &SND_CLICK;
        case 1: return &SND_GOOD;
        case 2: return &SND_BAD;
        case 3: return &SND_DISC;
        case 4: return &SND_REST_START;
        case 5: return &SND_REST_END;
        case 6: return &SND_HATCH;
        default: return nullptr;
    }
}

// ============ Public API ============

bool soundInit() {
    if (soundEs8311Init()) {
        return true;
    }

    // Fallback: PWM buzzer
#if defined(ESP_ARDUINO_VERSION) && ESP_ARDUINO_VERSION >= 0x030000
    ledcAttach(BUZZER_PIN, 4000, 8);
#else
    ledcSetup(BUZZER_CH, 4000, 8);
    ledcAttachPin(BUZZER_PIN, BUZZER_CH);
#endif
    ledcWriteTone(BUZZER_LEDC_TARGET, 0);
    return false;
}

void sndUpdate() {
    if (soundVolume == 0) {
        ledcWriteTone(BUZZER_LEDC_TARGET, 0);
        soundEs8311Stop();
        sndIndex = -1;
        sndStep  = 0;
        return;
    }

    if (sndIndex < 0) return;

    // --- ES8311 (I2S) path ---
    if (soundEs8311Available()) {
        if (soundEs8311IsPlaying()) return;

        const RetroSound *snd = sndLookup(sndIndex);
        if (!snd) { sndIndex = -1; sndStep = 0; return; }

        if (sndStep >= snd->length) {
            soundEs8311Stop();
            sndIndex = -1;
            sndStep  = 0;
            return;
        }
        soundEs8311SetTone(snd->freqs[sndStep], snd->times[sndStep]);
        sndStep++;
        return;
    }

    // --- PWM buzzer path ---
    unsigned long now = millis();
    if (now >= sndNext) {
        const RetroSound *snd = sndLookup(sndIndex);
        if (!snd) { sndIndex = -1; sndStep = 0; return; }

        if (sndStep >= snd->length) {
            ledcWriteTone(BUZZER_LEDC_TARGET, 0);
            sndIndex = -1;
            sndStep  = 0;
            return;
        }
        ledcWriteTone(BUZZER_LEDC_TARGET, snd->freqs[sndStep]);
        sndNext = now + snd->times[sndStep];
        sndStep++;
    }
}

bool soundFeed() {
    return soundEs8311Feed();
}

void soundStopAll() {
    ledcWriteTone(BUZZER_LEDC_TARGET, 0);
    soundEs8311Stop();
    buzzerEndTime = 0;
    sndIndex = -1;
    sndStep  = 0;
}

void soundSetVolume(uint8_t level) {
    // ES8311 hardware volume + tone amplitude mapping
    // 1 = тихий (бывший «3»), 2 = средний, 3 = громкий
    switch (level) {
        case 3:  soundEs8311SetHwVolume(100); soundEs8311SetAmplitude(8000); break;
        case 2:  soundEs8311SetHwVolume(88);  soundEs8311SetAmplitude(5000); break;
        case 1:  soundEs8311SetHwVolume(72);  soundEs8311SetAmplitude(2500); break;
        default: soundEs8311SetHwVolume(0);   soundEs8311SetAmplitude(0);    break;
    }
}

// --- Start sequence helpers ---

static void sndStart(int idx) {
    if (soundVolume == 0) return;
    sndNext  = 0;
    sndIndex = idx;
    sndStep  = 0;
}

void sndClick()     { sndStart(0); }
void sndGoodFeed()  { sndStart(1); }
void sndBadFeed()   { sndStart(2); }
void sndDiscover()  { sndStart(3); }
void sndRestStart() { sndStart(4); }
void sndRestEnd()   { sndStart(5); }
void sndHatch()     { sndStart(6); }

void sndBeep() {
    if (soundVolume == 0) return;
    if (soundEs8311Available()) {
        soundEs8311SetTone(800, 100);
    } else {
        buzzerPlay(800, 100);
    }
}

void sndBeepOk() {
    if (soundVolume == 0) return;
    if (soundEs8311Available()) {
        soundEs8311SetTone(1000, 100);
    } else {
        buzzerPlay(1000, 100);
    }
}
