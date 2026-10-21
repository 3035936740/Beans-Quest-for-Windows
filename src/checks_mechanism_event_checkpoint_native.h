#pragma once

// Regression coverage for mechanism event checkpoint native. Runs only in the isolated BQ_ENABLE_TESTS executable.
int mechanismEventVoicesNative(int kind) {
    const char *name = kind ? "enemyexplosion" : "switch";
    AudioClip *clip = 0;
    for (int i = 0; i < soundClipCount; i++)
        if (eq(soundClips[i].name, name))
            clip = &soundClips[i].clip;
    int n = 0;
    for (int i = 0; i < 32; i++)
        n += clip && audioVoices[i].clip == clip;
    return n;
}

int mechanismEventPrepareNative(int kind) {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    stopMusic();
    stopSounds();
    if (kind) {
        int bird = -1;
        return breakableImpactPrepareNative(bird, false);
    }
    startStage(stageById(4412182657474862604ULL));
    readyTimer = 0;
    current.geometryCount = 0;
    int sw = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == SWITCH && sw < 0)
            sw = i;
    if (sw < 0)
        return -1;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && i != sw)
            entities[i].state.gone = 1;
    if (switchOn(entities[sw]))
        alternationBits ^= entities[sw].mask;
    State &bean = entities[playerIndex].state;
    bean.x = entities[sw].state.x;
    bean.y = entities[sw].state.y + 20;
    bean.vx = bean.vy = 0;
    rebuildPhysics();
    return sw;
}

void mechanismEventMoveAwayNative(int target) {
    State &bean = entities[playerIndex].state;
    bean.x = entities[target].state.x + 300;
    bean.y = entities[target].state.y + 300;
    bean.vx = bean.vy = 0;
    bodies[playerIndex]->SetTransform(b2Vec2(bean.x * metres, bean.y * metres), 0);
    bodies[playerIndex]->SetLinearVelocity(b2Vec2(0, 0));
}

const char *mechanismEventFileNative(int kind) {
    return kind ? "mechanism-break-event-native.dat" : "mechanism-switch-event-native.dat";
}

bool mechanismEventObserveRestoredNative(int kind, int target, bool expected, float dt,
                                         float expectedAux = 0) {
    bool fields = entities[target].state.aux == expectedAux &&
                  entities[playerIndex].state.aux == checkpoint.state[playerIndex].aux;
    int bits = checkpoint.bits, hp = checkpoint.state[target].hp;
    mechanismEventMoveAwayNative(target);
    stopSounds();
    soundEnabled = true;
    u32 voices = audioVoicesStarted;
    tick(dt, false, false);
    bool ok = kind ? (entities[target].state.hp == hp - (expected ? 3 : 0) &&
                      ((entities[target].state.gone != 0) == expected))
                   : (alternationBits == (bits ^ (expected ? entities[target].mask : 0)));
    ok &= fields && mechanismEventVoicesNative(kind) == (expected ? 1 : 0) &&
          audioVoicesStarted == voices + (expected ? 1u : 0u);
    tick(dt, false, false);
    ok &= mechanismEventVoicesNative(kind) == (expected ? 1 : 0) &&
          audioVoicesStarted == voices + (expected ? 1u : 0u);
    if (!kind)
        ok &= alternationBits == (bits ^ (expected ? entities[target].mask : 0));
    soundEnabled = false;
    stopSounds();
    return ok;
}

bool mechanismEventRoundtripNative(int kind, bool saved, float dt, bool disk, bool &accepted) {
    int target = mechanismEventPrepareNative(kind);
    if (target < 0)
        return false;
    if (!saved)
        captureCheckpoint(false);
    simulateBodies(dt);
    accepted &= kind ? breakableDamageNative[target] == 3 : switchPending[target];
    if (saved)
        captureCheckpoint(false);
    bool ok = true;
    char path[MAX_PATH];
    if (disk) {
        savePath(path, mechanismEventFileNative(kind));
        ok = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    }
    if (saved) {
        if (kind)
            updateBreakableContacts();
        else
            mechanismsPostStep(dt);
    }
    if (disk)
        ok &= readCheckpoint(path);
    restoreCheckpoint();
    return mechanismEventObserveRestoredNative(kind, target, saved, dt) && ok;
}

