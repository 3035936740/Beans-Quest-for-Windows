#pragma once

// Original level-end state transitions and completion timing.

// LevelEnd::PostSolve0x24f868 casts the other PhysicsObject to Tom and
// latches +0x63 for a solved top normal (inclusive0.5), with no incoming
// velocity test. PostPhysics0x250ec4/0x250fe4 consumes it outside the solver.
bool levelEndPendingNative[MAX_ENTITIES];

void resetLevelEndContactsNative() {
    memset(levelEndPendingNative, 0, sizeof(levelEndPendingNative));
}

void levelEndPostSolveNative(b2Contact *contact) {
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++) {
        int source = (int)fixtures[side]->GetBody()->GetUserData() - 1,
            other = (int)fixtures[1 - side]->GetBody()->GetUserData() - 1;
        if (source < 0 || source >= entityCount || other != playerIndex ||
            entities[source].kind != EXIT)
            continue;
        b2WorldManifold m;
        contact->GetWorldManifold(&m);
        float normalY = side ? -m.normal.y : m.normal.y;
        if (normalY >= .5f)
            levelEndPendingNative[source] = true;
    }
}

bool levelEndPostStepNative() {
    int goal = -1;
    for (int i = 0; i < entityCount; i++)
        if (levelEndPendingNative[i] && entities[i].kind == EXIT && !entities[i].state.gone) {
            goal = i;
            break;
        }
    resetLevelEndContactsNative();
    if (goal < 0)
        return false;
    entities[goal].state.active = 2;
    entities[goal].state.timer = 0;
    finish();
    return complete;
}
