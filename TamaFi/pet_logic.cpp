#include "pet_logic.h"

// ============ Behavioral timing constants ============

static const unsigned long HUNGER_TICK_MS      = 5000;
static const unsigned long HAPPINESS_TICK_MS   = 7000;
static const unsigned long HEALTH_TICK_MS      = 10000;
static const unsigned long AGE_TICK_MS         = 60000;

static const unsigned long HUNGER_EFFECT_DELAY_MS = 100;
static const int           HUNGER_FRAME_MAX       = 4;

static const unsigned long REST_ENTER_DELAY_MS = 400;
static const unsigned long REST_WAKE_DELAY_MS  = 400;
static const unsigned long REST_MIN_MS         = 5000;
static const unsigned long REST_MAX_MS         = 15000;

static const uint32_t DECISION_INTERVAL_MIN = 8000;
static const uint32_t DECISION_INTERVAL_MAX = 15000;

// ============ Queue helpers ============

static void pushEvent(PetState &s, PetEvent evt) {
    uint8_t next = (s.evtHead + 1) % PET_QUEUE_SIZE;
    if (next == s.evtTail) return;          // full — drop oldest-style protection
    s.evtQueue[s.evtHead] = evt;
    s.evtHead = next;
}

static void pushCommand(PetState &s, PetCommand cmd) {
    uint8_t next = (s.cmdHead + 1) % PET_QUEUE_SIZE;
    if (next == s.cmdTail) return;
    s.cmdQueue[s.cmdHead] = cmd;
    s.cmdHead = next;
}

static PetCommand pollCommand(PetState &s) {
    if (s.cmdHead == s.cmdTail) return PET_CMD_NONE;
    PetCommand cmd = s.cmdQueue[s.cmdTail];
    s.cmdTail = (s.cmdTail + 1) % PET_QUEUE_SIZE;
    return cmd;
}

// ============ Internal: reset stats ============

static void resetStats(PetState &s, bool full, unsigned long now) {
    s.pet.hunger    = 70;
    s.pet.happiness = 70;
    s.pet.health    = 70;

    if (full) {
        s.pet.ageMinutes = 0;
        s.pet.ageHours   = 0;
        s.pet.ageDays    = 0;
        s.stage          = STAGE_BABY;
        s.traitCuriosity = random(40, 90);
        s.traitActivity  = random(30, 90);
        s.traitStress    = random(20, 80);
    }

    s.lastWifi         = WifiStats();
    s.lastWifiScanTime = 0;
    s.wifiResultReady  = false;

    s.activity          = ACT_NONE;
    s.restPhase         = REST_NONE;
    s.hungerEffectActive = false;
    s.isDead            = false;

    s.hungerTimer    = now;
    s.happinessTimer = now;
    s.healthTimer    = now;
    s.ageTimer       = now;
    s.lastDecisionTime = now;
}

// ============ Internal: mood ============

static void updateMood(PetState &s, unsigned long now) {
    if (s.pet.health < 25 ||
        (s.lastWifi.netCount == 0 && s.lastWifiScanTime > 0 &&
         now - s.lastWifiScanTime > 60000)) {
        s.mood = MOOD_SICK;
        return;
    }

    if (s.pet.hunger < 25) {
        s.mood = MOOD_HUNGRY;
        return;
    }

    if (s.pet.happiness > 80 && s.lastWifi.netCount > 8) {
        s.mood = MOOD_EXCITED;
        return;
    }

    if (s.pet.happiness > 60 && s.lastWifi.netCount > 0) {
        s.mood = MOOD_HAPPY;
        return;
    }

    if (s.lastWifi.netCount == 0 && now - s.lastWifiScanTime > 30000) {
        s.mood = MOOD_BORED;
        return;
    }

    if (s.lastWifi.hiddenCount > 0 || s.lastWifi.openCount > 0) {
        s.mood = MOOD_CURIOUS;
        return;
    }

    s.mood = MOOD_CALM;
}

// ============ Internal: evolution ============

