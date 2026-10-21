#pragma once

// Regression coverage for helper checkpoint disk native. Runs only in the isolated BQ_ENABLE_TESTS executable.
int helperCheckpointDiskPrepareNative() {
    interfaceEnabled = musicEnabled = soundEnabled = debugMode = false;
    int stage = -1;
    for (int i = 0; i < stageCount; i++)
        if (levels[stageList[i]].props.num("bean-world") == 3 &&
            levels[stageList[i]].props.num("bean-level") == 2)
            stage = i;
    if (stage < 0)
        return -1;
    startStage(stage);
    readyTimer = 0;
    int bird = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == HELPER)
            bird = i;
    if (bird < 0)
        return -1;
    Entity &helper = entities[bird];
    State &bean = entities[playerIndex].state;
    bean.x = 272;
    bean.y = 850;
    bean.vx = 0;
    bean.vy = 60;
    helper.state.active = 1;
    helper.state.timer = 1;
    helper.state.x = bean.x;
    helper.state.y = bean.y + 34;
    restorePhysics();
    for (int n = 0; n < 74; n++)
        tick(1.0f / 120, false, false);
    captureCheckpoint(false);
    return bird;
}

bool helperCheckpointDiskReloadNative() {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    startStage(stageById(641851699177ULL));
    bool read = readCheckpoint("test-output/helper-oneway-checkpoint.dat");
    int stage = checkpoint.stage;
    if (read) {
        startStage(stage);
        read = readCheckpoint("test-output/helper-oneway-checkpoint.dat");
        restoreCheckpoint();
    }
    int bird = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == HELPER)
            bird = i;
    bool following = read && bird >= 0 && oneWayCheckpointNative.count > 0;
    for (int n = 0; n < 240 && following; n++) {
        tick(1.0f / 240, false, false);
        following &= entities[bird].state.active == 1;
    }
    check(
        following,
        "Fresh-process disk reload keeps the saved one-way traversal until the following bird clears the actual checkpoint platform");
    return failures == 0;
}

void helperCheckpointDiskNativeChecks() {
    int bird = helperCheckpointDiskPrepareNative();
    if (bird < 0) {
        check(false, "Actual helper checkpoint disk fixture resolves");
        return;
    }
    const char *path = "test-output/helper-oneway-checkpoint.dat";
    bool saved = oneWayCheckpointNative.count > 0 &&
                 validOneWayCheckpointNative(oneWayCheckpointNative, checkpoint.checksum) &&
                 atomicWrite(path, &checkpoint, sizeof(checkpoint)) &&
                 writeOneWayCheckpointNative(path);
    check(
        saved && checkpoint.version == 28 && sizeof(checkpoint) == 543392,
        "Real penetrating helper traversal writes a portable checksummed supplement while retaining the exact Snapshot28 file bytes");
    int pairs = oneWayCheckpointNative.count;
    restoreCheckpoint();
    captureCheckpoint(false);
    check(
        oneWayCheckpointNative.count == pairs,
        "Saving again before the restored first contact consumes its refusal cannot lose the saved traversal");
    startStage(0);
    bool read = readCheckpoint(path);
    if (read) {
        startStage(checkpoint.stage);
        read = readCheckpoint(path);
        restoreCheckpoint();
    }
    bird = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == HELPER)
            bird = i;
    bool retained = read && bird >= 0 && oneWayCheckpointNative.count == pairs;
    for (int n = 0; n < 240 && retained; n++) {
        tick(1.0f / 240, false, false);
        retained &= entities[bird].state.active == 1;
    }
    check(
        retained,
        "Reloading actual primary and supplement files after a different scene preserves traversal on rebuilt body and fixture identities");
    bird = helperCheckpointDiskPrepareNative();
    const char *legacy = "test-output/helper-oneway-legacy28.dat";
    char extra[MAX_PATH];
    oneWayCheckpointPathNative(legacy, extra);
    DeleteFileA(extra);
    bool old = atomicWrite(legacy, &checkpoint, sizeof(checkpoint)) && readCheckpoint(legacy);
    restoreCheckpoint();
    bool migrated = old && oneWayCheckpointNative.magic == 0;
    for (int n = 0; n < 240 && migrated; n++) {
        tick(1.0f / 240, false, false);
        migrated &= entities[bird].state.active == 1;
    }
    check(
        migrated,
        "A real legacy Snapshot28 without a supplement safely reconstructs only its intersecting ascending helper traversal");
    bird = helperCheckpointDiskPrepareNative();
    OneWayCheckpointNative *copy = new OneWayCheckpointNative;
    *copy = oneWayCheckpointNative;
    oneWayCheckpointNative.snapshot ^= 1;
    oneWayCheckpointNative.checksum =
        hashBytes(&oneWayCheckpointNative, sizeof(oneWayCheckpointNative) - 4);
    bool rejects = !validOneWayCheckpointNative(oneWayCheckpointNative, checkpoint.checksum);
    oneWayCheckpointNative = *copy;
    oneWayCheckpointNative.count = 4097;
    oneWayCheckpointNative.checksum =
        hashBytes(&oneWayCheckpointNative, sizeof(oneWayCheckpointNative) - 4);
    rejects &= !validOneWayCheckpointNative(oneWayCheckpointNative, checkpoint.checksum);
    oneWayCheckpointNative = *copy;
    oneWayCheckpointNative.pairs[0].other = NATIVE_PHYSICS_CHECKPOINT_BODIES;
    oneWayCheckpointNative.checksum =
        hashBytes(&oneWayCheckpointNative, sizeof(oneWayCheckpointNative) - 4);
    rejects &= !validOneWayCheckpointNative(oneWayCheckpointNative, checkpoint.checksum);
    oneWayCheckpointNative = *copy;
    oneWayCheckpointNative.checksum ^= 1;
    rejects &= !validOneWayCheckpointNative(oneWayCheckpointNative, checkpoint.checksum);
    oneWayCheckpointNative = *copy;
    delete copy;
    check(
        rejects,
        "Stale mismatched primary files, oversized pair counts, invalid body keys and corrupted supplement bytes are rejected independently");
    // A descending helper is not the legacy ascending-through case.
    checkpoint.state[playerIndex].vy = -60;
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    memset(&oneWayCheckpointNative, 0, sizeof(oneWayCheckpointNative));
    restoreCheckpoint();
    int seeded = 0;
    for (int i = 0; i < oneWayRestoredRefusalCountNative; i++)
        seeded += oneWayRestoredRefusalsNative[i].platform != 0;
    tick(1.0f / 120, false, false);
    check(
        seeded == 0 && entities[bird].state.active == 2,
        "Legacy recovery does not suppress the real platform impact of a descending following bird");
    captureCheckpoint(false);
    bool resetFile =
        atomicWrite(legacy, &checkpoint, sizeof(checkpoint)) && writeOneWayCheckpointNative(legacy);
    oneWayCheckpointPathNative(legacy, extra);
    eraseCheckpointFilesNative(legacy);
    check(
        resetFile && GetFileAttributesA(legacy) == INVALID_FILE_ATTRIBUTES &&
            GetFileAttributesA(extra) == INVALID_FILE_ATTRIBUTES &&
            oneWayCheckpointNative.magic == 0,
        "The same progress-reset cleanup removes both real private checkpoint files and clears the retained traversal supplement");
    soundEnabled = musicEnabled = false;
    startStage(0);
}
