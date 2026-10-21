#pragma once

// Wizard encounter pose and its authored state transitions.
#include "assets.h"

// FinalBossBehaviourArm::Update 0x1da130. This is actor geometry, in original
// world pixels. No arm code changes actor rotation or scale: both remain the
// authored 48 x 48, zero-degree sleeve/hand sprites.
struct WizardArmMotion {
    float clock, recoilAge;
    Vec shotDirection;
};

struct WizardArmPose {
    Vec shoulder, hand, sleeve;
    int sleeveZ, handZ;
};

WizardArmMotion wizardArmInitial() {
    WizardArmMotion m = {0, 0.5f, {0, 0}};
    return m;
}

void wizardArmShot(WizardArmMotion &motion, Vec direction) {
    motion.recoilAge = 0;
    motion.shotDirection = direction;
}

void wizardArmTick(WizardArmMotion &motion, float dt, int mode) {
    motion.clock += dt;
    if (mode == 1)
        motion.recoilAge += dt;
}

WizardArmPose wizardArmPose(Vec body, bool left, int mode, const WizardArmMotion &motion) {
    // Globals initialized at 0x1dc830-0x1dc86c (not file-initialized BSS).
    Vec start = left ? Vec{29, -18} : Vec{-21, -12}, end = left ? Vec{-5, -14} : Vec{-55, -8};
    WizardArmPose result = {};
    result.shoulder = Vec{body.x + start.x, body.y + start.y};
    result.hand = Vec{body.x + end.x, body.y + end.y};
    float time = (motion.clock + (left ? 0.25f : -0.25f)) * 6.283185307f;
    if (mode <= 1) {
        result.hand.x += (float)bqSin(time / 10);
        result.hand.y += (float)bqSin(time * 0.5f) * 3;
        // HandShoot 0x1d9e28 resets age; 0x1da46c-0x1da5cc fades the
        // ten-pixel opposite-shot kick with sin(pi*age) until age=0.5.
        float kick = motion.recoilAge <= 0     ? 1
                     : motion.recoilAge < 0.5f ? 1 - (float)bqSin(motion.recoilAge * 3.141592654f)
                                               : 0;
        result.hand.x -= 10 * motion.shotDirection.x * kick;
        result.hand.y -= 10 * motion.shotDirection.y * kick;
    } else if (mode == 2) {
        result.hand.x += (float)bqSin(time * 0.5f) * 3;
        result.hand.y += (float)bqSin(time) * 5;
    }
    result.sleeve =
        Vec{(result.shoulder.x + result.hand.x) * 0.5f, (result.shoulder.y + result.hand.y) * 0.5f};
    // Activate 0x1d8864 / 0x1d8964: left arm is in front, right behind.
    result.sleeveZ = left ? 2 : -4;
    result.handZ = left ? 4 : -2;
    return result;
}

Vec wizardHandAim(Vec hand, Vec player) {
    // HandShoot 0x1d9da0 aims 24 pixels below Bean's actor centre.
    float dx = player.x - hand.x, dy = player.y - 24 - hand.y, length = root(dx * dx + dy * dy);
    if (length <= 0.0001f)
        return Vec{0, 0};
    return Vec{dx / length, dy / length};
}

Rect wizardPartRect(Vec position) {
    return Rect{position.x - 24, position.y - 24, position.x + 24, position.y + 24};
}