static void updateEvolution(PetState &s) {
    unsigned long a = s.pet.ageMinutes;
    int avg = (s.pet.hunger + s.pet.happiness + s.pet.health) / 3;

    if (a >= 180 && avg > 40 && s.stage < STAGE_ELDER) {
        s.stage = STAGE_ELDER;
        pushEvent(s, PET_EVT_EVOLUTION);
    } else if (a >= 60 && avg > 45 && s.stage < STAGE_ADULT) {
        s.stage = STAGE_ADULT;
        pushEvent(s, PET_EVT_EVOLUTION);
    } else if (a >= 20 && avg > 35 && s.stage < STAGE_TEEN) {
        s.stage = STAGE_TEEN;
        pushEvent(s, PET_EVT_EVOLUTION);
    }
}

// ============ Internal: rest state machine ============

static void stepRest(PetState &s, unsigned long now) {
    if (s.activity != ACT_REST || s.restPhase == REST_NONE) return;

    switch (s.restPhase) {
        case REST_ENTER:
            if (now - s.lastRestAnimTime >= REST_ENTER_DELAY_MS) {
                s.lastRestAnimTime = now;
                if (s.restFrameIndex > 0) {
                    s.restFrameIndex--;
                } else {
                    s.restFrameIndex   = 0;
                    s.restPhase        = REST_DEEP;
                    s.restPhaseStart   = now;
                    s.restStatsApplied = false;
                }
            }
            break;

        case REST_DEEP:
            if (!s.restStatsApplied && now - s.restPhaseStart > s.restDurationMs / 2) {
                s.pet.hunger    = constrain(s.pet.hunger    - 3,  0, 100);
                s.pet.happiness = constrain(s.pet.happiness + 10, 0, 100);
                s.pet.health    = constrain(s.pet.health    + 15, 0, 100);
                s.restStatsApplied = true;
            }
            if (now - s.restPhaseStart >= s.restDurationMs) {
                s.restPhase        = REST_WAKE;
                s.restPhaseStart   = now;
                s.lastRestAnimTime = now;
                s.restFrameIndex   = 0;
                pushEvent(s, PET_EVT_REST_END);
            }
            break;

        case REST_WAKE:
            if (now - s.lastRestAnimTime >= REST_WAKE_DELAY_MS) {
                s.lastRestAnimTime = now;
                if (s.restFrameIndex < 4) {
                    s.restFrameIndex++;
                } else {
                    s.restFrameIndex = 4;
                    s.restPhase      = REST_NONE;
                    s.activity       = ACT_NONE;
                    pushEvent(s, PET_EVT_ACTIVITY_END);
                }
            }
            break;

        default:
            break;
    }
}

// ============ Internal: WiFi-based activities ============

static void resolveHunt(PetState &s, unsigned long now) {
    int n = s.lastWifi.netCount;
    int hungerDelta = 0, happyDelta = 0, healthDelta = 0;

    if (n == 0) {
        hungerDelta = -15;
        happyDelta  = -10;
        healthDelta = -5;
        pushEvent(s, PET_EVT_BAD_FEED);
    } else {
        hungerDelta = min(35, n * 2 + s.lastWifi.strongCount * 3);
        int varietyScore = s.lastWifi.hiddenCount * 2 + s.lastWifi.openCount;
        happyDelta = min(30, varietyScore * 3 + (s.lastWifi.avgRSSI + 100) / 3);

        if (s.lastWifi.avgRSSI > -75) healthDelta += 5;
        if (s.lastWifi.avgRSSI > -65) healthDelta += 5;
        if (s.lastWifi.strongCount > 5) healthDelta += 3;

        pushEvent(s, PET_EVT_GOOD_FEED);
    }

    s.pet.hunger    = constrain(s.pet.hunger    + hungerDelta, 0, 100);
    s.pet.happiness = constrain(s.pet.happiness + happyDelta,  0, 100);
    s.pet.health    = constrain(s.pet.health    + healthDelta, 0, 100);

    s.hungerEffectActive  = true;
    s.hungerEffectFrame   = 0;
    s.lastHungerFrameTime = now;
}

static void resolveDiscover(PetState &s) {
    int n = s.lastWifi.netCount;
    int happyDelta = 0, hungerDelta = 0;

    if (n == 0) {
        happyDelta  = -5;
        hungerDelta = -3;
        pushEvent(s, PET_EVT_BAD_FEED);
    } else {
        int curiosity = s.lastWifi.hiddenCount * 4 + s.lastWifi.openCount * 3;
        curiosity += s.lastWifi.netCount;
        happyDelta  = min(35, curiosity / 2);
        hungerDelta = -5;
        pushEvent(s, PET_EVT_DISCOVER);
    }

    s.pet.happiness = constrain(s.pet.happiness + happyDelta,  0, 100);
    s.pet.hunger    = constrain(s.pet.hunger    + hungerDelta, 0, 100);
}

