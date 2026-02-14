#pragma once

#include <Arduino.h>

// ============ Pet behavior enums ============

enum Stage {
    STAGE_BABY  = 0,
    STAGE_TEEN  = 1,
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

enum Activity {
    ACT_NONE,
    ACT_HUNT,
    ACT_DISCOVER,
    ACT_REST
};

enum RestPhase {
    REST_NONE,
    REST_ENTER,
    REST_DEEP,
    REST_WAKE
};

// ============ WiFi data types (used by pet logic and wifi_service) ============

struct WifiStats {
    int netCount    = 0;
    int strongCount = 0;
    int hiddenCount = 0;
    int avgRSSI     = -100;
    int openCount   = 0;
    int wpaCount    = 0;
};

const int MAX_WIFI_LIST = 12;

struct WifiNetworkInfo {
    String ssid;
    int    rssi;
    bool   isOpen;
};

// ============ Pet core data ============

struct Pet {
    int hunger;
    int happiness;
    int health;

    unsigned long ageMinutes;
    unsigned long ageHours;
    unsigned long ageDays;
};

// ============ Commands (UI -> pet) ============

enum PetCommand {
    PET_CMD_NONE = 0,
    PET_CMD_RESET,         // reset stats, keep age/stage/traits
    PET_CMD_RESET_FULL,    // full reset: stats + age + stage + re-randomize traits
};

// ============ Events (pet -> orchestrator) ============

enum PetEvent {
    PET_EVT_NONE = 0,
    PET_EVT_GOOD_FEED,     // resolveHunt success
    PET_EVT_BAD_FEED,      // resolveHunt/discover fail (no networks)
    PET_EVT_DISCOVER,      // resolveDiscover success
    PET_EVT_EVOLUTION,     // stage changed (BABY->TEEN->ADULT->ELDER)
    PET_EVT_REST_START,    // entering rest
    PET_EVT_REST_END,      // waking from rest
    PET_EVT_WIFI_REQUEST,  // pet wants a WiFi scan (hunt or discover)
    PET_EVT_DEATH,         // pet died (hunger+happiness+health == 0)
    PET_EVT_ACTIVITY_END,  // activity finished, back to idle
};

// ============ Ring buffer size ============

#define PET_QUEUE_SIZE 8

// ============ Complete pet state (all data, zero hardware deps) ============

struct PetState {
    // --- Core stats ---
    Pet       pet;
    Stage     stage;
    Mood      mood;
    Activity  activity;
    RestPhase restPhase;

    // --- Personality traits ---
    uint8_t traitCuriosity;
    uint8_t traitActivity;
    uint8_t traitStress;

    // --- Logic timers ---
    unsigned long hungerTimer;
    unsigned long happinessTimer;
    unsigned long healthTimer;
    unsigned long ageTimer;

    // --- Rest state machine ---
    int           restFrameIndex;
    unsigned long lastRestAnimTime;
    unsigned long restPhaseStart;
    unsigned long restDurationMs;
    bool          restStatsApplied;

    // --- Hunger effect overlay ---
    bool          hungerEffectActive;
    int           hungerEffectFrame;
    unsigned long lastHungerFrameTime;

    // --- Autonomous decision ---
    unsigned long lastDecisionTime;
    uint32_t      currentDecisionInterval;

    // --- WiFi data (injected by orchestrator) ---
    WifiStats     lastWifi;
    unsigned long lastWifiScanTime;
    bool          wifiResultReady;

    // --- Death flag ---
    bool isDead;

    // --- Command queue (input: UI -> pet) ---
    PetCommand cmdQueue[PET_QUEUE_SIZE];
    uint8_t    cmdHead;
    uint8_t    cmdTail;

    // --- Event queue (output: pet -> orchestrator) ---
    PetEvent   evtQueue[PET_QUEUE_SIZE];
    uint8_t    evtHead;
    uint8_t    evtTail;
};

// ============ Public API ============

// Initialize pet state with defaults.
void petInit(PetState &state, unsigned long now);

// Main logic tick. Call every ~100 ms.
// allowAutonomous: true when pet can make autonomous decisions (e.g. on HOME screen).
void petTick(PetState &state, unsigned long now, bool allowAutonomous);

// Send command from UI/navigation to pet (queued, processed in next petTick).
void petSendCommand(PetState &state, PetCommand cmd);

// Poll next event from pet. Returns PET_EVT_NONE when queue is empty.
PetEvent petPollEvent(PetState &state);

// Immediately process all pending commands (useful before saveState).
void petFlushCommands(PetState &state, unsigned long now);

// Inject WiFi scan result (called by orchestrator when wifi scan completes).
void petInjectWifiResult(PetState &state, const WifiStats &wifi, unsigned long now);
