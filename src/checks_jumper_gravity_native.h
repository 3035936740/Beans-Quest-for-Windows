#pragma once

// Regression coverage for jumper gravity native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int prepareJumperGravityNative(int category, bool floor) {
    int selected[3], boss = wizardDismissPrepareNative(1, category, selected), target = selected[0];
    for (int i = 0; i < entityCount; i++)
        if (i != target && i != playerIndex && (floor || entities[i].kind == ENEMY))
            entities[i].state.gone = 1;
    entities[boss].state.gone = 1;
    Entity &e = entities[target];
    e.state.x = 500;
    e.state.y = 100;
    e.state.vx = e.state.vy = e.state.aux = 0;
    e.state.hp = 0;
    State &bean = entities[playerIndex].state;
    bean.x = 400;
    bean.y = 400;
    bean.vx = bean.vy = 0;
    if (floor) {
        current.geometryCount = 0;
        rebuildPhysics();
        b2BodyDef bd;
        bd.position.Set(500 * metres, (100 + e.hit.b - .5f - 10) * metres);
        b2Body *ground = physicsWorld->CreateBody(&bd);
        b2PolygonShape shape;
        shape.SetAsBox(200 * metres, 10 * metres);
        ground->CreateFixture(&shape, 0);
    } else
        restorePhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    return target;
}

void jumperGravityNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/jumper-gravity-saves");
    CreateDirectoryA(saveRoot, 0);
    bool ground = true, sleep = true, velocity = true, position = true, incoming = true,
         spring = true;
    char rows[5000] =
        "class,fps,launchVy1000,referenceVy1000,launchY1000,referenceY1000,springIncoming1000,referenceIncoming1000\r\n";
    for (int category = 5; category <= 6; category++)
        for (int rate = 0; rate < 3; rate++) {
            float dt = rate == 0 ? 1 / 30.0f : rate == 1 ? 1 / 120.0f : 1 / 240.0f;
            int target = prepareJumperGravityNative(category, true);
            Entity &e = entities[target];
            sleep &= !bodies[target]->IsSleepingAllowed();
            for (int n = 0; n < 120 && !jumperNativeGrounded(target); n++)
                tick(dt, false, false);
            ground &= jumperNativeGrounded(target);
            const Props &jump = *e.source->behavior("EnemyJumperBehaviour");
            e.state.aux = jump.num("jump-delay", 1) + .1f;
            b2World *reference = new b2World(b2Vec2(0, nativeWorldGravity * metres), true);
            b2Body *oracle = createPhysicsEntity(*reference, e, target);
            oracle->SetGravityScale(1);
            float launch = root(-2 * nativeWorldGravity * jump.num("jump-height", 60));
            oracle->SetLinearVelocity(
                b2Vec2(-jump.num("jump-x-velocity", 100) * metres, launch * metres));
            reference->Step(dt, 10, 10);
            tick(dt, false, false);
            float actualVy = e.state.vy, referenceVy = oracle->GetLinearVelocity().y / metres,
                  actualY = e.state.y, referenceY = oracle->GetPosition().y / metres;
            velocity &= e.state.hp == 1 && absf(actualVy - referenceVy) < .001f;
            position &= absf(actualY - referenceY) < .001f;
            delete reference;
            target = prepareJumperGravityNative(category, false);
            Entity &falling = entities[target];
            int pad = -1;
            for (int i = 0; i < entityCount; i++)
                if (entities[i].kind == SPRING && boxGroupMatrixOwner(entities[i])) {
                    Rect r = shifted(boxGroupMatrixFixture(entities[i]), entities[i].state.x,
                                     entities[i].state.y);
                    if (r.l < 4256 && r.r > 4256 && r.t > 63 && r.t < 65) {
                        pad = i;
                        break;
                    }
                }
            if (pad < 0) {
                incoming = spring = false;
                continue;
            }
            falling.state.x = 4256;
            falling.state.y = 64 - falling.hit.b + .5f;
            falling.state.vx = falling.state.vy = falling.state.aux = 0;
            restorePhysics();
            bodies[playerIndex]->SetType(b2_staticBody);
            reference = new b2World(b2Vec2(0, nativeWorldGravity * metres), true);
            oracle = createPhysicsEntity(*reference, falling, target);
            oracle->SetGravityScale(1);
            createPhysicsEntity(*reference, entities[pad], pad);
            WalkerGravityReferenceNative *ref = new WalkerGravityReferenceNative;
            ref->recorded.body = oracle;
            reference->SetContactListener(ref);
            WalkerGravityProbeNative *actual = new WalkerGravityProbeNative;
            actual->recorded.body = bodies[target];
            physicsWorld->SetContactListener(actual);
            reference->Step(dt, 10, 10);
            tick(dt, false, false);
            bool seen = actual->recorded.seen && ref->recorded.seen;
            incoming &= seen && absf(actual->recorded.first - ref->recorded.first) < .001f;
            spring &= seen && ref->recorded.first == 0 && falling.state.vy < 300;
            char row[256];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d\r\n", category,
                      rate == 0   ? 30
                      : rate == 1 ? 120
                                  : 240,
                      (int)(actualVy * 1000), (int)(referenceVy * 1000), (int)(actualY * 1000),
                      (int)(referenceY * 1000), (int)(actual->recorded.first * 1000),
                      (int)(ref->recorded.first * 1000));
            lstrcatA(rows, row);
            physicsWorld->SetContactListener(originalContacts);
            delete actual;
            delete reference;
            delete ref;
        }
    check(
        ground,
        "Both actual Wizard jumper classes acquire real ground contacts before their old-clock launch at30/120/240Hz");
    check(sleep,
          "Native EnemyJumper OnActivate disables body sleeping for every actual jumper fixture");
    check(
        velocity,
        "An actual jumper launch receives the current native gravity step after setting its authored launch velocity");
    check(
        position,
        "The first real jumper launch pose matches the independent native-gravity world at all three rates");
    check(
        incoming,
        "Jumper Spring PreSolve sees the native incoming velocity before current-step gravity integration");
    check(spring, "Zero incoming jumper contact never gains a false first-step spring launch");
    int target = prepareJumperGravityNative(6, false);
    Entity &actor = entities[target];
    actor.state.x = -1000;
    actor.state.y = 1000;
    actor.state.vy = -30;
    restorePhysics();
    bodies[target]->SetGravityScale(0);
    bodies[playerIndex]->SetType(b2_staticBody);
    tick(1 / 120.0f, false, false);
    check(
        absf(actor.state.vy + 30) < .001f,
        "An explicitly disabled jumper gravity flag is respected instead of being replaced by fallback acceleration");
    captureCheckpoint(false);
    char currentPath[MAX_PATH];
    savePath(currentPath, "current-jumper-off.dat");
    atomicWrite(currentPath, &checkpoint, sizeof(checkpoint));
    bodies[target]->SetGravityScale(1);
    restoreCheckpoint();
    bodies[playerIndex]->SetType(b2_staticBody);
    bool currentOff = checkpoint.version == 28 && bodies[target]->GetGravityScale() == 0;
    tick(1 / 120.0f, false, false);
    check(
        currentOff && absf(actor.state.vy + 30) < .001f,
        "Version28 restores an explicitly disabled jumper gravity flag and exact unaccelerated next-step velocity");
    captureCheckpoint(false);
    SnapshotV25 *old = (SnapshotV25 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV25));
    memcpy(old, &checkpoint, sizeof(*old));
    old->version = 25;
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    char path[MAX_PATH];
    savePath(path, "legacy-manual-jumper-v25.dat");
    bool read = atomicWrite(path, old, sizeof(*old)) && readCheckpoint(path);
    if (read)
        restoreCheckpoint();
    bodies[playerIndex]->SetType(b2_staticBody);
    bool migrated = read && checkpoint.version == 25 && sizeof(*old) == 543392 &&
                    bodies[target]->GetGravityScale() == 1;
    float oldVy = actor.state.vy;
    tick(1 / 120.0f, false, false);
    check(
        migrated && absf(actor.state.vy - (oldVy + nativeWorldGravity / 120)) < .001f,
        "Exact543392byte version25 files migrate the old manual-jumper gravity marker without changing saved pose or native layout");
    target = prepareWalkPermissionNative(2, 0);
    Entity &walker = entities[target];
    walker.state.x = -1000;
    walker.state.y = 1000;
    walker.state.vx = 0;
    walker.state.vy = -30;
    walker.walkCanWalk = 0;
    restorePhysics();
    bodies[target]->SetGravityScale(0);
    captureCheckpoint(false);
    memcpy(old, &checkpoint, sizeof(*old));
    old->version = 25;
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    savePath(path, "legacy-native-walker-v25.dat");
    read = atomicWrite(path, old, sizeof(*old)) && readCheckpoint(path);
    if (read)
        restoreCheckpoint();
    bodies[playerIndex]->SetType(b2_staticBody);
    bool walkerOff = read && bodies[target]->GetGravityScale() == 0;
    tick(1 / 120.0f, false, false);
    check(
        walkerOff && absf(walker.state.vy + 30) < .001f,
        "Version25 walker gravity already has native semantics and is not overwritten by the jumper-only migration");
    HeapFree(GetProcessHeap(), 0, old);
    atomicWrite("test-output/jumper-gravity-observations.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/jumper-gravity-results.txt", testLog, lstrlenA(testLog));
    lstrcpyA(saveRoot, oldRoot);
    startStage(0);
    readyTimer = 0;
}

bool jumperGravityDiskReloadNative() {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    lstrcpyA(saveRoot, "test-output/jumper-gravity-saves");
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    char path[MAX_PATH];
    savePath(path, "current-jumper-off.dat");
    bool read = readCheckpoint(path);
    if (read)
        restoreCheckpoint();
    int target = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].bossSpawn == 6) {
            target = i;
            break;
        }
    bool ok = read && checkpoint.version == 28 && target >= 0 &&
              bodies[target]->GetGravityScale() == 0 && !bodies[target]->IsSleepingAllowed();
    tick(1 / 120.0f, false, false);
    ok &= target >= 0 && absf(entities[target].state.vy + 30) < .001f;
    const char *result = ok ? "PASS fresh-process jumper gravity flag disk restoration\r\n"
                            : "FAIL fresh-process jumper gravity flag disk restoration\r\n";
    atomicWrite("test-output/jumper-gravity-fresh-process.txt", result, lstrlenA(result));
    return ok;
}
