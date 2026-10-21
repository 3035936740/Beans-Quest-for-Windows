#pragma once

// Restores saved mechanism event phase at checkpoint boundaries.
// Switch Save/Load0xef88c/0xefc00 stores its pending flag at+0x6c.
// Breakable Save/Load0x136a94/0x136f20 stores pending damage at+0x50.
// The primary Windows layout stays28; only these unused saved aux slots
// carry tagged metadata, never the live actor's angular/facing state.
void saveMechanismEventCheckpointNative(State *saved) {
    for (int i = 0; i < entityCount; i++) {
        const Entity &e = entities[i];
        if (e.kind == SWITCH && switchPending[i])
            saved[i].aux = encodeMechanismEventAuxNative(1);
        else if (e.breakable && breakableDamageNative[i] > 0)
            saved[i].aux = encodeMechanismEventAuxNative(breakableDamageNative[i]);
    }
}

void restoreMechanismEventCheckpointNative(const State *saved) {
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        if (e.kind != SWITCH && !e.breakable)
            continue;
        int event = decodeMechanismEventAuxNative(saved[i].aux);
        if (e.kind == SWITCH)
            switchPending[i] = event == 1;
        else
            breakableDamageNative[i] = event;
    }
}
