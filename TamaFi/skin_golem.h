#pragma once

#include "skin_types.h"
#include "pet_logic.h"
// Uses idle_1..4, egg_hatch_*, attack_*, dead_*, hunger* — defined by skin_registry.cpp

// Golem skin: idle_1..4, egg_hatch_1..5, egg_hatch_11..41, attack_0..2, dead_1..3, hunger1..4
static const uint16_t* GOLEM_IDLE[]     = { idle_1, idle_2, idle_3, idle_4 };
static const uint16_t* GOLEM_EGG_IDLE[] = { egg_hatch_11, egg_hatch_21, egg_hatch_31, egg_hatch_41 };
static const uint16_t* GOLEM_EGG_HATCH[]= { egg_hatch_1, egg_hatch_2, egg_hatch_3, egg_hatch_4, egg_hatch_5 };
static const uint16_t* GOLEM_ATTACK[]   = { attack_0, attack_1, attack_2 };
static const uint16_t* GOLEM_DEAD[]     = { dead_1, dead_2, dead_3 };
static const uint16_t* GOLEM_HUNGER[]   = { hunger1, hunger2, hunger3, hunger4 };

static const SkinDescriptor skinGolem = {
    .idleFrames      = GOLEM_IDLE,
    .eggIdleFrames   = GOLEM_EGG_IDLE,
    .eggHatchFrames  = GOLEM_EGG_HATCH,
    .attackFrames    = GOLEM_ATTACK,
    .deadFrames      = GOLEM_DEAD,
    .hungerFrames    = GOLEM_HUNGER,
    .idleFrameCount  = 4,
    .petFrameW       = 0,
    .petFrameH       = 0,
    .effectFrameW    = 0,
    .effectFrameH    = 0,
    .legacyPetFrameW = 0,
    .legacyPetFrameH = 0,
    .petDisplayScale = 0,   // 0 = 100% (default)
};
