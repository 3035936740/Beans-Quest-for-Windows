#pragma once

// Regression coverage for stage53 source native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int prepareStage53SourceNative(u64 object) {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    startStage(stageById(739118707375ULL));
    readyTimer = 0;
    current.geometryCount = 0;
    int target = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->id == object)
            target = i;
    if (target < 0)
        return -1;
    for (int i = 0; i < entityCount; i++)
        if (i != target && i != playerIndex)
            entities[i].state.gone = 1;
    Entity &t = entities[target];
    Entity &bean = entities[playerIndex];
    bean.state.x = t.state.x + 180;
    bean.state.y = t.state.y + 180;
    bean.state.vx = bean.state.vy = 0;
    bean.state.timer = .2f;
    rebuildPhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    return target;
}

void stage53SourceNativeChecks() {
    const u64 ids[] = {3089867997079985ULL, 4703067431869943ULL, 972498707169047ULL,
                       6515631630658036ULL, 172207971763ULL,     988146809796ULL};
    const float periods[] = {1, 1, 2.8f, 2.8f, 1, .5f}, offsets[] = {0, .5f, .5f, .5f, 0, 0};
    bool authored = true, timing = true, direction = true;
    char rows[18000] =
        "id,fps,step,refBefore1000000,refAfter1000000,actualBefore1000000,actualAfter1000000,refFire,actualFire,warningPhase,preAge1000000\r\n";
    int stage = stageById(739118707375ULL);
    check(
        stage >= 0 && levels[stageList[stage]].props.num("bean-jump-best") == 13 &&
            levels[stageList[stage]].props.num("bean-jump-par") == 15,
        "Original5-3resources retain13jump gold and15jump par targets without changing the player's route");
    for (int n = 0; n < 6; n++)
        for (int rate = 0; rate < 3; rate++) {
            int fps = rate == 0   ? 30
                      : rate == 1 ? 60
                                  : 120,
                target = prepareStage53SourceNative(ids[n]);
            if (target < 0) {
                authored = timing = direction = false;
                continue;
            }
            Entity &t = entities[target];
            const Props &config = *t.source->behavior("BulletSource");
            BulletSourceState &source = bulletSourceStates[target];
            // The original config fields are binary32. Do not compare an x87
            // decimal-parser return retained in an extended register to a float.
            volatile float low = config.num("timeMin"), high = config.num("timeMax"),
                           offset = config.num("timeOffset");
            authored &= low == periods[n] && high == periods[n] && offset == offsets[n] &&
                        source.interval == periods[n] && source.clock == offsets[n] &&
                        config.num("bulletPoolSize") == 10;
            volatile float clock = offsets[n], interval = periods[n];
            float dt = 1.0f / fps;
            int emitted = 0;
            for (int step = 1; step <= fps * 9; step++) {
                for (int shot = 0; shot < MAX_PROJECTILES; shot++)
                    if (bullets[shot].life > 0)
                        destroyShot(shot);
                float refBefore = clock, actualBefore = source.clock;
                clock = clock + dt;
                bool refFire = clock > interval;
                if (refFire) {
                    volatile float remainder = clock - (int)(clock / interval) * interval;
                    clock = remainder;
                }
                tick(dt, false, false);
                int actual = 0;
                for (int shot = 0; shot < MAX_PROJECTILES; shot++)
                    if (bullets[shot].life > 0 && bullets[shot].state.active == target + 1) {
                        actual++;
                        Bullet &b = bullets[shot];
                        direction &=
                            n == 5   ? absf(b.state.vx) < .01f && absf(b.state.vy - 125) < .02f
                            : n == 4 ? absf(b.state.vx - 125) < .02f && absf(b.state.vy) < .01f
                                     : absf(b.state.vx + 125) < .02f && absf(b.state.vy) < .01f;
                    }
                bool actualFire = actual == 1;
                emitted += actual;
                timing &= actualFire == refFire && absf(source.clock - clock) < .00001f;
                if (actualFire || refFire) {
                    char row[280];
                    wsprintfA(row, "%I64u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", ids[n], fps, step,
                              (int)(refBefore * 1000000), (int)(clock * 1000000),
                              (int)(actualBefore * 1000000), (int)(source.clock * 1000000), refFire,
                              actualFire, bulletSourceAnimations[target].phase,
                              (int)(bulletSourceAnimations[target].preAge * 1000000));
                    if (lstrlenA(rows) < 17000)
                        lstrcatA(rows, row);
                }
            }
            timing &= emitted > 0;
        }
    check(
        authored,
        "All six actual5-3cannons bind their original fixed intervals, initial offsets and ten-member per-source pools");
    check(
        timing,
        "All six original5-3cannons match an independent binary32 strict-expiry/fmod timeline for nine seconds at30/60/120Hz");
    check(
        direction,
        "Actual5-3shot creation preserves native125pixel-per-second left/right/up directions while checking timing");
    atomicWrite("test-output/stage53-source-timing.csv", rows, lstrlenA(rows));
    bool activation = true, reentry = true;
    char trace[3000] = "id,fps,boundary,firstFire,reentryFire\r\n";
    for (int n = 0; n < 6; n++)
        for (int rate = 0; rate < 3; rate++) {
            int fps = rate == 0   ? 30
                      : rate == 1 ? 60
                                  : 120,
                target = prepareStage53SourceNative(ids[n]);
            if (target < 0) {
                activation = reentry = false;
                continue;
            }
            Entity &t = entities[target];
            BulletSourceState &source = bulletSourceStates[target];
            State &bean = entities[playerIndex].state;
            // The first source's left boundary is outside the map. Approach its
            // right boundary so Tom's normal map clamp does not move the observer.
            float side = n == 0 ? 1.0f : -1.0f,
                  edge = t.state.x +
                         side * t.source->behavior("ProximityActivateBehaviour")->num("distance-x"),
                  dt = 1.0f / fps;
            bean.x = edge + side * .25f;
            bodies[playerIndex]->SetTransform(b2Vec2(bean.x * metres, bean.y * metres), 0);
            for (int step = 0; step < fps; step++)
                tick(dt, false, false);
            activation &= !t.state.active && source.clock == offsets[n];
            int fires[2] = {}, expected = 0;
            volatile float reference = offsets[n];
            for (int step = 1; step <= fps * 4; step++) {
                reference = reference + dt;
                if (reference > periods[n]) {
                    expected = step;
                    break;
                }
            }
            for (int attempt = 0; attempt < 2; attempt++) {
                for (int shot = 0; shot < MAX_PROJECTILES; shot++)
                    if (bullets[shot].life > 0)
                        destroyShot(shot);
                bean.x = edge;
                bodies[playerIndex]->SetTransform(b2Vec2(bean.x * metres, bean.y * metres), 0);
                tick(dt, false, false);
                activation &= t.state.active && source.clock == offsets[n];
                for (int step = 1; step <= fps * 4 && !fires[attempt]; step++) {
                    tick(dt, false, false);
                    for (int shot = 0; shot < MAX_PROJECTILES; shot++)
                        if (bullets[shot].life > 0 && bullets[shot].state.active == target + 1)
                            fires[attempt] = step;
                }
                bean.x = edge + side * .25f;
                bodies[playerIndex]->SetTransform(b2Vec2(bean.x * metres, bean.y * metres), 0);
                tick(dt, false, false);
                tick(dt, false, false);
                reentry &= !t.state.active && source.clock == offsets[n];
            }
            activation &= fires[0] == expected;
            reentry &= fires[1] == expected;
            if (n == 5)
                activation &= edge == 1936;
            char row[140];
            wsprintfA(row, "%I64u,%d,%d,%d,%d\r\n", ids[n], fps, (int)edge, fires[0], fires[1]);
            lstrcatA(trace, row);
        }
    check(
        activation,
        "Actual5-3sources wake at their inclusive native distance boundary after physics, then wait the original offset-adjusted interval at30/60/120Hz; upward-source boundary is worldX1936");
    check(
        reentry,
        "Leaving and re-entering all six real5-3source ranges resets the clock to its original offset and repeats the same first-shot delay");
    atomicWrite("test-output/stage53-source-activation.csv", trace, lstrlenA(trace));
    atomicWrite("test-output/stage53-source-results.txt", testLog, lstrlenA(testLog));
    startStage(0);
    readyTimer = 0;
}
