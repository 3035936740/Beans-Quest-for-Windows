#pragma once

// Regression coverage for credits runtime native. Runs only in the isolated BQ_ENABLE_TESTS executable.

u32 creditsRuntimeSpritePixels(int index) {
    Entity &e = index < 0 ? page.bean : page.playfield[index];
    glViewport(viewportX, viewportY, viewportW, viewportH);
    nativeViewportProjection();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();
    glTranslatef(128 - e.state.x, 128 - e.state.y, 0);
    drawCreditsRuntimeNode(page, *e.source);
    glPopMatrix();
    glFinish();
    float scale = viewportW / viewW;
    int width = (int)(96 * scale), size = width * width * 4;
    u8 *pixels = (u8 *)HeapAlloc(GetProcessHeap(), 0, size);
    glReadBuffer(GL_BACK);
    glReadPixels(viewportX + (int)(80 * scale), viewportY + (int)(80 * scale), width, width,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    u32 hash = hashBytes(pixels, size);
    HeapFree(GetProcessHeap(), 0, pixels);
    return hash;
}

void creditsRuntimeNativeChecks() {
    bool previousInterface = interfaceEnabled, previousSound = soundEnabled;
    interfaceEnabled = true;
    startStage(0);
    u32 entityHash = hashBytes(entities, sizeof(Entity) * entityCount),
        shotHash = hashBytes(bullets, sizeof(bullets)),
        wizardHash = hashBytes(&wizardMotion, sizeof(wizardMotion));
    int previousJumps = jumps, previousCoins = coins;
    navigate(137502285691ULL);
    transitionTime = endingCreditsFade = 0;
    int bat = creditsEntityById(page, 9727011878297452ULL),
        bird = creditsEntityById(page, 1656605438462651ULL),
        fish = creditsEntityById(page, 8720624069488508ULL),
        beetle = creditsEntityById(page, 781479982887461ULL),
        cloud = creditsEntityById(page, 1796755771799789ULL),
        pivot = creditsEntityById(page, 900349935716017ULL);
    int links = 0, ball = -1, sources = 0, axolotls = 0;
    for (int i = 0; i < page.playfieldCount; i++) {
        Entity &e = page.playfield[i];
        if (e.parent == pivot) {
            links++;
            ball = i;
        }
        if (e.kind == TURRET)
            sources++;
        if (e.kind == SALAMANDER)
            axolotls++;
    }
    check(
        screen == ENDING && page.physics && page.hasBean && links == 5 && sources == 2 &&
            axolotls == 3 && creditsRuntime.frogBall >= 0,
        "Ending scene owns its live physics, all five pendulum children, two original BulletSources, frog cooperation and three axolotls");
    Vec bean0 = {page.bean.state.x, page.bean.state.y};
    Vec bat0 = bat >= 0 ? Vec{page.playfield[bat].state.x, page.playfield[bat].state.y} : Vec{};
    float bird0 = bird >= 0 ? page.playfield[bird].state.x : 0,
          fish0 = fish >= 0 ? page.playfield[fish].state.x : 0,
          cloud0 = cloud >= 0 ? page.playfield[cloud].state.y : 0,
          ball0 = ball >= 0 ? page.playfield[ball].state.x : 0,
          beetle0 = beetle >= 0 ? page.playfield[beetle].state.x : 0;
    float lowBean = bean0.y, highBean = bean0.y, beetleRange = 0;
    bool soundRestored = true, warning = false, emission = false, frogLive = true;
    int exchanges = 0, previousFrog = 0;
    float dt = 1.0f / 120;
    u32 spriteFrames[4] = {};
    int spriteIds[4] = {-1, bat, bird, creditsEntityById(page, 1644610045676907ULL)};
    char *trace = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 262144);
    lstrcpyA(trace,
             "age_ms,id,kind,x_milli,y_milli,vx_milli,vy_milli,active,gone,emitted,warnings\r\n");
    for (int frame = 0; frame < 120 * 180; frame++) {
        tickInterface(dt);
        if (creditsRuntime.frogBall >= 0) {
            Entity &exchangeBall = page.playfield[creditsRuntime.frogBall];
            frogLive &= exchangeBall.state.hp == 1 && exchangeBall.state.y > 3200 &&
                        exchangeBall.state.y < 3600;
            if (exchangeBall.state.active != previousFrog) {
                previousFrog = exchangeBall.state.active;
                exchanges++;
            }
        }
        if (beetle >= 0)
            beetleRange = maxf(beetleRange, absf(page.playfield[beetle].state.x - beetle0));
        lowBean = minf(lowBean, page.bean.state.y);
        highBean = maxf(highBean, page.bean.state.y);
        soundRestored &= soundEnabled == previousSound;
        if (frame == 29)
            for (int i = 0; i < 4; i++)
                spriteFrames[i] = creditsRuntimeSpritePixels(spriteIds[i]);
        if (frame == 59) {
            int changed = 0;
            for (int i = 0; i < 4; i++)
                if (creditsRuntimeSpritePixels(spriteIds[i]) != spriteFrames[i])
                    changed++;
            check(
                changed >= 2,
                "Actual OpenGL ending sprites change animation pixels at two times, including Bean and authored enemy/axolotl AnimationInstances");
        }
        if (frame == 59) {
            check(
                bat >= 0 && absf(page.playfield[bat].state.y - bat0.y) > 5 && bird >= 0 &&
                    absf(page.playfield[bird].state.x - bird0) > 15 && fish >= 0 &&
                    absf(page.playfield[fish].state.x - fish0) > 15,
                "Actual ending bat, bird and fish advance their native orbit, oscillator and diagonal controllers at full scene scale");
            check(
                cloud >= 0 && absf(page.playfield[cloud].state.y - cloud0) > 15 && ball >= 0 &&
                    absf(page.playfield[ball].state.x - ball0) > 10,
                "Actual ending happy cloud and physically simulated four-link pendulum move from their original authored positions");
        }
        if (frame == 239) {
            warning = emission = true;
            for (int i = 0; i < page.playfieldCount; i++)
                if (page.playfield[i].kind == TURRET) {
                    warning &= creditsRuntime.warnings[i] > 0;
                    emission &= creditsRuntime.emitted[i] > 0;
                }
            bool plasma = false, fireball = false;
            for (int shot = 0; shot < 32; shot++)
                if (creditsRuntime.shots[shot].body) {
                    u64 id = creditsRuntime.shots[shot].entity.source->id;
                    plasma |= id == 12978618438667112518ULL;
                    fireball |= id == 6430600862785566ULL;
                }
            check(
                warning && emission && plasma && fireball,
                "Actual ending BulletSources run their retained original pre-shoot animations and emit real isolated-world projectile bodies");
            check(
                highBean - lowBean > 40 && absf(page.bean.state.x - bean0.x) < 1 &&
                    jumps == previousJumps,
                "Ending Tom bounces continuously without input, horizontal drift or writes to the stage jump counter");
            check(beetle >= 0 && beetleRange > 30,
                  "Actual ending blue beetle walks and interacts with its original world fixtures");
        }
        if (frame == 119 || frame == 359 || frame == 120 * 75 - 1 || frame == 120 * 140 - 1 ||
            frame == 120 * 177 - 1) {
            for (int i = 0; i < page.playfieldCount; i++) {
                Entity &e = page.playfield[i];
                if (e.parent < 0 && e.kind == DECOR)
                    continue;
                char row[240];
                wsprintfA(row, "%d,%I64u,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", (int)(page.age * 1000),
                          e.source->id, e.kind, (int)(e.state.x * 1000), (int)(e.state.y * 1000),
                          (int)(e.state.vx * 1000), (int)(e.state.vy * 1000), e.state.active,
                          e.state.gone, creditsRuntime.emitted[i], creditsRuntime.warnings[i]);
                if (lstrlenA(trace) < 250000)
                    lstrcatA(trace, row);
            }
        }
        if (frame == 120 * 75 - 1 || frame == 120 * 140 - 1 || frame == 120 * 177 - 1) {
            char name[130];
            wsprintfA(name, "test-output/credits-live-cast-%ds.bmp", (frame + 1) / 120);
            render();
            capture(name);
        }
    }
    check(
        frogLive && exchanges > 100,
        "Ending frog pair naturally exchanges its ball for three minutes using native half-size targets rather than losing the ball below the scene");
    atomicWrite("test-output/credits-live-runtime.csv", trace, lstrlenA(trace));
    HeapFree(GetProcessHeap(), 0, trace);
    check(
        screen == ENDING && page.age > 179 && !endingCredits.prompt && !endingCredits.exiting &&
            absf(page.cameraY + viewH * .5f - (6144 - 30 * page.age)) < .01f,
        "Live ending cast runs for three minutes while original automatic credits camera and no-click timeline remain intact");
    check(
        soundRestored && soundEnabled == previousSound &&
            hashBytes(entities, sizeof(Entity) * entityCount) == entityHash &&
            hashBytes(bullets, sizeof(bullets)) == shotHash &&
            hashBytes(&wizardMotion, sizeof(wizardMotion)) == wizardHash &&
            jumps == previousJumps && coins == previousCoins,
        "Ending object mute restores the user's audio preference each step and its independent actors/projectiles leave the running stage untouched");
    interfaceEnabled = previousInterface;
    startStage(0);
}
