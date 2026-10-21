#pragma once

// Regression coverage for spring phase native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int prepareSpringPhaseNative(float age = .1f, bool terrain = false) {
    startStage(2);
    readyTimer = deathTimer = 0;
    readyOpening.showing = false;
    if (!terrain)
        current.geometryCount = 0;
    int pad = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == SPRING && boxGroupMatrixOwner(entities[i])) {
            pad = i;
            break;
        }
    if (pad < 0)
        return -1;
    Object *source = entities[pad].source;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && entities[i].source != source)
            entities[i].state.gone = 1;
    Entity &bean = entities[playerIndex];
    Rect r = boxGroupMatrixFixture(entities[pad]);
    bean.state.x = entities[pad].state.x + (r.l + r.r) * .5f;
    bean.state.y = entities[pad].state.y + r.t - bean.hit.b + .2f;
    bean.state.vx = 0;
    bean.state.vy = -30;
    bean.state.hp = 1;
    bean.state.timer = age;
    jumps = 0;
    rebuildPhysics();
    stopSounds();
    return pad;
}

void springPhaseNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/spring-phase-saves");
    CreateDirectoryA(saveRoot, 0);
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldSound = soundEnabled,
         oldMusic = musicEnabled;
    interfaceEnabled = debugMode = musicEnabled = false;
    soundEnabled = true;
    stopMusic();
    const float dt = 1.0f / 120;
    int pad = prepareSpringPhaseNative();
    check(
        pad >= 0 && sizeof(PinballPendingNative) == 20 && sizeof(SnapshotV14) == 295292 &&
            sizeof(SnapshotV15) == 319868 && sizeof(SnapshotV16) == 344444 &&
            sizeof(SnapshotV17) == 364156 && sizeof(SnapshotV18) == 366720 &&
            sizeof(SnapshotV19) == 401536 && sizeof(SnapshotV21) == 486016 &&
            sizeof(SnapshotV22) == 510592 && sizeof(SnapshotV23) == 510624 &&
            sizeof(Snapshot) == 543392,
        "Spring shares the exact saved native MegaJump record without changing the current version28 checkpoint layout");
    if (pad >= 0) {
        tick(dt, false, false);
        PinballPendingNative request = pinballPendingNative;
        check(
            springPlayerContactThisStepNative && request.pending && request.source == pad &&
                request.normal.x == 0 && request.normal.y == 1 && !pinballLaunchedThisStepNative &&
                jumps == 0 && pinballDetailVoiceCount("megajump") == 1 &&
                pinballDetailVoiceCount("jump") == 0,
            "Real spring contact plays its strong-bounce sound and queues world-up MegaJump without changing count or launching in the same step");
        float vx = approach(entities[playerIndex].state.vx,
                            entities[playerIndex].physics->num("max-velocity", 200),
                            entities[playerIndex].physics->num("move-acceleration", 500) * dt);
        tick(dt, false, true);
        check(
            pinballLaunchedThisStepNative && pinballConsumedNative.source == pad && jumps == 1 &&
                absf(entities[playerIndex].state.vx - vx) < .02f &&
                absf(entities[playerIndex].state.vy -
                     (request.speed + entities[playerIndex].physics->num("gravity", -400) * dt)) <
                    .02f &&
                pinballDetailVoiceCount("jump") == 0 && pinballDetailVoiceCount("megajump") == 1,
            "Spring consumes after next-step horizontal input and before the solver, counting once and adding no sideways normal kick or second sound");

        pad = prepareSpringPhaseNative();
        tick(dt, false, false);
        entities[playerIndex].state.timer = .049f;
        entities[pad].state.gone = 1;
        tick(dt, false, false);
        bool early = !pinballLaunchedThisStepNative && !pinballPendingNative.pending && jumps == 0;
        tick(dt, false, false);
        check(
            early && !pinballLaunchedThisStepNative && jumps == 0,
            "An actual spring request below the old0.05-second age guard is discarded and cannot launch later without a fresh contact");
        pad = prepareSpringPhaseNative();
        tick(dt, false, false);
        entities[playerIndex].state.timer = .05f;
        tick(dt, false, false);
        check(pinballLaunchedThisStepNative && pinballConsumedNative.source == pad && jumps == 1,
              "An actual spring request accepts the exact native0.05-second age endpoint");

        pad = prepareSpringPhaseNative();
        tick(dt, false, false);
        request = pinballPendingNative;
        captureCheckpoint(false);
        Snapshot *saved = (Snapshot *)HeapAlloc(GetProcessHeap(), 0, sizeof(Snapshot));
        *saved = checkpoint;
        char path[MAX_PATH];
        savePath(path, "spring-pending-v14.dat");
        bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
        clearPinballPendingNative();
        bool read = written && readCheckpoint(path);
        check(
            read && checkpoint.version == 28 && checkpoint.pinball.pending &&
                checkpoint.pinball.source == pad && checkpoint.pinball.normal.y == 1 &&
                checkpoint.pinball.speed == request.speed && validSnapshot(checkpoint),
            "An actual current-format disk checkpoint retains a spring request using the same native pending direction and speed fields as Pinball");
        stopSounds();
        restoreCheckpoint();
        bool restored = pinballPendingNative.pending && pinballPendingNative.source == pad &&
                        pinballDetailVoiceCount("megajump") == 0;
        tick(dt, false, false);
        check(
            restored && pinballLaunchedThisStepNative && jumps == 1 &&
                pinballDetailVoiceCount("megajump") == 0 && pinballDetailVoiceCount("jump") == 0,
            "Reading between spring contact and launch resumes once without replaying the already-completed contact sound");

        pinballPendingNative = request;
        PinballPlayerStepNative *effects = new PinballPlayerStepNative;
        nativePinballReset(*effects);
        effects->pending = true;
        effects->source = pad;
        effects->normal = Vec{1, 0};
        effects->speed = 999;
        nativePinballFinishContacts(entities, entityCount, *effects);
        check(
            pinballPendingNative.source == pad && pinballPendingNative.normal.x == 0 &&
                pinballPendingNative.normal.y == 1 && pinballPendingNative.speed == request.speed,
            "Flushing earlier Pinball effects cannot overwrite a later spring request recorded by the actual contact callback");
        delete effects;
        nativeMegaJumpRequest(pad, Vec{3, 4}, -400, 74, 0, 200);
        bool first = pinballPendingNative.normal.x > .59f && pinballPendingNative.normal.y > .79f;
        nativeMegaJumpRequest(pad, Vec{0, 1}, -400, 74, 0, 155);
        bool second = pinballPendingNative.normal.x == 0 &&
                      absf(pinballPendingNative.speed - root(800 * 155)) < .001f;
        check(
            first && second,
            "The shared MegaJump request retains the last callback direction and power instead of selecting the strongest spring by entity order");
        nativeMegaJumpRequest(pad, Vec{0, 1}, -200, 37, 120, 130);
        check(
            absf(pinballPendingNative.speed - root(400 * 157)) < .001f,
            "Spring uses the native gravity and max basic-plus-bonus height rule of the shared MegaJump controller");
        HeapFree(GetProcessHeap(), 0, saved);

        pad = prepareSpringPhaseNative();
        bodies[pad]->GetFixtureList()->SetSensor(true);
        for (int f = 0; f < 10; f++)
            tick(dt, false, false);
        check(
            !pinballPendingNative.pending && !springPlayerContactThisStepNative && jumps == 0 &&
                pinballDetailVoiceCount("megajump") == 0,
            "A sensor overlap cannot schedule or sound a spring impact without a solved physical contact");
        pad = prepareSpringPhaseNative();
        tick(dt, false, false);
        restartStage();
        check(!pinballPendingNative.pending && !springPlayerContactThisStepNative,
              "Restart discards queued spring launch and contact effects with the rebuilt world");
    }
    stopSounds();
    startStage(0);
    readyTimer = 0;
    lstrcpyA(saveRoot, oldRoot);
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
}
