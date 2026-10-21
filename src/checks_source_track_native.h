#pragma once

// Regression coverage for source track native. Runs only in the isolated BQ_ENABLE_TESTS executable.

float sourceTrackDegreesCheckNative(float a) {
    return a * 57.29577951308232f;
}

int sourceTrackFindCheckNative() {
    for (int i = 0; i < entityCount; i++) {
        const Props *p = entities[i].source->behavior("BulletSource");
        if (p && eq(p->get("targetingMode"), "target"))
            return i;
    }
    return -1;
}

void sourceTrackNativeChecks() {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    debugMode = true;
    stopMusic();
    stopSounds();
    int sources = 0, targets = 0, scenes = 0;
    bool aligned = true, turning = true, shots = true, clocks = true;
    for (int stage = 0; stage < stageCount; stage++) {
        startStage(stage);
        readyTimer = 0;
        State initialBean = entities[playerIndex].state;
        int targetCount = 0;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].source->has("BulletSource"))
                sources++;
        for (int i = 0; i < entityCount; i++) {
            const Props *p = entities[i].source->behavior("BulletSource");
            if (!p || !eq(p->get("targetingMode"), "target"))
                continue;
            targets++;
            targetCount++;
            Entity &source = entities[i];
            BulletSourceState &state = bulletSourceStates[i];
            float initial = bqAtan2(initialBean.y - source.state.y, initialBean.x - source.state.x);
            aligned &= absf(state.angle - initial) < .00001f;
            memset(bullets, 0, sizeof(bullets));
            resetProjectileAnimationsNative();
            state.angle = 0;
            state.initialized = 2;
            state.clock = .75f;
            state.interval = 1;
            entities[playerIndex].state.x = source.state.x;
            entities[playerIndex].state.y = source.state.y + 100;
            for (int f = 0; f < 31; f++)
                updateBulletSource(source, 1.0f / 120);
            turning &= absf(sourceTrackDegreesCheckNative(state.angle) - 23.25f) < .001f;
            int count = 0;
            for (int slot = 0; slot < MAX_PROJECTILES; slot++)
                if (bullets[slot].life > 0 && bullets[slot].state.active == i + 1) {
                    count++;
                    shots &= absf(sourceTrackDegreesCheckNative(
                                      bqAtan2(bullets[slot].state.vy, bullets[slot].state.vx)) -
                                  23.25f) < .002f;
                }
            shots &= count == 1;
            clocks &= absf(state.clock - 1.0f / 120) < .00001f &&
                      absf(state.interval - p->num("timeMin")) < .000001f;
        }
        if (targetCount)
            scenes++;
    }
    check(
        sources == 33 && targets == 8 && scenes == 3,
        "Campaign tracking audit covers all33 actual launchers including8 target emitters in3 original scenes");
    check(
        aligned,
        "Actual green emitters initialize their retained direction toward the authored target rather than zero degrees");
    check(
        turning,
        "Every actual target emitter advances23.25degrees after31 controlled1/120updates at the original90degrees-per-second default");
    check(
        shots,
        "All8 actual green emitters fire along their retained23.25degree direction rather than snapping the shot to a90degree target");
    check(
        clocks,
        "Restoring native tracking leaves all8 emitters strict firing threshold, remainder and authored next interval intact");

    startStage(stageById(7138745870511843ULL));
    readyTimer = 0;
    int i = sourceTrackFindCheckNative();
    if (i < 0)
        return;
    Entity &source = entities[i];
    State &bean = entities[playerIndex].state;
    BulletSourceState &state = bulletSourceStates[i];
    state.angle = 0;
    state.initialized = 2;
    state.clock = 0;
    state.interval = 100;
    bean.x = source.state.x;
    bean.y = source.state.y + 100;
    updateBulletSource(source, .5f);
    bool capped = absf(sourceTrackDegreesCheckNative(state.angle) - 45) < .001f;
    updateBulletSource(source, 1);
    check(
        capped && absf(sourceTrackDegreesCheckNative(state.angle) - 90) < .001f,
        "Native turning caps angular travel by dt and lands exactly on a stationary target without overshoot");
    const float pi = 3.14159265358979323846f;
    state.angle = 179 * pi / 180;
    bean.x = source.state.x + bqCos(-179 * pi / 180) * 100;
    bean.y = source.state.y + bqSin(-179 * pi / 180) * 100;
    updateBulletSource(source, 1.0f / 120);
    check(absf(sourceTrackDegreesCheckNative(state.angle) - 179.75f) < .002f,
          "Tracking crosses the180degree boundary using the original shortest-angle direction");
    Object *original = source.source;
    Object custom = *original;
    Behavior behavior = {};
    Pair props[40];
    const Props &authored = *original->behavior("BulletSource");
    int n = 0;
    for (int k = 0; k < authored.count; k++)
        if (!eq(authored.pairs[k].key, "turnSpeed") &&
            !eq(authored.pairs[k].key, "targetValidAngles"))
            props[n++] = authored.pairs[k];
    props[n++] = Pair{(char *)"turnSpeed", (char *)"0"};
    behavior.type = "BulletSource";
    behavior.props = Props{n, props};
    custom.behaviorCount = 1;
    custom.behaviors = &behavior;
    source.source = &custom;
    state.angle = .25f;
    updateBulletSource(source, .5f);
    bool zero = state.angle == .25f;
    props[n - 1].value = "30";
    state.angle = 0;
    updateBulletSource(source, .5f);
    check(
        zero && absf(sourceTrackDegreesCheckNative(state.angle) - (-15)) < .002f,
        "Configured turnSpeed zero holds current aim and30degrees-per-second turns15degrees in half a second");
    props[n - 1].value = "90";
    props[n++] = Pair{(char *)"targetValidAngles", (char *)"0,90"};
    behavior.props.count = n;
    state.angle = 0;
    state.clock = 1;
    state.interval = 1;
    bean.x = source.state.x;
    bean.y = source.state.y + 100;
    memset(bullets, 0, sizeof(bullets));
    resetProjectileAnimationsNative();
    updateBulletSource(source, .25f);
    bool deferred = bullets[0].life == 0 && state.clock > 1;
    updateBulletSource(source, .75f);
    check(
        deferred && bullets[0].life > 0 && absf(bullets[0].state.vx) < .001f &&
            bullets[0].state.vy > 0,
        "Allowed-angle emitters keep their expired clock until native aim convergence then fire along the selected angle");
    source.source = original;
    state.angle = .4f;
    state.initialized = 2;
    state.clock = .2f;
    state.interval = 1;
    captureCheckpoint(false);
    char path[MAX_PATH];
    savePath(path, "source-track-v22.dat");
    bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    state.angle = -1;
    restoreCheckpoint();
    bool restored = state.angle == .4f;
    updateBulletSource(source, 1.0f / 120);
    check(
        restored && absf(state.angle - (.4f + pi / 240)) < .00001f,
        "Checkpoint loading retains the current aim and continues from that exact angle on the next source update");
    memset(&checkpoint, 0, sizeof(checkpoint));
    bool loaded = written && readCheckpoint(path);
    restoreCheckpoint();
    check(
        loaded && validSnapshot(checkpoint) && state.angle == .4f && state.initialized == 2,
        "An actual Snapshot28 disk round trip retains the native aiming marker and current radians");
    checkpoint.sources[i].initialized = 1;
    checkpoint.sources[i].angle = 0;
    restoreCheckpoint();
    check(
        absf(sourceTrackDegreesCheckNative(state.angle) - 90) < .001f && state.initialized == 2,
        "Legacy snapshots with an unused zero angle reconstruct target alignment without mistaking it for a valid retained aim");
    state.angle = .4f;
    screen = PAUSE;
    tick(.1f, false, false);
    bool paused = state.angle == .4f;
    screen = PLAY;
    source.state.active = 0;
    tick(1.0f / 120, false, false);
    check(paused && state.angle == .4f,
          "True task pause and a proximity-paused source do not advance the retained aiming angle");
    restartStage();
    float target = bqAtan2(bean.y - source.state.y, bean.x - source.state.x);
    check(
        absf(state.angle - target) < .00001f && sizeof(BulletSourceState) == 16 &&
            sizeof(SnapshotV22) == 510592 && sizeof(SnapshotV23) == 510624 &&
            sizeof(Snapshot) == 543392 && checkpoint.version == 28,
        "Restart realigns against the reset target while preserving Snapshot28 and the existing16byte source-state layout");
    for (int k = 0; k < entityCount; k++)
        if (k != playerIndex && k != i)
            entities[k].state.gone = 1;
    current.geometryCount = 0;
    bean.x = source.state.x;
    bean.y = source.state.y + 100;
    bean.vx = bean.vy = 0;
    source.state.active = 1;
    state.angle = 0;
    state.initialized = 2;
    state.clock = .75f;
    state.interval = 1;
    rebuildPhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    for (int f = 0; f < 31; f++)
        tick(1.0f / 120, false, false);
    int emitted = 0;
    bool integrated = true;
    for (int slot = 0; slot < MAX_PROJECTILES; slot++)
        if (bullets[slot].life > 0 && bullets[slot].state.active == i + 1) {
            emitted++;
            integrated &= shotBodies[slot] &&
                          absf(sourceTrackDegreesCheckNative(
                                   bqAtan2(bullets[slot].state.vy, bullets[slot].state.vx)) -
                               23.25f) < .002f;
        }
    check(
        emitted == 1 && integrated,
        "Actual gameplay tick and Box2D projectile creation preserve the gradually tracked firing vector");
    startStage(0);
    readyTimer = 0;
    debugMode = false;
}

bool sourceTrackDiskReloadNative() {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    debugMode = true;
    char path[MAX_PATH];
    savePath(path, "source-track-v22.dat");
    bool ok = readCheckpoint(path);
    haveCheckpoint = ok;
    int stage = checkpoint.stage;
    if (ok) {
        startStage(stage, true);
        int i = sourceTrackFindCheckNative();
        ok = i >= 0 && bulletSourceStates[i].initialized == 2 && bulletSourceStates[i].angle == .4f;
        if (ok) {
            updateBulletSource(entities[i], 1.0f / 120);
            ok =
                absf(bulletSourceStates[i].angle - (.4f + 3.14159265358979323846f / 240)) < .00001f;
        }
    }
    const char *result = ok ? "PASS fresh-process native source aim disk restoration\r\n"
                            : "FAIL fresh-process native source aim disk restoration\r\n";
    atomicWrite("test-output/source-track-fresh-process.txt", result, lstrlenA(result));
    return ok;
}