// ============ Internal: autonomous decisions ============

static void decideNextActivity(PetState &s, unsigned long now) {
    if (s.activity != ACT_NONE || s.restPhase != REST_NONE) return;

    if (now - s.lastDecisionTime < s.currentDecisionInterval) return;

    s.lastDecisionTime = now;
    s.currentDecisionInterval = random(DECISION_INTERVAL_MIN, DECISION_INTERVAL_MAX);

    int desireHunt = 0;
    int desireDisc = 0;
    int desireRest = 0;
    int desireIdle = 10;

    desireHunt = (100 - s.pet.hunger) + s.traitCuriosity / 2;
    if (s.lastWifi.netCount == 0) desireHunt /= 2;

    desireDisc = s.traitCuriosity + s.lastWifi.hiddenCount * 10
                 + s.lastWifi.openCount * 6
                 + s.lastWifi.netCount * 2 + random(0, 20);
    if (s.lastWifi.netCount == 0) desireDisc /= 2;

    desireRest = (100 - s.pet.health) + s.traitStress / 2;
    if (s.pet.hunger < 20) desireRest -= 10;

    if (s.mood == MOOD_HUNGRY)  { desireHunt += 20; desireRest -= 10; }
    if (s.mood == MOOD_CURIOUS) { desireDisc += 15; }
    if (s.mood == MOOD_SICK)    { desireRest += 20; desireDisc -= 10; }
    if (s.mood == MOOD_EXCITED) { desireDisc += 10; desireHunt += 5;  }
    if (s.mood == MOOD_BORED)   { desireDisc += 10; desireHunt += 5;  }

    desireHunt = max(desireHunt, 0);
    desireDisc = max(desireDisc, 0);
    desireRest = max(desireRest, 0);
    desireIdle = max(desireIdle, 0);

    int best = desireIdle;
    Activity chosen = ACT_NONE;

    if (desireHunt > best) { best = desireHunt; chosen = ACT_HUNT;     }
    if (desireDisc > best) { best = desireDisc; chosen = ACT_DISCOVER; }
    if (desireRest > best) { best = desireRest; chosen = ACT_REST;     }

    if (chosen == ACT_NONE) return;

    if (chosen == ACT_HUNT || chosen == ACT_DISCOVER) {
        s.activity = chosen;
        pushEvent(s, PET_EVT_WIFI_REQUEST);
    } else if (chosen == ACT_REST) {
        s.activity         = ACT_REST;
        s.restPhase        = REST_ENTER;
        s.restFrameIndex   = 4;
        s.lastRestAnimTime = now;
        s.restPhaseStart   = now;
        s.restDurationMs   = random(REST_MIN_MS, REST_MAX_MS);
        s.restStatsApplied = false;
        pushEvent(s, PET_EVT_REST_START);
    }
}

// ============ Internal: process command queue ============

static void processCommands(PetState &s, unsigned long now) {
    PetCommand cmd;
    while ((cmd = pollCommand(s)) != PET_CMD_NONE) {
        switch (cmd) {
            case PET_CMD_RESET:
                resetStats(s, false, now);
                break;
            case PET_CMD_RESET_FULL:
                resetStats(s, true, now);
                break;
            default:
                break;
        }
    }
}

// ============ Public API ============

void petInit(PetState &s, unsigned long now) {
    s.pet.hunger     = 70;
    s.pet.happiness  = 70;
    s.pet.health     = 70;
    s.pet.ageMinutes = 0;
    s.pet.ageHours   = 0;
    s.pet.ageDays    = 0;

    s.stage    = STAGE_BABY;
    s.mood     = MOOD_CALM;
    s.activity = ACT_NONE;
    s.restPhase = REST_NONE;

    s.traitCuriosity = random(40, 90);
    s.traitActivity  = random(30, 90);
    s.traitStress    = random(20, 80);

    s.hungerTimer    = now;
    s.happinessTimer = now;
    s.healthTimer    = now;
    s.ageTimer       = now;

    s.restFrameIndex    = 0;
    s.lastRestAnimTime  = now;
    s.restPhaseStart    = now;
    s.restDurationMs    = 0;
    s.restStatsApplied  = false;

    s.hungerEffectActive  = false;
    s.hungerEffectFrame   = 0;
    s.lastHungerFrameTime = now;

    s.lastDecisionTime       = now;
    s.currentDecisionInterval = 10000;

    s.lastWifi         = WifiStats();
    s.lastWifiScanTime = 0;
    s.wifiResultReady  = false;

    s.isDead = false;

    s.cmdHead = s.cmdTail = 0;
    s.evtHead = s.evtTail = 0;
}

