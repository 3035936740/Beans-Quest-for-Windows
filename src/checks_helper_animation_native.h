#pragma once

// Regression coverage for helper animation native. Runs only in the isolated BQ_ENABLE_TESTS executable.
u32 helperAnimationPixelsNative(Entity &bird) {
    float facing = bird.state.aux, x = bird.state.x, y = bird.state.y;
    bird.state.aux = 1;
    bird.state.x = 256;
    bird.state.y = 192;
    cloudRenderBegin();
    drawEntity(bird);
    u32 pixels = cloudRenderPixels(Rect{232, 168, 280, 216}, false);
    bird.state.aux = facing;
    bird.state.x = x;
    bird.state.y = y;
    return pixels;
}

void helperAnimationVisualContinuityChecks() {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    stopMusic();
    stopSounds();
    startStage(0);
    readyTimer = 0;
    Entity bird;
    initializeEntity(bird, *classById(937947234821ULL), Rect{232, 168, 280, 216});
    u32 first = helperAnimationPixelsNative(bird);
    bird.state.timer += .21f;
    updateHelper(bird, .21f);
    u32 sleeping = helperAnimationPixelsNative(bird);
    check(first != sleeping,
          "Actual helper sleeping clip visibly advances through its original multiframe artwork");
    collectHelper(bird);
    bird.state.timer += .23f;
    updateHelper(bird, .23f);
    u32 flying = helperAnimationPixelsNative(bird);
    stopHelper(bird);
    bird.state.x += 100;
    bird.state.timer += .11f;
    updateHelper(bird, .11f);
    u32 manic = helperAnimationPixelsNative(bird);
    bird.state.x = bird.originX;
    bird.state.y = bird.originY;
    updateHelper(bird, 0);
    check(
        bird.state.active == 0 && helperAnimationPixelsNative(bird) == sleeping,
        "Actual helper sleep pixels resume their previous phase after a full flying and manic return cycle");
    collectHelper(bird);
    check(
        helperAnimationPixelsNative(bird) == flying,
        "Collecting the same helper again resumes its previously displayed flying frame instead of frame0");
    stopHelper(bird);
    check(helperAnimationPixelsNative(bird) == manic,
          "Repeated StopFollowing resumes its previously displayed manic frame instead of frame0");
    startStage(0);
    readyTimer = 0;
}

int helperAnimationCampaignPrepareNative(u64 stage) {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    startStage(stageById(stage));
    readyTimer = 0;
    int bird = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == HELPER)
            bird = i;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != bird && i != playerIndex)
            entities[i].state.gone = 1;
    entities[playerIndex].state.x = current.size.x * .5f;
    entities[playerIndex].state.y = 1000;
    rebuildPhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    return bird;
}

bool helperAnimationDiskReloadNative() {
    int index = helperAnimationCampaignPrepareNative(669939580618ULL);
    if (index < 0)
        return false;
    char path[MAX_PATH];
    savePath(path, "helper-animation-v22.dat");
    bool read = readCheckpoint(path);
    restoreCheckpoint();
    Entity &bird = entities[index];
    bool correct = read && checkpoint.version == 28 && validSnapshot(checkpoint) &&
                   bird.state.active == 2 && bird.helperAnimation.age[0] == .21f &&
                   bird.helperAnimation.age[1] == .23f && bird.helperAnimation.age[2] == .11f;
    const char *result = correct ? "PASS fresh-process helper animation disk restoration\r\n"
                                 : "FAIL fresh-process helper animation disk restoration\r\n";
    atomicWrite("test-output/helper-animation-fresh-process.txt", result, lstrlenA(result));
    return correct;
}

