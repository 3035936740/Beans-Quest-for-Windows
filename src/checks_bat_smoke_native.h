#pragma once

// Regression coverage for bat smoke native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int batSmokePrepareNative(int area, int *selected) {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    int boss = -1, count = 0;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].kind == BOSS)
            boss = i;
        if (entities[i].bossSpawn == 1 && count < 3)
            selected[count++] = i;
        if (entities[i].bossSpawn) {
            entities[i].state.gone = 1;
            if (bodies[i])
                bodies[i]->SetActive(false);
        }
    }
    if (boss < 0 || count != 3)
        return -1;
    Entity &b = entities[boss];
    b.state.hp = 5 - area;
    b.state.active = 3;
    resetWizardMotion(b, 5489 + area);
    float arena = wizardArena(b);
    camX = arena - viewW * .5f;
    camY = 0;
    b.state.x = wizardMotion.baseX = arena + 192;
    b.state.y = wizardMotion.baseY = viewH * .5f;
    wizardMotion.pendingCount = 3;
    for (int n = 0; n < 3; n++)
        wizardMotion.pending[n] = selected[n];
    wizardMotion.shooting = 0;
    wizardMotion.cycleAge = .4f;
    wizardMotion.prespawnAge = 0;
    clearEffects();
    wizardCombatCycle(b, .1f);
    return boss;
}

Vec batSmokeEmitterNative(u64 klass, int instance, bool &found) {
    for (int i = 0; i < burstEmittersNative[0].count; i++) {
        BurstEmitterNative &e = burstEmittersNative[0].emitters[i];
        if (e.klass == klass && e.instance == instance) {
            found = true;
            return center(e.bounds);
        }
    }
    found = false;
    return Vec{0, 0};
}

void batSmokeNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/bat-smoke-saves");
    CreateDirectoryA(saveRoot, 0);
    stopMusic();
    stopSounds();
    bool pooled = true, warnings = true, aligned = true, bodyAligned = true, config = true,
         quarter = true, reused = true;
    char rows[10000] = "area,bat,smokeX100,smokeY100,birthX100,birthY100,phase1000\r\n";
    const float dx[] = {0, 40, 0, -40, 0}, dy[] = {0, 40, 80, 40, 0};
    for (int area = 0; area < 5; area++) {
        int selected[3], boss = batSmokePrepareNative(area, selected);
        if (boss < 0) {
            pooled = false;
            continue;
        }
        Entity &b = entities[boss];
        u64 pre = idnum(b.source->behavior("FinalBossBehaviour")->get("prespawn-effect"));
        Vec warning[3];
        for (int n = 0; n < 3; n++) {
            bool found = false;
            warning[n] = batSmokeEmitterNative(pre, n + 1, found);
            warnings &= found;
        }
        wizardMaterializeSummons(b);
        for (int n = 0; n < 3; n++) {
            Entity &e = entities[selected[n]];
            pooled &= e.bossSpawn == 1 && e.orbit && !e.state.gone;
            Vec smoke = warning[n];
            aligned &= absf(e.state.x - smoke.x) < .01f && absf(e.state.y - smoke.y) < .01f;
            config &= e.radius == 40 && e.period == 4 && absf(e.phase - 4.71238898f) < .00001f;
            bodyAligned &= bodies[selected[n]] &&
                           absf(bodies[selected[n]]->GetPosition().x / metres - smoke.x) < .01f &&
                           absf(bodies[selected[n]]->GetPosition().y / metres - smoke.y) < .01f;
            char row[220];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d\r\n", area, selected[n], (int)(smoke.x * 100),
                      (int)(smoke.y * 100), (int)(e.state.x * 100), (int)(e.state.y * 100),
                      (int)(e.phase * 1000));
            lstrcatA(rows, row);
            Entity probe = e;
            for (int t = 0; t < 5; t++) {
                probe.state.timer = (float)t;
                positionNativeOrbit(probe);
                quarter &= absf(probe.state.x - (smoke.x + dx[t])) < .02f &&
                           absf(probe.state.y - (smoke.y + dy[t])) < .02f;
            }
        }
        if (!area) {
            transitionTime = 0;
            captureCase("test-output/bat-smoke-materialization.bmp");
            captureCheckpoint(false);
            char file[MAX_PATH];
            savePath(file, "birth.dat");
            atomicWrite(file, &checkpoint, sizeof(checkpoint));
        }
        for (int n = 0; n < 3; n++) {
            Entity &e = entities[selected[n]];
            e.state.gone = 1;
            e.state.timer = 2.5f;
            e.state.x += 300;
            e.state.y += 100;
            wizardMotion.pending[n] = selected[n];
        }
        wizardMotion.pendingCount = 3;
        wizardMaterializeSummons(b);
        for (int n = 0; n < 3; n++)
            reused &= absf(entities[selected[n]].state.x - warning[n].x) < .01f &&
                      absf(entities[selected[n]].state.y - warning[n].y) < .01f &&
                      entities[selected[n]].state.timer == 0;
    }
    check(
        pooled,
        "All5Wizard arenas retain all3actual pooled orbiting bats for smoke-position verification");
    check(
        warnings,
        "Production warning effects create3independent white-smoke emitter centres in every original Wizard arena");
    check(
        aligned,
        "Every summoned bat is born at its own actual white-smoke emitter centre rather than40pixels to its upper right");
    check(
        bodyAligned,
        "All15real summoned bat physics bodies start at the smoke centre together with their visible actors");
    check(
        config,
        "Wizard bat pools use native40pixel radius,4second period and270degree start angle instead of the ordinary bat class angle0");
    check(
        quarter,
        "Summoned bat orbit visits bottom/right/top/left/bottom at native one-second quarter periods measured from its smoke centre");
    check(
        reused,
        "Reusing all3bat slots in all5arenas restarts each bat inside its own smoke instead of retaining old travel time or position");
    Entity ordinary;
    Object *klass = classById(4746640408543148145ULL);
    initializeEntity(ordinary, *klass, klass->rect);
    check(
        ordinary.orbit && ordinary.phase == 0 && ordinary.radius == 40 && ordinary.period == 4,
        "Ordinary authored bats retain their original0degree class configuration outside the Wizard summon pool");
    atomicWrite("test-output/bat-smoke-observations.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/bat-smoke-results.txt", testLog, lstrlenA(testLog));
    lstrcpyA(saveRoot, oldRoot);
    startStage(0);
    readyTimer = 0;
}

bool batSmokeDiskReloadNative() {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    lstrcpyA(saveRoot, "test-output/bat-smoke-saves");
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    char file[MAX_PATH];
    savePath(file, "birth.dat");
    bool ok = readCheckpoint(file);
    if (ok)
        restoreCheckpoint();
    int bats = 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].bossSpawn == 1 && !entities[i].state.gone) {
            Entity &e = entities[i];
            bats++;
            float x = e.state.x, y = e.state.y;
            Entity probe = e;
            probe.state.timer = 1;
            positionNativeOrbit(probe);
            ok &= absf(e.phase - 4.71238898f) < .00001f && absf(probe.state.x - x - 40) < .02f &&
                  absf(probe.state.y - y - 40) < .02f;
        }
    ok &= bats == 3;
    const char *report = ok ? "PASS fresh-process bat smoke orbit disk restoration\r\n"
                            : "FAIL fresh-process bat smoke orbit disk restoration\r\n";
    atomicWrite("test-output/bat-smoke-fresh-process.txt", report, lstrlenA(report));
    return ok;
}
