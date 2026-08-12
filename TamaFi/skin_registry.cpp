#include "skin_assets.h"
#include "skin_types.h"
// Asset headers included once here to avoid redefinition when both skins use them
#include "StoneGolem.h"
#include "egg_hatch.h"
#include "effect.h"
#include "GorgonIdle.h"
#include "skin_golem.h"
#include "skin_gorgon.h"

static const SkinDescriptor* descriptors[SKIN_COUNT] = {
    &skinGorgon,
    &skinGolem,
};

static const SkinDescriptor* skinGetDescriptor(uint8_t skin) {
    if (skin >= SKIN_COUNT) skin = SKIN_GORGON;
    return descriptors[skin];
}

const uint16_t** skinGetIdleFrames(uint8_t skin, Stage /*stage*/, int* frameCount) {
    const SkinDescriptor* d = skinGetDescriptor(skin);
    if (frameCount) *frameCount = d->idleFrameCount;
    return d->idleFrames;
}

const uint16_t** skinGetEggIdleFrames(uint8_t skin) {
    return skinGetDescriptor(skin)->eggIdleFrames;
}

const uint16_t** skinGetEggHatchFrames(uint8_t skin) {
    return skinGetDescriptor(skin)->eggHatchFrames;
}

const uint16_t** skinGetAttackFrames(uint8_t skin) {
    return skinGetDescriptor(skin)->attackFrames;
}

const uint16_t** skinGetDeadFrames(uint8_t skin) {
    return skinGetDescriptor(skin)->deadFrames;
}

const uint16_t** skinGetHungerFrames(uint8_t skin) {
    return skinGetDescriptor(skin)->hungerFrames;
}

void skinGetPetFrameSize(uint8_t skin, int* w, int* h) {
    const SkinDescriptor* d = skinGetDescriptor(skin);
    *w = d->petFrameW;
    *h = d->petFrameH;
}

void skinGetPetFrameSizeForLegacy(uint8_t skin, int* w, int* h) {
    const SkinDescriptor* d = skinGetDescriptor(skin);
    *w = d->legacyPetFrameW;
    *h = d->legacyPetFrameH;
}

int skinGetPetDisplayScale(uint8_t skin) {
    return skinGetDescriptor(skin)->petDisplayScale;
}

void skinGetEffectFrameSize(uint8_t skin, int* w, int* h) {
    const SkinDescriptor* d = skinGetDescriptor(skin);
    *w = d->effectFrameW;
    *h = d->effectFrameH;
}

bool skinUsesIdleSpriteSheet(uint8_t skin) {
    return skinGetDescriptor(skin)->idleSpriteSheet != nullptr;
}

void skinGetIdleSpriteSheet(uint8_t skin, const uint16_t** sheet, int* sheetW, int* sheetH, int* frameW, int* frameH) {
    const SkinDescriptor* d = skinGetDescriptor(skin);
    if (sheet) *sheet = d->idleSpriteSheet;
    if (sheetW) *sheetW = d->idleSheetW;
    if (sheetH) *sheetH = d->idleSheetH;
    if (frameW) *frameW = d->petFrameW > 0 ? d->petFrameW : 115;
    if (frameH) *frameH = d->petFrameH > 0 ? d->petFrameH : 110;
}
