#pragma once

// Regression coverage for ready native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void readyNativeChecks() {
    ReadyNativeState intro = readyNativeInitial(false);
    readyNativeAdvance(intro, 0.44f, false, true, false);
    check(intro.showing && absf(intro.age - 0.44f) < 0.0001f,
          "Ready entry cannot be skipped before its native0.45 second input window");
    readyNativeAdvance(intro, 0.011f, false, true, false);
    check(
        intro.showing && intro.age == 2.0f,
        "Held horizontal input at the native wait boundary advances Ready to2 seconds rather than hiding it immediately");
    readyNativeAdvance(intro, 0.5f, false, false, false);
    bool strict = intro.showing;
    readyNativeAdvance(intro, 0.001f, false, false, false);
    check(strict && !intro.showing,
          "Ready hides strictly after2.5 seconds including its2.25-to2.5 panel exit");
    ReadyNativeState automatic = readyNativeInitial(false), resume = readyNativeInitial(true);
    readyNativeAdvance(automatic, 2.25f, false, false, false);
    check(
        automatic.showing && !resume.showing && readyNativeExit(automatic.age) == 0,
        "No-input Ready continues its natural timeline while a resumed checkpoint has no opening overlay");
    Rect view = {0, 0, 512, 448};
    Vec ready = {259.5f, 101.5f}, upper = {256, 416}, lower = {256, 8};
    Vec entry = readyNativePosition(0.2f, ready, view, true),
        settled = readyNativePosition(0.45f, ready, view, true),
        out = readyNativePosition(2.5f, ready, view, true);
    Vec top = readyNativePosition(2.375f, upper, view, false),
        bottom = readyNativePosition(2.375f, lower, view, false);
    check(
        entry.x == -256 && settled.x == 256 && out.y == 101.5f - 896 &&
            absf(top.y - (416 + 896 * (1 - 0.707106781f))) < 0.001f &&
            absf(bottom.y - (8 - 896 * (1 - 0.707106781f))) < 0.001f,
        "Original Ready enters from the left and upper/lower authored HUD actors retreat in opposite vertical directions using native half-cosine easing");
    int latch = 0;
    readyNativeCountMovement(latch, 110, 100);
    bool threshold = latch == 0;
    readyNativeCountMovement(latch, 110.001f, 100);
    readyNativeCountMovement(latch, 100, 100);
    check(
        threshold && latch == 1 && readyNativeRestoreCountLatch(0, 200, 100, 0) == 0 &&
            readyNativeRestoreCountLatch(1, 100, 100, 0) == 1 &&
            readyNativeRestoreCountLatch(3, 100, 100, 0) == 0 &&
            readyNativeRestoreCountLatch(3, 100, 100, 2) == 1,
        "Tom count latch uses strict10px movement, never unlatches on returning, preserves new snapshot booleans and migrates old generic hp3 snapshots");
    Arena memory;
    memory.init(8 * 1024 * 1024);
    Level hud = {};
    bool resource = false;
    for (int i = 0; i < levelCount; i++)
        if (levels[i].id == 14897698531695279339ULL) {
            readLevelData(levels[i].file, hud, memory);
            int found = 0;
            for (int n = 0; n < hud.objectCount; n++) {
                Object &o = hud.objects[n];
                Vec c = center(o.rect);
                if (o.id == 17323956314806549368ULL && c.x == 259.5f && c.y == 101.5f)
                    found++;
                if (o.id == 17109890148369247723ULL && c.y == 416)
                    found++;
                if (o.id == 11001010908721541493ULL && c.y == 8)
                    found++;
            }
            resource = found == 3;
            break;
        }
    check(resource,
          "The original HUD binds Ready and both black panels at the native source coordinates");
    releaseLevel(hud, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    debugMode = false;
    startStage(0);
    readyTimer = 0;
    jumps = 0;
    float oldY = entities[playerIndex].state.y, oldVy = entities[playerIndex].state.vy;
    int rises = 0;
    bool moved = false;
    for (int f = 0; f < 420; f++) {
        tick(1.0f / 120, false, false);
        State &p = entities[playerIndex].state;
        if (oldVy <= 0 && p.vy > 100)
            rises++;
        moved |= absf(p.y - oldY) > 1;
        oldVy = p.vy;
    }
    check(
        rises >= 2 && moved && jumps == 0 && entities[playerIndex].state.hp == 0,
        "A real stage Bean continues several automatic stationary jumps without incrementing the native displacement-gated counter");
    int stationaryJumps = jumps;
    bool enabled = false;
    for (int f = 0; f < 240; f++) {
        tick(1.0f / 120, false, true);
        enabled |= entities[playerIndex].state.hp == 1;
        if (jumps > stationaryJumps)
            break;
    }
    check(
        enabled && jumps > stationaryJumps,
        "Real horizontal stage movement enables jump counting and a subsequent genuine automatic bounce increments it");
    // The full opening runs through production tick and actual HUD rendering.
    interfaceEnabled = true;
    startStage(0);
    transitionTime = 0;
    float bornY = entities[playerIndex].state.y;
    bool introBounce = false;
    int readyNode = stageClearNode(17323956314806549368ULL),
        upperNode = stageClearNode(17109890148369247723ULL),
        lowerNode = stageClearNode(11001010908721541493ULL);
    bool geometry = readyNode >= 0 && upperNode >= 0 && lowerNode >= 0;
    float topY = 0, bottomY = 0, settledX = 0;
    for (int step = 1; step <= 312; step++) {
        tickInterface(1.0f / 120);
        tick(1.0f / 120, false, false);
        introBounce |= absf(entities[playerIndex].state.y - bornY) > 4;
        if (step == 30 || step == 72 || step == 285 || step == 312) {
            char path[96];
            wsprintfA(path, "test-output/ready-native-%03d.bmp", step);
            captureCase(path);
            if (geometry && step == 72) {
                settledX = center(stageClearNodeRect(readyNode)).x;
                topY = center(stageClearNodeRect(upperNode)).y;
                bottomY = center(stageClearNodeRect(lowerNode)).y;
            }
            if (geometry && step == 285)
                geometry &= center(stageClearNodeRect(upperNode)).y > topY + 100 &&
                            center(stageClearNodeRect(lowerNode)).y < bottomY - 100;
        }
    }
    check(
        introBounce && jumps == 0 && !readyOpening.showing && readyTimer == 0,
        "Actual Ready lets Bean bounce throughout its full no-input opening and exits automatically without incrementing stationary jumps");
    check(
        geometry && absf(settledX - viewW * 0.5f) < 0.01f,
        "Actual authored HUD Ready settles at viewport centre and both black panels move outward through production visualRect");
    startStage(0);
    pointerDown(0, 0);
    for (int step = 0; step < 56; step++)
        tick(1.0f / 120, false, false);
    bool pointerSkip = readyOpening.showing && readyOpening.age >= 2 && readyOpening.age < 2.1f;
    pointerUp(0, 0);
    check(
        pointerSkip && !readyPointerDown,
        "A pointer held from entry into the native wait window advances Ready to its exit timeline without hiding it immediately");
    startStage(0);
    readyTimer = 0;
    Entity &bean = entities[playerIndex];
    bean.state.hp = 1;
    bean.state.x = bean.originX;
    captureCheckpoint(false);
    bean.state.hp = 0;
    restoreCheckpoint();
    check(
        entities[playerIndex].state.hp == 1 &&
            entities[playerIndex].state.x == entities[playerIndex].originX,
        "Checkpoint restores the enabled jump latch even after Bean has returned to its initial X");
    interfaceEnabled = false;
    debugMode = false;
    startStage(0);
    readyTimer = 0;
}
