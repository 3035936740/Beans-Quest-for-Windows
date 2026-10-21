#pragma once

// Shared touch alternator behavior used by the Windows reconstruction.

bool touchAlternatorPending[MAX_ENTITIES];

// Ctor0x000ff63c starts age0.1. Load0xff9d4 and its Box/BasePhysics
// chain retain the current age/latch, rather than loading or resetting them.
// Initialize the age in initializeEntity, never when reusing a physics world.
void resetTouchAlternatorNative() {
    memset(touchAlternatorPending, 0, sizeof(touchAlternatorPending));
}

struct TouchLoadEntryNative {
    int actor;
    float age;
    bool pending;
};

struct TouchLoadRuntimeNative {
    int count;
    TouchLoadEntryNative entries[1];
};

TouchLoadRuntimeNative *captureTouchLoadRuntimeNative() {
    int count = 0;
    for (int i = 0; i < entityCount; i++)
        count += entities[i].touch != 0;
    if (!count)
        return 0;
    TouchLoadRuntimeNative *saved = (TouchLoadRuntimeNative *)HeapAlloc(
        GetProcessHeap(), 0,
        sizeof(TouchLoadRuntimeNative) + (count - 1) * sizeof(TouchLoadEntryNative));
    if (!saved)
        fatal("Checkpoint transient allocation failed.");
    saved->count = 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch)
            saved->entries[saved->count++] =
                TouchLoadEntryNative{i, entities[i].state.timer, touchAlternatorPending[i]};
    return saved;
}

void restoreTouchLoadRuntimeNative(TouchLoadRuntimeNative *saved) {
    if (!saved)
        return;
    for (int n = 0; n < saved->count; n++) {
        const TouchLoadEntryNative &entry = saved->entries[n];
        if (entry.actor >= 0 && entry.actor < entityCount && entities[entry.actor].touch) {
            entities[entry.actor].state.timer = entry.age;
            touchAlternatorPending[entry.actor] = entry.pending;
        }
    }
    HeapFree(GetProcessHeap(), 0, saved);
}

void touchAlternatorAlternateNative(int mask) {
    // DoAlternate0x00100a8c resets every matching target, including off ones.
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch && (entities[i].mask & mask))
            entities[i].state.timer = 0;
}

// The native latch is cleared only after PostPhysics consumes it.
// A same-object load must not lose its accepted event before that update.
void touchAlternatorPostSolveNative(b2Contact *contact) {
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++) {
        int source = (int)fixtures[side]->GetBody()->GetUserData() - 1,
            other = (int)fixtures[1 - side]->GetBody()->GetUserData() - 1;
        if (source < 0 || source >= entityCount || other != playerIndex ||
            !entities[source].touch || entities[source].state.gone)
            continue;
        b2WorldManifold manifold;
        contact->GetWorldManifold(&manifold);
        // PostSolve0x000ff548 tests the actual oriented normal, independently
        // of Tom's incoming velocity and whichever other support is selected.
        float normalY = side ? -manifold.normal.y : manifold.normal.y;
        if (normalY > 0.9f)
            touchAlternatorPending[source] = true;
    }
}

void touchAlternatorPostStepNative(float dt) {
    // PostPhysics0x0010244c adds dt before its strict age>0.1 test. Its latch
    // is consumed even when the guard rejects a contact; no deferred toggle.
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        if (!e.touch)
            continue;
        bool pending = touchAlternatorPending[i];
        touchAlternatorPending[i] = false;
        if (e.state.gone)
            continue;
        e.state.timer += dt;
        if (pending && e.state.timer > 0.1f) {
            sound("switch");
            alternateScene(e.mask);
        }
    }
}
