#pragma once

// Regression coverage for wizard followup. Runs only in the isolated BQ_ENABLE_TESTS executable.

void wizardNativeFollowupChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    const float dt = 1.0f / 120.0f;
    int stage = stageById(976871008727ULL);
    startStage(stage);
    readyTimer = 0;
    int bi = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            bi = i;
    check(bi >= 0,
          "Actual Wizard scene resolves for native camera handoff and complete opening shot pair");
    if (bi < 0)
        return;
    Entity &boss = entities[bi];
    resetWizardMotion(boss, 5489);
    bool nativeHalf = absf(wizardMotion.offsetX - (viewW * 0.5f - 64)) < 0.001f;
    boss.state.active = 2;
    wizardMotion.offsetX = 192;
    wizardMotion.offsetY = 0;
    wizardMotion.amplitudeY = -10;
    wizardMotion.baseX = 3856 + 192;
    camX = 3856 - viewW * 0.5f;
    camY = 0;
    wizardMoveBody(boss, dt);
    check(
        nativeHalf && wizardMotion.offsetX == 192 &&
            absf(wizardMotion.baseX - (3856 + 192)) < 0.001f,
        "Wizard native ActorGetSize half-width64 preserves camera-half-width-minus64 waiting offset and authored224minus64plus32 combat offset192");
    // Keep the camera follower's result separate from the boss correction.
    boss.state.active = 2;
    boss.state.hp = 4;
    camX = 3600;
    camY = 20;
    wizardStartCameraBlend();
    camX = 4000;
    camY = 120;
    wizardPostCameraStep(dt);
    bool starts = camX == 3600 && camY == 20;
    wizardCameraPreStep(0.5f);
    camX = 4000;
    camY = 120;
    wizardPostCameraStep(dt);
    check(
        starts && absf(camX - 3800) < 0.001f && absf(camY - 70) < 0.001f && cameraState.x == camX,
        "Wizard retains the pre-hit camera on the transition frame then blends the actual follower result with native smoothstep over1s");
    wizardCameraPreStep(0.5f);
    camX = 4300;
    camY = 120;
    wizardPostCameraStep(dt);
    check(
        camX == 4368 - 256 && camY == 120,
        "Native state2 camera stops at the next arena centre instead of overshooting the gate trigger");
    boss.state.active = 3;
    camX = 3900;
    camY = 30;
    wizardCameraBlend.remaining = 1;
    wizardPostCameraStep(dt);
    check(camX == 3900 && camY == 30,
          "Native combat camera is not modified by the between-arena camera handoff");
    boss.state.active = 5;
    boss.state.gone = 1;
    camX = 5000;
    camY = 40;
    wizardStartCameraBlend();
    wizardCameraPreStep(0.25f);
    camX = 5300;
    camY = 140;
    wizardPostCameraStep(dt);
    check(
        absf(camX - 5046.875f) < 0.001f && absf(camY - 55.625f) < 0.001f,
        "Final Wizard state5 retains the original smooth camera return even when the actor is hidden");

    startStage(stage);
    readyTimer = 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            bi = i;
    Entity &opening = entities[bi];
    State &player = entities[playerIndex].state;
    // Original idle movement runs before the displacement trigger; the boss
    // settles into view without attacking. No arbitrary second shot is added.
    camX = camY = 0;
    for (int step = 0; step < 240; step++)
        updateWizard(opening, dt);
    bool idle = true;
    for (int n = 0; n < 64; n++)
        idle &= bullets[n].life <= 0;
    player.x = wizardMotion.initialBeanX + 11;
    player.y = 192;
    int early = 0, later = 0, handBits = 0;
    for (int step = 0; step < 96; step++) {
        updateWizard(opening, dt);
        if (step == 35)
            for (int n = 0; n < 64; n++)
                early += bullets[n].life > 0;
    }
    for (int n = 0; n < 64; n++)
        if (bullets[n].life > 0) {
            later++;
            handBits |= 1 << bullets[n].state.hp;
        }
    check(
        idle && early == 1 && later == 2 && handBits == 6,
        "Original idle then displacement wake produces both independently phased hand projectiles by0.8s");
    memset(bullets, 0, sizeof(bullets));
    opening.state.hp = 5;
    wizardBeginCombat(opening);
    wizardCombatCycle(opening, 0.8f);
    bool disabled = true;
    for (int n = 0; n < 64; n++)
        disabled &= bullets[n].life <= 0;
    check(
        disabled && wizardMotion.leftAge < 0,
        "Native arena entry disables both firing arms until a later four-second shooting transition");

    bool ranges = true, variation = true;
    unsigned int seen[5][5] = {};
    for (unsigned int seed = 1; seed <= 128; seed++)
        for (int area = 0; area < 5; area++)
            for (int cycle = 0; cycle < 5; cycle++) {
                for (int i = 0; i < entityCount; i++)
                    if (entities[i].bossSpawn)
                        entities[i].state.gone = 1;
                opening.state.hp = 5 - area;
                resetWizardMotion(opening, seed);
                wizardMotion.cycleCount = cycle;
                int category = wizardChooseCategory(opening);
                unsigned int allowed =
                    cycle == 0 ? ((1u << 2) | (1u << 6) | (1u << 5) | (1u << 8))
                    : area == 4
                        ? (cycle == 3  ? (1u << 3)
                           : cycle < 3 ? ((1u << 2) | (1u << 6) | (1u << 5) | (1u << 8) | (1u << 9))
                                       : ((1u << 2) | (1u << 6) | (1u << 5) | (1u << 8) |
                                          (1u << 9) | (1u << 3)))
                    : cycle == 1 ? ((1u << 2) | (1u << 6) | (1u << 5) | (1u << 8) | (1u << 4) |
                                    (1u << 1) | (1u << 9))
                                 : 0x3feu;
                ranges &= category > 0 && (allowed & (1u << category));
                seen[area][cycle] |= 1u << category;
            }
    for (int area = 0; area < 5; area++)
        for (int cycle = 0; cycle < 5; cycle++)
            if (!(area == 4 && cycle == 3))
                variation &= (seen[area][cycle] & (seen[area][cycle] - 1)) != 0;
    check(
        ranges && variation,
        "All five Wizard arenas choose variable native category pools across128 real MT seeds, retaining the authored final-arena cycle3 bird constraint");
    // No reset between calls: a live sequence must advance the same engine.
    opening.state.hp = 5;
    resetWizardMotion(opening, 91827);
    wizardMotion.cycleCount = 2;
    unsigned int liveSeen = 0;
    for (int cycle = 0; cycle < 32; cycle++)
        liveSeen |= 1u << wizardChooseCategory(opening);
    check(
        (liveSeen & (liveSeen - 1)) != 0,
        "A live Wizard summon sequence advances RNG rather than repeatedly reseeding or selecting one fixed category");
    // Actual play from authored initial positions. Log an emitted shot even
    // when camera culling disables it in that very physics step; counting
    // only active slots confuses emission with lifetime.
    char trace[7000] =
        "event,ready_ms,step,hand,alive_or_angle_allowed,bean_x_milli,bean_y_milli,boss_x_milli,boss_y_milli,camera_left_milli,shot_x_milli_or_angle_milli,shot_y_milli\r\n";
    bool naturalPair = true, widePair = false;
    RECT originalWindow;
    GetWindowRect(window, &originalWindow);
    for (int mode = 0; mode < 3; mode++) {
        if (mode == 2)
            resizeWindow(892, 402);
        startStage(stage);
        readyTimer = mode == 1 ? 1.8f : 0;
        int originalBoss = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == BOSS)
                originalBoss = i;
        float lastShotTime[64] = {};
        int lastShotHand[64] = {};
        int events = 0, hands = 0, scheduled = 0, gated = 0, settled = 0;
        for (int step = 0; step < 600 && settled < 3; step++) {
            Entity &beforeBoss = entities[originalBoss];
            if (readyTimer <= 0 && beforeBoss.state.active == 1) {
                float previous = wizardMotion.leftAge, now = previous + dt;
                for (int hand = 0; hand < 2; hand++) {
                    float phase = hand ? 4.8f : 4.3f;
                    if ((int)((now + phase) / 5) > (int)((previous + phase) / 5)) {
                        WizardArmPose pose =
                            wizardArmPose(Vec{beforeBoss.state.x, beforeBoss.state.y}, hand == 1, 1,
                                          wizardArms[hand]);
                        State &p = entities[playerIndex].state;
                        Vec aim = wizardHandAim(pose.hand, Vec{p.x, p.y});
                        float angle = bqAtan2(aim.y, aim.x) * 57.29578f;
                        bool allowed = angle >= 140 || angle <= -90;
                        scheduled |= 1 << (hand + 1);
                        if (!allowed)
                            gated |= 1 << (hand + 1);
                        char row[250];
                        wsprintfA(row, "attempt,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,0\r\n",
                                  mode == 2 ? 892
                                  : mode    ? 1800
                                            : 0,
                                  step, hand + 1, allowed, (int)(p.x * 1000), (int)(p.y * 1000),
                                  (int)(beforeBoss.state.x * 1000),
                                  (int)(beforeBoss.state.y * 1000), (int)(camX * 1000),
                                  (int)(angle * 1000));
                        lstrcatA(trace, row);
                    }
                }
            }
            tick(dt, false, true);
            if (scheduled == 6)
                settled++;
            for (int n = 0; n < 64; n++) {
                Bullet &b = bullets[n];
                bool emitted = (b.state.hp == 1 || b.state.hp == 2) && b.state.timer >= 0 &&
                               b.state.timer <= dt * 1.01f &&
                               (b.state.timer != lastShotTime[n] || b.state.hp != lastShotHand[n]);
                if (emitted && events < 2) {
                    Entity &original = entities[originalBoss];
                    State &p = entities[playerIndex].state;
                    char row[250];
                    wsprintfA(row, "emission,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
                              mode == 2 ? 892
                              : mode    ? 1800
                                        : 0,
                              step, b.state.hp, b.life > 0, (int)(p.x * 1000), (int)(p.y * 1000),
                              (int)(original.state.x * 1000), (int)(original.state.y * 1000),
                              (int)(camX * 1000), (int)(b.state.x * 1000), (int)(b.state.y * 1000));
                    lstrcatA(trace, row);
                    events++;
                    hands |= 1 << b.state.hp;
                }
                lastShotTime[n] = b.state.timer;
                lastShotHand[n] = b.state.hp;
            }
        }
        naturalPair &= scheduled == 6 && ((hands | gated) & 6) == 6;
        if (mode == 2) {
            int alive = 0;
            for (int n = 0; n < 64; n++)
                if (bullets[n].life > 0 && (bullets[n].state.hp == 1 || bullets[n].state.hp == 2))
                    alive |= 1 << bullets[n].state.hp;
            widePair = viewW == 892 && viewH == 402 && hands == 6 && alive == 6;
        }
    }
    SetWindowPos(window, 0, originalWindow.left, originalWindow.top,
                 originalWindow.right - originalWindow.left,
                 originalWindow.bottom - originalWindow.top, SWP_NOZORDER | SWP_NOACTIVATE);
    nativeViewportProjection();
    atomicWrite("test-output/wizard-opening-natural.csv", trace, lstrlenA(trace));
    check(
        naturalPair,
        "Authored Wizard opening evaluates both hand opportunities and preserves the original aim-angle rejection with and without the ready pause");
    check(
        widePair,
        "Actual892by402window retains both independently emitted Wizard opening projectiles after their first complete physics steps from authored actor positions");
    bool spawnedAlive = true, spawnPosition = true;
    const int basics[] = {2, 5, 6, 8};
    for (int kind = 0; kind < 4; kind++) {
        startStage(stage);
        readyTimer = 0;
        int actualBoss = -1;
        for (int i = 0; i < entityCount; i++) {
            if (entities[i].kind == BOSS)
                actualBoss = i;
            if (entities[i].bossSpawn)
                entities[i].state.gone = 1;
        }
        Entity &sourceBoss = entities[actualBoss];
        sourceBoss.state.hp = 4;
        wizardBeginCombat(sourceBoss);
        camX = 4368 - 256;
        camY = 0;
        cameraState.x = camX;
        cameraState.y = camY;
        State &observer = entities[playerIndex].state;
        observer.x = 4160;
        observer.y = 340;
        observer.vx = observer.vy = 0;
        restorePhysics();
        bodies[playerIndex]->SetType(b2_staticBody);
        wizardMotion.pendingCount = 0;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].bossSpawn == basics[kind])
                wizardMotion.pending[wizardMotion.pendingCount++] = i;
        int ids[3];
        for (int n = 0; n < 3; n++)
            ids[n] = wizardMotion.pending[n];
        wizardMaterializeSummons(sourceBoss);
        tick(dt, false, false);
        for (int n = 0; n < 3; n++) {
            Entity &e = entities[ids[n]];
            spawnedAlive &= !e.state.gone && bodies[ids[n]] && bodies[ids[n]]->IsActive();
            float spacing = basics[kind] == 8 ? 196.0f : 112.0f;
            float targetY = basics[kind] == 5 || basics[kind] == 6 ? 160.0f : 128.0f;
            spawnPosition &=
                absf(e.state.x - (4368 + (n - 1) * spacing)) < 3 && absf(e.state.y - targetY) < 1;
        }
    }
    check(
        spawnedAlive && spawnPosition,
        "All three members of every basic Wizard summon pool survive their first real second-arena physics step at the authored formation above the original clouds and spikes");
    startStage(stage);
    readyTimer = 0;
    int timelineBoss = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            timelineBoss = i;
    Entity &tb = entities[timelineBoss];
    tb.state.hp = 4;
    wizardBeginCombat(tb);
    camX = 4368 - 256;
    camY = 0;
    cameraState.x = camX;
    cameraState.y = camY;
    alternationBits = 20;
    State &watcher = entities[playerIndex].state;
    watcher.x = 4160;
    watcher.y = 340;
    watcher.vx = watcher.vy = 0;
    restorePhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    clearEffects();
    wizardMotion.cycleAge = 0;
    wizardMotion.cycleCount = 0;
    wizardMotion.shooting = 0;
    wizardMotion.leftAge = wizardMotion.rightAge = -1;
    wizardMotion.pendingCount = 0;
    bool beforePrepare = false, preparing = false, hiddenUntilBoundary = false, born = false;
    char phases[4000] =
        "step,age_milli,pending,live_enemies,prespawn_particles,spawn_particles,death_particles\r\n";
    for (int step = 0; step < 330; step++) {
        tick(dt, false, false);
        int live = 0, pre = 0, spawn = 0, dead = 0;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].bossSpawn && !entities[i].state.gone)
                live++;
        for (int i = 0; i < MAX_EFFECT_PARTICLES; i++)
            if (effectParticles[i].definition && effectParticles[i].art) {
                u64 id = effectParticles[i].art->id;
                if (id == 998576489116ULL)
                    pre++;
                if (id == 446471755065ULL)
                    spawn++;
                if (id == 13237067977624096758ULL)
                    dead++;
            }
        if (step == 22)
            beforePrepare = wizardMotion.pendingCount == 0 && live == 0 && pre == 0;
        if (step == 35)
            preparing = wizardMotion.pendingCount == 3 && live == 0 && pre > 0 && dead == 0;
        if (step == 321)
            hiddenUntilBoundary =
                wizardMotion.pendingCount == 3 && live == 0 && pre > 0 && dead == 0;
        if (step == 326)
            born = wizardMotion.pendingCount == 0 && live == 3 && spawn > 0 && dead == 0;
        if (step == 22 || step == 35 || step == 321 || step == 326) {
            char row[150];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d\r\n", step, (int)(wizardMotion.cycleAge * 1000),
                      wizardMotion.pendingCount, live, pre, spawn, dead);
            lstrcatA(phases, row);
        }
    }
    atomicWrite("test-output/wizard-materialize-timeline.csv", phases, lstrlenA(phases));
    check(
        beforePrepare && preparing && hiddenUntilBoundary && born,
        "Actual second-arena physics retains the original0.2s colored prespawn then2.7s white-puff materialization with three live enemies and no death explosion");
    startStage(0);
    readyTimer = 0;
}
