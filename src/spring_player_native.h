#pragma once

// Applies spring launch behavior to the player using the original jump parameters.

// Spring PostSolve0x2fd2fc uses saved PreSolve velocity<0 and normalY>0.1.
// For Tom it always requests world-up MegaJump, not the contact normal.
// Tom consumes the shared+c9 request in the following PrePhysics update.
int springPlayerHitsNative[MAX_ENTITIES];
bool springPlayerContactThisStepNative;

void resetSpringPlayerStepNative() {
    memset(springPlayerHitsNative, 0, sizeof(springPlayerHitsNative));
    springPlayerContactThisStepNative = false;
}

void springPlayerPostSolveNative(b2Contact *contact) {
    if (playerIndex < 0 || !contact->IsEnabled() || !contact->GetManifold()->pointCount)
        return;
    for (int i = 0; i < springIncomingCount; i++) {
        SpringIncomingVelocity &impact = springIncoming[i];
        int source = impact.source;
        if (impact.contact != contact || impact.target != playerIndex || impact.y >= 0 ||
            source < 0 || source >= entityCount || entities[source].state.gone ||
            entities[playerIndex].state.gone || contact->GetFixtureA()->IsSensor() ||
            contact->GetFixtureB()->IsSensor())
            continue;
        b2WorldManifold m;
        contact->GetWorldManifold(&m);
        float ny = contact->GetFixtureB() == impact.other ? m.normal.y : -m.normal.y;
        if (ny <= .1f)
            continue;
        const Props &p = *entities[playerIndex].physics;
        Entity &spring = entities[source];
        nativeMegaJumpRequest(source, Vec{0, 1}, p.num("gravity", -400), p.num("jump-height", 74),
                              0, spring.jumpPower);
        springPlayerContactThisStepNative = true;
        springPlayerHitsNative[source]++;
        playerSpringContact = source;
        markMatrixContactActlets(entities, entityCount, spring, m,
                                 contact->GetManifold()->pointCount, true);
    }
}

void springPlayerFinishContactsNative() {
    for (int i = 0; i < entityCount; i++)
        while (springPlayerHitsNative[i] > 0) {
            sound("megajump");
            springPlayerHitsNative[i]--;
        }
}