void petTick(PetState &s, unsigned long now, bool allowAutonomous) {
    // 1. Process commands
    processCommands(s, now);

    // 2. If dead, skip logic
    if (s.isDead) return;

    // 3. Hunger decay
    if (now - s.hungerTimer >= HUNGER_TICK_MS) {
        s.pet.hunger = max(0, s.pet.hunger - 2);
        s.hungerTimer = now;
    }

    // 4. Happiness decay
    if (now - s.happinessTimer >= HAPPINESS_TICK_MS) {
        if (s.lastWifi.netCount == 0 && (now - s.lastWifiScanTime) > 30000) {
            s.pet.happiness = max(0, s.pet.happiness - 3);
        } else {
            s.pet.happiness = max(0, s.pet.happiness - 1);
        }
        s.happinessTimer = now;
    }

    // 5. Health decay
    if (now - s.healthTimer >= HEALTH_TICK_MS) {
        if (s.pet.hunger < 20 || s.pet.happiness < 20) {
            s.pet.health = max(0, s.pet.health - 2);
        } else {
            s.pet.health = max(0, s.pet.health - 1);
        }
        s.healthTimer = now;
    }

    // 6. Age tick
    if (now - s.ageTimer >= AGE_TICK_MS) {
        s.pet.ageMinutes++;
        if (s.pet.ageMinutes >= 60) {
            s.pet.ageMinutes -= 60;
            s.pet.ageHours++;
        }
        if (s.pet.ageHours >= 24) {
            s.pet.ageHours -= 24;
            s.pet.ageDays++;
        }
        s.ageTimer = now;
    }

    // 7. Hunger effect animation
    if (s.hungerEffectActive && now - s.lastHungerFrameTime >= HUNGER_EFFECT_DELAY_MS) {
        s.lastHungerFrameTime = now;
        s.hungerEffectFrame++;
        if (s.hungerEffectFrame >= HUNGER_FRAME_MAX) {
            s.hungerEffectActive = false;
        }
    }

    // 8. WiFi-based activity resolution
    if ((s.activity == ACT_HUNT || s.activity == ACT_DISCOVER) && s.wifiResultReady) {
        if (s.activity == ACT_HUNT)         resolveHunt(s, now);
        else if (s.activity == ACT_DISCOVER) resolveDiscover(s);
        s.activity = ACT_NONE;
        s.wifiResultReady = false;
        pushEvent(s, PET_EVT_ACTIVITY_END);
    }

    // 9. Rest state machine
    stepRest(s, now);

    // 10. Mood & evolution
    updateMood(s, now);
    updateEvolution(s);

    // 11. Death check
    if (s.pet.hunger <= 0 && s.pet.happiness <= 0 && s.pet.health <= 0) {
        s.isDead    = true;
        s.activity  = ACT_NONE;
        s.restPhase = REST_NONE;
        pushEvent(s, PET_EVT_DEATH);
    }

    // 12. Autonomous decisions (only when allowed and idle)
    if (allowAutonomous && s.activity == ACT_NONE && s.restPhase == REST_NONE) {
        decideNextActivity(s, now);
    }
}

void petSendCommand(PetState &s, PetCommand cmd) {
    pushCommand(s, cmd);
}

void petFlushCommands(PetState &s, unsigned long now) {
    processCommands(s, now);
}

PetEvent petPollEvent(PetState &s) {
    if (s.evtHead == s.evtTail) return PET_EVT_NONE;
    PetEvent evt = s.evtQueue[s.evtTail];
    s.evtTail = (s.evtTail + 1) % PET_QUEUE_SIZE;
    return evt;
}

void petInjectWifiResult(PetState &s, const WifiStats &wifi, unsigned long now) {
    s.lastWifi         = wifi;
    s.lastWifiScanTime = now;
    s.wifiResultReady  = true;
}
