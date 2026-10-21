#pragma once

// Defers checkpoint capture to the task boundary and tracks the authored checkpoint rearm conditions.

// CheckpointBehaviour::BeginContact 0x159c20 only accepts native state0.
// Existing serialized hp3/hp2 encode state0/state1; active2 retains the save
// artwork even when distance rearms state0 without changing the animation.
bool sensorBodyContact(b2Body *sensor, b2Body *player);
bool checkpointWasTouching[MAX_ENTITIES], checkpointNewTouch[MAX_ENTITIES];

// LevelTask::SaveState0x2678f8 only sets request1. OnTick0x270568..84
// processes it on the next unpaused update, BEFORE object/physics updates.
void checkpointFlushSaveNative() {
    if (checkpointSavePendingNative) {
        checkpointSavePendingNative = false;
        captureCheckpoint(true);
    }
}

void checkpointResetContacts() {
    memset(checkpointWasTouching, 0, sizeof(checkpointWasTouching));
    memset(checkpointNewTouch, 0, sizeof(checkpointNewTouch));
}

bool checkpointSaveVisual(const Entity &point) {
    return point.state.hp == 2 || point.state.active == 2;
}

bool checkpointRearmNative(Entity &point, const State &bean) {
    if (point.kind != CHECKPOINT || point.state.hp != 2)
        return false;
    const Props *config = point.source->behavior("CheckpointBehaviour");
    float dx = config->num("reactivate-dist-x", 240), dy = config->num("reactivate-dist-y", 160);
    // PrePhysicsStep 0x15fcc0..0x15fe3c: strict OR of axis distances.
    // Merely leaving the sensor does not arm the checkpoint again.
    if (absf(bean.x - point.state.x) > dx || absf(bean.y - point.state.y) > dy) {
        point.state.hp = 3;
        point.state.active = 2;
        return true;
    }
    return false;
}

void checkpointPrePhysics() {
    if (playerIndex < 0 || respawnBusy(respawnTransition))
        return;
    for (int i = 0; i < entityCount; i++)
        if (!entities[i].state.gone)
            checkpointRearmNative(entities[i], entities[playerIndex].state);
}

void checkpointObserveContacts() {
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == CHECKPOINT) {
            bool touching = playerIndex >= 0 && !entities[i].state.gone &&
                            sensorBodyContact(bodies[i], bodies[playerIndex]);
            checkpointNewTouch[i] = touching && !checkpointWasTouching[i];
            checkpointWasTouching[i] = touching;
        }
}

bool checkpointBeginNative(int index) {
    if (index < 0 || index >= entityCount || entities[index].kind != CHECKPOINT)
        return false;
    bool began = checkpointNewTouch[index];
    checkpointNewTouch[index] = false;
    Entity &point = entities[index];
    if (!began || point.state.hp != 3 || respawnBusy(respawnTransition))
        return false;
    // Native binds Reactivate @0x15b904 across all checkpoints, then sets
    // its state1 @0x15abd4 BEFORE LevelTask::SaveState @0x15ad4c.
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == CHECKPOINT) {
            entities[i].state.hp = 3;
            entities[i].state.active = 1;
        }
    point.state.hp = 2;
    point.state.active = 2;
    point.state.timer = 0;
    checkpointSavePendingNative = true;
    sound("checkpoint-save");
    return true;
}

void checkpointRestoreAnimations() {
    // LoadState 0x159480..0x15949c positions saved animation at GetLength.
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == CHECKPOINT && checkpointSaveVisual(entities[i])) {
            entities[i].state.active = 2;
            entities[i].state.timer = animationLength(animation(entities[i].art, "save"));
        }
}
