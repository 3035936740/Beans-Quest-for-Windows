#pragma once

// Regression coverage for paused actor animation native. Runs only in the isolated BQ_ENABLE_TESTS executable.

u32 pausedActorAnimationPixels(Entity &e, Level &scene) {
    glViewport(viewportX, viewportY, viewportW, viewportH);
    nativeViewportProjection();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();
    glTranslatef(128 - e.state.x, 128 - e.state.y, 0);
    drawEntity(e, &scene, 0, 0, 0);
    glPopMatrix();
    glFinish();
    float scale = viewportW / viewW;
    int width = (int)(96 * scale), size = width * width * 4;
    u8 *data = (u8 *)HeapAlloc(GetProcessHeap(), 0, size);
    glReadBuffer(GL_BACK);
    glReadPixels(viewportX + (int)(80 * scale), viewportY + (int)(80 * scale), width, width,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    u32 result = hashBytes(data, size);
    HeapFree(GetProcessHeap(), 0, data);
    return result;
}

void pausedActorAnimationNativeChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    debugMode = false;
    startStage(0);
    readyTimer = 0;
    Arena memory;
    memory.init(32 * 1024 * 1024);
    Level scan = {};
    int flagged = 0, paused = 0, pixels = 0, normalPaused = 0;
    u64 seen[4] = {};
    bool safe = true, visual = true, normal = true, unchanged = true;
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scan, memory);
        for (int i = 0; i < scan.objectCount; i++) {
            Object &o = scan.objects[i];
            if (!o.has("ProximityActivateBehaviour"))
                continue;
            Entity e;
            initializeEntity(e, o, o.rect);
            if (!nativeActorAnimateWhilePaused(e)) {
                if (o.paused) {
                    normalPaused++;
                    float t = e.state.timer;
                    advanceNativePausedActorAnimation(e, .15f);
                    normal &= e.state.timer == t;
                }
                continue;
            }
            flagged++;
            paused += o.paused != 0;
            safe &= nativeActorPauseAnimationClockSafe(e);
            if (hasFishAnimationNative(e))
                selectFishAnimationNative(e, e.moveX);
            e.state.hp = 0;
            e.state.timer = 0;
            State before = e.state;
            int k = -1;
            for (int j = 0; j < 4; j++)
                if (seen[j] == o.klass)
                    k = j;
            bool take = k < 0 && pixels < 4;
            u32 first = take ? pausedActorAnimationPixels(e, scan) : 0;
            advanceNativePausedActorAnimation(e, .15f);
            before.timer = e.state.timer;
            unchanged &= hashBytes(&before, sizeof(before)) == hashBytes(&e.state, sizeof(e.state));
            visual &= absf(e.state.timer - .15f) < .00001f;
            if (take) {
                visual &= first != pausedActorAnimationPixels(e, scan);
                seen[pixels++] = o.klass;
            }
        }
    }
    releaseLevel(scan, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        levelCount == 74 && flagged == 30 && paused == 26 && safe,
        "All74 scenes contain30 animate-while-paused enemies,26 initially paused, and none shares the visual timer with rail/orbit motion");
    check(
        pixels == 4 && visual && unchanged,
        "Actual OpenGL fish/red-beetle/red-hopper/purple-hopper clips advance while every nonvisual entity state field stays unchanged");
    check(normalPaused == 15 && normal,
          "All15 unflagged initially paused proximity actors keep their visual clocks frozen");
    // This is the actual tick integration, not a helper-only phase test.
    // The first original Trampolines hopper starts paused outside proximity.
    startStage(stageById(15974012089803411243ULL));
    readyTimer = 0;
    int hopper = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->id == 14558783277546895461ULL)
            hopper = i;
    bool dormant = false, menu = false, gpu = false, bodyHeld = false;
    if (hopper >= 0) {
        Entity &e = entities[hopper];
        State before = e.state;
        float first = e.state.timer;
        u32 a = pausedActorAnimationPixels(e, current);
        for (int i = 0; i < 18; i++)
            tick(1.0f / 120, false, false);
        dormant = !e.state.active && e.state.timer > first + .14f && e.state.x == before.x &&
                  e.state.y == before.y && e.state.vx == before.vx && e.state.vy == before.vy &&
                  e.state.aux == before.aux;
        gpu = a != pausedActorAnimationPixels(e, current);
        bodyHeld = bodies[hopper] && !bodies[hopper]->IsActive();
        screen = PAUSE;
        State saved = e.state;
        float age = elapsed;
        u32 shotHash = hashBytes(bullets, sizeof(bullets));
        for (int i = 0; i < 18; i++)
            tick(1.0f / 120, false, false);
        menu = e.state.timer > saved.timer + .14f && e.state.x == saved.x && e.state.y == saved.y &&
               e.state.vx == saved.vx && e.state.vy == saved.vy && e.state.aux == saved.aux &&
               elapsed == age && shotHash == hashBytes(bullets, sizeof(bullets));
    }
    check(
        hopper >= 0 && dormant && gpu && bodyHeld,
        "Real game ticks animate an authored dormant hopper without activating or moving its physical body");
    check(
        menu,
        "Actual pause-menu ticks preserve all movement and emitted shots while the opted-in actor animation advances");
    // A real auto-deactivating source and ordinary actors must remain frozen.
    startStage(stageById(806701038719ULL));
    readyTimer = 0;
    screen = PAUSE;
    int source = -1, ordinary = -1;
    for (int i = 0; i < entityCount; i++) {
        if (source < 0 && entities[i].source->has("BulletSource"))
            source = i;
        if (ordinary < 0 && entities[i].kind == PLAYER)
            ordinary = i;
    }
    bool sources = false;
    if (source >= 0 && ordinary >= 0) {
        State sourceState = entities[source].state, playerState = entities[ordinary].state;
        BulletSourceState clock = bulletSourceStates[source];
        u32 shots = hashBytes(bullets, sizeof(bullets));
        for (int i = 0; i < 120; i++)
            tick(1.0f / 120, false, false);
        sources = hashBytes(&sourceState, sizeof(sourceState)) ==
                      hashBytes(&entities[source].state, sizeof(State)) &&
                  hashBytes(&playerState, sizeof(playerState)) ==
                      hashBytes(&entities[ordinary].state, sizeof(State)) &&
                  hashBytes(&clock, sizeof(clock)) ==
                      hashBytes(&bulletSourceStates[source], sizeof(clock)) &&
                  shots == hashBytes(bullets, sizeof(bullets));
    }
    check(
        sources,
        "Actual pause-menu ticks leave an authored turret, its emission clock, bullets and unflagged Bean entirely frozen");
    startStage(0);
    readyTimer = 0;
}
