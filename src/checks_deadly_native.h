#pragma once

// Regression coverage for deadly native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void deadlyNativeChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    debugMode = false;
    Object *regular = classById(17501560557495693516ULL);
    Object *summoned = classById(3019662981890856ULL);
    Object *beetle = classById(14264481342587988014ULL);
    Object *bossClass = classById(489958974619ULL);
    check(
        regular && summoned && beetle && bossClass && !nativeDeadlyKillsEnemy(*regular, *beetle) &&
            nativeDeadlyKillsEnemy(*summoned, *beetle) &&
            !nativeDeadlyKillsEnemy(*summoned, *bossClass),
        "Original40px regular and Wizard-spawn spike blocks have distinct kill-enemies flags, and the Wizard has no KillableEnemy behaviour");
    if (!regular || !summoned || !beetle || !bossClass)
        return;
    bool slowRegular = false, fastRegular = false, slowSummon = false, sensorSurvived = false,
         sensorTouched = false, farSurvived = false, wizardImmune = false, wizardFiltered = false;
    float mass = 0;
    for (int scenario = 0; scenario < 7; scenario++) {
        startStage(0);
        readyTimer = 0;
        current.geometryCount = 0;
        for (int i = 0; i < entityCount; i++)
            if (i != playerIndex)
                entities[i].state.gone = 1;
        int source = entityCount;
        addEntity(scenario < 2 ? *regular : *summoned, Rect{236, 108, 276, 148});
        int target = entityCount;
        float x = scenario == 3 ? 288.0f : scenario == 4 ? 290.0f : scenario >= 5 ? 305.0f : 283.0f;
        Rect targetRect =
            scenario >= 5 ? Rect{x - 64, 48, x + 64, 208} : Rect{x - 16, 112, x + 16, 144};
        addEntity(scenario >= 5 ? *bossClass : *beetle, targetRect);
        Entity &victim = entities[target];
        victim.state.active = scenario >= 5 ? 3 : 1;
        victim.state.vx = victim.state.vy = 0;
        if (scenario != 3)
            victim.walk = 0;
        entities[source].state.vx = scenario == 1 ? 650.0f : 0;
        entities[source].state.vy = 0;
        entities[playerIndex].state.x = 48;
        entities[playerIndex].state.y = 300;
        entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
        rebuildPhysics();
        mass = bodies[source]->GetMass();
        // The original Wizard excludes all loose/oscillating contacts. The
        // extra diagnostic case deliberately admits this one physical pair
        // to separately exercise the lack of KillableEnemy on its actor.
        if (scenario == 6) {
            b2Fixture *f = bodies[target]->GetFixtureList();
            b2Filter filter = f->GetFilterData();
            filter.maskBits = 0xffff;
            f->SetFilterData(filter);
        }
        // Real solver contacts only. No visual bounds or source kill helper
        // is invoked to manufacture the expected death in these scenarios.
        simulateBodies(1.0f / 120.0f);
        if (scenario == 0)
            slowRegular = !victim.state.gone;
        if (scenario == 1)
            fastRegular = victim.state.gone;
        if (scenario == 2) {
            slowSummon = killableDeathVisibleNative(victim) && !bodies[target]->IsActive();
            advanceKillableDeathNative(victim, .11f);
            slowSummon &= victim.state.gone == 1 && liveEffects() > 0;
        }
        if (scenario == 3) {
            sensorSurvived = !victim.state.gone;
            for (b2ContactEdge *edge = bodies[source]->GetContactList(); edge; edge = edge->next)
                if (edge->other == bodies[target] && edge->contact->IsTouching())
                    sensorTouched |= edge->contact->GetFixtureA()->IsSensor() ||
                                     edge->contact->GetFixtureB()->IsSensor();
        }
        if (scenario == 4)
            farSurvived = !victim.state.gone;
        if (scenario >= 5) {
            bool touch = false;
            for (b2ContactEdge *edge = bodies[source]->GetContactList(); edge; edge = edge->next)
                touch |= edge->other == bodies[target] && edge->contact->IsTouching();
            bool alive = !victim.state.gone && victim.state.hp == 5 && victim.state.active == 3;
            if (scenario == 5)
                wizardFiltered = alive && !touch &&
                                 bodies[target]->GetFixtureList()->GetFilterData().maskBits == 2;
            else
                wizardImmune = alive && touch;
        }
    }
    check(
        absf(mass - 1.5f * 30 * 30 * metres * metres) < 0.00001f && slowRegular && fastRegular,
        "Regular40px spike block retains its native30px density1.5 fixture: weak contact survives, sufficient actual momentum kills");
    check(
        slowSummon,
        "Wizard's kill-enemies spike block disables the original beetle on a real slow contact, then completes the native delayed death explosion");
    check(
        sensorTouched && sensorSurvived && farSurvived,
        "Deadly block touching only the beetle walking sensor, or only overlapping sprite artwork, cannot trigger an invented kill");
    check(
        wizardFiltered && wizardImmune,
        "Original Wizard filters exclude the spike block; an explicitly admitted diagnostic solid contact still cannot kill its non-Killable actor");
    // Exercise the actual preallocated Wizard pools and materialization path,
    // preserving the entire real boss terrain and source actor configuration.
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    int boss = -1, target = -1, source = -1;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].kind == BOSS)
            boss = i;
        if (entities[i].bossSpawn == 2 && target < 0)
            target = i;
        if (entities[i].bossSpawn == 9 && source < 0)
            source = i;
    }
    bool spawned = false, killed = false;
    if (boss >= 0 && target >= 0 && source >= 0) {
        wizardMotion.pendingCount = 2;
        wizardMotion.pending[0] = target;
        wizardMotion.pending[1] = source;
        wizardMaterializeSummons(entities[boss]);
        spawned = !entities[target].state.gone && !entities[source].state.gone &&
                  entities[source].source->klass == 3019662981890856ULL;
        entities[target].state.x = 4027;
        entities[target].state.y = 1000;
        entities[target].state.vx = entities[target].state.vy = 0;
        entities[source].state.x = 4000;
        entities[source].state.y = 1000;
        entities[source].state.vx = entities[source].state.vy = 0;
        entities[playerIndex].state.x = 48;
        entities[playerIndex].state.y = 300;
        restorePhysics();
        simulateBodies(1.0f / 120.0f);
        killed = entities[target].state.gone && !entities[source].state.gone;
    }
    check(
        spawned && killed,
        "Actual Wizard materialization pool preserves kill-enemies on its emitted spike block and kills its emitted beetle through real collision");
    char trace[300];
    wsprintfA(
        trace,
        "regular-slow=%d regular-fast=%d summon-slow=%d sensor-touched=%d sensor-survived=%d sprite-only-survived=%d wizard-immune=%d wizard-filtered=%d actual-spawn=%d actual-spawn-killed=%d mass-times1e6=%d\r\n",
        slowRegular, fastRegular, slowSummon, sensorTouched, sensorSurvived, farSurvived,
        wizardImmune, wizardFiltered, spawned, killed, (int)(mass * 1000000));
    atomicWrite("test-output/deadly-native-contact.txt", trace, lstrlenA(trace));
    startStage(0);
}