void mechanismEventFreshDiskNativeChecks() {
    for (int kind = 0; kind < 2; kind++) {
        int target = mechanismEventPrepareNative(kind);
        char path[MAX_PATH];
        savePath(path, mechanismEventFileNative(kind));
        bool ok = target >= 0 && readCheckpoint(path);
        if (ok) {
            restoreCheckpoint();
            ok = mechanismEventObserveRestoredNative(kind, target, true, 1.0f / 120);
        }
        check(
            ok,
            kind
                ? "Fresh process restores the original pending breakable damage and consumes it once through a real complete tick"
                : "Fresh process restores the original pending switch event and consumes it once through a real complete tick");
    }
}

// A legacy Windows aux value is not a second, delayed native switch event.
void switchLegacyIdleNativeChecks() {
    bool accepted = true;
    for (int rate = 0; rate < 3; rate++) {
        int fps = rate == 0 ? 30 : rate == 1 ? 120 : 240;
        float dt = 1.0f / fps;
        bool stable = true, silent = true;
        for (int version = 0; version < 2; version++)
            for (int on = 0; on < 2; on++)
                for (int aux = -1; aux <= 1; aux++) {
                    int target = mechanismEventPrepareNative(0);
                    if (target < 0) {
                        accepted = stable = false;
                        continue;
                    }
                    if (!on)
                        captureCheckpoint(false);
                    simulateBodies(dt);
                    accepted &= switchPending[target];
                    if (on) {
                        mechanismsPostStep(dt);
                        accepted &= switchOn(entities[target]);
                        captureCheckpoint(false);
                    }
                    checkpoint.state[target].aux = (float)aux;
                    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
                    char path[MAX_PATH];
                    savePath(path, "switch-legacy-idle-native.dat");
                    bool loaded = false;
                    if (version) {
                        loaded = atomicWrite(path, &checkpoint, sizeof(checkpoint)) &&
                                 readCheckpoint(path);
                    } else {
                        SnapshotV14 *old =
                            (SnapshotV14 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV14));
                        memcpy(old, &checkpoint, sizeof(*old) - 4);
                        old->version = 14;
                        old->checksum = hashBytes(old, sizeof(*old) - 4);
                        loaded = atomicWrite(path, old, sizeof(*old)) && readCheckpoint(path);
                        HeapFree(GetProcessHeap(), 0, old);
                    }
                    stable &= loaded && checkpoint.version == (version ? 28 : 14);
                    restoreCheckpoint();
                    mechanismEventMoveAwayNative(target);
                    bodies[playerIndex]->SetType(b2_staticBody);
                    int bits = checkpoint.bits;
                    stopSounds();
                    soundEnabled = true;
                    u32 voices = audioVoicesStarted;
                    for (int frame = 0; frame < fps * 2; frame++) {
                        tick(dt, false, false);
                        stable &=
                            alternationBits == bits && switchOn(entities[target]) == (on != 0);
                        silent &= !switchPending[target] && audioVoicesStarted == voices;
                    }
                    soundEnabled = false;
                    stopSounds();
                }
        char label[260];
        wsprintfA(
            label,
            "Legacy version14/28 switches keep saved ON and OFF states for two full seconds at%dHz with untagged aux-1/0/+1 and no new contact",
            fps);
        check(stable, label);
        wsprintfA(
            label,
            "Legacy switch idle at%dHz cannot invent a pending event or start any new sound after restore",
            fps);
        check(silent, label);
    }
    check(
        accepted,
        "All36legacy switch idle controls use real accepted BeginContact callbacks; ON saves consume them and OFF saves precede them");
    startStage(0);
    soundEnabled = musicEnabled = false;
}

