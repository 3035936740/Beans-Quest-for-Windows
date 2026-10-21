#pragma once

// Regression coverage for wizard dismiss native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int wizardDismissSmokeCountNative(u64 effect) {
    int count = 0;
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++) {
        EffectParticle &p = effectParticles[i];
        if (p.definition && p.domain == 0 && p.art && p.art->id == effect)
            count++;
    }
    return count;
}

int wizardDismissPrepareNative(int area, int category, int *selected) {
    int unused[3], boss = batSmokePrepareNative(area, unused), count = 0;
    if (boss < 0)
        return -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].bossSpawn == category && count < 3)
            selected[count++] = i;
    if (count != 3)
        return -1;
    wizardMotion.pendingCount = 3;
    for (int n = 0; n < 3; n++)
        wizardMotion.pending[n] = selected[n];
    wizardMaterializeSummons(entities[boss]);
    for (int n = 0; n < 3; n++) {
        Entity &e = entities[selected[n]];
        e.state.x += 23 + n * 9;
        e.state.y += 37 + n * 7;
    }
    entities[boss].state.timer = 3;
    restorePhysics();
    clearEffects();
    return boss;
}

void wizardDismissNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/wizard-dismiss-saves");
    CreateDirectoryA(saveRoot, 0);
    bool smoke = true, positions = true, hidden = true, bodiesOff = true, once = true,
         configured = true;
    int cases = 0;
    for (int area = 0; area < 5; area++)
        for (int category = 1; category <= 9; category++) {
            int selected[3], boss = wizardDismissPrepareNative(area, category, selected);
            if (boss < 0) {
                configured = false;
                continue;
            }
            Entity &b = entities[boss];
            u64 effect = idnum(b.source->behavior("FinalBossBehaviour")->get("spawn-effect"));
            Object *fx = effectClass(effect);
            int size = fx ? (int)fx->props.num("explosion-size", 1) : 0;
            Vec point[3];
            for (int n = 0; n < 3; n++)
                point[n] = Vec{entities[selected[n]].state.x, entities[selected[n]].state.y};
            u32 before = emittedParticles;
            wizardHurt(b);
            int found[3] = {};
            smoke &= size > 0 && wizardDismissSmokeCountNative(effect) == 3 * size &&
                     emittedParticles >= before + 3 * size;
            for (int i = 0; i < MAX_EFFECT_PARTICLES; i++) {
                EffectParticle &p = effectParticles[i];
                if (!p.definition || p.domain || !p.art || p.art->id != effect)
                    continue;
                bool matched = false;
                for (int n = 0; n < 3; n++)
                    if (absf(p.initialX - point[n].x) < 1 && absf(p.initialY - point[n].y) < 1 &&
                        p.layer == entities[selected[n]].layer) {
                        found[n]++;
                        matched = true;
                    }
                positions &= matched;
            }
            for (int n = 0; n < 3; n++) {
                positions &= found[n] == size;
                hidden &= entities[selected[n]].state.gone == 1;
                bodiesOff &= (!bodies[selected[n]] || !bodies[selected[n]]->IsActive()) &&
                             (!oscillatorProxies[selected[n]] ||
                              !oscillatorProxies[selected[n]]->IsActive());
            }
            u32 after = emittedParticles;
            wizardHurt(b);
            once &= emittedParticles == after;
            cases++;
            if (area == 0 && category == 1) {
                transitionTime = 0;
                captureCase("test-output/wizard-dismiss-smoke.bmp");
            }
        }
    check(
        configured && cases == 45,
        "All9actual Wizard summon categories across all5hits including the final hit enter the dismissal test");
    check(
        smoke,
        "Every live summoned enemy and obstacle emits the authored white spawn smoke when the Wizard is hit including the final hit");
    check(
        positions,
        "All135dismissal puffs originate at each moving summon current actor position and layer rather than its old spawn location");
    check(
        hidden && bodiesOff,
        "Wizard dismissal immediately hides summons and disables both real bodies and oscillator proxies");
    check(
        once,
        "Repeated damage callbacks during Wizard retreat or death never duplicate dismissal smoke");
    int selected[3], boss = wizardDismissPrepareNative(0, 1, selected);
    Entity &b = entities[boss];
    u64 effect = idnum(b.source->behavior("FinalBossBehaviour")->get("spawn-effect"));
    int size = (int)effectClass(effect)->props.num("explosion-size", 1);
    killEnemy(entities[selected[0]]);
    entities[selected[1]].state.gone = 1;
    clearEffects();
    wizardHurt(b);
    check(
        wizardDismissSmokeCountNative(effect) == 2 * size && entities[selected[0]].state.gone == 1,
        "A still-visible squashed summon also becomes smoke while an already hidden member produces no extra puff");
    boss = wizardDismissPrepareNative(0, 1, selected);
    Entity &warningBoss = entities[boss];
    for (int n = 0; n < 3; n++) {
        entities[selected[n]].state.gone = 1;
        wizardMotion.pending[n] = selected[n];
    }
    wizardMotion.pendingCount = 3;
    wizardMotion.shooting = 0;
    wizardMotion.cycleAge = .4f;
    wizardCombatCycle(warningBoss, .1f);
    u64 pre = idnum(warningBoss.source->behavior("FinalBossBehaviour")->get("prespawn-effect"));
    wizardHurt(warningBoss);
    check(
        wizardDismissSmokeCountNative(effect) == 0 && wizardDismissSmokeCountNative(pre) == 0 &&
            wizardMotion.pendingCount == 0,
        "A hit during the warning window cancels unmaterialized summons without inventing dismissal puffs at hidden pool members");
    boss = wizardDismissPrepareNative(0, 1, selected);
    entities[boss].state.timer = 1.99f;
    wizardHurt(entities[boss]);
    check(
        entities[boss].state.hp == 4 && entities[selected[0]].state.gone &&
            wizardDismissSmokeCountNative(effect) == 3 * size,
        "The source-backed combat hit transition has no invented timer1.99 invulnerability gate and dismisses its live summons once");
    boss = wizardDismissPrepareNative(0, 1, selected);
    Entity &contactBoss = entities[boss];
    Entity &bean = entities[playerIndex];
    bean.state.x = contactBoss.state.x;
    bean.state.y = contactBoss.state.y + contactBoss.hit.t - bean.hit.b + .25f;
    bean.state.vx = 0;
    bean.state.vy = -120;
    restorePhysics();
    for (int n = 0; n < 8 && contactBoss.state.hp == 5; n++)
        tick(1 / 120.0f, false, false);
    bool realContact =
        contactBoss.state.hp == 4 && wizardDismissSmokeCountNative(effect) == 3 * size;
    check(
        realContact,
        "An actual Bean head contact through the real Wizard fixture produces all3dismissal puffs through production tick");
    tickEffects(5);
    check(
        realContact && wizardDismissSmokeCountNative(effect) == 0,
        "Dismissal smoke expires through the authored particle lifetime without leaving a persistent summon or white sprite");
    atomicWrite("test-output/wizard-dismiss-results.txt", testLog, lstrlenA(testLog));
    lstrcpyA(saveRoot, oldRoot);
    startStage(0);
    readyTimer = 0;
}
