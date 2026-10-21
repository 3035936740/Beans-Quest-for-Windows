#pragma once

// Regression coverage for wizard transition native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void wizardTransitionNativeChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    const float dt = 1.0f / 120.0f;
    int stage = stageById(976871008727ULL);
    char *csv = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 600000);
    if (!csv) {
        check(false, "Wizard transition trace allocation");
        return;
    }
    lstrcpyA(
        csv,
        "arena,step,hp,mode,actor_x,actor_y,body_x,body_y,base_x,base_y,camera_left,actor_screen_x,body_active,script_error_milli\r\n");
    bool initialActive = true, hurtAll = true, travelAll = true, coherent = true,
         noBacktrack = true, activeAll = true, completeAll = true;
    float worst = 0, maxJump = 0;
    int rows = 0;
    for (int area = 0; area < 4; area++) {
        startStage(stage);
        readyTimer = 0;
        int bi = -1;
        for (int i = 0; i < entityCount; i++) {
            if (entities[i].kind == BOSS)
                bi = i;
            if (entities[i].kind == ENEMY)
                entities[i].state.gone = 1;
        }
        if (bi < 0) {
            hurtAll = false;
            continue;
        }
        initialActive &= bodies[bi] && bodies[bi]->IsActive() && entities[bi].state.active == 0;
        Entity &boss = entities[bi];
        Entity &bean = entities[playerIndex];
        float arena =
            listNumber(boss.source->behavior("FinalBossBehaviour")->get("area-offsets"), area);
        boss.state.hp = 5 - area;
        boss.state.active = 3;
        alternationBits = legacyWizardArenaParity(boss.state.hp, 3, false);
        boss.state.timer = 3;
        resetWizardMotion(boss, 5489 + area);
        wizardMotion.baseX = arena + 192;
        wizardMotion.baseY = viewH * 0.5f;
        wizardMotion.offsetX = 192;
        wizardMotion.amplitudeY = -10;
        wizardMotion.leftAge = wizardMotion.rightAge = -1;
        wizardMotion.shooting = 1;
        wizardMotion.cycleAge = 0;
        boss.state.x = wizardMotion.baseX;
        boss.state.y = wizardMotion.baseY;
        camX = arena - viewW * 0.5f;
        camY = 0;
        cameraState.x = camX;
        cameraState.y = 0;
        cameraState.valid = 1;
        bean.state.x = boss.state.x;
        bean.state.y = boss.state.y + boss.hit.t - bean.hit.b + 0.25f;
        bean.state.vx = 0;
        bean.state.vy = -120;
        restorePhysics();
        // Enter the existing collision callback through the real Boss fixture,
        // rather than calling wizardHurt or assigning the next HP by hand.
        for (int attempt = 0; attempt < 8 && boss.state.hp == 5 - area; attempt++)
            tick(dt, false, false);
        bool hit = boss.state.hp == 4 - area && boss.state.active == 2;
        hurtAll &= hit;
        if (!hit)
            continue;
        bodies[playerIndex]->SetType(b2_staticBody);
        bean.state.y = 900;
        bean.state.vx = bean.state.vy = 0;
        float previousX = boss.state.x, firstBase = wizardMotion.baseX;
        int travelling = 0, arrived = -1;
        for (int step = 0; step < 600; step++) {
            // Isolate the Boss transition from unrelated hazards: a static
            // observer advances through the actual camera follower. Arena
            // geometry, Boss physics, hit processing and world Step stay live.
            bean.state.x = arena + 150 + step * 200 * dt;
            tick(dt, false, false);
            float expectedX = wizardMotion.baseX +
                              sine(6.28318530718f * wizardMotion.age / wizardMotion.periodX) *
                                  wizardMotion.amplitudeX;
            float expectedY = wizardMotion.baseY +
                              sine(6.28318530718f * wizardMotion.age / wizardMotion.periodY) *
                                  wizardMotion.amplitudeY;
            b2Vec2 bp = bodies[bi]->GetPosition();
            float error = maxf(
                absf(boss.state.x - expectedX),
                maxf(absf(boss.state.y - expectedY),
                     maxf(absf(bp.x / metres - boss.state.x), absf(bp.y / metres - boss.state.y))));
            worst = maxf(worst, error);
            coherent &= error < 0.01f;
            float change = absf(boss.state.x - previousX);
            maxJump = maxf(maxJump, change);
            noBacktrack &= change <= (220 + 10 * 6.28318530718f / 7) * dt + 0.02f;
            previousX = boss.state.x;
            if (boss.state.active == 2) {
                travelling++;
                activeAll &= bodies[bi]->IsActive();
            }
            if (boss.state.active == 3 && arrived < 0)
                arrived = step;
            char row[260];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", area + 1, step,
                      boss.state.hp, boss.state.active, (int)(boss.state.x * 1000),
                      (int)(boss.state.y * 1000), (int)(bp.x / metres * 1000),
                      (int)(bp.y / metres * 1000), (int)(wizardMotion.baseX * 1000),
                      (int)(wizardMotion.baseY * 1000), (int)(camX * 1000),
                      (int)((boss.state.x - camX) * 1000), bodies[bi]->IsActive(),
                      (int)(error * 1000));
            lstrcatA(csv, row);
            rows++;
            if (arrived >= 0 && step > arrived + 12 &&
                absf(wizardMotion.baseX - (arena + 512 + 192)) < 0.05f)
                break;
        }
        travelAll &= travelling > 100 && wizardMotion.baseX > firstBase + 400;
        completeAll &= arrived >= 0 && boss.state.active == 3 && boss.state.hp == 4 - area;
    }
    atomicWrite("test-output/wizard-transitions-native.csv", csv, lstrlenA(csv));
    HeapFree(GetProcessHeap(), 0, csv);
    check(
        initialActive,
        "Native Wizard BoxPhysics starts active in idle mode0 for its authored unpaused actor instead of being disabled until combat");
    check(
        hurtAll,
        "All four actual Wizard head contacts advance native HP5to4to3to2to1 through the real authored fixture before transition tracking");
    check(
        travelAll && completeAll && rows > 400,
        "All four actual arena transitions travel continuously through the camera follower and enter their next authored combat area");
    check(
        activeAll,
        "Native non-final Wizard transitions retain active physics after SetPhysicsActive(true) instead of disabling the travelling actor");
    check(
        coherent && noBacktrack && worst < 0.01f && maxJump < 2.0f,
        "Wizard actor, physical body and native smoothed sine pose remain coherent every frame without a stale-body128px flashback at any arena transition");
    startStage(0);
    readyTimer = 0;
}