void helperAnimationNativeChecks() {
    helperAnimationVisualContinuityChecks();
    const float dt = 1.0f / 120;
    bool resources = true, clocks = true, pickups = true, paused = true;
    const u64 stages[] = {669939580618ULL, 641851699177ULL};
    int count = 0;
    for (int stage = 0; stage < 2; stage++) {
        int index = helperAnimationCampaignPrepareNative(stages[stage]);
        if (index < 0) {
            resources = false;
            continue;
        }
        count++;
        Entity &bird = entities[index];
        resources &= animation(bird.art, helperAnimationNameNative(bird))->count == 8 &&
                     animation(bird.art, "flying")->count == 6 &&
                     animation(bird.art, "manic")->count == 2;
        for (int f = 0; f < 27; f++)
            tick(dt, false, false);
        float asleep = bird.helperAnimation.age[0];
        clocks &= absf(asleep - 27 * dt) < .00001f && bird.helperAnimation.age[1] == 0 &&
                  bird.helperAnimation.age[2] == 0;
        State &bean = entities[playerIndex].state;
        bean.x = bird.state.x;
        bean.y = bird.state.y;
        bean.vx = bean.vy = 0;
        restorePhysics();
        tick(dt, false, false);
        pickups &= bird.state.active == 1 && bird.helperAnimation.age[1] == 0 &&
                   bird.helperAnimation.age[0] > asleep;
        char trace[300], tracePath[100];
        wsprintfA(tracePath, "test-output/helper-animation-stage%d.txt", stage);
        wsprintfA(
            trace, "pickupActive=%d sleepBefore10000=%d sleepAfter10000=%d flyAfter10000=%d\r\n",
            bird.state.active, (int)(asleep * 10000), (int)(bird.helperAnimation.age[0] * 10000),
            (int)(bird.helperAnimation.age[1] * 10000));
        atomicWrite(tracePath, trace, lstrlenA(trace));
        bean.x = current.size.x * .5f;
        bean.y = 1000;
        restorePhysics();
        bodies[playerIndex]->SetType(b2_staticBody);
        asleep = bird.helperAnimation.age[0];
        for (int f = 0; f < 23; f++)
            tick(dt, false, false);
        float flight = bird.helperAnimation.age[1];
        stopHelper(bird);
        for (int f = 0; f < 7; f++)
            tick(dt, false, false);
        clocks &= bird.state.active == 2 && bird.helperAnimation.age[0] == asleep &&
                  bird.helperAnimation.age[1] == flight &&
                  absf(bird.helperAnimation.age[2] - 7 * dt) < .00001f;
        wsprintfA(
            trace,
            "returnActive=%d sleep10000=%d flyBefore10000=%d flyAfter10000=%d manic10000=%d\r\n",
            bird.state.active, (int)(bird.helperAnimation.age[0] * 10000), (int)(flight * 10000),
            (int)(bird.helperAnimation.age[1] * 10000), (int)(bird.helperAnimation.age[2] * 10000));
        wsprintfA(tracePath, "test-output/helper-animation-return%d.txt", stage);
        atomicWrite(tracePath, trace, lstrlenA(trace));
        HelperAnimationNative before = bird.helperAnimation;
        screen = PAUSE;
        tick(.1f, false, false);
        screen = PLAY;
        paused &=
            hashBytes(&before, sizeof(before)) == hashBytes(&bird.helperAnimation, sizeof(before));
    }
    check(
        resources && count == 2,
        "Both authored helper stages bind the original8frame sleep,6frame flight and2frame manic instances");
    check(
        pickups,
        "Real Bean helper pickup changes the selected clip after the sleep tick without pre-aging the newly selected flying instance");
    check(
        clocks,
        "Actual gameplay ticks advance only the helper's selected independent instance and retain both inactive phases");
    check(
        paused,
        "Ordinary helper AnimationInstances freeze with their paused stage rather than advancing a hidden clock");
    int index = helperAnimationCampaignPrepareNative(stages[0]);
    if (index < 0) {
        check(false, "Helper stage resolves for animation checkpoint migration");
        return;
    }
    Entity &bird = entities[index];
    bird.helperAnimation = HelperAnimationNative{{.21f, .23f, .11f}};
    bird.state.active = 2;
    bird.state.x += 100;
    captureCheckpoint(false);
    char path[MAX_PATH];
    savePath(path, "helper-animation-v22.dat");
    bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    bird.helperAnimation = HelperAnimationNative{};
    memset(&checkpoint, 0, sizeof(checkpoint));
    bool loaded = written && readCheckpoint(path);
    restoreCheckpoint();
    check(
        loaded && checkpoint.version == 28 && sizeof(HelperAnimationNative) == 12 &&
            sizeof(SnapshotV21) == 486016 && sizeof(SnapshotV22) == 510592 &&
            sizeof(SnapshotV23) == 510624 && sizeof(Snapshot) == 543392 &&
            validSnapshot(checkpoint) && bird.helperAnimation.age[0] == .21f &&
            bird.helperAnimation.age[1] == .23f && bird.helperAnimation.age[2] == .11f,
        "Actual version28 disk restore retains all three helper instance phases while preserving the complete486016byte legacy prefix");
    SnapshotV21 *old = (SnapshotV21 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV21));
    memcpy(old, &checkpoint, sizeof(*old) - 4);
    old->version = 21;
    old->state[index].active = 1;
    old->state[index].timer = .23f;
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    savePath(path, "helper-animation-legacy21.dat");
    written = atomicWrite(path, old, sizeof(*old));
    HeapFree(GetProcessHeap(), 0, old);
    loaded = written && readCheckpoint(path);
    bird.helperAnimation = HelperAnimationNative{{.5f, .5f, .1f}};
    restoreCheckpoint();
    check(
        loaded && checkpoint.version == 21 && validSnapshot(checkpoint) &&
            bird.helperAnimation.age[0] == 0 && bird.helperAnimation.age[1] == .23f &&
            bird.helperAnimation.age[2] == 0,
        "Actual old version21 bytes preserve the known visible helper phase and safely default unsaved inactive instances");
    captureCheckpoint(false);
    u32 bad = 0x7fc00000;
    memcpy(&checkpoint.helperAnimations[index].age[2], &bad, 4);
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    bool reject = !validSnapshot(checkpoint);
    checkpoint.helperAnimations[index].age[2] = -1;
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    check(
        reject && !validSnapshot(checkpoint),
        "Checkpoint validation rejects NaN and negative helper animation ages even with a matching checksum");
    restartStage();
    check(
        bird.helperAnimation.age[0] == 0 && bird.helperAnimation.age[1] == 0 &&
            bird.helperAnimation.age[2] == 0,
        "Fresh stage retry recreates all helper animation instances at their original initial phases");
    bird.state.active = 2;
    bird.state.x = bird.originX;
    bird.state.y = bird.originY;
    bird.helperAnimation = HelperAnimationNative{{.21f, .23f, .11f}};
    updateHelper(bird, .01f);
    check(
        bird.state.active == 0 && absf(bird.helperAnimation.age[0] - .22f) < .00001f &&
            bird.helperAnimation.age[2] == .11f,
        "Native Actor PreUpdate advances sleeping after return-arrival OnUpdate selects it in the same frame");
    u32 stageHash = hashBytes(entities, sizeof(Entity) * entityCount),
        snapshotHash = hashBytes(&checkpoint, sizeof(checkpoint));
    interfaceEnabled = true;
    navigate(137502285691ULL);
    int endingBird = -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].kind == HELPER)
            endingBird = i;
    bool ending = endingBird >= 0;
    if (ending) {
        Entity &e = page.playfield[endingBird];
        e.state.active = 1;
        e.helperAnimation = HelperAnimationNative{{.21f, .23f, .11f}};
        tickInterface(dt);
        ending &= absf(e.helperAnimation.age[1] - (.23f + dt)) < .00001f &&
                  e.helperAnimation.age[0] == .21f && e.helperAnimation.age[2] == .11f;
    }
    check(
        ending && stageHash == hashBytes(entities, sizeof(Entity) * entityCount) &&
            snapshotHash == hashBytes(&checkpoint, sizeof(checkpoint)),
        "Actual ending helper advances its own retained clip without touching campaign helper clocks or checkpoint state");
    interfaceEnabled = false;
    startStage(0);
    readyTimer = 0;
}
