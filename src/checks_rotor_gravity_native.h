#pragma once

// Regression coverage for rotor gravity native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int prepareRotorGravityNative(int type, bool floor) {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    startStage(type == 1 ? stageById(8458860001749117ULL) : 0);
    readyTimer = 0;
    int target = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].rotate == type) {
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

void rotorGravityNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/rotor-gravity-saves");
    CreateDirectoryA(saveRoot, 0);
    interfaceEnabled = musicEnabled = soundEnabled = false;
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    int scenes = 0, rotors = 0, seesaws = 0;
    bool valid = true, binding = true, gravity = true, materials = true, reaction = true;
    char rows[12000] =
        "scene,id,type,gravity1000,density1000,friction1000,restitution1000,pinFy1000,expectedFy1000\r\n";
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scene, memory);
        scenes++;
        for (int i = 0; i < scene.objectCount; i++) {
            Object &o = scene.objects[i];
            if (!o.has("RotatingBehaviour") && !o.has("SeesawBehaviour"))
                continue;
            Entity e;
            initializeEntity(e, o, o.rect);
            b2World *world = new b2World(b2Vec2(0, nativeWorldGravity * metres), true);
            b2RevoluteJoint *joint = 0;
            b2Body *body = createPhysicsEntity(*world, e, 0, &joint);
            if (!body || !joint) {
                valid = false;
                delete world;
                continue;
            }
            body->SetActive(true);
            b2Fixture *f = body->GetFixtureList();
            float expected = 0, force = 0;
            gravity &= body->GetGravityScale() == 1;
            if (e.rotate == 1)
                rotors++;
            else {
                seesaws++;
                const Props *source = o.behavior("SeesawBehaviour");
                binding &= e.physics == source;
                materials &= absf(f->GetDensity() - source->num("density", 2.5f)) < .001f &&
                             absf(f->GetFriction() - source->num("friction", .2f)) < .001f &&
                             absf(f->GetRestitution() - source->num("restitution", .5f)) < .001f;
                world->Step(1 / 120.0f, 10, 10);
                force = joint->GetReactionForce(120).y;
                expected = body->GetMass() * (-nativeWorldGravity) * metres;
                reaction &= absf(force - expected) < .001f;
            }
            char uid[24], row[260];
            idstr(o.id, uid);
            wsprintfA(row, "%d,%s,%d,%d,%d,%d,%d,%d,%d\r\n", level, uid, e.rotate,
                      (int)(body->GetGravityScale() * 1000), (int)(f->GetDensity() * 1000),
                      (int)(f->GetFriction() * 1000), (int)(f->GetRestitution() * 1000),
                      (int)(force * 1000), (int)(expected * 1000));
            lstrcatA(rows, row);
            delete world;
        }
    }
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        valid && scenes == 74 && rotors == 12 && seesaws == 18,
        "All74original scenes construct every12rotors and18seesaws with their actual dynamic bodies and centre pins");
    check(
        binding,
        "Every Seesaw derived BoxPhysics behaviour supplies its own serialized physics configuration");
    check(
        gravity,
        "Every authored rotor and seesaw owns native body gravity rather than a manual or missing gravity path");
    check(
        materials,
        "All18seesaws use authored density and native inherited friction0.2/restitution0.5 defaults");
    check(reaction,
          "Every isolated actual seesaw pin supplies the native mass-times-gravity support force");
    bool contacts = true;
    char contactRows[3000] = "type,fps,actualIncoming1000,referenceIncoming1000\r\n";
    for (int type = 1; type <= 2; type++)
        for (int rate = 0; rate < 3; rate++) {
            int target = prepareRotorGravityNative(type, true);
            if (target < 0) {
                contacts = false;
                continue;
            }
            Entity &e = entities[target];
            float dt = rate == 0 ? 1 / 30.0f : rate == 1 ? 1 / 120.0f : 1 / 240.0f;
            b2World *reference = new b2World(b2Vec2(0, nativeWorldGravity * metres), true);
            b2RevoluteJoint *pin = 0;
            b2Body *oracle = createPhysicsEntity(*reference, e, target, &pin);
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
            wsprintfA(row, "%d,%d,%d,%d\r\n", type,
                      rate == 0   ? 30
                      : rate == 1 ? 120
                                  : 240,
                      (int)(actual->recorded.first * 1000), (int)(ref->recorded.first * 1000));
            lstrcatA(contactRows, row);
            physicsWorld->SetContactListener(originalContacts);
            delete actual;
            delete reference;
            delete ref;
        }
    check(
        contacts,
        "Actual rotor/seesaw first PreSolve velocities match native body-gravity timing at30/120/240Hz");
    int target = prepareRotorGravityNative(1, true);
    bool off = target >= 0;
    if (off) {
        bodies[target]->SetGravityScale(0);
        captureCheckpoint(false);
        restoreCheckpoint();
        bodies[playerIndex]->SetType(b2_staticBody);
        WalkerGravityProbeNative *probe = new WalkerGravityProbeNative;
        probe->recorded.body = bodies[target];
        physicsWorld->SetContactListener(probe);
        tick(1 / 120.0f, false, false);
        off = bodies[target]->GetGravityScale() == 0 && probe->recorded.seen &&
              absf(probe->recorded.first) < .001f;
        physicsWorld->SetContactListener(originalContacts);
        delete probe;
    }
    check(
        off,
        "An explicitly disabled rotor gravity flag survives restore without a hidden manual incoming-velocity increment");
    bool migration = true, currentOff = true, legacyFlags = true, corrupt = true, ui = true;
    SnapshotV26 *old = (SnapshotV26 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV26));
    for (int type = 1; type <= 2; type++) {
        target = prepareRotorGravityNative(type, false);
        if (target < 0) {
            migration = currentOff = false;
            continue;
        }
        bodies[target]->SetGravityScale(0);
        captureCheckpoint(false);
        char path[MAX_PATH];
        savePath(path, type == 1 ? "current-rotor-off.dat" : "current-seesaw-off.dat");
        bool read = atomicWrite(path, &checkpoint, sizeof(checkpoint)) && readCheckpoint(path);
        if (read)
            restoreCheckpoint();
        currentOff &= read && checkpoint.version == 28 && bodies[target]->GetGravityScale() == 0;
        float savedX = entities[target].state.x, savedY = entities[target].state.y,
              savedAngle = entities[target].state.angle;
        memcpy(old, &checkpoint, sizeof(*old));
        old->version = 26;
        old->checksum = hashBytes(old, sizeof(*old) - 4);
        savePath(path, type == 1 ? "legacy-rotor-v26.dat" : "legacy-seesaw-v26.dat");
        read = atomicWrite(path, old, sizeof(*old)) && readCheckpoint(path);
        if (read)
            restoreCheckpoint();
        migration &= read && checkpoint.version == 26 && sizeof(*old) == 543392 &&
                     bodies[target]->GetGravityScale() == 1 && entities[target].state.x == savedX &&
                     entities[target].state.y == savedY &&
                     entities[target].state.angle == savedAngle;
        old->checksum ^= 1;
        savePath(path, "corrupt-rotor-v26.dat");
        corrupt &= atomicWrite(path, old, sizeof(*old)) && !readCheckpoint(path);
    }
    check(
        currentOff,
        "Actual version28 disk reloads preserve explicit gravity-off flags for both rotors and seesaws");
    check(
        migration && corrupt,
        "Exact543392byte version26 files migrate both old rotor gravity markers without moving saved poses and still reject corrupt files");
    for (int category = 0; category < 2; category++) {
        target =
            category ? prepareWalkPermissionNative(2, 0) : prepareJumperGravityNative(6, false);
        if (target < 0) {
            legacyFlags = false;
            continue;
        }
        bodies[target]->SetGravityScale(0);
        captureCheckpoint(false);
        memcpy(old, &checkpoint, sizeof(*old));
        old->version = 26;
        old->checksum = hashBytes(old, sizeof(*old) - 4);
        char path[MAX_PATH];
        savePath(path, "legacy-native-other-v26.dat");
        bool read = atomicWrite(path, old, sizeof(*old)) && readCheckpoint(path);
        if (read)
            restoreCheckpoint();
        legacyFlags &= read && bodies[target]->GetGravityScale() == 0;
    }
    check(
        legacyFlags,
        "Version26 native walker and jumper gravity-off flags survive the rotation-only migration unchanged");
    HeapFree(GetProcessHeap(), 0, old);
    for (int type = 1; type <= 2; type++)
        for (int rate = 0; rate < 3; rate++) {
            target = prepareRotorGravityNative(type, false);
            if (target < 0) {
                ui = false;
                continue;
            }
            Entity copy = entities[target];
            UiScene sample = {};
            sample.playfield = &copy;
            sample.playfieldCount = 1;
            b2Body *localBodies[2] = {};
            sample.bodies = localBodies;
            sample.physics = new b2World(b2Vec2(0, nativeWorldGravity * metres), true);
            b2RevoluteJoint *joint = 0;
            localBodies[0] = createPhysicsEntity(*sample.physics, copy, 0, &joint);
            float dt = rate == 0 ? 1 / 30.0f : rate == 1 ? 1 / 120.0f : 1 / 240.0f;
            tickUiPlayfield(sample, dt, false);
            ui &= copy.state.vy == 0 && localBodies[0]->GetGravityScale() == 1;
            sample.physics->Step(dt, 10, 10);
            float expected = localBodies[0]->GetMass() * (-nativeWorldGravity) * metres;
            ui &= joint && absf(joint->GetReactionForce(1 / dt).y - expected) < .001f;
            delete sample.physics;
        }
    check(
        ui,
        "Independent selector playfields keep native rotor/seesaw gravity in the solver at30/120/240Hz without manual pre-step acceleration");
    atomicWrite("test-output/rotor-gravity-observations.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/rotor-gravity-contact-observations.csv", contactRows,
                lstrlenA(contactRows));
    atomicWrite("test-output/rotor-gravity-results.txt", testLog, lstrlenA(testLog));
    lstrcpyA(saveRoot, oldRoot);
    startStage(0);
    readyTimer = 0;
}

bool rotorGravityDiskReloadNative() {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    lstrcpyA(saveRoot, "test-output/rotor-gravity-saves");
    bool ok = true;
    for (int type = 1; type <= 2; type++) {
        startStage(type == 1 ? stageById(8458860001749117ULL) : 0);
        readyTimer = 0;
        char path[MAX_PATH];
        savePath(path, type == 1 ? "current-rotor-off.dat" : "current-seesaw-off.dat");
        bool read = readCheckpoint(path);
        if (read)
            restoreCheckpoint();
        int target = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].rotate == type) {
                target = i;
                break;
            }
        ok &= read && checkpoint.version == 28 && target >= 0 &&
              bodies[target]->GetGravityScale() == 0;
    }
    const char *result =
        ok ? "PASS fresh-process rotor and seesaw gravity flag disk restoration\r\n"
           : "FAIL fresh-process rotor and seesaw gravity flag disk restoration\r\n";
    atomicWrite("test-output/rotor-gravity-fresh-process.txt", result, lstrlenA(result));
    return ok;
}
