#pragma once

#include <stdint.h>

// Descriptor for a pet skin. Each skin module (skin_golem.h, skin_gorgon.h)
// defines its own const SkinDescriptor instance.
typedef struct {
    // Pointers to frame arrays (in PROGMEM)
    const uint16_t** idleFrames;      // array of 4 or 5 pointers
    const uint16_t** eggIdleFrames;  // 4 frames
    const uint16_t** eggHatchFrames; // 5 frames
    const uint16_t** attackFrames;   // 3 frames
    const uint16_t** deadFrames;     // 3 frames
    const uint16_t** hungerFrames;   // 4 frames
    // Metadata
    int idleFrameCount;              // 4 or 5
    int petFrameW, petFrameH;        // 0 = native 115x110
    int effectFrameW, effectFrameH;  // 0 = native 100x95
    // Idle sprite sheet (when non-null, idleFrames ignored for drawing; frames horizontal)
    const uint16_t* idleSpriteSheet;
    int idleSheetW, idleSheetH;      // full sheet size, e.g. 640x128
    // Legacy anims (attack, dead, egg) — when non-zero, use for Golem-style 115x110 frames
    int legacyPetFrameW, legacyPetFrameH;
    // Display scale %: 100 = default 115x110, 120 = 1.2x, 0 = use 100
    int petDisplayScale;
} SkinDescriptor;
