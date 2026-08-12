#pragma once

#include "skin_types.h"

// Gorgon skin: idle from GorgonIdle.h (5 frames, 128x128), rest from Golem.
// GorgonIdle.h is included by skin_registry.cpp before this header.
extern const unsigned short gorgon_idle_2[];
extern const unsigned short egg_hatch_1[], egg_hatch_2[], egg_hatch_3[], egg_hatch_4[], egg_hatch_5[];
extern const unsigned short egg_hatch_11[], egg_hatch_21[], egg_hatch_31[], egg_hatch_41[];
extern const uint16_t attack_0[], attack_1[], attack_2[];
extern const unsigned short dead_1[], dead_2[], dead_3[];
extern const unsigned short hunger1[], hunger2[], hunger3[], hunger4[];

// Idle_2 sprite sheet: 640x128, 5 frames of 128x128 arranged horizontally.
// Use idleSpriteSheet — frames are NOT contiguous (row-major for full sheet).
static const uint16_t* GORGON_IDLE[] = { nullptr };  // unused when idleSpriteSheet set
static const uint16_t* GORGON_EGG_IDLE[] = { egg_hatch_11, egg_hatch_21, egg_hatch_31, egg_hatch_41 };
static const uint16_t* GORGON_EGG_HATCH[]= { egg_hatch_1, egg_hatch_2, egg_hatch_3, egg_hatch_4, egg_hatch_5 };
static const uint16_t* GORGON_ATTACK[]   = { attack_0, attack_1, attack_2 };
static const uint16_t* GORGON_DEAD[]     = { dead_1, dead_2, dead_3 };
static const uint16_t* GORGON_HUNGER[]   = { hunger1, hunger2, hunger3, hunger4 };

static const SkinDescriptor skinGorgon = {
    .idleFrames      = GORGON_IDLE,
    .eggIdleFrames   = GORGON_EGG_IDLE,
    .eggHatchFrames  = GORGON_EGG_HATCH,
    .attackFrames    = GORGON_ATTACK,
    .deadFrames      = GORGON_DEAD,
    .hungerFrames    = GORGON_HUNGER,
    .idleFrameCount  = 5,
    .petFrameW       = 128,
    .petFrameH       = 128,
    .effectFrameW    = 0,
    .effectFrameH    = 0,
    .idleSpriteSheet   = gorgon_idle_2,
    .idleSheetW        = 640,
    .idleSheetH        = 128,
    .legacyPetFrameW   = 115,
    .legacyPetFrameH   = 110,
    .petDisplayScale   = 150,   // 150% — пропорционально больше
};
