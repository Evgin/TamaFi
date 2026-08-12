#pragma once

#include <stdint.h>
#include "pet_logic.h"

#define SKIN_COUNT 2
#define SKIN_GORGON 0
#define SKIN_GOLEM  1

const uint16_t** skinGetIdleFrames(uint8_t skin, Stage stage, int* frameCount);
const uint16_t** skinGetEggIdleFrames(uint8_t skin);
const uint16_t** skinGetEggHatchFrames(uint8_t skin);
const uint16_t** skinGetAttackFrames(uint8_t skin);
const uint16_t** skinGetDeadFrames(uint8_t skin);
const uint16_t** skinGetHungerFrames(uint8_t skin);
void skinGetPetFrameSize(uint8_t skin, int* w, int* h);
// For attack/dead/egg (Golem assets): use legacy size when set. Gorgon=115x110, Golem=0,0.
void skinGetPetFrameSizeForLegacy(uint8_t skin, int* w, int* h);
// Display scale %: 100 = default, 120 = 1.2x. Returns 0 → treat as 100.
int skinGetPetDisplayScale(uint8_t skin);
void skinGetEffectFrameSize(uint8_t skin, int* w, int* h);
// Idle sprite sheet: when non-null, use drawSpriteSheetFrameToContentScaled instead of drawPetFrame.
bool skinUsesIdleSpriteSheet(uint8_t skin);
void skinGetIdleSpriteSheet(uint8_t skin, const uint16_t** sheet, int* sheetW, int* sheetH, int* frameW, int* frameH);
