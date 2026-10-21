#pragma once

// Regression coverage for native. Runs only in the isolated BQ_ENABLE_TESTS executable.

// Numeric fixtures derived from Camera::GetParalaxPosition at 0x155cd8,
// not from the port's previous background workaround.
void nativeChecks() {
    const float dt = 1 / 120.0f;
    musicEnabled = soundEnabled = false;
    interfaceEnabled = false;
    debugMode = false;
    progress.unlocked = stageCount - 1;
    check(b2_version.major == 2 && b2_version.minor == 2 && b2_version.revision == 0,
          "Linked Box2D version matches the original native library's 2.2.0");
    check(absf(layerCameraAxis(100, 512, 2000, 0.25f, 0, 2000, false, 0) - 281) < 0.001f,
          "Native parallax blends the camera centre with half the viewport");
    check(absf(layerCameraAxis(100, 512, 2000, 0.25f, 0, 2000, true, 0) - 839) < 0.001f,
          "Scroll-from-centre uses the layer centre without disabling scroll");
    check(absf(layerCameraAxis(624, 384, 1632, 1, 640, 1632, false, 0) - 320) < 0.001f,
          "Native auto-scroll maps the middle of a tall map into its authored sky");
    check(layerCameraAxis(-1000, 384, 1632, 1, 640, 1632, false, 0) == 192 &&
              layerCameraAxis(3000, 384, 1632, 1, 640, 1632, false, 0) == 448,
          "Auto-scroll clamps at both map boundaries");
    float lo = 144, hi = 240;
    bool deadzone = followCameraAxis(200, 96, 384, 1632, 0, &lo, &hi) == 192;
    deadzone &= followCameraAxis(300, 96, 384, 1632, 0, &lo, &hi) == 252;
    deadzone &= followCameraAxis(250, 96, 384, 1632, 0, &lo, &hi) == 252;
    deadzone &= followCameraAxis(180, 96, 384, 1632, 0, &lo, &hi) == 228;
    check(deadzone,
          "Native camera dead-zone remains still inside its window and tracks both exits");
    startStage(0);
    float fixedY = camY;
    entities[playerIndex].state.x = 900;
    entities[playerIndex].state.y = 700;
    updateCamera();
    check(camX == 644 && camY == fixedY,
          "Horizontal-only camera follows x immediately while preserving its disabled y axis");
    startStage(5);
    Layer &sky = current.layers[0];
    check(sky.autoScroll.y == 640 && sky.offset.y == 0,
          "Layer reader separates auto-scroll dimensions from runtime offset");
    bool skyCovers = true;
    for (int y = 0; y <= 1248; y += 16) {
        Vec shift = layerTranslation(current, 0, 0, (float)y);
        Rect r = shifted(current.tiles[0].rect, shift.x, shift.y);
        skyCovers &= r.b <= 0 && r.t >= 384;
    }
    check(skyCovers, "Stage 1-06 sky covers every camera height without stretching tile UVs");
    entities[playerIndex].state.y = 1410;
    updateCamera(true);
    check(camX == 0 && camY == 1170, "Vertical-only camera uses the authored 96-pixel dead-zone");
    captureCheckpoint(true);
    float savedCameraY = camY, savedCameraWindow = cameraState.window.b;
    entities[playerIndex].state.y = 300;
    updateCamera();
    restoreCheckpoint();
    check(camY == savedCameraY && cameraState.window.b == savedCameraWindow,
          "Checkpoint restoration preserves camera position and its dead-zone state");
    captureCase("test-output/native-sky-top.bmp");
    interfaceEnabled = true;
    navigate(worldScenes[4]);
    page.age = 3;
    page.minimized = true;
    page.minimizePhase = 1;
    page.cameraX = 0;
    page.cameraY = 0;
    float x3, y3, x4, y4;
    layerOffset(page, 3, &x3, &y3);
    layerOffset(page, 4, &x4, &y4);
    bool aligned = x3 == x4 && y3 == y4;
    page.cameraX = 352;
    layerOffset(page, 3, &x3, &y3);
    layerOffset(page, 4, &x4, &y4);
    aligned &= x3 == x4 && y3 == y4;
    check(aligned && !page.menuLayers[3] && !page.menuLayers[4] && sceneLayerVisible(page, 4),
          "World 5 terrain, gems and Bean use the same moving camera");
    check(!sceneLayerVisible(page, 6) && sceneLayerVisible(page, 7),
          "Minimized world 5 hides stage buttons while preserving back and restore controls");
    captureCase("test-output/native-select-world5-right.bmp");
    page.cameraX = 0;
    captureCase("test-output/native-select-world5-left.bmp");
    page.minimized = false;
    page.minimizePhase = 0;
    page.cameraX = page.homeX;
    captureCase("test-output/native-select-world5-menu.bmp");
    bool allSelectors = true;
    for (int w = 0; w < 5; w++) {
        navigate(worldScenes[w]);
        page.minimized = true;
        page.cameraX = 48;
        for (int i = 0; i < page.playfieldCount; i++) {
            Entity &e = page.playfield[i];
            allSelectors &= !page.menuLayers[e.layer] && sceneLayerVisible(page, e.layer);
        }
    }
    check(allSelectors, "All five selectors preserve authored playfield layers when minimized");
    interfaceEnabled = false;
    startStage(49);
    int boss = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            boss = i;
    if (boss >= 0) {
        Entity &b = entities[boss];
        b.state.x = 4000;
        b.state.hp = 5;
        b.state.timer = 0.72f;
        wizardMotion.pendingCount = 0;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].bossSpawn == 3 && wizardMotion.pendingCount < 3)
                wizardMotion.pending[wizardMotion.pendingCount++] = i;
        wizardMaterializeSummons(b);
        int bird = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].bossSpawn == 3 && !entities[i].state.gone)
                bird = i;
        if (bird >= 0) {
            float origin = entities[bird].originX;
            captureCheckpoint(true);
            tick(dt, false, false);
            float expectedX = entities[bird].state.x, expectedY = entities[bird].state.y;
            readProgress();
            musicEnabled = soundEnabled = false;
            startStage(49, true);
            tick(dt, false, false);
            check(entities[bird].originX == origin &&
                      absf(entities[bird].state.x - expectedX) < 1 &&
                      absf(entities[bird].state.y - expectedY) < 1,
                  "Checkpoint reload preserves a summoned bird's moving origin");
            restartStage();
            check(entities[bird].originX == initialMotion[bird].originX,
                  "Full retry resets mutable enemy origins as well as positions");
        }
        b.state.x = 4000;
        b.state.hp = 5;
        b.state.timer = 0.58f;
        wizardMotion.pendingCount = 0;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].bossSpawn == 2 && wizardMotion.pendingCount < 3)
                wizardMotion.pending[wizardMotion.pendingCount++] = i;
        wizardMaterializeSummons(b);
        bool walker = false;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].bossSpawn == 2 && !entities[i].state.gone)
                walker = entities[i].speed ==
                             entities[i].source->behavior("WalkBehaviour")->num("walk-speed") &&
                         entities[i].speed > 0 && entities[i].dynamic;
        check(walker, "Boss spawning preserves the enemy class's speed and dynamic physics");
    }
    startStage(0);
    SnapshotV4 *v4 = (SnapshotV4 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV4));
    memset(v4, 0, sizeof(*v4));
    memcpy(v4, &checkpoint, sizeof(LegacySnapshot) - 4);
    for (int i = 0; i < entityCount; i++) {
        MotionState &m = checkpoint.motion[i];
        v4->motion[i] = LegacyMotionState{m.originX, m.originY, m.turnCooldown, m.valid};
    }
    v4->version = 4;
    v4->checksum = hashBytes(v4, sizeof(*v4) - 4);
    char path[MAX_PATH];
    savePath(path, "native-v4-checkpoint.dat");
    atomicWrite(path, v4, sizeof(*v4));
    check(readCheckpoint(path) && checkpoint.version == 4 && !checkpoint.camera.valid &&
              checkpoint.motion[playerIndex].valid,
          "Version 4 snapshots migrate with entity motion intact and camera safely unset");
    HeapFree(GetProcessHeap(), 0, v4);
    startStage(0);
    SnapshotV7 *v7 = (SnapshotV7 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV7));
    memcpy(v7, &checkpoint, sizeof(*v7) - 4);
    v7->version = 7;
    v7->checksum = hashBytes(v7, sizeof(*v7) - 4);
    savePath(path, "native-v7-checkpoint.dat");
    atomicWrite(path, v7, sizeof(*v7));
    check(readCheckpoint(path) && checkpoint.version == 7 && checkpoint.camera.valid &&
              checkpoint.motion[playerIndex].lastLaunchSpeed > 0,
          "Version seven checkpoints migrate while retaining camera and actual jump velocity");
    HeapFree(GetProcessHeap(), 0, v7);
    LegacySnapshot *old = (LegacySnapshot *)HeapAlloc(GetProcessHeap(), 0, sizeof(LegacySnapshot));
    memcpy(old, &checkpoint, sizeof(*old) - 4);
    old->version = 3;
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    savePath(path, "native-legacy-checkpoint.dat");
    atomicWrite(path, old, sizeof(*old));
    check(readCheckpoint(path) && checkpoint.version == 3 && checkpoint.stage == 0 &&
              !checkpoint.motion[playerIndex].valid,
          "Existing version 3 checkpoint files remain readable after adding motion state");
    old->coins++;
    atomicWrite(path, old, sizeof(*old));
    check(!readCheckpoint(path), "Legacy checkpoint conversion still rejects a damaged checksum");
    HeapFree(GetProcessHeap(), 0, old);
    interfaceEnabled = true;
    startStage(49);
    openSelect();
    check(screen == WORLD && page.level.id == 18006219643615701355ULL,
          "Final stage returns to its authored world selector");
}
