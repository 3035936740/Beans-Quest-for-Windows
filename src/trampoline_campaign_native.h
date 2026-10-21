#pragma once

// Shared trampoline campaign behavior used by the Windows reconstruction.
#include "trampoline_native.h"

// TrampolineBehaviour::PostSolve0x317e00 calls Tom::Trampoline0x3128ac
// for every actual qualifying contact, independently of normal ground owner.
void trampolineCampaignPostSolveNative(b2Contact *contact) {
    float y = 0;
    int source = nativeTrampolineSolvedContact(contact, entities, entityCount, playerIndex, y);
    if (source < 0 || playerIndex < 0 || entities[playerIndex].state.gone)
        return;
    const Props &p = *entities[playerIndex].physics;
    trampolineStateNative.speed = nativeTrampolineLaunch(
        y, p.num("gravity", -400), p.num("jump-height", 74), 0, trampolineStateNative.apex);
    trampolineStateNative.pending = 1;
    trampolineStateNative.source = source;
    trampolineContactThisStepNative = true;
}

// Tom PrePhysics consumes the preceding solver's request after movement,
// with the OLD jump age>=0.05. MegaJump wins; rejected flags never linger.
bool trampolineCampaignConsumeNative(float oldAge, bool megaPending) {
    TrampolineStateNative request = trampolineStateNative;
    trampolineStateNative.pending = 0;
    trampolineStateNative.source = -1;
    trampolineLaunchedThisStepNative = false;
    if (!request.pending || megaPending || deathTimer > 0 || oldAge < .05f || playerIndex < 0 ||
        !validTrampolineStateNative(request, entityCount) || entities[playerIndex].state.gone)
        return false;
    Entity &bean = entities[playerIndex];
    bean.state.vy = request.speed;
    bean.lastLaunchSpeed = request.speed;
    bean.state.timer = 0;
    bean.state.active = 1;
    if (bean.state.hp == 1 && !ordinaryJumpStateNative.stomp && jumps < 999)
        jumps++;
    const Props &props = *bean.physics;
    sound("jump", nativeTrampolineSoundRateFromSpeed(request.speed, props.num("gravity", -400),
                                                     props.num("jump-height", 74)));
    return trampolineLaunchedThisStepNative = true;
}
