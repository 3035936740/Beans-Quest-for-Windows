#pragma once

// Regression coverage for camera restore native. Runs only in the isolated BQ_ENABLE_TESTS executable.
void cameraRestoreNativeChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = debugMode = false;
    stopMusic();
    stopSounds();
    resize(1024, 768);
    startStage(stageById(11666231891817675739ULL));
    readyTimer = 0;
    entities[playerIndex].state.y = 1000;
    updateCamera(true);
    captureCheckpoint(false);
    entities[playerIndex].state.y = 1400;
    updateCamera();
    restoreCheckpoint();
    check(
        absf(cameraState.window.b - 1000) < .001f && absf(cameraState.window.t - 1096) < .001f &&
            absf(camY - 856) < .001f,
        "Camera-before-Bean native load order rebuilds from the old target then settles against restored Bean before reveal");
    startStage(stageById(177640755988ULL));
    readyTimer = 0;
    entities[playerIndex].state.y = 1000;
    updateCamera(true);
    captureCheckpoint(false);
    entities[playerIndex].state.y = 1400;
    updateCamera();
    restoreCheckpoint();
    check(
        absf(cameraState.window.b - 952) < .001f && absf(cameraState.window.t - 1048) < .001f &&
            absf(camY - 808) < .001f,
        "Bean-before-camera native load order rebuilds the dead zone around the restored target rather than replaying its saved edge");
    startStage(0);
    readyTimer = 0;
    entities[playerIndex].state.x = 900;
    updateCamera();
    captureCheckpoint(false);
    check(
        checkpoint.version == 28 && sizeof(SnapshotV21) == 486016 &&
            sizeof(SnapshotV22) == 510592 && sizeof(SnapshotV23) == 510624 &&
            sizeof(Snapshot) == 543392 && absf(checkpoint.camera.x - 900) < .001f,
        "Version28 records native world camera centres while retaining the exact486016byte version20/21 prefix");
    resize(1800, 768);
    entities[playerIndex].state.x = 1500;
    updateCamera();
    restoreCheckpoint();
    float restored = camX;
    updateCamera();
    check(
        absf(restored - 450) < .001f && camX == restored,
        "Widening from512to900world pixels before reload settles the camera immediately with no first-live-frame horizontal flash");
    // Real legacy bytes use edge coordinates, not version21+ centre semantics.
    resize(1024, 768);
    startStage(0);
    readyTimer = 0;
    entities[playerIndex].state.x = 900;
    updateCamera();
    captureCheckpoint(false);
    bool centreFormat = checkpoint.version >= 21;
    checkpoint.version = 20;
    if (centreFormat) {
        checkpoint.camera.x -= viewW * .5f;
        checkpoint.camera.y -= viewH * .5f;
        checkpoint.bossCamera.fromX -= viewW * .5f;
        checkpoint.bossCamera.fromY -= viewH * .5f;
    }
    SnapshotV21 *legacy20 = (SnapshotV21 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV21));
    memcpy(legacy20, &checkpoint, sizeof(*legacy20) - 4);
    legacy20->checksum = hashBytes(legacy20, sizeof(*legacy20) - 4);
    char path[MAX_PATH];
    savePath(path, "camera-legacy20.dat");
    bool written = atomicWrite(path, legacy20, sizeof(*legacy20));
    HeapFree(GetProcessHeap(), 0, legacy20);
    resize(1800, 768);
    entities[playerIndex].state.x = 1500;
    updateCamera();
    bool loaded = written && readCheckpoint(path);
    restoreCheckpoint();
    check(
        loaded && checkpoint.version == 20 && validSnapshot(checkpoint) && absf(camX - 450) < .001f,
        "Actual legacy version20 edge-coordinate file remains readable and settles its ordinary follower under a wider viewport");
    resize(1024, 768);
    startStage(0);
    readyTimer = 0;
    entities[playerIndex].state.x = 900;
    updateCamera();
    captureCheckpoint(false);
    resize(1800, 768);
    entities[playerIndex].state.x = 1500;
    updateCamera();
    respawnBegin(respawnTransition, RESPAWN_CHECKPOINT);
    tick(.35f, false, false);
    bool black = respawnAlpha(respawnTransition) == 1 && absf(camX - 450) < .001f;
    float first = camX;
    tick(.175f, false, false);
    bool revealing = absf(respawnAlpha(respawnTransition) - .5f) < .001f && camX == first;
    tick(.175f, false, false);
    updateCamera();
    check(
        black && revealing && camX == first,
        "Actual checkpoint fade restores and settles the wide camera while opaque and keeps it stable throughout reveal");
    resize(1024, 768);
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    int boss = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            boss = i;
    if (boss >= 0) {
        entities[boss].state.active = 3;
        Rect before = cameraState.window;
        entities[playerIndex].state.x += 300;
        updateCamera();
        check(hashBytes(&before, sizeof(before)) == hashBytes(&cameraState.window, sizeof(before)),
              "Wizard combat detaches the follower without advancing its dead-zone rectangle");
        entities[boss].state.active = 4;
        entities[boss].state.hp = 0;
        camX = 1000;
        camY = 0;
        cameraState.x = camX;
        cameraState.y = camY;
        captureCheckpoint(false);
        resize(1800, 768);
        restoreCheckpoint();
        check(
            absf(camX + viewW * .5f - 1256) < .001f,
            "Detached Wizard death camera restores its saved world centre across viewport widths");
        resize(1024, 768);
        entities[boss].state.active = 5;
        entities[boss].state.gone = 1;
        entities[playerIndex].state.x = 900;
        wizardCameraBlend = WizardCameraBlendState{.5f, 1000, 0};
        updateCamera();
        wizardPostCameraStep(0);
        captureCheckpoint(false);
        savePath(path, "camera-centre21.dat");
        written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
        resize(1800, 768);
        entities[playerIndex].state.x = 1400;
        updateCamera();
        wizardCameraBlend = WizardCameraBlendState{};
        memset(&checkpoint, 0, sizeof(checkpoint));
        loaded = written && readCheckpoint(path);
        restoreCheckpoint();
        float firstBlend = camX;
        updateCamera();
        wizardPostCameraStep(0);
        check(
            loaded && checkpoint.version == 28 && validSnapshot(checkpoint) &&
                wizardCameraBlend.remaining == .5f && absf(wizardCameraBlend.fromX - 806) < .001f &&
                absf(firstBlend - 628) < .001f && camX == firstBlend,
            "Actual version28 disk reload preserves the Wizard blend origin as a world centre across window widths without a reveal-frame jump");
    } else {
        check(false,
              "Wizard combat detaches the follower without advancing its dead-zone rectangle");
        check(
            false,
            "Detached Wizard death camera restores its saved world centre across viewport widths");
        check(
            false,
            "Actual version28 disk reload preserves the Wizard blend origin as a world centre across window widths without a reveal-frame jump");
    }
    resize(1024, 768);
    startStage(0);
    readyTimer = 0;
}
