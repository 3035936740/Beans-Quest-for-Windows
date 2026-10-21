#pragma once

// Regression coverage for enemy animation boundaries. Runs only in the isolated BQ_ENABLE_TESTS executable.

u32 enemyAnimationBoundaryPixels(Entity &e) {
    glViewport(viewportX, viewportY, viewportW, viewportH);
    nativeViewportProjection();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();
    glTranslatef(128 - e.state.x, 128 - e.state.y, 0);
    drawEntity(e, &current, 0, 0, 0);
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

void enemyAnimationBoundaryNativeChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    debugMode = false;
    startStage(0);
    Arena memory;
    memory.init(32 * 1024 * 1024);
    Level scan = {};
    int walks = 0, rails = 0, fallbacks = 0, directional = 0, paused = 0;
    bool walkChanges = true, sameDirection = true, force = true, railPreserves = true,
         railRestarts = true;
    int gpuFallbacks = 0, gpuKinds = 0;
    bool gpuKept = true;
    for (int scene = 0; scene < levelCount; scene++) {
        readLevelData(levels[scene].file, scan, memory);
        for (int i = 0; i < scan.objectCount; i++) {
            Object &o = scan.objects[i];
            if (o.has("WalkBehaviour")) {
                walks++;
                Entity e;
                initializeEntity(e, o, o.rect);
                nativeWalkDirection(e, false, true);
                e.state.timer = .37f;
                nativeWalkDirection(e, true);
                walkChanges &= e.state.aux < 0 && e.state.timer == 0;
                e.state.timer = .17f;
                nativeWalkDirection(e, true);
                sameDirection &= e.state.timer == .17f;
                nativeWalkDirection(e, true, true);
                force &= e.state.timer == 0;
                e.state.timer = .29f;
                nativeWalkDirection(e, false);
                walkChanges &= e.state.aux > 0 && e.state.timer == 0;
            }
            if (!o.has("OscillatingBehaviour") ||
                (!o.has("KillableEnemyBehaviour") && !o.has("DeadlyBehaviour")))
                continue;
            rails++;
            paused += o.paused != 0;
            Entity e;
            initializeEntity(e, o, o.rect);
            e.state.active = 1;
            e.state.aux = 1;
            e.state.timer = .37f;
            DiagonalRuntimeState d;
            OscillatorRuntimeState clock;
            resetNativeMotionState(e, d, clock);
            clock.clock = (e.hi - e.lo + .5f) / e.speed;
            bool has = nativeOscillatorDirectionAnimation(e, -1);
            int kind = o.klass == 8234400683372717661ULL   ? 1
                       : o.klass == 4615236400452776625ULL ? 2
                       : o.klass == 481780730859ULL        ? 4
                                                           : 0;
            bool pixelCase = !has && kind && !(gpuKinds & kind);
            u32 pixels = 0;
            if (pixelCase)
                pixels = enemyAnimationBoundaryPixels(e);
            advanceNativeOscillatorState(e, clock, 1.0f / 120, 75);
            if (has) {
                directional++;
                railRestarts &= e.state.timer == 0;
            } else {
                fallbacks++;
                railPreserves &= e.state.timer == .37f;
                if (pixelCase) {
                    gpuKept &= enemyAnimationBoundaryPixels(e) == pixels;
                    gpuFallbacks++;
                    gpuKinds |= kind;
                }
            }
        }
    }
    releaseLevel(scan, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        levelCount == 74 && walks == 78 && rails == 86 && fallbacks == 53 && directional == 33 &&
            paused == 7,
        "All 74 original scenes resolve 78 walkers and 86 enemy rails: 53 keep authored idle and 33 own directional animations");
    check(
        walkChanges && sameDirection && force,
        "Every original walker restarts the selected direction on a real change or explicit force while repeated same-direction requests retain phase");
    check(
        railPreserves && railRestarts,
        "All 53 idle-only enemy rails preserve their clock across a turn while all 33 directional bird rails restart the newly selected instance");
    check(
        gpuFallbacks == 3 && gpuKept,
        "Actual OpenGL idle-only enemy artwork remains on its native animation frame across rail direction changes");
    // Real Walk PostSolve side detector at a controlled wall, using an authored
    // stage enemy and the actual game tick; no direction helper is called here.
    startStage(6);
    readyTimer = 0;
    int walker = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].walk) {
            walker = i;
            break;
        }
    bool contactTurn = false, turnFrame = false, resumes = false;
    if (walker >= 0) {
        for (int i = 0; i < entityCount; i++)
            if (i != playerIndex && i != walker)
                entities[i].state.gone = 1;
        current.geometryCount = 0;
        Entity &e = entities[walker];
        e.state.x = 500;
        e.state.y = 100;
        e.state.active = 1;
        e.state.aux = e.state.vx = e.speed;
        e.state.vy = 0;
        e.state.timer = .37f;
        e.reverseEdge = 0;
        State &bean = entities[playerIndex].state;
        bean.x = 360;
        bean.y = 300;
        bean.vx = bean.vy = 0;
        rebuildPhysics();
        bodies[playerIndex]->SetType(b2_staticBody);
        b2BodyDef bd;
        bd.position.Set(500 * metres, 60 * metres);
        b2Body *floor = physicsWorld->CreateBody(&bd);
        b2PolygonShape shape;
        shape.SetAsBox(200 * metres, 10 * metres);
        floor->CreateFixture(&shape, 0);
        bd.position.Set(540 * metres, 130 * metres);
        b2Body *wall = physicsWorld->CreateBody(&bd);
        shape.SetAsBox(10 * metres, 80 * metres);
        wall->CreateFixture(&shape, 0);
        for (int step = 0; step < 90 && !contactTurn; step++) {
            tick(1.0f / 120, false, false);
            if (e.state.aux < 0) {
                contactTurn = true;
                turnFrame =
                    e.state.timer == 0 &&
                    animationFrame(exactEnemyAnimation(e.art, "walk-left"), e.state.timer) == 0;
            }
        }
        float previous = e.state.timer;
        for (int i = 0; i < 3; i++)
            tick(1.0f / 120, false, false);
        resumes = e.state.aux < 0 && e.state.timer > previous + .01f;
    }
    check(
        contactTurn && turnFrame && resumes,
        "Real walking-enemy wall contact displays frame zero exactly on turning, then advances instead of restarting on every persistent contact");
    startStage(0);
}
