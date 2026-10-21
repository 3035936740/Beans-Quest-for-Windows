#pragma once

// Regression coverage for coin sparkle native. Runs only in the isolated BQ_ENABLE_TESTS executable.

u32 coinSparklePixelsNative(Entity &e, Level &scene, const char *reference = 0) {
    glViewport(viewportX, viewportY, viewportW, viewportH);
    nativeViewportProjection();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();
    glTranslatef(128 - e.state.x, 128 - e.state.y, 0);
    if (reference)
        drawActor(e.art, entityRect(e), reference, e.state.timer, e.state.angle);
    else
        drawEntity(e, &scene, 0, 0, 0);
    glPopMatrix();
    glFinish();
    float scale = viewportW / viewW;
    int width = (int)(96 * scale), size = width * width * 4;
    u8 *data = (u8 *)HeapAlloc(GetProcessHeap(), 0, size);
    glReadBuffer(GL_BACK);
    glReadPixels(viewportX + (int)(80 * scale), viewportY + (int)(80 * scale), width, width,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    u32 value = hashBytes(data, size);
    HeapFree(GetProcessHeap(), 0, data);
    return value;
}

void coinSparkleNativeChecks() {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    startStage(0);
    readyTimer = 0;
    const char *names[] = {"diamond 1", "diamond 2", "diamond 3", "diamond 4", "diamond 5"};
    Actor *art = actor(314060601782828169ULL);
    bool resources = true, stage = true, changes = true;
    for (int n = 0; n < 5; n++) {
        Animation *clip = animation(art, names[n]);
        resources &= clip && eq(clip->name, names[n]) && clip->count == 8 && clip->duration == 10 &&
                     clip->frames[0].duration == 10 && absf(animationLength(clip) - 1.7f) < .00001f;
        for (int f = 1; f < 8; f++)
            resources &= clip->frames[f].duration == 1;
        Entity gem;
        Object &o = *classById(314060601782828169ULL);
        initializeEntity(gem, o, Rect{112, 112, 144, 144});
        gem.state.aux = -(float)(n + 1);
        u32 first = 0;
        bool changed = false;
        for (int f = 0; f < 8; f++) {
            gem.state.timer = f ? 1.0f + .1f * (f - .5f) : 0;
            u32 actual = coinSparklePixelsNative(gem, current),
                expected = coinSparklePixelsNative(gem, current, names[n]);
            stage &= actual == expected;
            if (!f)
                first = actual;
            else
                changed |= first != actual;
        }
        changes &= changed;
    }
    check(
        resources,
        "All five original diamond sparkle clips have eight frames at10ticks per second, ten idle ticks then seven sparkle ticks in a1.7second cycle");
    check(
        stage && changes,
        "Actual game gem rendering plays every original sparkle frame and variant instead of the unnamed static preview");
    navigate(worldScenes[0]);
    int index = -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].kind == COIN) {
            index = i;
            break;
        }
    bool ui = false;
    if (index >= 0) {
        Entity &gem = page.playfield[index];
        gem.state.aux = -4;
        gem.state.timer = 1.15f;
        u32 first = coinSparklePixelsNative(gem, page.level),
            expected = coinSparklePixelsNative(gem, page.level, "diamond 4");
        gem.state.timer = 0;
        ui = first == expected && first != coinSparklePixelsNative(gem, page.level);
    }
    check(
        ui,
        "The actual selector world's half-size gem uses the same original sparkle animation instead of retaining its static preview");
    u64 rng = 0x1234abcd330eULL;
    const u32 expectedRandom[] = {851401618, 1804928587, 758783491, 959030623,
                                  684387517, 1903590565, 33463914,  1254324197};
    bool random = true;
    for (int i = 0; i < 8; i++)
        random &= coinAnimationRandomNextNative(rng) == expectedRandom[i];
    check(
        random,
        "Coin clip selection uses the POSIX48bit generator's independent default-seed reference vectors without borrowing Wizard or particle RNG");
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    int groups = 0, cells = 0, variants = 0;
    bool initialized = true;
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scene, memory);
        for (int i = 0; i < scene.objectCount; i++) {
            Object &o = scene.objects[i];
            if (!o.has("CoinGroupBehaviour"))
                continue;
            groups++;
            float w = o.props.num("actlet-width", 32), h = o.props.num("actlet-height", 32);
            for (float y = o.rect.b; y < o.rect.t - .5f; y += h)
                for (float x = o.rect.l; x < o.rect.r - .5f; x += w) {
                    Entity gem;
                    initializeEntity(gem, o,
                                     Rect{x, y, minf(x + w, o.rect.r), minf(y + h, o.rect.t)});
                    cells++;
                    variants |= 1 << coinAnimationVariantNative(gem.state);
                    initialized &=
                        coinAnimationSavedNative(gem.state) &&
                        absf(gem.state.timer - coinAnimationInitialTimeNative(
                                                   gem.state.x, gem.state.y, 1.7f)) < .0001f &&
                        gem.state.timer >= 0 && gem.state.timer <= 1.7001f;
                }
        }
    }
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        groups == 476 && cells == 2178 && variants == 31 && initialized,
        "All2178authored gem cells in476groups initialize independent original clips and signed world-position phases, including all five selector scenes");
    int second = -1, firstGem = prepareGroupCollectableNative(second);
    bool clock = false;
    if (firstGem >= 0) {
        entities[playerIndex].state.x = entities[playerIndex].state.y = 1000;
        restorePhysics();
        float before = entities[firstGem].state.timer;
        tick(.1f, false, false);
        float advanced = entities[firstGem].state.timer;
        screen = PAUSE;
        tick(.2f, false, false);
        float paused = entities[firstGem].state.timer;
        screen = PLAY;
        tick(.1f, false, false);
        clock = absf(advanced - before - .1f) < .00001f && paused == advanced &&
                absf(entities[firstGem].state.timer - advanced - .1f) < .00001f;
    }
    check(
        clock,
        "A real uncollected game gem advances its own flash clock during play and freezes with the normal stage pause");
    bool disk = false, legacy = false, restart = false;
    if (firstGem >= 0) {
        Entity &gem = entities[firstGem];
        gem.state.aux = -3;
        gem.state.timer = 1.15f;
        u32 pixels = coinSparklePixelsNative(gem, current);
        captureCheckpoint(false);
        char path[MAX_PATH];
        savePath(path, "coin-sparkle-checkpoint.dat");
        disk = atomicWrite(path, &checkpoint, sizeof(checkpoint));
        gem.state.aux = -5;
        gem.state.timer = .2f;
        if (disk)
            disk = readCheckpoint(path);
        if (disk)
            restoreCheckpoint();
        disk &= gem.state.aux == -3 && gem.state.timer == 1.15f &&
                coinSparklePixelsNative(gem, current) == pixels && checkpoint.version == 28 &&
                sizeof(Snapshot) == 543392;
        float oldAge = 2.35f,
              expected = coinAnimationInitialTimeNative(gem.state.x, gem.state.y, 1.7f) + oldAge;
        checkpoint.state[firstGem].aux = 0;
        checkpoint.state[firstGem].timer = oldAge;
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        restoreCheckpoint();
        legacy = coinAnimationSavedNative(gem.state) &&
                 gem.state.aux == initialState[firstGem].aux &&
                 absf(gem.state.timer - expected) < .0001f && !gem.state.gone && coins == 0;
        State initial = initialState[firstGem];
        restartStage();
        restart = gem.state.aux == initial.aux && gem.state.timer == initial.timer;
    }
    check(
        disk,
        "Private disk reload preserves the selected sparkle variant, exact animation clock and rendered pixels without changing the28/543392checkpoint layout");
    check(
        legacy,
        "An older saved gem with unused aux0 migrates to its activation clip and world-position phase plus saved age without changing collection state");
    check(
        restart,
        "Restart restores each gem's original selected clip and activation phase rather than synchronizing every flash to frame zero");
    navigate(worldScenes[0]);
    index = -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].kind == COIN) {
            index = i;
            break;
        }
    bool uiClock = false;
    if (index >= 0) {
        page.bean.state.x = page.bean.state.y = 1000;
        page.bean.state.vx = page.bean.state.vy = 0;
        resetUiPhysics(page);
        Entity &gem = page.playfield[index];
        gem.state.aux = -2;
        gem.state.timer = .95f;
        u32 before = coinSparklePixelsNative(gem, page.level),
            campaign = hashBytes(entities, sizeof(Entity) * entityCount),
            saved = hashBytes(&checkpoint, sizeof(checkpoint));
        tickInterface(.2f);
        uiClock = absf(gem.state.timer - 1.15f) < .00001f &&
                  coinSparklePixelsNative(gem, page.level) != before &&
                  campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
                  saved == hashBytes(&checkpoint, sizeof(checkpoint));
    }
    check(
        uiClock,
        "Real selector ticking advances its own gem flash while leaving campaign entities and checkpoint bytes unchanged");
    glViewport(viewportX, viewportY, viewportW, viewportH);
    nativeViewportProjection();
    glClearColor(.04f, .08f, .12f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int n = 0; n < 5; n++)
        for (int f = 0; f < 8; f++) {
            Entity gem;
            Object &o = *classById(314060601782828169ULL);
            float x = 40 + f * 58, y = 60 + n * 58;
            initializeEntity(gem, o, Rect{x - 16, y - 16, x + 16, y + 16});
            gem.state.aux = -(float)(n + 1);
            gem.state.timer = f ? 1 + .1f * (f - .5f) : 0;
            drawEntity(gem, &current, 0, 0, 0);
        }
    glFinish();
    SwapBuffers(gameDC);
    capture("test-output/coin-sparkle-original-frames.bmp");
    atomicWrite("test-output/coin-sparkle-results.txt", testLog, lstrlenA(testLog));
    startStage(0);
    readyTimer = 0;
}
