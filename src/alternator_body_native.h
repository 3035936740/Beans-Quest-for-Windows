#pragma once

// Synchronizes alternator activation with the corresponding physics body.

// Normal SetOff/SetOn0xec9c4/0xeccd8 call b2Body::SetActive during
// DoAlternate. Touch blocks do the same via BasePhysics::SetPhysicsActive
// at0x100640/0x100928. Do not defer collision membership to the next Step.
void alternateBodiesNative(int mask) {
    if (!physicsWorld)
        return;
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        if (e.kind != ALTERNATOR || !(e.mask & mask) || !bodies[i])
            continue;
        bool active = activeBlock(e) && (!proximityControlled(e) || e.state.active != 0);
        bodies[i]->SetActive(active);
        if (oscillatorProxies[i])
            oscillatorProxies[i]->SetActive(active);
    }
}
