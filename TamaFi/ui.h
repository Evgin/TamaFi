#pragma once
#include <Arduino.h>
#include "display_amoled.h"

// ============ Enums & structs shared between UI and main ============

void sndHatch();

// Позиция питомца на экране (определение в ui.cpp)
extern int petPosX;
extern int petPosY;

enum Screen {
  SCREEN_BOOT,
  SCREEN_HATCH,
  SCREEN_HOME,
  SCREEN_MENU,
  SCREEN_PET_STATUS,
  SCREEN_ENVIRONMENT,
  SCREEN_SYSINFO,
  SCREEN_CONTROLS,
  SCREEN_SETTINGS,
  SCREEN_DIAGNOSTICS,
  SCREEN_GAMEOVER
};

enum Activity {
  ACT_NONE,
  ACT_HUNT,
  ACT_DISCOVER,
  ACT_REST
};

enum Stage {
  STAGE_BABY = 0,
  STAGE_TEEN = 1,
  STAGE_ADULT = 2,
  STAGE_ELDER = 3
};

enum Mood {
  MOOD_HUNGRY,
  MOOD_HAPPY,
  MOOD_CURIOUS,
  MOOD_BORED,
  MOOD_SICK,
  MOOD_EXCITED,
  MOOD_CALM
};

enum RestPhase {
  REST_NONE,
  REST_ENTER,
  REST_DEEP,
  REST_WAKE
};

struct Pet {
  int hunger;
  int happiness;
  int health;

  unsigned long ageMinutes;
  unsigned long ageHours;
  unsigned long ageDays;
};

struct WifiStats {
  int netCount    = 0;
  int strongCount = 0;
  int hiddenCount = 0;
  int avgRSSI     = -100;
  int openCount   = 0;
  int wpaCount    = 0;
};

// Список WiFi‑сетей последнего сканирования (для экрана Environment)
const int MAX_WIFI_LIST = 12;

struct WifiNetworkInfo {
  String ssid;
  int    rssi;
  bool   isOpen;
};

// ============ Display: use getContentCanvas(), flushContentAndDrawControlBar(), drawSpriteToContent() ============

// ============ Shared game state (defined in TamaFi.ino) ============

extern Screen    currentScreen;
extern Activity  currentActivity;
extern RestPhase restPhase;

extern Pet       pet;
extern WifiStats wifiStats;

extern WifiNetworkInfo wifiList[MAX_WIFI_LIST];
extern int             wifiListCount;

extern Mood      currentMood;
extern Stage     petStage;

extern bool      hungerEffectActive;
extern int       hungerEffectFrame;
extern bool      hasHatchedOnce;

extern bool      wifiScanInProgress;
extern unsigned long lastWifiScanTime;
extern unsigned long lastSaveTime;

extern bool      soundEnabled;
extern uint8_t   tftBrightnessIndex;
extern bool      autoSleep;
extern uint16_t  autoSaveMs;

extern uint8_t   traitCuriosity;
extern uint8_t   traitActivity;
extern uint8_t   traitStress;

extern unsigned long lastDecisionTime;
extern uint32_t      currentDecisionInterval;

extern int       restFrameIndex;

// menus (main file owns, UI reads)
extern int       mainMenuIndex;
extern int       controlsIndex;
extern int       settingsMenuIndex;

// Hatch trigger: main sets true on OK, UI consumes it
extern bool      hatchTriggered;

// ============ UI API ============

void uiInit();                                      // Call in setup()
void uiOnScreenChange(Screen newScreen);            // Call whenever currentScreen changes
void uiDrawScreen(Screen screen,
                  int mainMenuIndex,
                  int controlsIndex,
                  int settingsMenuIndex);           // Call every loop after logic/buttons
