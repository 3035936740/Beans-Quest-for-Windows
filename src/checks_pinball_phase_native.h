#pragma once

// Regression coverage for pinball phase native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void pinballPhaseNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/pinball-phase-saves");
    CreateDirectoryA(saveRoot, 0);
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldSound = soundEnabled,
         oldMusic = musicEnabled;
    interfaceEnabled = debugMode = musicEnabled = false;
    soundEnabled = true;
    stopMusic();
    const float dt = 1.0f / 120;
    int pin = preparePinballDetailNative(0);
    check(
        pin >= 0 && sizeof(SnapshotV12) == 295256 && sizeof(PinballPendingNative) == 20 &&
            sizeof(SnapshotV13) == 295276 && sizeof(SnapshotV14) == 295292 &&
            sizeof(SnapshotV15) == 319868 && sizeof(SnapshotV16) == 344444 &&
            sizeof(SnapshotV17) == 364156 && sizeof(SnapshotV18) == 366720 &&
            sizeof(SnapshotV19) == 401536 && sizeof(SnapshotV21) == 486016 &&
            sizeof(SnapshotV22) == 510592 && sizeof(SnapshotV23) == 510624 &&
            sizeof(Snapshot) == 543392,
        "Pinball pending state retains its original20byte version13 extension while preserving the full original version12 layout");
    if (pin >= 0) {
        tick(dt, false, false);
        PinballPendingNative request = pinballPendingNative;
        State first = entities[playerIndex].state;
        check(
            request.pending && request.source == pin && !pinballLaunchedThisStepNative &&
                jumps == 0 && pinballDetailVoiceCount("jump") == 0 &&
                pinballDetailVoiceCount("pinball") == 1 && first.vx > -100,
            "A real first Pinball contact plays its effect and queues MegaJump without changing jump count or applying its velocity in the same step");
        float expectedX =
            approach(first.vx, entities[playerIndex].physics->num("max-velocity", 200),
                     entities[playerIndex].physics->num("move-acceleration", 500) * dt) +
            request.speed * request.normal.x;
        tick(dt, false, true);
        check(
            pinballLaunchedThisStepNative && pinballConsumedNative.source == pin && jumps == 1 &&
                absf(entities[playerIndex].state.vx - expectedX) < .02f &&
                pinballDetailVoiceCount("jump") == 0,
            "The queued Pinball consumes after next-step movement input and before the solver, preserving the native horizontal addition order");

        pinballPendingNative = request;
        int count = jumps;
        float vx = entities[playerIndex].state.vx;
        bool early = nativePinballConsumePending(entities, entityCount, playerIndex, true, true,
                                                 jumps, .049f);
        bool cleared = !early && !pinballPendingNative.pending && jumps == count &&
                       entities[playerIndex].state.vx == vx;
        bool delayed =
            nativePinballConsumePending(entities, entityCount, playerIndex, true, true, jumps, .2f);
        check(
            cleared && !delayed && jumps == count,
            "The native fifty-millisecond guard discards an early Pinball request instead of allowing it to fire later without a fresh contact");
        pinballPendingNative = request;
        bool endpoint = nativePinballConsumePending(entities, entityCount, playerIndex, true, true,
                                                    jumps, .05f);
        check(
            endpoint && jumps == count + 1 && !pinballPendingNative.pending,
            "A Pinball request is eligible at the exact native0.05 jump-age endpoint and is consumed once");

        preparePinballDetailNative(0);
        tick(dt, false, false);
        request = pinballPendingNative;
        captureCheckpoint(false);
        Snapshot *retained = (Snapshot *)HeapAlloc(GetProcessHeap(), 0, sizeof(Snapshot));
        *retained = checkpoint;
        char path[MAX_PATH];
        savePath(path, "pending-v13.dat");
        bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
        clearPinballPendingNative();
        bool loaded = written && readCheckpoint(path);
        check(
            loaded && checkpoint.version == 28 && checkpoint.pinball.pending &&
                checkpoint.pinball.source == request.source &&
                checkpoint.pinball.normal.x == request.normal.x &&
                checkpoint.pinball.normal.y == request.normal.y &&
                checkpoint.pinball.speed == request.speed && validSnapshot(checkpoint),
            "An actual current-format disk checkpoint retains the pending Pinball flag, original contact direction and calculated speed");
        int sounds = pinballDetailVoiceCount("pinball");
        restoreCheckpoint();
        bool restored = pinballPendingNative.pending &&
                        pinballPendingNative.speed == request.speed && jumps == 0 &&
                        pinballDetailVoiceCount("pinball") == sounds;
        tick(dt, false, false);
        check(
            restored && pinballLaunchedThisStepNative &&
                pinballConsumedNative.source == request.source && jumps == 1,
            "Restoring between contact and launch resumes the queued MegaJump on the following step without replaying contact effects during restore");

        SnapshotV12 *legacy = (SnapshotV12 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV12));
        memcpy(legacy, retained, sizeof(*legacy) - 4);
        legacy->version = 12;
        legacy->checksum = hashBytes(legacy, sizeof(*legacy) - 4);
        savePath(path, "legacy-v12.dat");
        written = atomicWrite(path, legacy, sizeof(*legacy));
        bool migrated = written && readCheckpoint(path);
        check(
            migrated && checkpoint.version == 12 && !checkpoint.pinball.pending &&
                checkpoint.count == legacy->count && checkpoint.jumps == legacy->jumps &&
                checkpoint.bits == legacy->bits &&
                checkpoint.state[playerIndex].x == legacy->state[playerIndex].x &&
                validSnapshot(checkpoint),
            "Exact old version12 bytes migrate with scene and player state intact and no invented historical pending launch");
        legacy->checksum ^= 1;
        savePath(path, "corrupt-v12.dat");
        atomicWrite(path, legacy, sizeof(*legacy));
        check(!readCheckpoint(path),
              "A corrupt legacy version12 checksum is rejected by the new migration path");
        checkpoint = *retained;
        checkpoint.pinball.normal.x = 2;
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        bool invalidDirection = !validSnapshot(checkpoint);
        checkpoint = *retained;
        checkpoint.pinball.source = checkpoint.count;
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        check(
            invalidDirection && !validSnapshot(checkpoint),
            "Version13 rejects out-of-range pending directions and source indices even when the checksum is otherwise valid");
        HeapFree(GetProcessHeap(), 0, legacy);
        HeapFree(GetProcessHeap(), 0, retained);
    }
    stopSounds();
    clearPinballPendingNative();
    startStage(0);
    readyTimer = 0;
    lstrcpyA(saveRoot, oldRoot);
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
}
