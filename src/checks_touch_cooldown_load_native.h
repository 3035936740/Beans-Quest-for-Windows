#pragma once

// Regression coverage for touch cooldown load native. Runs only in the isolated BQ_ENABLE_TESTS executable.
int touchClockVoicesNative() {
    AudioClip *clip = 0;
    for (int i = 0; i < soundClipCount; i++)
        if (eq(soundClips[i].name, "switch"))
            clip = &soundClips[i].clip;
    int n = 0;
    for (int i = 0; i < 32; i++)
        n += clip && audioVoices[i].clip == clip;
    return n;
}

bool touchClockFreshDiskNative() {
    int target = touchAlternatorNativeFixture(false);
    char path[MAX_PATH];
    savePath(path, "touch-clock-native.dat");
    bool ok = target >= 0 && readCheckpoint(path);
    if (ok)
        restoreCheckpoint();
    int count = 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch) {
            count++;
            ok &= entities[i].state.timer == .1f && !touchAlternatorPending[i];
        }
    check(
        ok && count == 10 && checkpoint.version == 28 && sizeof(Snapshot) == 543392,
        "Fresh-process touch checkpoint load retains newly constructed0.1cooldowns rather than loading the saved guard ages");
    return ok && count == 10;
}

void touchCooldownLoadNativeChecks() {
    int target = touchAlternatorNativeFixture(false), count = 0;
    bool ctor = true;
    char inventory[1800] = "class_id,on_frames,off_frames\r\n";
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch) {
            count++;
            ctor &= entities[i].state.timer == .1f && !touchAlternatorPending[i];
        }
    check(
        target >= 0 && count == 10 && ctor,
        "All ten original touch blocks construct0.1guard ages and no pending contact once per actor lifetime");
    for (int i = 0; i < classCount; i++)
        if (classes[i].has("AlternatorTouchBlockBehaviour")) {
            Entity e;
            initializeEntity(e, classes[i], classes[i].rect);
            char row[150];
            wsprintfA(row, "%I64u,%d,%d\r\n", classes[i].id, animation(e.art, "on")->count,
                      animation(e.art, "off")->count);
            lstrcatA(inventory, row);
        }
    atomicWrite("test-output/touch-clock-class-inventory.csv", inventory, lstrlenA(inventory));
    const float ages[] = {0, .012f, .091f, .1f, .123f, .75f};
    bool matrix = true;
    for (int save = 0; save < 3; save++)
        for (int age = 0; age < 6; age++) {
            target = touchAlternatorNativeFixture(false);
            for (int i = 0; i < entityCount; i++)
                if (entities[i].touch)
                    entities[i].state.timer = .001f + .025f * save;
            captureCheckpoint(false);
            float before[MAX_ENTITIES];
            for (int i = 0; i < entityCount; i++)
                if (entities[i].touch) {
                    entities[i].state.timer = ages[age] + i * .000001f;
                    before[i] = entities[i].state.timer;
                }
            restoreCheckpoint();
            for (int i = 0; i < entityCount; i++)
                if (entities[i].touch)
                    matrix &= entities[i].state.timer == before[i];
        }
    check(
        matrix,
        "All ten original touch blocks retain their current unsaved guard across18saved/current age combinations without rewinding or refilling it");
    bool actual = true, guard = true, mature = true, pending = true, death = true, manual = true;
    char trace[5000] = "fps,guard_contact,guard_bits,guard_age1000,mature,pending,death,manual\r\n";
    for (int rate = 0; rate < 3; rate++) {
        int fps = rate == 0 ? 30 : rate == 1 ? 120 : 240;
        float dt = 1.0f / fps;
        target = touchAlternatorNativeFixture();
        touchAlternatorNativePlace(target, -30);
        captureCheckpoint(false);
        entities[target].state.timer = .012f;
        restoreCheckpoint();
        int bits = alternationBits;
        TouchAlternatorNativeObserver observer;
        observer.target = target;
        observer.solved = 0;
        observer.normalY = -2;
        physicsWorld->SetContactListener(&observer);
        stopSounds();
        soundEnabled = true;
        tick(dt, false, false);
        bool real = observer.solved > 0 && observer.normalY > .9f,
             blocked = alternationBits == bits && touchClockVoicesNative() == 0;
        actual &= real;
        guard &= blocked;
        float guardAge = entities[target].state.timer;
        physicsWorld->SetContactListener(originalContacts);
        stopSounds();
        soundEnabled = false;
        target = touchAlternatorNativeFixture();
        touchAlternatorNativePlace(target, -30);
        captureCheckpoint(false);
        entities[target].state.timer = .45f;
        restoreCheckpoint();
        bool kept = entities[target].state.timer == .45f;
        bits = alternationBits;
        stopSounds();
        soundEnabled = true;
        tick(dt, false, false);
        bool matureCase = kept && alternationBits == (bits ^ entities[target].mask) &&
                          touchClockVoicesNative() == 1;
        touchAlternatorPostStepNative(0);
        matureCase &= touchClockVoicesNative() == 1;
        mature &= matureCase;
        stopSounds();
        soundEnabled = false;
        target = touchAlternatorNativeFixture();
        touchAlternatorNativePlace(target, -30);
        captureCheckpoint(false);
        simulateBodies(dt);
        bool solved = touchAlternatorPending[target];
        entities[target].state.timer = .45f;
        restoreCheckpoint();
        bool retained = touchAlternatorPending[target] && entities[target].state.timer == .45f;
        bits = alternationBits;
        stopSounds();
        soundEnabled = true;
        touchAlternatorPostStepNative(dt);
        bool pendingCase = solved && retained &&
                           alternationBits == (bits ^ entities[target].mask) &&
                           !touchAlternatorPending[target] && touchClockVoicesNative() == 1;
        pending &= pendingCase;
        stopSounds();
        soundEnabled = false;
        bool transitions[2] = {};
        for (int kind = 0; kind < 2; kind++) {
            target = touchAlternatorNativeFixture();
            touchAlternatorNativePlace(target, -30);
            captureCheckpoint(false);
            int savedBits = checkpoint.bits;
            simulateBodies(dt);
            touchAlternatorPostStepNative(dt);
            bool toggled = alternationBits == (savedBits ^ entities[target].mask) &&
                           entities[target].state.timer == 0;
            if (kind == 0)
                die();
            else
                respawnBegin(respawnTransition, RESPAWN_CHECKPOINT);
            bool loaded = false;
            for (int f = 0; f < fps * 6; f++) {
                RespawnRequest old = respawnTransition.request;
                tick(dt, false, false);
                if (old != RESPAWN_NONE && respawnTransition.phase == RESPAWN_FADE_IN) {
                    loaded = true;
                    break;
                }
            }
            transitions[kind] = toggled && loaded && alternationBits == savedBits &&
                                entities[target].state.timer == 0;
        }
        death &= transitions[0];
        manual &= transitions[1];
        char row[250];
        wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d\r\n", fps, real, blocked, (int)(guardAge * 1000),
                  matureCase, pendingCase, transitions[0], transitions[1]);
        lstrcatA(trace, row);
    }
    check(
        actual,
        "Restored original touch platforms receive genuine top PostSolve contacts at30/120/240Hz");
    check(
        guard,
        "A restored young native guard rejects the first real landing and emits no switch voice at30/120/240Hz");
    check(
        mature,
        "A restored mature guard retains its age and accepts one actual landing/switch voice without repeat at30/120/240Hz");
    check(
        pending,
        "A genuine solved contact remains pending across same-object LoadState and is consumed once by the original touch guard");
    check(
        death,
        "Death fade restores saved touch parity while retaining the current zero guard from the actual last toggle at30/120/240Hz");
    check(
        manual,
        "Manual checkpoint fade restores saved touch parity without refilling the current guard at30/120/240Hz");
    atomicWrite("test-output/touch-clock-load-trace.csv", trace, lstrlenA(trace));
    target = touchAlternatorNativeFixture(false);
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch)
            entities[i].state.timer = .012f;
    captureCheckpoint(false);
    char path[MAX_PATH];
    savePath(path, "touch-clock-native.dat");
    bool disk = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch)
            entities[i].state.timer = .067f;
    disk &= readCheckpoint(path);
    restoreCheckpoint();
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch)
            disk &= entities[i].state.timer == .067f;
    check(
        disk && checkpoint.version == 28 && sizeof(Snapshot) == 543392,
        "Actual version28 disk read restores touch state and mask while preserving current unsaved guard ages");
    target = touchAlternatorNativeFixture(false);
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch)
            entities[i].state.timer = .023f;
    restartStage();
    bool restart = true;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch)
            restart &= entities[i].state.timer == .023f;
    check(
        restart,
        "RestartStage loads initial state on existing touch actors without resetting their unsaved guards to a constructor value");
    bool carried = true;
    for (int rate = 0; rate < 3; rate++) {
        float dt = 1.0f / (rate == 0 ? 30 : rate == 1 ? 120 : 240);
        target = touchAlternatorNativeFixture();
        touchAlternatorNativePlace(target, -30);
        captureCheckpoint(false);
        simulateBodies(dt);
        bool accepted = touchAlternatorPending[target];
        entities[target].state.timer = .45f;
        restoreCheckpoint();
        bool restored = touchAlternatorPending[target];
        State &bean = entities[playerIndex].state;
        bean.x = entities[target].state.x + 300;
        bean.y = entities[target].state.y + 300;
        bean.vx = bean.vy = 0;
        bodies[playerIndex]->SetTransform(b2Vec2(bean.x * metres, bean.y * metres), 0);
        bodies[playerIndex]->SetLinearVelocity(b2Vec2(0, 0));
        int bits = alternationBits;
        stopSounds();
        soundEnabled = true;
        tick(dt, false, false);
        bool once = alternationBits == (bits ^ entities[target].mask) &&
                    touchClockVoicesNative() == 1 && !touchAlternatorPending[target];
        tick(dt, false, false);
        carried &= accepted && restored && once &&
                   alternationBits == (bits ^ entities[target].mask) &&
                   touchClockVoicesNative() == 1;
        soundEnabled = false;
        stopSounds();
    }
    check(
        carried,
        "A loaded genuine touch event survives the next complete physics tick without a new contact and is consumed exactly once at30/120/240Hz");
    startStage(stageById(1966799885083198ULL));
    bool fresh = true;
    count = 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch) {
            count++;
            fresh &= entities[i].state.timer == .1f && !touchAlternatorPending[i];
        }
    check(
        fresh && count == 10,
        "Starting a genuinely new level creates fresh0.1touch guards rather than leaking previous scene ages");
    soundEnabled = false;
    stopSounds();
    startStage(0);
}
