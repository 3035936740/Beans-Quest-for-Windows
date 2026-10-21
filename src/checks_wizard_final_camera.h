#pragma once

// Regression coverage for wizard final camera. Runs only in the isolated BQ_ENABLE_TESTS executable.

void wizardFinalCameraChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    const float dt = 1.0f / 120.0f;
    int stage = stageById(976871008727ULL);
    char *csv = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 240000);
    if (!csv) {
        check(false, "Wizard final camera trace allocation");
        return;
    }
    lstrcpyA(
        csv,
        "case,step,mode,death_age_milli,blend_remaining_milli,bean_x_milli,bean_y_milli,camera_x_milli,camera_y_milli,locked_x_milli,follow_target_x_milli\r\n");
    bool contacts = true, held = true, windowHeld = true, released = true, gradual = true,
         formula = true, finished = true, movedBean = true;
    float worst = 0;
    for (int sample = 0; sample < 2; sample++) {
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
            contacts = false;
            continue;
        }
        Entity &boss = entities[bi];
        Entity &bean = entities[playerIndex];
        boss.state.hp = 1;
        boss.state.active = 3;
        boss.state.timer = 3;
        alternationBits = legacyWizardArenaParity(1, 3, false);
        resetWizardMotion(boss, 5489 + sample);
        float arena = wizardArena(boss);
        camX = arena - viewW * 0.5f;
        camY = (current.size.y - viewH) * 0.5f;
        cameraState.x = camX;
        cameraState.y = camY;
        cameraState.valid = 1;
        wizardMotion.baseX = arena + 192;
        wizardMotion.baseY = camY + viewH * 0.5f;
        wizardMotion.offsetX = 192;
        wizardMotion.amplitudeY = -10;
        wizardMotion.leftAge = wizardMotion.rightAge = -1;
        wizardMotion.shooting = 1;
        wizardMotion.cycleAge = 0;
        boss.state.x = wizardMotion.baseX;
        boss.state.y = wizardMotion.baseY;
        bean.state.x = boss.state.x;
        bean.state.y = boss.state.y + boss.hit.t - bean.hit.b + 0.25f;
        bean.state.vx = 0;
        bean.state.vy = -120;
        restorePhysics();
        float lockedX = camX, lockedY = camY;
        Rect oldWindow = cameraState.window;
        for (int attempt = 0; attempt < 8 && boss.state.hp; attempt++)
            tick(dt, false, false);
        bool actual = boss.state.hp == 0 && boss.state.active == 4;
        contacts &= actual;
        held &= absf(camX - lockedX) < 0.001f && absf(camY - lockedY) < 0.001f;
        if (!actual)
            continue;
        bodies[playerIndex]->SetType(b2_staticBody);
        float firstBean = 0, lastBean = 0, followX = lockedX;
        bool reached = false;
        int holdSteps = 0, releaseStep = -1;
        for (int step = 0; step < 400; step++) {
            bean.state.x = arena - (sample ? 310.0f : 180.0f) + step * (sample ? 0.2f : -0.3f);
            bean.state.y = sample ? 300.0f - step * 0.05f : 64.0f + step * 0.05f;
            bean.state.vx = bean.state.vy = 0;
            if (!step)
                firstBean = bean.state.x;
            lastBean = bean.state.x;
            tick(dt, false, false);
            if (boss.state.active == 4) {
                holdSteps++;
                held &= absf(camX - lockedX) < 0.001f && absf(camY - lockedY) < 0.001f;
                windowHeld &=
                    cameraState.window.l == oldWindow.l && cameraState.window.r == oldWindow.r &&
                    cameraState.window.b == oldWindow.b && cameraState.window.t == oldWindow.t;
            }
            if (boss.state.active == 5) {
                reached = true;
                releaseStep = step;
                released &= absf(camX - lockedX) < 0.001f && absf(camY - lockedY) < 0.001f &&
                            wizardCameraBlend.remaining == 1 &&
                            wizardCameraBlend.fromX == lockedX &&
                            wizardCameraBlend.fromY == lockedY;
                followX =
                    clamp(bean.state.x, viewW * 0.5f, current.size.x - viewW * 0.5f) - viewW * 0.5f;
            }
            char row[220];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", sample, step, boss.state.active,
                      (int)(boss.state.timer * 1000), (int)(wizardCameraBlend.remaining * 1000),
                      (int)(bean.state.x * 1000), (int)(bean.state.y * 1000), (int)(camX * 1000),
                      (int)(camY * 1000), (int)(lockedX * 1000), (int)(followX * 1000));
            lstrcatA(csv, row);
            if (reached)
                break;
        }
        released &= reached && holdSteps >= 350 && holdSteps <= 365;
        movedBean &= absf(lastBean - firstBean) > 50;
        if (!reached) {
            finished = false;
            continue;
        }
        float previous = camX, distance = absf(lockedX - followX);
        int progressing = 0;
        for (int step = 0; step < 126; step++) {
            tick(dt, false, false);
            float t = clamp(wizardCameraBlend.remaining, 0, 1), weight = t * t * (3 - 2 * t);
            float expected = followX * (1 - weight) + lockedX * weight,
                  error = maxf(absf(camX - expected), absf(camY - lockedY));
            worst = maxf(worst, error);
            formula &= error < 0.02f;
            gradual &= camX <= previous + 0.001f && camX >= followX - 0.001f;
            if (camX < previous - 0.001f)
                progressing++;
            if (!step)
                gradual &= absf(camX - lockedX) < distance * 0.001f;
            previous = camX;
            char row[220];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", sample, releaseStep + 1 + step,
                      boss.state.active, (int)(boss.state.timer * 1000),
                      (int)(wizardCameraBlend.remaining * 1000), (int)(bean.state.x * 1000),
                      (int)(bean.state.y * 1000), (int)(camX * 1000), (int)(camY * 1000),
                      (int)(lockedX * 1000), (int)(followX * 1000));
            lstrcatA(csv, row);
        }
        gradual &= progressing > 100;
        finished &= absf(camX - followX) < 0.02f && absf(camY - lockedY) < 0.001f &&
                    wizardCameraBlend.remaining <= 0;
    }
    atomicWrite("test-output/wizard-final-camera-native.csv", csv, lstrlenA(csv));
    HeapFree(GetProcessHeap(), 0, csv);
    check(
        contacts,
        "Both actual final Wizard head contacts enter HP0death mode through the original physical head fixture");
    check(
        held && windowHeld && movedBean,
        "Final Wizard death holds both rendered camera axes and follower dead-zone window fixed for the entire three-second animation while Bean moves at two different heights");
    check(
        released,
        "Final Wizard mode5 releases input and captures the held camera with a fresh one-second blend, preserving the exact release-frame position");
    check(
        gradual && formula && finished && worst < 0.02f,
        "Final Wizard camera returns gradually to the actual zero-offset Bean follower through the native one-second smoothstep without an instantaneous jump");
    startStage(0);
    readyTimer = 0;
}