void mechanismEventCheckpointNativeChecks() {
    bool unused = true;
    int classesChecked = 0;
    for (int c = 0; c < classCount; c++)
        if (classes[c].has("BreakableBehaviour") || classes[c].has("AlternatorSwitchBehaviour")) {
            Entity e;
            initializeEntity(e, classes[c], classes[c].rect);
            unused &= e.state.aux == 0 && !e.dynamic && !e.rotate && !e.move && !e.orbit;
            classesChecked++;
        }
    check(
        unused && classesChecked == 2,
        "Both original static mechanism classes have a zero unused aux slot and no competing angular or movement controller");
    bool accepted = true;
    for (int kind = 0; kind < 2; kind++)
        for (int saved = 0; saved < 2; saved++) {
            bool restored = true;
            for (int rate = 0; rate < 3; rate++)
                restored &= mechanismEventRoundtripNative(kind, saved != 0,
                                                          1.0f / (rate == 0   ? 30
                                                                  : rate == 1 ? 120
                                                                              : 240),
                                                          false, accepted);
            char label[240];
            wsprintfA(
                label,
                "Original %s saved pending%d overrides current pending state and is consumed once through the next full tick at30/120/240Hz",
                kind ? "breakable" : "switch", saved);
            check(restored, label);
        }
    check(
        accepted,
        "All event roundtrips start with actual switch BeginContact or three-point bottom-hit damage from real PostSolve callbacks");
    for (int kind = 0; kind < 2; kind++) {
        bool seen = true;
        check(
            mechanismEventRoundtripNative(kind, true, 1.0f / 120, true, seen) && seen,
            kind
                ? "Actual version28 disk checkpoint stores pending breakable damage before post-physics consumption"
                : "Actual version28 disk checkpoint stores an accepted switch before post-physics consumption");
    }
    for (int kind = 0; kind < 2; kind++) {
        bool compatible = true;
        for (int legacy = -1; legacy <= 1; legacy++) {
            int target = mechanismEventPrepareNative(kind);
            captureCheckpoint(false);
            simulateBodies(1.0f / 120);
            bool actual = kind ? breakableDamageNative[target] == 3 : switchPending[target];
            checkpoint.state[target].aux = (float)legacy;
            checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
            char path[MAX_PATH];
            savePath(path, "mechanism-legacy-untagged-native.dat");
            bool ok = atomicWrite(path, &checkpoint, sizeof(checkpoint)) && readCheckpoint(path);
            restoreCheckpoint();
            compatible &=
                actual && ok && checkpoint.version == 28 && sizeof(Snapshot) == 543392 &&
                mechanismEventObserveRestoredNative(kind, target, false, 1.0f / 120, (float)legacy);
        }
        check(
            compatible,
            kind
                ? "Legacy untagged0/+1/-1aux values cannot invent saved breakable damage or leak metadata into live state"
                : "Legacy untagged0/+1/-1aux values cannot invent saved switch events or leak metadata into live state");
    }
    for (int kind = 0; kind < 2; kind++) {
        int target = mechanismEventPrepareNative(kind);
        captureCheckpoint(false);
        simulateBodies(1.0f / 120);
        SnapshotV14 *legacy = (SnapshotV14 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV14));
        memcpy(legacy, &checkpoint, sizeof(*legacy) - 4);
        legacy->version = 14;
        legacy->checksum = hashBytes(legacy, sizeof(*legacy) - 4);
        char path[MAX_PATH];
        savePath(path, "mechanism-legacy14-native.dat");
        bool loaded = atomicWrite(path, legacy, sizeof(*legacy)) && readCheckpoint(path);
        HeapFree(GetProcessHeap(), 0, legacy);
        restoreCheckpoint();
        check(
            loaded && checkpoint.version == 14 &&
                mechanismEventObserveRestoredNative(kind, target, false, 1.0f / 120),
            kind
                ? "A real legacy version14 primary checkpoint loads without inventing a pending breakable event"
                : "A real legacy version14 primary checkpoint loads without inventing a pending switch event");
    }

    switchLegacyIdleNativeChecks();
    stopSounds();
    soundEnabled = false;
    startStage(0);
}
