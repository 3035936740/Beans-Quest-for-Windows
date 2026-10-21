#pragma once

// Regression coverage for force area native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void physicsCheckpointForceNativeChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    startStage(stageById(905968796386ULL));
    readyTimer = 0;
    int field = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->id == 606585976342ULL)
            field = i;
    if (field < 0) {
        check(false, "Actual uncapped force field is present for checkpoint continuation");
        return;
    }
    // Isolate the original uncapped900 field and Tom. Preserve their actual
    // authored configurations, while removing terrain and unrelated actors.
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != field && i != playerIndex)
            entities[i].state.gone = 1;
    Vec c = center(entityRect(entities[field]));
    State &bean = entities[playerIndex].state;
    bean.x = c.x;
    bean.y = c.y;
    bean.vx = bean.vy = 0;
    rebuildPhysics();
    const float dt = 1.0f / 120;
    tick(dt, false, false);
    captureCheckpoint(false);
    simulateBodies(dt);
    float continued = bean.vy;
    restoreCheckpoint();
    simulateBodies(dt);
    float restored = bean.vy;
    char row[160];
    wsprintfA(row, "continued_vy_milli,restored_vy_milli\r\n%d,%d\r\n", (int)(continued * 1000),
              (int)(restored * 1000));
    atomicWrite("test-output/physics-force-checkpoint.csv", row, lstrlenA(row));
    check(
        absf(continued - restored) < .001f,
        "Actual uncapped force-field checkpoint retains the queued acceleration on its first resumed Box2D step");
    char path[MAX_PATH];
    savePath(path, "physics-force-v20.dat");
    bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    physicsWorld->ClearForces();
    memset(&checkpoint, 0, sizeof(checkpoint));
    bool read = written && readCheckpoint(path);
    restoreCheckpoint();
    simulateBodies(dt);
    check(
        read && checkpoint.version == 28 && validSnapshot(checkpoint) &&
            absf(bean.vy - continued) < .001f,
        "Version28 disk reload retains queued field acceleration after clearing every live body force and checkpoint cache");
    SnapshotV19 *legacy = (SnapshotV19 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV19));
    memcpy(legacy, &checkpoint, sizeof(*legacy) - 4);
    legacy->version = 19;
    legacy->checksum = hashBytes(legacy, sizeof(*legacy) - 4);
    savePath(path, "physics-force-legacy-v19.dat");
    written = atomicWrite(path, legacy, sizeof(*legacy));
    HeapFree(GetProcessHeap(), 0, legacy);
    read = written && readCheckpoint(path);
    bool clean = read && checkpoint.version == 19 && validSnapshot(checkpoint) &&
                 sizeof(SnapshotV19) == 401536 && sizeof(SnapshotV21) == 486016 &&
                 sizeof(SnapshotV22) == 510592 && sizeof(SnapshotV23) == 510624 &&
                 sizeof(Snapshot) == 543392;
    for (int i = 0; i < NATIVE_PHYSICS_CHECKPOINT_BODIES; i++)
        clean &= checkpoint.physics[i].valid == 0;
    restoreCheckpoint();
    check(
        clean && bodies[playerIndex]->GetCheckpointForce().LengthSquared() == 0,
        "An actual401536-byte version19 checkpoint migrates without changing its legacy state and defaults unavailable pending-force records to zero");
    captureCheckpoint(false);
    u32 nan = 0x7fc00000;
    memcpy(&checkpoint.physics[playerIndex].force.x, &nan, 4);
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    bool rejected = !validSnapshot(checkpoint);
    checkpoint.physics[playerIndex].force.x = 0;
    checkpoint.physics[playerIndex].valid = 2;
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    rejected &= !validSnapshot(checkpoint);
    check(
        rejected,
        "Checkpoint validation rejects NaN force and invalid body-record flags even with a matching checksum");

    startStage(stageById(13151310487044325505ULL));
    readyTimer = 0;
    int box = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->klass == 2913132245228912724ULL) {
            box = i;
            break;
        }
    bool crate = false;
    if (box >= 0) {
        current.geometryCount = 0;
        for (int i = 0; i < entityCount; i++)
            if (i != playerIndex && i != box)
                entities[i].state.gone = 1;
        State &s = entities[box].state;
        s.x = s.y = 500;
        s.vx = s.vy = s.aux = 0;
        entities[playerIndex].state.x = 200;
        entities[playerIndex].state.y = 800;
        rebuildPhysics();
        b2Body *b = bodies[box];
        b->SetGravityScale(0);
        b->ApplyForce(b2Vec2(.3f, -.2f), b->GetWorldCenter() + b2Vec2(.1f, .2f));
        b->ApplyTorque(.04f);
        captureCheckpoint(false);
        simulateBodies(dt);
        b2Vec2 v = b->GetLinearVelocity();
        float omega = b->GetAngularVelocity();
        restoreCheckpoint();
        simulateBodies(dt);
        crate = b->GetGravityScale() == 0 && absf(b->GetLinearVelocity().x - v.x) < .000001f &&
                absf(b->GetLinearVelocity().y - v.y) < .000001f &&
                absf(b->GetAngularVelocity() - omega) < .000001f && omega != 0;
    }
    check(
        crate,
        "Real loose-crate checkpoint preserves pending off-centre force, torque and gravity flag with matching next-step linear and angular velocities");

    startStage(stageById(789619714186ULL));
    readyTimer = 0;
    bool fired = false;
    for (int i = 0; i < entityCount; i++) {
        if (!fired)
            if (const Props *p = entities[i].source->behavior("BulletSource")) {
                shoot(entities[i], *p);
                fired = true;
            }
        if (i != playerIndex)
            entities[i].state.gone = 1;
    }
    current.geometryCount = 0;
    bullets[0].state.x = 1000;
    bullets[0].state.y = 1000;
    restorePhysics();
    bool shot = false;
    if (fired && shotBodies[0]) {
        shotBodies[0]->ApplyForce(b2Vec2(.02f, .04f), shotBodies[0]->GetWorldCenter());
        captureCheckpoint(false);
        simulateBodies(dt);
        b2Vec2 v = shotBodies[0]->GetLinearVelocity();
        restoreCheckpoint();
        simulateBodies(dt);
        shot = absf(v.x - shotBodies[0]->GetLinearVelocity().x) < .000001f &&
               absf(v.y - shotBodies[0]->GetLinearVelocity().y) < .000001f;
    }
    check(
        shot,
        "An actual launcher projectile restores pending force through its separate saved body slot and reproduces the next-step velocity");
    startStage(0);
    readyTimer = 0;
}

void forceAreaNativeChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    const float dt = 1.0f / 120.0f;
    check(
        absf(nativeForceAreaCappedAxis(-800, 700, 288, dt) - (-800 + 700 * dt)) < 0.001f &&
            nativeForceAreaCappedAxis(800, 700, 288, dt) == 800,
        "Original positive force cap preserves fast downward inertia and existing above-cap ascent instead of symmetric velocity clamping");
    check(
        nativeForceAreaCappedAxis(-800, 700, -288, dt) == -800 &&
            absf(nativeForceAreaCappedAxis(800, 700, -288, dt) - (800 - 700 * dt)) < 0.001f &&
            nativeForceAreaCappedAxis(287, 700, 288, dt) == 288 &&
            nativeForceAreaCappedAxis(-287, 700, -288, dt) == -288,
        "Native negative cap reverses target direction and both cap signs stop acceleration precisely at their configured endpoint");
    check(
        nativeForceAreaCappedAxis(4, 700, 0, dt) == 0 &&
            nativeForceAreaCappedAxis(-4, 700, 0, dt) == 0 &&
            nativeForceAreaCappedAxis(0, -700, 288, dt) < 0,
        "Native zero-cap and signed force branches retain the actual target-velocity instructions");
    Pair pairs[] = {{(char *)"sw-force-x", (char *)"-40"},  {(char *)"sw-force-y", (char *)"100"},
                    {(char *)"se-force-x", (char *)"80"},   {(char *)"se-force-y", (char *)"200"},
                    {(char *)"nw-force-x", (char *)"120"},  {(char *)"nw-force-y", (char *)"300"},
                    {(char *)"ne-force-x", (char *)"-160"}, {(char *)"ne-force-y", (char *)"400"}};
    Props synthetic = {8, pairs};
    Rect bounds = {10, 20, 110, 220};
    Vec points[] = {{10, 20}, {110, 20}, {10, 220}, {110, 220}, {60, 120}, {-30, 500}};
    Vec expected[] = {{-40, 100}, {80, 200}, {120, 300}, {-160, 400}, {0, 250}, {120, 300}};
    bool interpolation = true;
    for (int i = 0; i < 6; i++) {
        Vec f = nativeForceAreaResolve(synthetic, bounds, points[i]);
        interpolation &= absf(f.x - expected[i].x) < 0.001f && absf(f.y - expected[i].y) < 0.001f;
    }
    check(
        interpolation,
        "Native force interpolation preserves all four signed XY corners, midpoint weighting and clamped out-of-box coordinates");
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    int total = 0, scenes = 0, capped = 0, gradient = 0;
    bool shape = true, massInvariant = true, corners = true;
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scene, memory);
        bool found = false;
        for (int i = 0; i < scene.objectCount; i++) {
            Object &o = scene.objects[i];
            const Props *p = o.behavior("ForceAreaBehaviour");
            if (!p)
                continue;
            found = true;
            total++;
            capped += p->flag("use-velocity-cap-y");
            gradient += p->num("sw-force-y") != p->num("nw-force-y");
            Entity area;
            initializeEntity(area, o, o.rect);
            b2World *world = new b2World(b2Vec2(0, 0), true);
            b2Body *field = createNativeForceAreaBody(*world, area, 0);
            b2Fixture *fixture = field->GetFixtureList();
            const b2PolygonShape *polygon = (const b2PolygonShape *)fixture->GetShape();
            shape &=
                field->GetType() == b2_kinematicBody && fixture->IsSensor() &&
                fixture->GetFilterData().categoryBits == 4 &&
                fixture->GetFilterData().maskBits == 0xffff && fixture->GetFriction() == 0.2f &&
                fixture->GetDensity() == 0 &&
                absf(polygon->m_vertices[2].x / metres - (o.rect.r - o.rect.l) * 0.5f) < 0.001f;
            Vec c = center(o.rect);
            Vec force = nativeForceAreaResolve(*p, o.rect, c);
            b2Vec2 results[2];
            for (int density = 0; density < 2; density++) {
                b2BodyDef bd;
                bd.type = b2_dynamicBody;
                bd.position.Set(c.x * metres, c.y * metres);
                b2Body *body = world->CreateBody(&bd);
                b2PolygonShape box;
                box.SetAsBox(4 * metres, 4 * metres);
                body->CreateFixture(&box, density ? 12.0f : 1.0f);
                nativeForceAreaApply(area, *body, dt);
                world->Step(dt, 10, 10);
                results[density] = body->GetLinearVelocity();
                world->DestroyBody(body);
                world->ClearForces();
            }
            massInvariant &= absf(results[0].x - results[1].x) < 0.00001f &&
                             absf(results[0].y - results[1].y) < 0.00001f &&
                             absf(results[0].y / metres - force.y * dt) < 0.001f;
            Vec sw = nativeForceAreaResolve(*p, o.rect, Vec{o.rect.l, o.rect.b}),
                ne = nativeForceAreaResolve(*p, o.rect, Vec{o.rect.r, o.rect.t});
            corners &= sw.x == p->num("sw-force-x") && sw.y == p->num("sw-force-y") &&
                       ne.x == p->num("ne-force-x") && ne.y == p->num("ne-force-y");
            delete world;
        }
        if (found)
            scenes++;
    }
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        total == 15 && scenes == 6 && capped == 11 && gradient == 1 && shape && corners,
        "All74 original scenes resolve15 force fields across6 scenes, including all11 capped fields and the actual1200to700gradient with original full-box sensors");
    check(
        massInvariant,
        "All15 authored force configurations accelerate actual Box2D bodies independently of mass through native capped velocity or uncapped mass-scaled force branches");
    // Actual third force field in displayed4-3:760 upward and independent
    // Tom gravity-400. Preserve the expected deeper high-fall stopping path.
    float depth[2] = {}, predicted[2] = {};
    bool recovered = true;
    char trace[1800] =
        "drop_height,initial_vy,minimum_foot,penetration,source_continuous_prediction\r\n";
    for (int pass = 0; pass < 2; pass++) {
        startStage(stageById(177640755988ULL));
        readyTimer = 0;
        int field = -1;
        for (int i = 0; i < entityCount; i++) {
            if (entities[i].source->id == 191706825706ULL)
                field = i;
            if (entities[i].kind == ENEMY)
                entities[i].state.gone = 1;
        }
        if (field < 0) {
            recovered = false;
            continue;
        }
        Entity &a = entities[field];
        Rect r = entityRect(a);
        Entity &bean = entities[playerIndex];
        float height = pass ? 128.0f : 32.0f;
        float gravity = bean.physics->num("gravity", -400), speed = root(-2 * gravity * height),
              net = a.source->behavior("ForceAreaBehaviour")->num("sw-force-y") + gravity;
        predicted[pass] = speed * speed / (2 * net);
        bean.state.x = (r.l + r.r) * 0.5f;
        bean.state.y = r.t - bean.hit.b + 0.1f;
        bean.state.vx = 0;
        bean.state.vy = -speed;
        restorePhysics();
        float minimum = r.t;
        bool rose = false;
        for (int step = 0; step < 300; step++) {
            tick(dt, false, false);
            minimum = minf(minimum, hitRect(entities[playerIndex]).b);
            if (entities[playerIndex].state.vy >= 0) {
                rose = true;
                break;
            }
            if (deathTimer > 0)
                break;
        }
        depth[pass] = r.t - minimum;
        float tolerance = 4 * speed * dt + 3;
        recovered &= rose && absf(depth[pass] - predicted[pass]) < tolerance;
        char row[200];
        wsprintfA(row, "%d,%d,%d,%d,%d\r\n", (int)height, (int)(-speed * 1000),
                  (int)(minimum * 1000), (int)(depth[pass] * 1000), (int)(predicted[pass] * 1000));
        lstrcatA(trace, row);
    }
    atomicWrite("test-output/force-area-drop-native.csv", trace, lstrlenA(trace));
    check(
        recovered && depth[1] > depth[0] + 80,
        "Actual4-3field reverses low and high falls under original760lift and400gravity, preserving deeper high-drop penetration without invented water damping");
    startStage(stageById(177640755988ULL));
    readyTimer = 0;
    int memberField = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->id == 191706825706ULL)
            memberField = i;
    bool exactSet = false;
    if (memberField >= 0) {
        Entity &a = entities[memberField];
        Entity &bean = entities[playerIndex];
        Rect r = entityRect(a);
        bean.state.x = (r.l + r.r) * 0.5f;
        bean.state.y = (r.b + r.t) * 0.5f;
        bean.state.vx = bean.state.vy = 0;
        restorePhysics();
        b2PolygonShape extra;
        extra.SetAsBox(3 * metres, 3 * metres);
        b2FixtureDef detector;
        detector.shape = &extra;
        detector.isSensor = true;
        b2Fixture *temporary = bodies[playerIndex]->CreateFixture(&detector);
        simulateBodies(dt);
        NativeForceAreaMembers *members = nativeForceAreaSet(memberField, false);
        bool entered = members && members->members[playerIndex];
        bodies[playerIndex]->DestroyFixture(temporary);
        exactSet = entered && members && !members->members[playerIndex] &&
                   sensorBodyContact(bodies[memberField], bodies[playerIndex]);
    }
    check(
        exactSet,
        "Actual force sensor matches the original set semantics: ending one of two fixture contacts erases the body even while its other fixture still touches");
    startStage(0);
    readyTimer = 0;
}
