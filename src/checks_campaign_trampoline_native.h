#pragma once

// Regression coverage for campaign trampoline native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int prepareCampaignTrampolineNative(bool high = true, bool keepTerrain = false) {
    startStage(stageById(806701038719ULL));
    readyTimer = 0;
    readyOpening.showing = false;
    if (!keepTerrain)
        current.geometryCount = 0;
    int pad = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == TRAMPOLINE && entities[i].source->id == 186898655684ULL) {
            pad = i;
            break;
        }
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && entities[i].kind != TRAMPOLINE)
            entities[i].state.gone = 1;
    if (pad < 0)
        return -1;
    Entity &bean = entities[playerIndex];
    bean.state.x = entities[pad].state.x;
    bean.state.y = high ? 340 : hitRect(entities[pad]).t - bean.hit.b + .2f;
    bean.state.vx = 0;
    bean.state.vy = -30;
    bean.state.hp = 1;
    bean.state.timer = .1f;
    jumps = 0;
    rebuildPhysics();
    stopSounds();
    return pad;
}

void campaignTrampolineNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/campaign-trampoline-saves");
    CreateDirectoryA(saveRoot, 0);
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldSound = soundEnabled,
         oldMusic = musicEnabled;
    interfaceEnabled = debugMode = musicEnabled = false;
    soundEnabled = true;
    stopMusic();
    const float dt = 1.0f / 120;
    int pad = prepareCampaignTrampolineNative();
    check(
        pad >= 0 && sizeof(SnapshotV13) == 295276 && sizeof(TrampolineStateNative) == 16 &&
            sizeof(SnapshotV14) == 295292 && sizeof(SnapshotV15) == 319868 &&
            sizeof(SnapshotV16) == 344444 && sizeof(SnapshotV17) == 364156 &&
            sizeof(SnapshotV18) == 366720 && sizeof(SnapshotV19) == 401536 &&
            sizeof(SnapshotV21) == 486016 && sizeof(SnapshotV22) == 510592 &&
            sizeof(SnapshotV23) == 510624 && sizeof(Snapshot) == 543392,
        "Campaign trampoline state appends16bytes and retains the exact prior version13 save layout");
    if (pad >= 0) {
        for (int f = 0; f < 400 && !trampolineStateNative.pending; f++)
            tick(dt, false, false);
        TrampolineStateNative first = trampolineStateNative;
        float contactY = bodies[playerIndex]->GetPosition().y / metres;
        int queuedCount = jumps;
        check(
            first.pending && first.source == pad && !trampolineLaunchedThisStepNative &&
                queuedCount == 0 && pinballDetailVoiceCount("jump") == 0 &&
                pinballDetailVoiceCount("megajump") == 0 && first.apex == 340,
            "An original campaign jelly landing retains its absolute340px apex and queues silently without counting a same-step launch");
        float expected = root(-2 * entities[playerIndex].physics->num("gravity", -400) *
                              (first.apex - nativeTrampolineRoundedY(contactY)));
        tick(dt, false, false);
        check(
            trampolineLaunchedThisStepNative && jumps == 1 &&
                absf(entities[playerIndex].lastLaunchSpeed - expected) < .001f &&
                pinballDetailVoiceCount("jump") == 1 && pinballDetailVoiceCount("megajump") == 0,
            "Campaign jelly consumes on the following step using rounded contact height and gravity, with one normal jump sound and no strong-bounce sound");

        int second = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == TRAMPOLINE && i != pad) {
                second = i;
                break;
            }
        bool transferred = false;
        if (second >= 0) {
            bool moved = false;
            for (int f = 0; f < 1000; f++) {
                if (!moved && entities[playerIndex].state.vy <= 0) {
                    entities[playerIndex].state.x = entities[second].state.x;
                    bodies[playerIndex]->SetTransform(
                        b2Vec2(entities[playerIndex].state.x * metres,
                               entities[playerIndex].state.y * metres),
                        0);
                    moved = true;
                }
                tick(dt, false, false);
                if (moved && trampolineStateNative.pending &&
                    trampolineStateNative.source == second) {
                    transferred = true;
                    break;
                }
            }
        }
        float secondY = bodies[playerIndex]->GetPosition().y / metres;
        TrampolineStateNative next = trampolineStateNative;
        float baseSpeed = entities[playerIndex].jumpPower;
        check(
            transferred && next.apex >= 340 && next.speed > baseSpeed + 100 &&
                absf(next.speed - root(800 * (next.apex - nativeTrampolineRoundedY(secondY)))) <
                    .001f,
            "A real second jelly at a different height inherits the first absolute apex instead of resetting to Tom's basic bounce height");
        tick(dt, false, false);
        check(
            transferred && trampolineLaunchedThisStepNative && jumps == 2 &&
                pinballDetailVoiceCount("jump") == 2 && pinballDetailVoiceCount("megajump") == 0,
            "Two original campaign pads produce two counted deferred launches without duplicate ordinary branches or strong-bounce voices");

        pad = prepareCampaignTrampolineNative(false, true);
        for (int f = 0; f < 60 && !trampolineStateNative.pending; f++)
            tick(dt, false, false);
        bool terrainContact = trampolineStateNative.pending && trampolineStateNative.source == pad;
        tick(dt, false, false);
        check(
            terrainContact && trampolineLaunchedThisStepNative && jumps == 1,
            "An authored jelly overlapping original terrain still schedules its own solved-contact launch independently of selected ground support");

        pad = prepareCampaignTrampolineNative(false);
        tick(dt, false, false);
        TrampolineStateNative pending = trampolineStateNative;
        int before = jumps;
        bool rejected = trampolineCampaignConsumeNative(.049f, false);
        bool noStale =
            !trampolineStateNative.pending && !trampolineCampaignConsumeNative(.1f, false);
        check(
            pending.pending && !rejected && noStale && jumps == before,
            "The native old-age guard discards an early campaign jelly request rather than retaining a delayed stale launch");
        trampolineStateNative = pending;
        check(trampolineCampaignConsumeNative(.05f, false) && jumps == before + 1,
              "Campaign trampoline accepts the exact native0.05-second old-age boundary once");
        trampolineStateNative = pending;
        before = jumps;
        check(
            !trampolineCampaignConsumeNative(.1f, true) && !trampolineStateNative.pending &&
                jumps == before,
            "An existing MegaJump request takes precedence and consumes the competing trampoline flag");

        pad = prepareCampaignTrampolineNative();
        for (int f = 0; f < 400 && !trampolineStateNative.pending; f++)
            tick(dt, false, false);
        pending = trampolineStateNative;
        captureCheckpoint(false);
        Snapshot *saved = (Snapshot *)HeapAlloc(GetProcessHeap(), 0, sizeof(Snapshot));
        *saved = checkpoint;
        char path[MAX_PATH];
        savePath(path, "pending-v14.dat");
        bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
        resetTrampolineStateNative(0);
        bool loaded = written && readCheckpoint(path);
        check(
            loaded && checkpoint.version == 28 && checkpoint.trampoline.pending &&
                checkpoint.trampoline.apex == pending.apex &&
                checkpoint.trampoline.speed == pending.speed && validSnapshot(checkpoint),
            "An actual version28 disk save preserves jelly pending state, shared apex and calculated speed");
        stopSounds();
        restoreCheckpoint();
        bool restored = trampolineStateNative.pending &&
                        trampolineStateNative.apex == pending.apex &&
                        pinballDetailVoiceCount("jump") == 0;
        tick(dt, false, false);
        check(
            restored && trampolineLaunchedThisStepNative && jumps == saved->jumps + 1 &&
                pinballDetailVoiceCount("jump") == 1,
            "Restoring between jelly contact and launch preserves the inherited height and plays its sound only on the next consumed launch");

        SnapshotV13 *legacy = (SnapshotV13 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV13));
        memcpy(legacy, saved, sizeof(*legacy) - 4);
        legacy->version = 13;
        legacy->pinball = PinballPendingNative{1, pad, Vec{0, 1}, 300};
        legacy->checksum = hashBytes(legacy, sizeof(*legacy) - 4);
        savePath(path, "legacy-v13.dat");
        written = atomicWrite(path, legacy, sizeof(*legacy));
        bool migrated = written && readCheckpoint(path);
        check(
            migrated && checkpoint.version == 13 && checkpoint.pinball.pending &&
                checkpoint.pinball.speed == 300 && !checkpoint.trampoline.pending &&
                checkpoint.count == legacy->count && checkpoint.jumps == legacy->jumps &&
                validSnapshot(checkpoint),
            "Exact version13 bytes retain existing Pinball and scene state while migration invents no historical jelly request");
        legacy->checksum ^= 1;
        savePath(path, "corrupt-v13.dat");
        atomicWrite(path, legacy, sizeof(*legacy));
        check(!readCheckpoint(path),
              "The exact version13 migration rejects a damaged historical checksum");
        checkpoint = *saved;
        checkpoint.trampoline.source = checkpoint.count;
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        bool badSource = !validSnapshot(checkpoint);
        checkpoint = *saved;
        checkpoint.trampoline.apex = 100001;
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        check(
            badSource && !validSnapshot(checkpoint),
            "Version14 validates pending jelly source and apex bounds even when its checksum is valid");
        HeapFree(GetProcessHeap(), 0, legacy);
        HeapFree(GetProcessHeap(), 0, saved);

        pad = prepareCampaignTrampolineNative(false);
        bodies[pad]->GetFixtureList()->SetSensor(true);
        for (int f = 0; f < 8; f++)
            tick(dt, false, false);
        check(!trampolineStateNative.pending && !trampolineLaunchedThisStepNative && jumps == 0,
              "An overlapping sensor jelly cannot synthesize a solved trampoline launch");
        pad = prepareCampaignTrampolineNative();
        trampolineStateNative.apex = 900;
        restartStage();
        check(
            !trampolineStateNative.pending &&
                trampolineStateNative.apex == entities[playerIndex].state.y,
            "Restart clears old campaign jelly requests and inherited height before a fresh opening");

        startStage(2);
        readyTimer = 0;
        readyOpening.showing = false;
        current.geometryCount = 0;
        int spring = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == SPRING && boxGroupMatrixOwner(entities[i])) {
                spring = i;
                break;
            }
        bool springCorrect = false;
        if (spring >= 0) {
            Object *source = entities[spring].source;
            for (int i = 0; i < entityCount; i++)
                if (i != playerIndex && entities[i].source != source)
                    entities[i].state.gone = 1;
            Entity &bean = entities[playerIndex];
            bean.state.x = entities[spring].state.x;
            bean.state.y = hitRect(entities[spring]).t - bean.hit.b + .2f;
            bean.state.vy = -30;
            bean.state.vx = 0;
            bean.state.hp = 1;
            bean.state.timer = .1f;
            jumps = 0;
            rebuildPhysics();
            stopSounds();
            for (int f = 0; f < 40 && jumps == 0; f++)
                tick(dt, false, false);
            springCorrect = jumps == 1 && pinballDetailVoiceCount("megajump") == 1 &&
                            pinballDetailVoiceCount("jump") == 0 &&
                            absf(bean.state.vy - (root(800 * entities[spring].jumpPower) +
                                                  bean.physics->num("gravity", -400) * dt)) < .1f;
        }
        check(
            springCorrect,
            "An original spring plays only megajump while preserving its contact-derived speed and single jump count");
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
