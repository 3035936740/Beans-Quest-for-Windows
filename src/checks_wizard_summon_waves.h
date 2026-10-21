#pragma once

// Regression coverage for wizard summon waves. Runs only in the isolated BQ_ENABLE_TESTS executable.
struct WizardSummonDeathProbe : public OriginalContacts {
    int step, area, first, second, events;
    int cause[MAX_ENTITIES];
    char *csv;

    WizardSummonDeathProbe() {
        csv = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 2000000);
        step = area = first = second = events = 0;
        memset(cause, 0, sizeof(cause));
    }

    ~WizardSummonDeathProbe() {
        HeapFree(GetProcessHeap(), 0, csv);
    }

    void log(b2Contact *c, const char *reason, int index) {
        cause[index] = step + 1;
        if (events >= 3000)
            return;
        b2Fixture *a = c->GetFixtureA(), *b = c->GetFixtureB();
        int ia = (int)a->GetBody()->GetUserData() - 1, ib = (int)b->GetBody()->GetUserData() - 1;
        int other = ia == index ? ib : ia;
        b2Body *ob = ia == index ? b->GetBody() : a->GetBody();
        Vec v = {ob->GetLinearVelocity().x / metres, ob->GetLinearVelocity().y / metres};
        char row[300];
        wsprintfA(row, "%d,%d,%d,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", area, first, second,
                  step, reason, index, entities[index].bossSpawn, other,
                  other >= 0 && other < entityCount ? entities[other].bossSpawn : 0, ob->GetType(),
                  (int)(v.x * 100), (int)(v.y * 100), (int)(ob->GetMass() * 10000),
                  a->GetUserData() == (void *)15, b->GetUserData() == (void *)15, c->IsEnabled());
        lstrcatA(csv, row);
        events++;
    }

    void BeginContact(b2Contact *c) {
        bool before[2];
        int indices[] = {(int)c->GetFixtureA()->GetBody()->GetUserData() - 1,
                         (int)c->GetFixtureB()->GetBody()->GetUserData() - 1};
        for (int k = 0; k < 2; k++)
            before[k] =
                indices[k] >= 0 && indices[k] < entityCount ? spikeGroupPending[indices[k]] : true;
        OriginalContacts::BeginContact(c);
        for (int k = 0; k < 2; k++)
            if (!before[k] && spikeGroupPending[indices[k]])
                log(c, "spike_begin", indices[k]);
    }

    void PreSolve(b2Contact *c, const b2Manifold *m) {
        bool before[2];
        int indices[] = {(int)c->GetFixtureA()->GetBody()->GetUserData() - 1,
                         (int)c->GetFixtureB()->GetBody()->GetUserData() - 1};
        for (int k = 0; k < 2; k++)
            before[k] = indices[k] >= 0 && indices[k] < entityCount
                            ? killableContactPending[indices[k]]
                            : true;
        OriginalContacts::PreSolve(c, m);
        for (int k = 0; k < 2; k++)
            if (!before[k] && killableContactPending[indices[k]])
                log(c, "presolve", indices[k]);
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *m) {
        bool before[2];
        int indices[] = {(int)c->GetFixtureA()->GetBody()->GetUserData() - 1,
                         (int)c->GetFixtureB()->GetBody()->GetUserData() - 1};
        for (int k = 0; k < 2; k++)
            before[k] = indices[k] >= 0 && indices[k] < entityCount
                            ? killableContactPending[indices[k]]
                            : true;
        OriginalContacts::PostSolve(c, m);
        for (int k = 0; k < 2; k++)
            if (!before[k] && killableContactPending[indices[k]])
                log(c, "deadly", indices[k]);
    }
};

void wizardSummonWaveNativeChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    const float dt = 1.0f / 120;
    WizardSummonDeathProbe *probe = new WizardSummonDeathProbe;
    char *summary = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 200000);
    lstrcpyA(
        summary,
        "arena,previous_category,new_category,entity,category,born_step,died_step,age_ms,x100,y100,body_x100,body_y100\r\n");
    lstrcpyA(
        probe->csv,
        "arena,previous_category,new_category,step,reason,victim,victim_category,other,other_category,body_type,vx100,vy100,mass10000,fixture_a_proxy,fixture_b_proxy,enabled_after\r\n");
    int unexpected = 0, rows = 0, birthErrors = 0, cases = 0, reused = 0;
    for (int area = 0; area < 5; area++)
        for (int previous = 0; previous <= 9; previous++)
            for (int category = 1; category <= 9; category++) {
                startStage(stageById(976871008727ULL));
                readyTimer = 0;
                int bossIndex = -1;
                for (int i = 0; i < entityCount; i++) {
                    if (entities[i].kind == BOSS)
                        bossIndex = i;
                    if (entities[i].bossSpawn)
                        entities[i].state.gone = 1;
                }
                if (bossIndex < 0)
                    continue;
                Entity &boss = entities[bossIndex];
                boss.state.hp = 5 - area;
                boss.state.active = 3;
                boss.state.timer = 3;
                resetWizardMotion(boss, 5489 + area);
                float arena = wizardArena(boss);
                boss.state.x = wizardMotion.baseX = arena + 192;
                boss.state.y = wizardMotion.baseY = viewH * 0.5f;
                wizardMotion.amplitudeY = -10;
                wizardMotion.offsetX = 192;
                wizardMotion.shooting = 1;
                wizardMotion.cycleAge = 0;
                wizardMotion.leftAge = wizardMotion.rightAge = -1;
                alternationBits = legacyWizardArenaParity(boss.state.hp, 3, false);
                camX = arena - viewW * 0.5f;
                camY = 0;
                cameraState.x = camX;
                cameraState.y = 0;
                cameraState.valid = 1;
                Entity &bean = entities[playerIndex];
                bean.state.x = arena - 200;
                bean.state.y = viewH * 0.5f;
                bean.state.vx = bean.state.vy = 0;
                restorePhysics();
                bodies[playerIndex]->SetType(b2_staticBody);
                for (b2Fixture *f = bodies[playerIndex]->GetFixtureList(); f; f = f->GetNext()) {
                    b2Filter filter = f->GetFilterData();
                    filter.maskBits = 0;
                    f->SetFilterData(filter);
                }
                physicsWorld->SetContactListener(probe);
                probe->area = area;
                probe->first = previous;
                probe->second = category;
                memset(probe->cause, 0, sizeof(probe->cause));
                if (previous) {
                    wizardMotion.pendingCount = 0;
                    for (int i = 0; i < entityCount; i++)
                        if (entities[i].bossSpawn == previous && wizardMotion.pendingCount < 3)
                            wizardMotion.pending[wizardMotion.pendingCount++] = i;
                    wizardMaterializeSummons(boss);
                    for (int frame = 0; frame < 240; frame++) {
                        probe->step = frame - 240;
                        tick(dt, false, false);
                    }
                    if (previous == category) {
                        for (int i = 0; i < entityCount; i++)
                            if (entities[i].bossSpawn == previous) {
                                entities[i].state.gone = 1;
                                if (bodies[i])
                                    bodies[i]->SetActive(false);
                                if (oscillatorProxies[i])
                                    oscillatorProxies[i]->SetActive(false);
                            }
                        reused++;
                    }
                }
                wizardMotion.pendingCount = 0;
                int selected[3], count = 0;
                for (int i = 0; i < entityCount; i++)
                    if (entities[i].bossSpawn == category && count < 3) {
                        selected[count++] = i;
                        wizardMotion.pending[wizardMotion.pendingCount++] = i;
                    }
                wizardMaterializeSummons(boss);
                bool logged[3] = {};
                cases++;
                for (int n = 0; n < count; n++) {
                    int i = selected[n];
                    Entity &e = entities[i];
                    if (!bodies[i] ||
                        absf(bodies[i]->GetPosition().x / metres - e.state.x) > 0.01f ||
                        absf(bodies[i]->GetPosition().y / metres - e.state.y) > 0.01f)
                        birthErrors++;
                    if (oscillatorProxies[i] &&
                        (oscillatorProxies[i]->GetPosition() - bodies[i]->GetPosition())
                                .LengthSquared() > 0.000001f)
                        birthErrors++;
                }
                memset(probe->cause, 0, sizeof(probe->cause));
                for (int frame = 0; frame < 180; frame++) {
                    probe->step = frame;
                    tick(dt, false, false);
                    for (int n = 0; n < count; n++) {
                        int i = selected[n];
                        Entity &e = entities[i];
                        if (e.state.gone && !logged[n]) {
                            logged[n] = true;
                            if (frame < 120 && category != 9 && probe->cause[i] != frame + 1)
                                unexpected++;
                            b2Vec2 p = bodies[i] ? bodies[i]->GetPosition() : b2Vec2(0, 0);
                            char row[230];
                            wsprintfA(row, "%d,%d,%d,%d,%d,0,%d,%d,%d,%d,%d,%d\r\n", area, previous,
                                      category, i, e.bossSpawn, frame,
                                      (int)((frame + 1) * dt * 1000), (int)(e.state.x * 100),
                                      (int)(e.state.y * 100), (int)(p.x / metres * 100),
                                      (int)(p.y / metres * 100));
                            lstrcatA(summary, row);
                            rows++;
                        }
                    }
                }
                physicsWorld->SetContactListener(originalContacts);
            }
    atomicWrite("test-output/wizard-summon-wave-deaths.csv", summary, lstrlenA(summary));
    atomicWrite("test-output/wizard-summon-contact-deaths.csv", probe->csv, lstrlenA(probe->csv));
    char totals[180];
    wsprintfA(
        totals,
        "unexplained_early_deaths=%d death_rows=%d contact_rows=%d birth_errors=%d cases=%d reused=%d\r\n",
        unexpected, rows, probe->events, birthErrors, cases, reused);
    atomicWrite("test-output/wizard-summon-wave-totals.txt", totals, lstrlenA(totals));
    check(
        cases == 450 && reused == 45,
        "Actual Wizard materialization covers all9 pools in all5 arenas, alone and after every previous pool including45 reused-pool cases");
    check(
        birthErrors == 0,
        "Every summoned actor and independent motion detector starts at its new native spawn pose before the first solver step instead of sweeping from an old arena");
    check(
        unexpected == 0,
        "All450 Wizard wave scenarios classify every first-second death through a real native contact instead of unexplained self-destruction");
    HeapFree(GetProcessHeap(), 0, summary);
    delete probe;
    startStage(0);
    readyTimer = 0;
}
