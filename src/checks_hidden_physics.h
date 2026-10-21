#pragma once

// Regression coverage for hidden physics. Runs only in the isolated BQ_ENABLE_TESTS executable.

int hiddenPhysicsOwner(u64 id) {
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->id == id && boxGroupMatrixOwner(entities[i]))
            return i;
    return -1;
}

bool hiddenPhysicsTouch(b2Body *first, b2Body *second) {
    if (!first || !second)
        return false;
    for (b2ContactEdge *e = first->GetContactList(); e; e = e->next)
        if (e->other == second && e->contact->IsTouching())
            return true;
    return false;
}

void hiddenPhysicsNativeChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    // Visibility belongs to rendering. Object::Activate0x279c28 activates
    // every behaviour, including BoxGroupPhysics::OnActivate0x1341ec.
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    int scenes = 0, hidden = 0, hiddenScenes = 0, invisibleSpikes = 0;
    bool matrixOnly = true;
    for (int n = 0; n < levelCount; n++) {
        readLevelData(levels[n].file, scene, memory);
        scenes++;
        int here = 0;
        for (int i = 0; i < scene.objectCount; i++) {
            Object &o = scene.objects[i];
            if (!o.visible && o.has("SpikeGroupBehaviour"))
                invisibleSpikes++;
            if (!o.visible && o.has("BoxGroupPhysicsBehaviour")) {
                hidden++;
                here++;
                matrixOnly &= o.base == 7;
            }
        }
        if (here)
            hiddenScenes++;
    }
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        scenes == 74 && hidden == 5 && hiddenScenes == 3 && matrixOnly && invisibleSpikes == 15,
        "All74 original scenes contain five invisible BoxGroup matrices in three stages and15 separately authored invisible SpikeGroups");
    const u64 ids[] = {8360153663018162ULL, 69523992855ULL, 52833676806ULL, 837060697639ULL,
                       1398718240969741ULL};
    const u64 stages[] = {5770908237787719ULL, 176011402805ULL, 176011402805ULL, 176011402805ULL,
                          1071369106910786ULL};
    bool all = true, filters = true, appended = true;
    int retained = 0;
    u64 last = 0;
    for (int n = 0; n < 5; n++) {
        if (stages[n] != last) {
            startStage(stageById(stages[n]));
            last = stages[n];
        }
        int owner = hiddenPhysicsOwner(ids[n]);
        all &= owner >= 0;
        if (owner < 0)
            continue;
        Entity &e = entities[owner];
        retained++;
        all &= !e.source->visible && bodies[owner] && bodies[owner]->IsActive();
        appended &= owner >= legacyEntityCount;
        if (bodies[owner]) {
            b2Fixture *f = bodies[owner]->GetFixtureList();
            filters &= f && !f->IsSensor() &&
                       f->GetFilterData().maskBits == nativePhysicsMask(e.physics) &&
                       bodies[owner]->GetType() == b2_kinematicBody;
        }
    }
    check(
        retained == 5 && all && filters,
        "All five original invisible cage matrices retain real whole-matrix bodies and authored category filters");
    check(
        appended,
        "Newly retained invisible physical actors follow existing visible and attachment indices for checkpoint compatibility");
    startStage(stageById(5770908237787719ULL));
    readyTimer = 0;
    int guard = hiddenPhysicsOwner(ids[0]), fish[2] = {-1, -1};
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].source->id == 9319735670056212ULL)
            fish[0] = i;
        if (entities[i].source->id == 5660228151801940ULL)
            fish[1] = i;
    }
    check(
        guard >= 0 && fish[0] >= 0 && fish[1] >= 0 && current.geometryCount == 7,
        "Moonlight Oasis retains its original invisible surface guard, both authored fish and all seven terrain geometries");
    if (guard < 0 || fish[0] < 0 || fish[1] < 0)
        return;
    Entity &cage = entities[guard];
    Rect bounds = shifted(boxGroupMatrixFixture(cage), cage.state.x, cage.state.y);
    b2Fixture *cageFixture = bodies[guard] ? bodies[guard]->GetFixtureList() : 0;
    check(
        bounds.l == 288 && bounds.r == 1120 && bounds.b == 160 && bounds.t == 192 && cageFixture &&
            (cageFixture->GetFilterData().maskBits & 32) &&
            !(cageFixture->GetFilterData().maskBits & 2),
        "Original Moonlight surface guard spans288..1120 at160..192, blocks the fish proxy and excludes Bean");
    // Keep all original terrain and physical mechanisms. Unrelated enemy AI
    // cannot kill the observation Bean or stomp a fish during this probe.
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == ENEMY && i != fish[0] && i != fish[1])
            entities[i].state.gone = 1;
    State &bean = entities[playerIndex].state;
    bean.x = 600;
    bean.y = 300;
    bean.vx = bean.vy = 0;
    restorePhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    float high[2] = {entities[fish[0]].state.y, entities[fish[1]].state.y},
          low[2] = {high[0], high[1]};
    bool touch[2] = {}, reflected[2] = {}, contained = true, alive = true;
    for (int step = 0; step < 1440; step++) {
        tick(1.0f / 120.0f, false, false);
        for (int n = 0; n < 2; n++) {
            int index = fish[n];
            Entity &f = entities[index];
            high[n] = maxf(high[n], f.state.y);
            low[n] = minf(low[n], f.state.y);
            bool hit = hiddenPhysicsTouch(oscillatorProxies[index], bodies[guard]);
            touch[n] |= hit;
            reflected[n] |= hit && diagonalRuntime[index].directionY < 0;
            contained &= hitRect(f).t <= bounds.b + 2;
            alive &= !f.state.gone && f.state.active && screen == PLAY;
        }
    }
    check(
        touch[0] && touch[1] && reflected[0] && reflected[1],
        "Both original Moonlight fish reverse downward through real proxy contact with the invisible surface guard");
    check(
        alive && contained && high[0] - low[0] > 12 && high[1] - low[1] > 12,
        "Both fish keep swimming for twelve seconds within the original geometry without an artificial water-height clamp");
    char trace[400];
    wsprintfA(
        trace,
        "scene=5770908237787719 guard=8360153663018162 geometry=%d fishA-min=%d max=%d touched=%d reflected=%d fishB-min=%d max=%d touched=%d reflected=%d contained=%d alive=%d\r\n",
        current.geometryCount, (int)(low[0] * 100), (int)(high[0] * 100), touch[0], reflected[0],
        (int)(low[1] * 100), (int)(high[1] * 100), touch[1], reflected[1], contained, alive);
    atomicWrite("test-output/moonlight-hidden-fish-guard.txt", trace, lstrlenA(trace));
    // A separate real upward traversal proves that the same guard does not
    // become an invisible platform or ceiling for the player. Use enough
    // upward momentum to cross the water drag independently of this guard.
    for (int n = 0; n < 2; n++)
        entities[fish[n]].state.gone = 1;
    bodies[playerIndex]->SetType(b2_dynamicBody);
    bean.x = 700;
    bean.y = bounds.b - entities[playerIndex].hit.t - 2;
    bean.vx = 0;
    bean.vy = 700;
    restorePhysics();
    float maxBean = bean.y;
    bool beanTouch = false;
    for (int step = 0; step < 50; step++) {
        tick(1.0f / 120.0f, false, false);
        maxBean = maxf(maxBean, bean.y);
        beanTouch |= hiddenPhysicsTouch(bodies[playerIndex], bodies[guard]);
    }
    char crossing[256];
    wsprintfA(
        crossing,
        "touch=%d max1000=%d feet1000=%d boundTop1000=%d death1000=%d walk=%d vx1000=%d vy1000=%d\r\n",
        beanTouch, (int)(maxBean * 1000), (int)((maxBean + entities[playerIndex].hit.b) * 1000),
        (int)(bounds.t * 1000), (int)(deathTimer * 1000), playerWalkMode, (int)(bean.vx * 1000),
        (int)(bean.vy * 1000));
    atomicWrite("test-output/fish-guard-player-crossing.txt", crossing, lstrlenA(crossing));
    check(
        !beanTouch && maxBean + entities[playerIndex].hit.b > bounds.t,
        "Bean crosses the original fish-only surface guard from below without a phantom floor or ceiling collision");
    // Real GL pixels: visibility is not implemented by deleting the collider.
    cloudRenderBegin();
    drawEntity(cage);
    u32 invisible = cloudRenderPixels(Rect{288, 160, 512, 192}, true);
    // The guard's own actor can be transparent even when forced visible.
    // A known visible original Bean actor is the positive control for the
    // renderer's visibility gate, independently of that asset's alpha.
    Object visibleSource = *cage.source;
    visibleSource.visible = 1;
    Entity witness = cage;
    witness.art = entities[playerIndex].art;
    witness.local = Rect{-16, -16, 16, 16};
    witness.state.x = 400;
    witness.state.y = 176;
    cloudRenderBegin();
    drawEntity(witness);
    u32 hiddenWitness = cloudRenderPixels(Rect{384, 160, 416, 192}, true);
    witness.source = &visibleSource;
    cloudRenderBegin();
    drawEntity(witness);
    u32 visible = cloudRenderPixels(Rect{384, 160, 416, 192}, true);
    check(
        invisible == 0 && hiddenWitness == 0 && visible > 0,
        "The real hidden fish guard and its known-visible-art witness render no pixels, while the visibility-enabled control renders pixels");
    // Write the actual pre-v11 binary layout with the old visible-only
    // count, then read it through the real file migration path.
    startStage(stageById(5770908237787719ULL));
    readyTimer = 0;
    coins = 7;
    jumps = 13;
    entities[playerIndex].state.x = 600;
    entities[playerIndex].state.y = 300;
    captureCheckpoint(false);
    SnapshotV10 *old =
        (SnapshotV10 *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SnapshotV10));
    memcpy(old, &checkpoint, sizeof(*old) - 4);
    old->version = 10;
    old->count = legacyEntityCount;
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    bool wrote = atomicWrite("test-output/hidden-physics-v10.dat", old, sizeof(*old));
    HeapFree(GetProcessHeap(), 0, old);
    bool migrated = wrote && readCheckpoint("test-output/hidden-physics-v10.dat");
    if (migrated)
        restoreCheckpoint();
    int restoredGuard = hiddenPhysicsOwner(8360153663018162ULL);
    check(
        migrated && coins == 7 && jumps == 13 && entities[playerIndex].state.x == 600 &&
            entities[playerIndex].state.y == 300 && restoredGuard >= legacyEntityCount &&
            bodies[restoredGuard]->IsActive(),
        "An actual version10 visible-only checkpoint preserves Bean and counters while restoring the newly retained invisible fish guard");
    bool currentSave = false;
    captureCheckpoint(false);
    currentSave = checkpoint.version == 28 && validSnapshot(checkpoint);
    restoreCheckpoint();
    check(
        currentSave && checkpoint.count == entityCount && bodies[restoredGuard]->IsActive(),
        "Current checkpoints preserve the appended hidden actor list and remain valid after restoration");
}
