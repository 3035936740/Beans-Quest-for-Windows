#pragma once

// Regression coverage for wizard retry rng. Runs only in the isolated BQ_ENABLE_TESTS executable.

// These copies are test oracles only. Production retries must never roll the
// live MT back or force a different category after a legitimate repeated draw.
void wizardRetryCheckEngine(SourceRandomState &r) {
    memcpy(r.values, wizardMotion.random, sizeof(r.values));
    r.index = wizardMotion.randomIndex;
}

bool wizardRetryCheckSame(const SourceRandomState &a, const SourceRandomState &b) {
    return a.index == b.index && !memcmp(a.values, b.values, sizeof(a.values));
}

bool wizardRetryCheckCurrent(const SourceRandomState &r) {
    return r.index == wizardMotion.randomIndex &&
           !memcmp(r.values, wizardMotion.random, sizeof(r.values));
}

bool wizardRetryCheckTransition() {
    int frames = 0;
    while (respawnBusy(respawnTransition) && frames++ < 500)
        tick(1.0f / 120.0f, false, false);
    return !respawnBusy(respawnTransition) && screen == PLAY && deathTimer == 0;
}

void wizardRetryRngChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    debugMode = false;
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    int index = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            index = i;
    check(index >= 0,
          "The original Wizard stage resolves for real death/checkpoint random-lifetime checks");
    if (index < 0)
        return;
    Entity &boss = entities[index];
    const float dt = 1.0f / 120.0f;
    resetWizardMotion(boss, 5489);
    boss.state.active = 3;
    boss.state.hp = 5;
    wizardBeginCombat(boss);
    wizardMotion.shooting = 0;
    wizardMotion.cycleAge = 0;
    wizardMotion.cycleCount = 0;
    wizardMotion.baseX = boss.state.x = wizardArena(boss) + 160;
    wizardMotion.baseY = boss.state.y = 320;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].bossSpawn)
            entities[i].state.gone = 1;
    // Isolate Bean from unrelated scenery while preserving the original boss,
    // real summon pools, stage geometry, actual tick and death/fade/load path.
    State &bean = entities[playerIndex].state;
    bean.x = 64;
    bean.y = 400;
    bean.vx = bean.vy = 0;
    restorePhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    captureCheckpoint(false);
    bool pool = true, restored = true, continued = true, transitions = true;
    int kinds[12] = {}, distinct = 0, repeats = 0;
    for (int attempt = 0; attempt < 12; attempt++) {
        bodies[playerIndex]->SetType(b2_staticBody);
        for (int frame = 0;
             frame < 30 && !wizardMotion.pendingCount && !respawnBusy(respawnTransition); frame++)
            tick(dt, false, false);
        int count = wizardMotion.pendingCount,
            category = count ? entities[wizardMotion.pending[0]].bossSpawn : 0;
        kinds[attempt] = category;
        pool &= count == 3 && (category == 2 || category == 5 || category == 6 || category == 8);
        for (int n = 0; n < count; n++)
            pool &= entities[wizardMotion.pending[n]].bossSpawn == category &&
                    entities[wizardMotion.pending[n]].state.gone;
        bool seen = false;
        for (int n = 0; n < attempt; n++)
            seen |= kinds[n] == category;
        if (!seen)
            distinct++;
        if (attempt && kinds[attempt - 1] == category)
            repeats++;
        static SourceRandomState live, sourceLive;
        wizardRetryCheckEngine(live);
        sourceLive = sourceRandom;
        die();
        transitions &= respawnTransition.request == RESPAWN_DEATH && wizardRetryCheckTransition();
        continued &=
            wizardRetryCheckCurrent(live) && wizardRetryCheckSame(sourceLive, sourceRandom);
        restored &= boss.state.active == 3 && boss.state.hp == 5 && wizardMotion.cycleAge == 0 &&
                    wizardMotion.cycleCount == 0 && wizardMotion.pendingCount == 0 &&
                    bean.x == 64 && bean.y == 400;
    }
    check(
        pool,
        "Twelve real retry cycles prepare three original enemies from the native first-cycle restricted pool");
    check(
        transitions && restored,
        "Real death wait and black-screen restoration reset Wizard motion and summon progress on every attempt");
    check(
        continued,
        "Death/checkpoint restoration preserves every live Wizard and cannon MT word plus both indices");
    check(
        distinct > 1,
        "A fixed deterministic seed produces varying legal summon categories across twelve retries without reseeding or forcing differences");
    // Cross an MT twist boundary. Restoring only the index would accidentally
    // pass a short run, but cannot pass this full-engine continuation check.
    for (int i = 0; i < 700; i++) {
        wizardRandomWord();
        sourceRandomWord();
    }
    static SourceRandomState beforeS, sourceBeforeS;
    wizardRetryCheckEngine(beforeS);
    sourceBeforeS = sourceRandom;
    keyPressed('S');
    bool s = respawnTransition.request == RESPAWN_CHECKPOINT && wizardRetryCheckTransition();
    check(
        s && wizardRetryCheckCurrent(beforeS) &&
            wizardRetryCheckSame(sourceBeforeS, sourceRandom) && wizardMotion.cycleAge == 0,
        "Actual S input restores checkpoint motion after an MT twist while leaving both random streams live");
    for (int i = 0; i < 7; i++) {
        wizardRandomWord();
        sourceRandomWord();
    }
    static SourceRandomState beforeR, sourceBeforeR;
    wizardRetryCheckEngine(beforeR);
    sourceBeforeR = sourceRandom;
    keyPressed('R');
    bool r = respawnTransition.request == RESPAWN_RESTART && wizardRetryCheckTransition();
    check(
        r && wizardRetryCheckCurrent(beforeR) &&
            wizardRetryCheckSame(sourceBeforeR, sourceRandom) && wizardMotion.initialized &&
            boss.state.active == initialState[index].active &&
            boss.state.hp == initialState[index].hp,
        "Actual R input resets the whole encounter while preserving the current LevelTask random lifetime");
    char trace[512], sequence[96] = {};
    for (int i = 0; i < 12; i++) {
        char part[12];
        wsprintfA(part, "%s%d", i ? "," : "", kinds[i]);
        lstrcatA(sequence, part);
    }
    wsprintfA(
        trace,
        "scene=976871008727 seed=5489 retries=12 categories=%s distinct=%d adjacent-repeats=%d pool=%d transitions=%d motion-restored=%d engine-continued=%d S=%d R=%d\r\n",
        sequence, distinct, repeats, pool, transitions, restored, continued, s, r);
    atomicWrite("test-output/wizard-retry-rng.txt", trace, lstrlenA(trace));
    respawnCancel(respawnTransition);
    startStage(0);
    readyTimer = 0;
}
