#pragma once

// Regression coverage for activation native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int proximityPhasePrepareNative(u64 stage, u64 object) {
    startStage(stageById(stage));
    readyTimer = 0;
    int target = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->id == object)
            target = i;
    if (target < 0)
        return -1;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != target && i != playerIndex)
            entities[i].state.gone = 1;
    rebuildPhysics();
    return target;
}

void proximityPhaseNativeChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    const float dt = 1.0f / 120;
    const u64 railStage = 17830145214697761354ULL, railObject = 5355595713014929411ULL;
    int target = proximityPhasePrepareNative(railStage, railObject);
    bool delay = false, crossed = false, bat = false, left = false, woke = false;
    if (target >= 0) {
        Entity &e = entities[target];
        const Props &p = *e.source->behavior("ProximityActivateBehaviour");
        State &bean = entities[playerIndex].state;
        bean.x = e.state.x;
        bean.y = e.state.y + p.num("distance-y") - .5f;
        bean.vx = bean.vy = 0;
        restorePhysics();
        float clock = oscillatorRuntime[target].clock;
        tick(dt, false, false);
        delay = e.state.active == 1 && e.state.timer == 0 &&
                oscillatorRuntime[target].clock == clock && bodies[target]->IsActive();
    }
    check(
        delay,
        "Authored paused2-4 rail wakes after physics and retains its pre-activation animation and oscillator clocks for that step");
    target = proximityPhasePrepareNative(railStage, railObject);
    if (target >= 0) {
        Entity &e = entities[target];
        const Props &p = *e.source->behavior("ProximityActivateBehaviour");
        State &bean = entities[playerIndex].state;
        float boundary = e.state.y + p.num("distance-y");
        bean.x = e.state.x;
        bean.y = boundary + .5f;
        bean.vx = 0;
        bean.vy = -200;
        restorePhysics();
        float clock = oscillatorRuntime[target].clock;
        tick(dt, false, false);
        crossed = bean.y < boundary && e.state.active == 1 && e.state.timer == 0 &&
                  oscillatorRuntime[target].clock == clock;
        tick(dt, false, false);
        crossed &= oscillatorRuntime[target].clock > clock;
        char row[300];
        wsprintfA(
            row,
            "cross=%d y1000=%d boundary1000=%d active=%d clock1000000=%d initial1000000=%d timer1000000=%d death1000=%d screen=%d\r\n",
            crossed, (int)(bean.y * 1000), (int)(boundary * 1000), e.state.active,
            (int)(oscillatorRuntime[target].clock * 1000000), (int)(clock * 1000000),
            (int)(e.state.timer * 1000000), (int)(deathTimer * 1000), (int)screen);
        atomicWrite("test-output/proximity-cross.txt", row, lstrlenA(row));
    }
    check(
        crossed,
        "A real Bean crosses the original rail activation boundary during Box2D: it wakes that post-update and advances motion on the following step");
    target = proximityPhasePrepareNative(5770908237787719ULL, 9319735670056212ULL);
    if (target >= 0) {
        Entity &e = entities[target];
        const Props &p = *e.source->behavior("ProximityActivateBehaviour");
        State &bean = entities[playerIndex].state;
        bean.x = e.state.x + p.num("distance-x") - .5f;
        bean.y = e.state.y;
        bean.vx = bean.vy = 0;
        restorePhysics();
        float x = e.state.x, y = e.state.y;
        tick(dt, false, false);
        bat = e.state.active == 1 && e.state.x == x && e.state.y == y &&
              e.state.timer == (nativeActorAnimateWhilePaused(e) ? dt : 0);
        tick(dt, false, false);
        bat &= absf(e.state.x - x) + absf(e.state.y - y) > .1f;
        char row[300];
        wsprintfA(
            row,
            "bat=%d x1000000=%d originx1000000=%d y1000000=%d originy1000000=%d active=%d timer1000000=%d death1000=%d screen=%d\r\n",
            bat, (int)(e.state.x * 1000000), (int)(x * 1000000), (int)(e.state.y * 1000000),
            (int)(y * 1000000), e.state.active, (int)(e.state.timer * 1000000),
            (int)(deathTimer * 1000), (int)screen);
        atomicWrite("test-output/proximity-bat.txt", row, lstrlenA(row));
    }
    check(
        bat,
        "The actual paused4-9 diagonal bat retains its authored pose on activation and starts moving one physics pass later");
    target = proximityPhasePrepareNative(739118707375ULL, 3089867997079985ULL);
    if (target >= 0) {
        Entity &e = entities[target];
        const Props &p = *e.source->behavior("ProximityActivateBehaviour");
        State &bean = entities[playerIndex].state;
        float boundary = e.state.x + p.num("distance-x");
        bean.x = boundary - .5f;
        bean.y = e.state.y;
        bean.vx = 200;
        bean.vy = 0;
        restorePhysics();
        bulletSourceStates[target].clock = .4f;
        tick(dt, false, true);
        left = bean.x > boundary && e.state.active == 0 && !bodies[target]->IsActive() &&
               absf(bulletSourceStates[target].clock - (.4f + dt)) < .000001f;
        tick(dt, false, true);
        left &= bulletSourceStates[target].clock == p.num("timeOffset");
        bean.x = boundary + .5f;
        bean.y = e.state.y;
        bean.vx = -200;
        bean.vy = 0;
        restorePhysics();
        tick(dt, true, false);
        woke = bean.x < boundary && e.state.active == 1 &&
               bulletSourceStates[target].clock == p.num("timeOffset");
        tick(dt, false, false);
        woke &= bulletSourceStates[target].clock > p.num("timeOffset");
    }
    check(
        left,
        "An actual auto-pausing emitter finishes its current firing-clock step before leaving range, then uses the native paused timeOffset reset on the next step");
    check(
        woke,
        "Re-entering a real emitter range wakes it after the current physics step without advancing its firing clock prematurely");
    target = proximityPhasePrepareNative(railStage, railObject);
    bool preview = false;
    if (target >= 0) {
        Object *source = entities[target].source;
        u32 state = hashBytes(entities, sizeof(Entity) * entityCount),
            saved = hashBytes(&checkpoint, sizeof(checkpoint));
        interfaceEnabled = true;
        navigate(worldScenes[0]);
        destroyUiPhysics(page);
        page.level.geometryCount = 0;
        for (int i = 0; i < page.playfieldCount; i++)
            page.playfield[i].state.gone = 1;
        Vec c = center(source->rect);
        initializeEntity(page.playfield[0], *source, shifted(source->rect, 300 - c.x, 100 - c.y));
        page.bean.state.x = 300;
        page.bean.state.y = 250;
        page.bean.state.vx = page.bean.state.vy = 0;
        page.miniDeath = 0;
        createUiPhysics(page);
        float clock = page.oscillator[0].clock;
        tickInterface(dt);
        preview = page.playfield[0].state.active == 1 && page.playfield[0].state.timer == 0 &&
                  page.oscillator[0].clock == clock && page.bodies[0]->IsActive() &&
                  hashBytes(entities, sizeof(Entity) * entityCount) == state &&
                  hashBytes(&checkpoint, sizeof(checkpoint)) == saved;
        navigate(12192614360109026946ULL);
        interfaceEnabled = false;
    }
    check(
        preview,
        "Real selector update uses its own solved miniature Bean position for post-physics activation and leaves campaign entities and checkpoint untouched");
    startStage(0);
    readyTimer = 0;
}

void activationNativeChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    startStage(0);
    readyTimer = 0;
    State savedBean = entities[playerIndex].state;
    int objects = 0, paused = 0, awake = 0, automatic = 0, scenes = 0, classesChecked = 0;
    bool initial = true, sticky = true, recheck = true, clocks = true;
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    // All74 original scenes, including selector backgrounds, are inspected in
    // a separate arena. No physics tick or forced screen activation is used.
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scene, memory);
        scenes++;
        for (int i = 0; i < scene.objectCount; i++) {
            Object &source = scene.objects[i];
            const Props *proximity = source.behavior("ProximityActivateBehaviour");
            if (!proximity)
                continue;
            Entity e;
            initializeEntity(e, source, source.rect);
            objects++;
            if (source.paused)
                paused++;
            else
                awake++;
            initial &= e.state.active == (source.paused ? 0 : 1);
            if (e.move)
                clocks &= e.state.timer == 0 && e.state.aux == 1;
            State &bean = entities[playerIndex].state;
            bean.x = e.state.x + 10000;
            bean.y = e.state.y + 10000;
            if (proximity->flag("auto-deactivate")) {
                automatic++;
                recheck &= !updateProximity(e) && e.state.active == 0;
                bean.x = e.state.x;
                bean.y = e.state.y;
                recheck &= updateProximity(e) && e.state.active != 0;
                bean.x += 10000;
                bean.y += 10000;
                recheck &= !updateProximity(e) && e.state.active == 0;
            } else {
                bool farActive = updateProximity(e);
                sticky &= farActive == !source.paused;
                bean.x = e.state.x;
                bean.y = e.state.y;
                sticky &= updateProximity(e) && e.state.active != 0;
                bean.x += 10000;
                bean.y += 10000;
                sticky &= updateProximity(e) && e.state.active != 0;
            }
        }
    }
    for (int i = 0; i < classCount; i++)
        if (classes[i].has("ProximityActivateBehaviour")) {
            Entity e;
            initializeEntity(e, classes[i], classes[i].rect);
            classesChecked++;
            initial &= e.state.active == (classes[i].paused ? 0 : 1);
        }
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    entities[playerIndex].state = savedBean;
    check(
        scenes == levelCount && objects == 221 && paused == 41 && awake == 180 &&
            classesChecked == 14 && initial,
        "All74 original scenes and14 proximity classes preserve the native paused-to-initial-active flags for all221 placed proximity actors");
    check(
        sticky && clocks,
        "Every original sticky proximity actor starts from authored pause state and keeps running after activation without resetting its oscillator clock");
    check(
        automatic == 22 && recheck,
        "All22 original auto-deactivating actors re-evaluate distance and wake again after Bean returns");
    char audit[256];
    wsprintfA(
        audit,
        "scenes=%d placed=%d paused=%d awake=%d auto-deactivate=%d classes=%d initial=%d sticky=%d auto=%d clocks=%d\r\n",
        scenes, objects, paused, awake, automatic, classesChecked, initial, sticky, recheck,
        clocks);
    atomicWrite("test-output/native-activation-resource-audit.txt", audit, lstrlenA(audit));
    int stage = -1;
    for (int i = 0; i < stageCount; i++)
        if (levels[stageList[i]].props.num("bean-world") == 6 &&
            levels[stageList[i]].props.num("bean-level") == 1)
            stage = i;
    check(stage >= 0,
          "The original Wizard scene exists for the reported horizontal stone-wall rail");
    if (stage >= 0) {
        startStage(stage);
        readyTimer = 0;
        int stone = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].source->id == 7890796677221068ULL)
                stone = i;
        check(stone >= 0,
              "The reported Wizard rail stone resolves to its original authored instance");
        if (stone >= 0) {
            Entity &e = entities[stone];
            const Props &p = *e.source->behavior("ProximityActivateBehaviour");
            check(
                e.source->paused == 0 && e.state.active == 1 && p.num("distance-x") == 100 &&
                    p.num("distance-y") == 100 && !p.flag("auto-deactivate") && e.speed == 200 &&
                    e.lo == -128 && e.hi == 128 && e.moveX == 1 && e.moveY == 0,
                "Reported Wizard stone starts active at its authored200-pixel rail speed despite the100-pixel proximity range");
            // Reconstruct screenshot: checkpoint(2656,164), Bean about(2712,180),
            // stone initially(2880,256). It is visible but168px from Bean.
            State &bean = entities[playerIndex].state;
            bean.x = 2712;
            bean.y = 180;
            bean.vx = bean.vy = 0;
            restorePhysics();
            bool initiallyOutside = absf(bean.x - e.state.x) > p.num("distance-x"), right = false,
                 left = false, active = true, finite = true;
            float low = e.state.x, high = low, y = e.state.y, drift = 0;
            for (int step = 0; step < 960; step++) {
                bean.x = 2712;
                bean.y = 180;
                bean.vx = bean.vy = 0;
                tick(1.0f / 120.0f, false, false);
                low = minf(low, e.state.x);
                high = maxf(high, e.state.x);
                drift = maxf(drift, absf(e.state.y - y));
                right |= e.state.vx > 100;
                left |= e.state.vx < -100;
                active &= e.state.active && bodies[stone] && bodies[stone]->IsActive();
                finite &= e.state.x == e.state.x && e.state.y == e.state.y;
            }
            check(
                initiallyOutside && active && finite && right && left && high - low > 200,
                "At the screenshot's actual Bean location the originally unpaused Wizard stone keeps moving and reverses repeatedly for eight seconds");
            check(
                low >= 2751 && high <= 3009 && drift < 0.01f && low + e.hit.l >= 2719 &&
                    high + e.hit.r <= 3041,
                "The actual Wizard stone remains on its authored horizontal rail between the original chamber walls without drifting into them");
            camX = 2584;
            camY = 0;
            captureCase("test-output/wizard-stone-activation-native.bmp");
            char trace[256];
            wsprintfA(
                trace,
                "id=7890796677221068 paused=%d active=%d min-x=%d max-x=%d y-drift=%d initially-outside=%d\r\n",
                e.source->paused, e.state.active, (int)(low * 100), (int)(high * 100),
                (int)(drift * 1000), initiallyOutside);
            atomicWrite("test-output/wizard-stone-activation-native.txt", trace, lstrlenA(trace));
        }
    }
    startStage(0);
    readyTimer = 0;
}
