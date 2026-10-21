#pragma once

// Regression coverage for checkpoint save phase native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int checkpointPhasePrepareNative(u64 scene, u64 point, float x, float y) {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    debugMode = true;
    startStage(stageById(scene));
    readyTimer = 0;
    int found = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == CHECKPOINT && entities[i].source->id == point)
            found = i;
    if (found >= 0) {
        State &bean = entities[playerIndex].state;
        bean.x = x;
        bean.y = y;
        bean.vx = bean.vy = 0;
        restorePhysics();
        checkpointResetContacts();
    }
    return found;
}

void checkpointSavePhaseNativeChecks() {
    const u64 scenes[] = {641851699177ULL, 641851699177ULL, 5679669680221079ULL,
                          5679669680221079ULL};
    const u64 points[] = {750001166099ULL, 750001166099ULL, 8060124985373868ULL,
                          8060124985373868ULL};
    const float xs[] = {468, 472, 468, 472}, ys[] = {1080, 1080, 472, 472};
    bool actual = true, deferred = true, counters = true, objects = true, restored = true,
         coherent = true;
    int touched = 0;
    for (int c = 0; c < 4; c++) {
        int point = checkpointPhasePrepareNative(scenes[c], points[c], xs[c], ys[c]);
        if (point < 0) {
            actual = false;
            continue;
        }
        u32 old = checkpoint.checksum;
        tick(1.0f / 120, false, false);
        int collected = coins;
        actual &= entities[point].state.hp == 2 && collected > 0 && deathTimer == 0;
        if (collected)
            touched++;
        deferred &= checkpoint.checksum == old;
        float cameraX = camX + viewW * .5f, cameraY = camY + viewH * .5f;
        State before = entities[playerIndex].state;
        bool gone[MAX_ENTITIES] = {};
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == COIN && entities[i].state.gone)
                gone[i] = true;
        tick(1.0f / 120, false, false);
        counters &= checkpoint.coins == collected;
        coherent &= checkpoint.camera.x == cameraX && checkpoint.camera.y == cameraY &&
                    checkpoint.state[playerIndex].x == before.x &&
                    checkpoint.state[playerIndex].y == before.y;
        for (int i = 0; i < entityCount; i++)
            if (gone[i])
                objects &= checkpoint.state[i].gone != 0;
        coins = 0;
        restoreCheckpoint();
        restored &= coins == collected;
        for (int i = 0; i < entityCount; i++)
            if (gone[i])
                restored &= entities[i].state.gone != 0;
    }
    check(
        actual && touched == 4,
        "Four actual checkpoint/gem contact arrangements in two original scenes reproduce with authored terrain intact and no death");
    check(
        deferred,
        "A checkpoint contact requests saving without snapshotting the middle of the current contact-dispatch loop");
    check(counters,
          "The next unpaused update captures every gem collected later in the same contact frame");
    check(
        objects,
        "The unified checkpoint includes all simultaneously collected gem gone-flags instead of reviving those gems after load");
    check(
        restored,
        "Loading each of the four real checkpoints preserves the collected gems and matching gem counter");
    check(
        coherent,
        "Deferred checkpoint player coordinates and camera centre both describe the completed prior update before any next-step integration");
    int point = checkpointPhasePrepareNative(scenes[0], points[0], xs[0], ys[0]);
    u32 old = checkpoint.checksum;
    tick(1.0f / 120, false, false);
    int collected = coins;
    screen = PAUSE;
    tick(.1f, false, false);
    bool paused = checkpoint.checksum == old;
    screen = PLAY;
    tick(1.0f / 120, false, false);
    check(
        point >= 0 && paused && checkpoint.coins == collected,
        "A task pause retains the save request without capturing paused UI state and resumes it before physics");
    point = checkpointPhasePrepareNative(scenes[0], points[0], xs[0], ys[0]);
    Snapshot *saved = (Snapshot *)HeapAlloc(GetProcessHeap(), 0, sizeof(Snapshot));
    *saved = checkpoint;
    tick(1.0f / 120, false, false);
    checkpoint = *saved;
    restoreCheckpoint();
    u32 loaded = checkpoint.checksum;
    tick(1.0f / 120, false, false);
    check(
        point >= 0 && checkpoint.checksum == loaded && coins == saved->coins,
        "Loading a prior checkpoint supersedes an unconsumed save request rather than writing the loaded state as a new checkpoint");
    HeapFree(GetProcessHeap(), 0, saved);
    checkpointPhasePrepareNative(scenes[0], points[0], xs[0], ys[0]);
    tick(1.0f / 120, false, false);
    restartStage();
    u32 retry = checkpoint.checksum;
    tick(1.0f / 120, false, false);
    bool reset = checkpoint.checksum == retry;
    startStage(0);
    u32 fresh = checkpoint.checksum;
    tick(1.0f / 120, false, false);
    check(
        reset && checkpoint.checksum == fresh,
        "Retry and new scene construction discard stale requests instead of overwriting their initial snapshots on the following tick");
    checkpointPhasePrepareNative(scenes[0], points[0], xs[0], ys[0]);
    tick(1.0f / 120, false, false);
    tick(1.0f / 120, false, false);
    char path[MAX_PATH];
    savePath(path, "checkpoint-phase-v22.dat");
    bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    int count = checkpoint.coins;
    memset(&checkpoint, 0, sizeof(checkpoint));
    bool read = written && readCheckpoint(path);
    restoreCheckpoint();
    check(
        read && count > 0 && coins == count && checkpoint.version == 28 &&
            sizeof(SnapshotV22) == 510592 && sizeof(SnapshotV23) == 510624 &&
            sizeof(Snapshot) == 543392 && validSnapshot(checkpoint),
        "The unified checkpoint remains an actual compatible Snapshot28 disk file with consistent collected gems");
    startStage(0);
    debugMode = false;
    readyTimer = 0;
}

bool checkpointSavePhaseDiskReloadNative() {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    debugMode = true;
    char path[MAX_PATH];
    savePath(path, "checkpoint-phase-v22.dat");
    bool ok = readCheckpoint(path);
    haveCheckpoint = ok;
    int stage = checkpoint.stage, count = checkpoint.coins;
    u32 saved = checkpoint.checksum;
    if (ok) {
        startStage(stage, true);
        int gone = 0;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == COIN && entities[i].state.gone)
                gone++;
        ok = count > 0 && coins == count && gone == count;
        tick(1.0f / 120, false, false);
        ok &= checkpoint.checksum == saved;
    }
    const char *report = ok ? "PASS fresh-process unified checkpoint gem restoration\r\n"
                            : "FAIL fresh-process unified checkpoint gem restoration\r\n";
    atomicWrite("test-output/checkpoint-phase-fresh-process.txt", report, lstrlenA(report));
    return ok;
}
