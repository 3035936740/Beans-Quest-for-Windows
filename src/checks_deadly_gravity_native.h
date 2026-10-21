#pragma once

// Regression coverage for deadly gravity native. Runs only in the isolated BQ_ENABLE_TESTS executable.

bool deadlyFreeCandidateNative(const Entity &e) {
    return e.kind == ENEMY && e.deadly && e.dynamic && !e.walk &&
           !e.source->has("EnemyJumperBehaviour") && !e.move && !e.orbit && !e.motion &&
           !e.rotate && !e.source->has("DiagonalBehaviour");
}

int prepareDeadlyGravityNative(bool floor) {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    debugMode = false;
    startStage(stageById(11491836511846746094ULL));
    readyTimer = 0;
    int target = -1;
    for (int i = 0; i < entityCount; i++)
        if (deadlyFreeCandidateNative(entities[i])) {
            target = i;
            break;
        }
    if (target < 0)
        return -1;
    for (int i = 0; i < entityCount; i++)
        if (i != target && i != playerIndex)
            entities[i].state.gone = 1;
    current.geometryCount = 0;
    Entity &e = entities[target];
    e.state.x = 500;
    e.state.y = 100;
    e.state.angle = 0;
    e.state.vx = e.state.vy = e.state.aux = 0;
    State &bean = entities[playerIndex].state;
    bean.x = 400;
    bean.y = 400;
    bean.vx = bean.vy = 0;
    rebuildPhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    if (floor) {
        b2BodyDef bd;
        bd.position.Set(500 * metres, (100 + e.hit.b - .5f - 10) * metres);
        b2Body *ground = physicsWorld->CreateBody(&bd);
        b2PolygonShape shape;
        shape.SetAsBox(200 * metres, 10 * metres);
        ground->CreateFixture(&shape, 0);
    }
    return target;
}

void deadlyGravityNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/deadly-gravity-saves");
    CreateDirectoryA(saveRoot, 0);
    interfaceEnabled = musicEnabled = soundEnabled = false;
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    int scenes = 0, count = 0;
    bool config = true, gravity = true;
    char rows[5000] = "scene,id,gravity1000,density1000,friction1000,restitution1000\r\n";
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scene, memory);
        scenes++;
        for (int i = 0; i < scene.objectCount; i++) {
            Object &o = scene.objects[i];
            Entity e;
            initializeEntity(e, o, o.rect);
            if (!deadlyFreeCandidateNative(e))
                continue;
            count++;
            b2World *world = new b2World(b2Vec2(0, nativeWorldGravity * metres), true);
            b2Body *body = createPhysicsEntity(*world, e, 0);
            if (!body) {
                config = gravity = false;
                delete world;
                continue;
            }
            b2Fixture *f = body->GetFixtureList();
            config &= body->GetType() == b2_dynamicBody && body->IsSleepingAllowed() &&
                      !body->IsFixedRotation() && absf(f->GetDensity() - 1.5f) < .001f &&
                      absf(f->GetRestitution() - .5f) < .001f &&
                      absf(f->GetFriction() - .2f) < .001f;
            gravity &= body->GetGravityScale() == 1;
            char id[24], row[180];
            idstr(o.id, id);
            wsprintfA(row, "%d,%s,%d,%d,%d,%d\r\n", level, id,
                      (int)(body->GetGravityScale() * 1000), (int)(f->GetDensity() * 1000),
                      (int)(f->GetFriction() * 1000), (int)(f->GetRestitution() * 1000));
            lstrcatA(rows, row);
            delete world;
        }
    }
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        scenes == 74 && count == 17 && config,
        "All74scenes retain the17authored deadly free boxes and their native density1.5 friction0.2 restitution0.5 and rotation/sleep flags");
    check(
        gravity,
        "All17deadly free boxes receive gravity through native BoxPhysics bodies rather than the pre-step fallback");
    bool contacts = true, ui = true;
    char contactRows[2000] = "fps,actualIncoming1000,referenceIncoming1000\r\n";
    for (int rate = 0; rate < 3; rate++) {
        int target = prepareDeadlyGravityNative(true);
        if (target < 0) {
            contacts = false;
            continue;
        }
        Entity &e = entities[target];
        float dt = rate == 0 ? 1 / 30.0f : rate == 1 ? 1 / 120.0f : 1 / 240.0f;
        b2World *reference = new b2World(b2Vec2(0, nativeWorldGravity * metres), true);
        b2Body *oracle = createPhysicsEntity(*reference, e, target);
        oracle->SetGravityScale(1);
        b2BodyDef bd;
        bd.position.Set(500 * metres, (100 + e.hit.b - .5f - 10) * metres);
        b2Body *ground = reference->CreateBody(&bd);
        b2PolygonShape shape;
        shape.SetAsBox(200 * metres, 10 * metres);
        ground->CreateFixture(&shape, 0);
        WalkerGravityReferenceNative *ref = new WalkerGravityReferenceNative;
        ref->recorded.body = oracle;
        reference->SetContactListener(ref);
        WalkerGravityProbeNative *actual = new WalkerGravityProbeNative;
        actual->recorded.body = bodies[target];
        physicsWorld->SetContactListener(actual);
        reference->Step(dt, 10, 10);
        tick(dt, false, false);
        contacts &= actual->recorded.seen && ref->recorded.seen &&
                    absf(actual->recorded.first - ref->recorded.first) < .001f;
        char row[160];
        wsprintfA(row, "%d,%d,%d\r\n",
                  rate == 0   ? 30
                  : rate == 1 ? 120
                              : 240,
                  (int)(actual->recorded.first * 1000), (int)(ref->recorded.first * 1000));
        lstrcatA(contactRows, row);
        physicsWorld->SetContactListener(originalContacts);
        delete actual;
        delete reference;
        delete ref;
        target = prepareDeadlyGravityNative(false);
        Entity copy = entities[target];
        copy.state.vy = -30;
        UiScene sample = {};
        sample.playfield = &copy;
        sample.playfieldCount = 1;
        b2Body *local[2] = {};
        sample.bodies = local;
        sample.physics = new b2World(b2Vec2(0, nativeWorldGravity * metres), true);
        local[0] = createPhysicsEntity(*sample.physics, copy, 0);
        tickUiPlayfield(sample, dt, false);
        ui &= copy.state.vy == -30 && local[0]->GetGravityScale() == 1;
        sample.physics->Step(dt, 10, 10);
        ui &= absf(local[0]->GetLinearVelocity().y / metres - (-30 + nativeWorldGravity * dt)) <
              .001f;
        delete sample.physics;
    }
    check(
        contacts,
        "Actual deadly-box first PreSolve observes native unaccelerated incoming velocity at30/120/240Hz");
    check(
        ui,
        "Independent selector and credits worlds apply deadly-box gravity once inside the solver at30/120/240Hz");
    int target = prepareDeadlyGravityNative(false);
    bool disabled = target >= 0;
    if (disabled) {
        entities[target].state.vy = -30;
        restorePhysics();
        bodies[target]->SetGravityScale(0);
        tick(1 / 120.0f, false, false);
        disabled =
            bodies[target]->GetGravityScale() == 0 && absf(entities[target].state.vy + 30) < .001f;
    }
    check(
        disabled,
        "Explicitly disabled deadly-box body gravity cannot be overridden by manual fallback acceleration");
    captureCheckpoint(false);
    char path[MAX_PATH];
    savePath(path, "current-deadly-off.dat");
    bool read = atomicWrite(path, &checkpoint, sizeof(checkpoint)) && readCheckpoint(path);
    if (read)
        restoreCheckpoint();
    check(
        read && checkpoint.version == 28 && bodies[target]->GetGravityScale() == 0 &&
            absf(entities[target].state.vy + 30) < .001f,
        "Actual version28 disk restoration preserves an explicitly disabled deadly-box gravity flag and saved velocity");
    SnapshotV27 *old = (SnapshotV27 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV27));
    memcpy(old, &checkpoint, sizeof(*old));
    old->version = 27;
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    savePath(path, "legacy-deadly-v27.dat");
    State saved = entities[target].state;
    read = atomicWrite(path, old, sizeof(*old)) && readCheckpoint(path);
    if (read)
        restoreCheckpoint();
    bool migrated = read && checkpoint.version == 27 && sizeof(*old) == 543392 &&
                    bodies[target]->GetGravityScale() == 1 && entities[target].state.x == saved.x &&
                    entities[target].state.y == saved.y && entities[target].state.vy == saved.vy;
    old->checksum ^= 1;
    savePath(path, "corrupt-deadly-v27.dat");
    bool reject = atomicWrite(path, old, sizeof(*old)) && !readCheckpoint(path);
    check(
        migrated && reject,
        "Exact543392byte version27 files migrate only the old free-enemy gravity marker without moving saved poses and still reject corruption");
    bool otherFlags = true;
    for (int kind = 0; kind < 3; kind++) {
        target = kind == 0   ? prepareWalkPermissionNative(2, 0)
                 : kind == 1 ? prepareJumperGravityNative(6, false)
                             : prepareRotorGravityNative(1, false);
        if (target < 0) {
            otherFlags = false;
            continue;
        }
        bodies[target]->SetGravityScale(0);
        captureCheckpoint(false);
        memcpy(old, &checkpoint, sizeof(*old));
        old->version = 27;
        old->checksum = hashBytes(old, sizeof(*old) - 4);
        savePath(path, "legacy-native-other-v27.dat");
        read = atomicWrite(path, old, sizeof(*old)) && readCheckpoint(path);
        if (read)
            restoreCheckpoint();
        otherFlags &= read && bodies[target]->GetGravityScale() == 0;
    }
    check(
        otherFlags,
        "Version27 native walker jumper and rotor gravity flags remain untouched by the free-enemy-only migration");
    HeapFree(GetProcessHeap(), 0, old);
    bool trajectory = true;
    for (int rate = 0; rate < 3; rate++) {
        target = prepareDeadlyGravityNative(false);
        float dt = rate == 0 ? 1 / 30.0f : rate == 1 ? 1 / 120.0f : 1 / 240.0f;
        Entity &e = entities[target];
        b2World *world = new b2World(b2Vec2(0, nativeWorldGravity * metres), true);
        b2Body *reference = createPhysicsEntity(*world, e, target);
        reference->SetGravityScale(1);
        for (int n = 0; n < (int)(.5f / dt); n++) {
            world->Step(dt, 10, 10);
            tick(dt, false, false);
            trajectory &= absf(e.state.y - reference->GetPosition().y / metres) < .001f &&
                          absf(e.state.vy - reference->GetLinearVelocity().y / metres) < .001f;
        }
        delete world;
    }
    check(
        trajectory,
        "Half-second actual campaign free-enemy falls match independent native-gravity trajectories at30/120/240Hz");
    atomicWrite("test-output/deadly-gravity-observations.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/deadly-gravity-contact-observations.csv", contactRows,
                lstrlenA(contactRows));
    atomicWrite("test-output/deadly-gravity-results.txt", testLog, lstrlenA(testLog));
    lstrcpyA(saveRoot, oldRoot);
    startStage(0);
    readyTimer = 0;
}

bool deadlyGravityDiskReloadNative() {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    lstrcpyA(saveRoot, "test-output/deadly-gravity-saves");
    startStage(stageById(11491836511846746094ULL));
    readyTimer = 0;
    char path[MAX_PATH];
    savePath(path, "current-deadly-off.dat");
    bool read = readCheckpoint(path);
    if (read)
        restoreCheckpoint();
    int target = -1;
    for (int i = 0; i < entityCount; i++)
        if (deadlyFreeCandidateNative(entities[i])) {
            target = i;
            break;
        }
    bool ok =
        read && checkpoint.version == 28 && target >= 0 && bodies[target]->GetGravityScale() == 0;
    tick(1 / 120.0f, false, false);
    ok &= target >= 0 && absf(entities[target].state.vy + 30) < .001f;
    const char *result = ok ? "PASS fresh-process deadly free body gravity flag restoration\r\n"
                            : "FAIL fresh-process deadly free body gravity flag restoration\r\n";
    atomicWrite("test-output/deadly-gravity-fresh-process.txt", result, lstrlenA(result));
    return ok;
}
