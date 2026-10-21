#pragma once

// Regression coverage for helper camera restore native. Runs only in the isolated BQ_ENABLE_TESTS executable.
// Use the actual 4-2 automatic checkpoint and intact ascent geometry.
void helperCameraFreshDiskNativeChecks() {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    resize(1280, 720);
    startStage(stageById(669939580618ULL));
    readyTimer = 0;
    int bird = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == HELPER)
            bird = i;
    char path[MAX_PATH];
    savePath(path, "helper-camera-flight-native.dat");
    bool loaded = readCheckpoint(path);
    if (loaded)
        restoreCheckpoint();
    State &bean = entities[playerIndex].state;
    check(
        loaded && checkpoint.version == 28 && bird >= 0 && entities[bird].state.active == 1 &&
            absf(cameraState.window.t - bean.y) < .005f &&
            absf(camY - (bean.y - .45f * viewH)) < .005f,
        "A fresh process loads actual4-2flight checkpoint bytes at their existing tracking edge without recentering the dead zone");
    float before = bean.y, cy = camY;
    tick(1.0f / 120, false, false);
    check(
        loaded && bird >= 0 && entities[bird].state.active == 1 && bean.y > before &&
            absf((camY - cy) - (bean.y - before)) < .005f,
        "The first complete fresh-process4-2flight update moves the camera by Bean's actual ascent without a waiting distance");
    startStage(0);
    readyTimer = 0;
}

