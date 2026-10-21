#pragma once

// Wizard encounter contact and its authored state transitions.

// FinalBoss +0x168 is a separately saved binary32 hit clock. Boss aux was
// unused and nonnegative in legacy snapshots. Store the clock with its sign
// bit set: negative zero is pending, negative2 is armed, and legacy positive
// zero decodes as armed. Bitwise encoding preserves every mantissa bit and
// keeps Snapshot28 unchanged; adding an offset would lose tiny positive ages.
float wizardHitClockNative(const Entity &boss) {
    u32 bits;
    memcpy(&bits, &boss.state.aux, 4);
    if (!(bits & 0x80000000u))
        return 2;
    bits &= 0x7fffffffu;
    float clock;
    memcpy(&clock, &bits, 4);
    return clock;
}

void setWizardHitClockNative(Entity &boss, float clock) {
    u32 bits;
    memcpy(&bits, &clock, 4);
    bits |= 0x80000000u;
    memcpy(&boss.state.aux, &bits, 4);
}

// PostSolve0x1d3a68: upper contact only clears the hit clock. Side/underside
// contact calls Tom::MegaJump130 and leaves it queued until Tom PrePhysics.
void wizardPostSolveNative(b2Contact *contact) {
    b2Fixture *fs[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++) {
        int index = (int)fs[side]->GetBody()->GetUserData() - 1,
            other = (int)fs[1 - side]->GetBody()->GetUserData() - 1;
        if (index < 0 || index >= entityCount || entities[index].kind != BOSS ||
            other != playerIndex || fs[side]->GetBody() != bodies[index] ||
            fs[1 - side]->GetBody() != bodies[playerIndex])
            continue;
        b2WorldManifold m;
        contact->GetWorldManifold(&m);
        Vec normal = {side ? -m.normal.x : m.normal.x, side ? -m.normal.y : m.normal.y};
        if (normal.y > .1f)
            setWizardHitClockNative(entities[index], 0);
        else {
            const Props &p = *entities[playerIndex].physics;
            nativeMegaJumpRequest(index, normal, p.num("gravity", -400), p.num("jump-height", 74),
                                  0, 130);
        }
    }
}
