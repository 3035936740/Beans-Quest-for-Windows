#pragma once

// Regression coverage for tom jump diagnostics. Runs only in the isolated BQ_ENABLE_TESTS executable.

void tomJumpDiagnosticChecks() {
    char oldSave[MAX_PATH];
    lstrcpyA(oldSave, saveRoot);
    Progress oldProgress = progress;
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldMusic = musicEnabled,
         oldSound = soundEnabled;
    lstrcpyA(saveRoot, "test-output/tom-jump-diagnostic-saves");
    CreateDirectoryA("test-output", 0);
    CreateDirectoryA(saveRoot, 0);
    memset(&progress, 0, sizeof(progress));
    progress.unlocked = stageCount - 1;
    if (!hudScene.level.id)
        initInterface();
    interfaceEnabled = false;
    debugMode = true;
    musicEnabled = soundEnabled = false;
    const float dt = 1.0f / 120;
    short samples[368];
    int actualLaunches = 0, matched = 0, qualified = 0, rejected = 0;
    for (int enabled = 0; enabled < 2; enabled++) {
        soundEnabled = false;
        startStage(stageById(976871008727ULL));
        readyTimer = 0;
        int boss = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == BOSS)
                boss = i;
        if (boss >= 0) {
            Entity &wizard = entities[boss];
            wizard.state.hp = 1;
            wizard.state.active = 3;
            wizard.state.timer = 2.1f;
            wizardHurt(wizard);
            State &bean = entities[playerIndex].state;
            bean.x = 7100;
            bean.y = 320;
            bean.vx = bean.vy = 0;
            restorePhysics();
            for (int i = 0; i < 390; i++) {
                tick(dt, false, false);
                mixAudio(samples, 184);
            }
        }
        State &bean = entities[playerIndex].state;
        bean.x = 6816;
        bean.y = 300;
        bean.vx = 0;
        bean.vy = -40;
        restorePhysics();
        stopSounds();
        resetAudioJumpTrace();
        resetTomJumpDiagnostics();
        soundEnabled = enabled != 0;
        for (int i = 0; i < 720; i++) {
            float oldVy = bean.vy;
            u32 oldSeq = tomJumpDiagnosticSequence;
            tick(dt, false, false);
            if (ordinaryJumpLaunchedNative && bean.vy > 30) {
                actualLaunches++;
                if (tomJumpDiagnosticSequence > oldSeq) {
                    TomJumpDiagnostic &d = tomJumpDiagnostics[(tomJumpDiagnosticSequence - 1) &
                                                              (TOM_JUMP_DIAGNOSTIC_CAPACITY - 1)];
                    matched += d.launch_committed && d.branch_entered && d.jump_requests == 1 &&
                               d.sound_sequence_after == d.sound_sequence_before + 1 &&
                               (d.gate_mask & 3) == 3;
                    TomJumpDiagnostic &contact =
                        tomJumpDiagnostics[(oldSeq - 1) & (TOM_JUMP_DIAGNOSTIC_CAPACITY - 1)];
                    qualified += oldSeq && contact.sequence == oldSeq &&
                                 contact.step + 1 == d.step && contact.eligible > 0 &&
                                 contact.normal_y1000 >= 500;
                    rejected += d.rejected_requests == 1;
                }
            }
            mixAudio(samples, 184);
        }
    }
    check(
        actualLaunches >= 8 && actualLaunches == matched && qualified == actualLaunches,
        "Causal diagnostic links actual post-Wizard statue-platform solved support, upward impulse, positive launch and one jump request on every real bounce");
    check(
        rejected * 2 == actualLaunches,
        "Sound-disabled real bounces retain physics launches and record rejected requests separately from missing physics branches");

    soundEnabled = false;
    int stage = -1;
    for (int i = 0; i < stageCount; i++)
        if (levels[stageList[i]].props.num("bean-world") == 5 &&
            levels[stageList[i]].props.num("bean-level") == 4)
            stage = i;
    // Use the authored matrix and its actual contact callback, isolated from
    // unrelated terrain exactly as the existing magnet seam regression does.
    bool magnetOk = false;
    u32 magnetRows = 0;
    if (stage >= 0) {
        startStage(stage);
        readyTimer = 0;
        int owner = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == MAGNET && entities[i].source->id == 4715698496963111ULL &&
                magnetMatrixOwner(entities[i]))
                owner = i;
        if (owner >= 0) {
            Object *source = entities[owner].source;
            current.geometryCount = 0;
            for (int i = 0; i < entityCount; i++)
                if (i != playerIndex && entities[i].source != source)
                    entities[i].state.gone = 1;
            rebuildPhysics();
            State &bean = entities[playerIndex].state;
            bean.x = 656;
            bean.y = 224 - entities[playerIndex].hit.b + .1f;
            bean.vx = 0;
            bean.vy = -20;
            restorePhysics();
            resetAudioJumpTrace();
            resetTomJumpDiagnostics();
            for (int i = 0; i < 360; i++)
                tick(dt, false, false);
            magnetRows = tomJumpDiagnosticSequence;
            magnetOk = playerMagnetContact && onGround && magnetRows > 0 && magnetRows <= 8 &&
                       audioJumpTraceSequence == 0;
            for (u32 n = 0; n < magnetRows; n++) {
                TomJumpDiagnostic &d = tomJumpDiagnostics[n];
                magnetOk =
                    magnetOk && !d.launch_committed && !d.jump_requests && (d.context_flags & 16);
            }
        }
    }
    check(
        magnetOk,
        "Actual magnetic support records intentional suppressed launches and changes/one-second heartbeats without flooding the bounded ring");

    State before = entities[playerIndex].state;
    u32 audioBefore = audioJumpTraceSequence;
    resetTomJumpDiagnostics();
    for (int i = 0; i < TOM_JUMP_DIAGNOSTIC_CAPACITY + 17; i++) {
        tomJumpDiagnosticBegin(dt, false, false);
        tomJumpDiagnosticDecision();
        tomJumpDiagnosticBranch(120);
        tomJumpDiagnosticCommit();
        tomJumpDiagnosticEnd();
    }
    bool bounded = tomJumpDiagnosticSequence == TOM_JUMP_DIAGNOSTIC_CAPACITY + 17;
    u32 first = tomJumpDiagnosticSequence - TOM_JUMP_DIAGNOSTIC_CAPACITY + 1;
    for (u32 n = first; n <= tomJumpDiagnosticSequence; n++)
        bounded = bounded &&
                  tomJumpDiagnostics[(n - 1) & (TOM_JUMP_DIAGNOSTIC_CAPACITY - 1)].sequence == n;
    check(
        bounded && memcmp(&before, &entities[playerIndex].state, sizeof(State)) == 0 &&
            audioBefore == audioJumpTraceSequence,
        "Synthetic committed diagnostic events are never downsampled, wrap in sequence, and cannot mutate Bean or request audio");
    check(flushTomJumpDiagnostics("test-output/tom-jump-diagnostic-check.csv"),
          "Explicit diagnostic flush serializes the retained causal ring to its separate CSV");
    resetTomJumpDiagnostics();
    stopSounds();
    soundEnabled = musicEnabled = false;
    startStage(0);
    readyTimer = 0;
    lstrcpyA(saveRoot, oldSave);
    progress = oldProgress;
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    musicEnabled = oldMusic;
    soundEnabled = oldSound;
}
