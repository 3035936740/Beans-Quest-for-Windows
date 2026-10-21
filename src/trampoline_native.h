#pragma once

// Shared trampoline behavior used by the Windows reconstruction.

// Tom::Trampoline 0x003128ac keeps a shared absolute apex, rounded body Y,
// and derives a launch from distance/gravity rather than reflected velocity.
float nativeTrampolineRoundedY(float y) {
    return (float)(int)(y + (y < 0 ? -.5f : .5f));
}

void nativeTrampolineObserve(float y, float &apex) {
    apex = maxf(apex, nativeTrampolineRoundedY(y));
}

float nativeTrampolineLaunch(float y, float gravity, float height, float bonusHeight, float &apex) {
    float rounded = nativeTrampolineRoundedY(y);
    apex = maxf(apex, rounded + height + bonusHeight);
    return root(maxf(0, -2 * gravity * (apex - rounded)));
}

void nativeTrampolineOrdinaryJump(float y, float height, float bonusHeight, float &apex) {
    apex = y + height + bonusHeight;
}

// Tom::Trampoline0x312950..0x312998: VNMLS forms5*h-h, then maps
// inherited distance from h..5*h to playback rate1..2. This is pitch, not gain.
float nativeTrampolineSoundRate(float distance, float height, float bonusHeight = 0) {
    float basic = height + bonusHeight;
    return basic > 0 ? 1 + clamp((distance - basic) / (4 * basic), 0, 1) : 1;
}

// Saved requests already contain speed=sqrt(-2*g*distance). Recover distance
// from it so old format14 checkpoints need no additional sound-only field.
float nativeTrampolineSoundRateFromSpeed(float speed, float gravity, float height,
                                         float bonusHeight = 0) {
    return gravity < 0
               ? nativeTrampolineSoundRate(speed * speed / (-2 * gravity), height, bonusHeight)
               : 1;
}

// Actual solved pad contacts are independent of the final ground support.
int nativeTrampolineSolvedContact(b2Contact *contact, Entity *things, int count, int player,
                                  float &bodyY) {
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    if (fixtures[0]->IsSensor() || fixtures[1]->IsSensor() || !contact->GetManifold()->pointCount)
        return -1;
    for (int side = 0; side < 2; side++) {
        int source = (int)fixtures[side]->GetBody()->GetUserData() - 1,
            target = (int)fixtures[1 - side]->GetBody()->GetUserData() - 1;
        if (source < 0 || source >= count || target != player ||
            things[source].kind != TRAMPOLINE || things[source].state.gone)
            continue;
        b2WorldManifold manifold;
        contact->GetWorldManifold(&manifold);
        float normalY = side ? -manifold.normal.y : manifold.normal.y;
        if (normalY <= .1f)
            continue;
        bodyY = fixtures[1 - side]->GetBody()->GetPosition().y / metres;
        markMatrixContactActlets(things, count, things[source], manifold,
                                 contact->GetManifold()->pointCount, true);
        return source;
    }
    return -1;
}
