#pragma once

// Regression coverage for resize checkpoint position native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void resizeCheckpointPositionNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/resize-position-saves");
    CreateDirectoryA(saveRoot, 0);
    interfaceEnabled = false;
    debugMode = true;
    musicEnabled = soundEnabled = false;
    const int sizes[][2] = {{1024, 768},  {1509, 177}, {2400, 280}, {800, 1200},
                            {1920, 1080}, {640, 320},  {1024, 768}};
    bool savedSame = true, restoredSame = true, bodiesSame = true, traceSame = true;
    State reference[61];
    char csv[48000] =
        "case,step,viewW100,viewH100,actorX1000,actorY1000,bodyX1000,bodyY1000,cameraX1000,cameraY1000\r\n";
    resize(1024, 768);
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    Entity &bean = entities[playerIndex];
    bean.state.x = 3600;
    bean.state.y = 80;
    bean.state.vx = bean.state.vy = 0;
    bean.state.timer = .2f;
    bean.state.hp = 1;
    restorePhysics();
    updateCamera();
    tick(1 / 120.0f, false, false);
    tick(1 / 120.0f, false, false);
    captureCheckpoint(false);
    u32 hash = checkpoint.checksum;
    State saved = checkpoint.state[playerIndex];
    for (int c = 0; c < 7; c++) {
        resize(sizes[c][0], sizes[c][1]);
        savedSame &=
            checkpoint.checksum == hash && hashBytes(&checkpoint, sizeof(checkpoint) - 4) == hash;
        bean.state.x = 3700;
        bean.state.y = 120;
        bean.state.vx = 60;
        restorePhysics();
        restoreCheckpoint();
        restoredSame &= bean.state.x == saved.x && bean.state.y == saved.y &&
                        bean.state.vx == saved.vx && bean.state.vy == saved.vy;
        for (int step = 0; step <= 60; step++) {
            if (step)
                tick(1 / 120.0f, false, false);
            b2Vec2 body = bodies[playerIndex]->GetPosition();
            bodiesSame &= absf(body.x / metres - bean.state.x) < .001f &&
                          absf(body.y / metres - bean.state.y) < .001f;
            if (!c)
                reference[step] = bean.state;
            else
                traceSame &= absf(reference[step].x - bean.state.x) < .001f &&
                             absf(reference[step].y - bean.state.y) < .001f;
            char row[220];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", c, step, (int)(viewW * 100),
                      (int)(viewH * 100), (int)(bean.state.x * 1000), (int)(bean.state.y * 1000),
                      (int)(body.x / metres * 1000), (int)(body.y / metres * 1000),
                      (int)(camX * 1000), (int)(camY * 1000));
            lstrcatA(csv, row);
        }
    }
    check(
        savedSame,
        "Changing among7normal ultra-wide and tall viewports cannot mutate the existing checkpoint bytes");
    check(
        restoredSame && bodiesSame,
        "Wizard checkpoint restore returns Bean and its real physics body to the exact saved world position and velocity at every viewport size");
    check(
        traceSame,
        "Bean follows the same first60physical steps after checkpoint restoration at normal ultra-wide and tall window sizes");
    bool entry = true;
    for (int c = 0; c < 7; c++) {
        resize(sizes[c][0], sizes[c][1]);
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == BOSS) {
                Entity &boss = entities[i];
                for (int area = 0; area < 5; area++) {
                    boss.state.hp = 5 - area;
                    boss.state.active = 2;
                    float arena = wizardArena(boss);
                    float offset = cameraFollow->num("offset-x"),
                          entryX = arena - offset * minf(viewW, 512);
                    bean.state.x = entryX - .25f;
                    camX = bean.state.x + offset * viewW - viewW * .5f;
                    entry &= !wizardArenaEntryReachedNative(boss);
                    bean.state.x = entryX + .25f;
                    camX = bean.state.x + offset * viewW - viewW * .5f;
                    entry &= wizardArenaEntryReachedNative(boss);
                }
            }
    }
    // The narrow phone-like viewport uses the native camera condition; for
    // desktop-expanded widths, Tom progress adds the authored framing guard.
    check(
        entry,
        "All5Wizard gates remain blocked until Tom crosses the authored entry line then remain reachable at all7tested viewport sizes");
    resize(1024, 768);
    wizardCameraBlend = WizardCameraBlendState{.5f, 1000, 20};
    float centreX = wizardCameraBlend.fromX + viewW * .5f,
          centreY = wizardCameraBlend.fromY + viewH * .5f;
    State before = bean.state;
    resize(1509, 177);
    check(
        absf(wizardCameraBlend.fromX + viewW * .5f - centreX) < .001f &&
            absf(wizardCameraBlend.fromY + viewH * .5f - centreY) < .001f &&
            bean.state.x == before.x && bean.state.y == before.y,
        "A live Wizard camera blend retains its world centre while the window becomes ultra-wide without changing Bean world coordinates");
    atomicWrite("test-output/resize-position-observations.csv", csv, lstrlenA(csv));
    atomicWrite("test-output/resize-position-results.txt", testLog, lstrlenA(testLog));
    resize(1024, 768);
    lstrcpyA(saveRoot, oldRoot);
    startStage(0);
    readyTimer = 0;
    debugMode = false;
}