void helperCameraRestoreNativeChecks() {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    stopMusic();
    stopSounds();
    bool capturedAll = true;
    char rows[30000] =
        "fps,viewport,request,saved_y1000,first_y1000,first_camera1000,reference_camera1000,restored_camera1000,flight,trajectory\r\n";
    for (int rate = 0; rate < 4; rate++) {
        int fps = rate == 0 ? 30 : rate == 1 ? 60 : rate == 2 ? 120 : 240, frames = fps / 2;
        float dt = 1.0f / fps;
        bool camera = true, flight = true, reveal = true;
        for (int viewport = 0; viewport < 3; viewport++)
            for (int request = 0; request < 3; request++) {
                resize(1280, 720);
                startStage(stageById(669939580618ULL));
                readyTimer = 0;
                int bird = -1, point = -1;
                for (int i = 0; i < entityCount; i++) {
                    if (entities[i].kind == HELPER)
                        bird = i;
                    if (entities[i].source->id == 987869988781ULL)
                        point = i;
                }
                if (bird < 0 || point < 0) {
                    capturedAll = false;
                    continue;
                }
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
                updateCamera();
                bool captured = false;
                for (int step = 0; step < fps * 3; step++) {
                    tick(dt, false, false);
                    if (checkpoint.state[point].hp == 2 && checkpoint.state[bird].active == 1) {
                        captured = true;
                        break;
                    }
                }
                capturedAll &= captured;
                if (!captured)
                    continue;
                float savedY = checkpoint.state[playerIndex].y;
                char path[MAX_PATH];
                savePath(path, "helper-camera-flight-native.dat");
                capturedAll &= atomicWrite(path, &checkpoint, sizeof(checkpoint));
                if (viewport == 1)
                    resize(1800, 720);
                if (viewport == 2)
                    resize(1024, 768);
                float y[120], cy[120];
                for (int step = 0; step < frames; step++) {
                    tick(dt, false, false);
                    y[step] = bean.y;
                    cy[step] = camY;
                    capturedAll &= helper.state.active == 1;
                }
                bean.x = 1200;
                bean.y = 600;
                helper.state.x = 1200;
                helper.state.y = 634;
                helper.state.active = 2;
                restorePhysics();
                updateCamera();
                capturedAll &= readCheckpoint(path);
                if (request == 0)
                    restoreCheckpoint();
                else {
                    if (request == 1)
                        respawnBegin(respawnTransition, RESPAWN_CHECKPOINT);
                    else {
                        die();
                        tick(2.01f, false, false);
                    }
                    tick(respawnNativeFadeSeconds, false, false);
                    tick(respawnNativeFadeSeconds, false, false);
                }
                float restored = camY;
                bool caseFlight = !respawnBusy(respawnTransition) && helper.state.active == 1 &&
                                  absf(bean.y - savedY) < .001f &&
                                  absf(helper.state.y - checkpoint.state[bird].y) < .001f,
                     caseCamera = true;
                float expectedRestored = cy[0] - (y[0] - savedY);
                reveal &= absf(restored - expectedRestored) < .005f;
                float firstY = 0, firstCam = 0, previousY = bean.y;
                for (int step = 0; step < frames; step++) {
                    tick(dt, false, false);
                    caseFlight &= helper.state.active == 1 && bean.y > previousY;
                    previousY = bean.y;
                    caseCamera &= absf((camY - bean.y) - (cy[step] - y[step])) < .005f;
                    if (step == 0) {
                        firstY = bean.y;
                        firstCam = camY;
                    }
                }
                camera &= caseCamera;
                flight &= caseFlight;
                char row[300];
                wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", fps, viewport, request,
                          (int)(savedY * 1000), (int)(firstY * 1000), (int)(firstCam * 1000),
                          (int)((cy[0] + firstY - y[0]) * 1000), (int)(restored * 1000), caseFlight,
                          caseCamera);
                lstrcatA(rows, row);
            }
        char label[260];
        wsprintfA(
            label,
            "Actual4-2following-helper checkpoint camera retains uninterrupted Bean-relative tracking on every ascent frame at%dHz across direct/manual/death returns and three viewports",
            fps);
        check(camera, label);
        wsprintfA(
            label,
            "Actual4-2restored helper and Bean start at their exact saved positions and continue ascending at%dHz while the camera is checked independently",
            fps);
        check(flight, label);
        wsprintfA(
            label,
            "Actual4-2camera is already at its continuous-flight tracking position before the first revealed movement at%dHz",
            fps);
        check(reveal, label);
    }
    check(
        capturedAll,
        "All36camera cases capture the actual4-2automatic checkpoint while following and reload real private version28 disk bytes");
    atomicWrite("test-output/helper-camera-restore-trace.csv", rows, lstrlenA(rows));
    resize(1280, 720);
    startStage(stageById(669939580618ULL));
    readyTimer = 0;
    int bird = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == HELPER)
            bird = i;
    Entity &helper = entities[bird];
    State &bean = entities[playerIndex].state;
    bean.x = 272;
    bean.y = 1000;
    helper.state.active = 0;
    updateCamera(true);
    captureCheckpoint(false);
    helper.state.active = 1;
    bean.y = 1400;
    updateCamera();
    restoreCheckpoint();
    check(
        helper.state.active == 0 && absf(cameraState.window.b - (1000 - viewH * .125f)) < .005f &&
            absf(cameraState.window.t - (1000 + viewH * .125f)) < .005f,
        "Actual4-2nonflight saves retain native dead-zone reconstruction even if the current helper was following before restore");
    helper.state.active = 1;
    helper.state.timer = 1;
    helper.state.x = bean.x;
    helper.state.y = bean.y + 34;
    bean.vy = 60;
    restorePhysics();
    updateCamera(true);
    captureCheckpoint(false);
    checkpoint.camera.x -= viewW * .5f;
    checkpoint.camera.y -= viewH * .5f;
    checkpoint.bossCamera.fromX -= viewW * .5f;
    checkpoint.bossCamera.fromY -= viewH * .5f;
    SnapshotV21 *old = (SnapshotV21 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV21));
    memcpy(old, &checkpoint, sizeof(*old) - 4);
    old->version = 20;
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    char legacyPath[MAX_PATH];
    savePath(legacyPath, "helper-camera-flight-legacy20.dat");
    bool legacy = atomicWrite(legacyPath, old, sizeof(*old));
    HeapFree(GetProcessHeap(), 0, old);
    bean.y = 1400;
    helper.state.active = 2;
    restorePhysics();
    updateCamera();
    legacy &= readCheckpoint(legacyPath);
    restoreCheckpoint();
    check(
        legacy && checkpoint.version == 20 && helper.state.active == 1 &&
            absf(bean.y - 1000) < .005f && absf(cameraState.window.t - bean.y) < .005f &&
            absf(camY - (bean.y - .45f * viewH)) < .005f,
        "Real legacy version20 edge-coordinate flight saves keep immediate4-2tracking without a save-format change");
    resize(1024, 768);
    startStage(0);
    readyTimer = 0;
    soundEnabled = musicEnabled = false;
}
