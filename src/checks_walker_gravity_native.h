#pragma once

// Regression coverage for walker gravity native. Runs only in the isolated BQ_ENABLE_TESTS executable.

// A separate Box2D world implements the native BoxPhysics body definition:
// world gravity is -500, is-gravitated enables body gravity, and PreSolve
// observes the velocity before this step's force/gravity integration.
struct WalkerGravityRecorderNative {
    b2Body *body;
    bool seen;
    float first;

    WalkerGravityRecorderNative() : body(0), seen(false), first(0) {}

    void pre(b2Contact *c) {
        if (seen || !c->GetManifold()->pointCount || c->GetFixtureA()->IsSensor() ||
            c->GetFixtureB()->IsSensor())
            return;
        if (c->GetFixtureA()->GetBody() == body || c->GetFixtureB()->GetBody() == body) {
            seen = true;
            first = body->GetLinearVelocity().y / metres;
        }
    }
};

class WalkerGravityReferenceNative : public b2ContactListener {
  public:
    WalkerGravityRecorderNative recorded;

    void PreSolve(b2Contact *c, const b2Manifold *) {
        recorded.pre(c);
    }
};

class WalkerGravityProbeNative : public OriginalContacts {
  public:
    WalkerGravityRecorderNative recorded;

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        recorded.pre(c);
        OriginalContacts::PreSolve(c, old);
    }
};

void walkerGravityNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/walker-gravity-saves");
    CreateDirectoryA(saveRoot, 0);
    bool contacts = true, phase = true, launch = true, ui = true, disabled = true;
    char rows[4096] =
        "class,fps,actualSeen,referenceSeen,actualIncoming1000,referenceIncoming1000,actualFinal1000,referenceFinal1000\r\n";
    interfaceEnabled = true;
    musicEnabled = soundEnabled = false;
    navigate(137502285691ULL);
    b2Vec2 actualUiGravity = page.physics->GetGravity();
    check(
        absf(actualUiGravity.y / metres - nativeWorldGravity) < .001f,
        "The actual credits/selector factory creates native world gravity rather than compensating for manual integration with a zero-gravity world");
    interfaceEnabled = false;
    for (int category = 2; category <= 8; category += 6)
        for (int rate = 0; rate < 3; rate++) {
            float dt = rate == 0 ? 1 / 30.0f : rate == 1 ? 1 / 120.0f : 1 / 240.0f;
            int target = prepareWalkPermissionNative(category, 0), spring = -1;
            for (int i = 0; i < entityCount; i++)
                if (entities[i].kind == SPRING && boxGroupMatrixOwner(entities[i])) {
                    Rect r = shifted(boxGroupMatrixFixture(entities[i]), entities[i].state.x,
                                     entities[i].state.y);
                    if (r.l < 4256 && r.r > 4256 && r.t > 63 && r.t < 65) {
                        spring = i;
                        break;
                    }
                }
            if (spring < 0) {
                contacts = false;
                continue;
            }
            Entity &e = entities[target];
            e.state.x = 4256;
            e.state.y = 64 - e.hit.b + .5f;
            e.state.vx = e.state.vy = 0;
            e.walkCanWalk = 0;
            restorePhysics();
            bodies[playerIndex]->SetType(b2_staticBody);
            b2World *reference = new b2World(b2Vec2(0, nativeWorldGravity * metres), true);
            b2Body *oracle = createPhysicsEntity(*reference, e, target);
            oracle->SetGravityScale(1);
            createPhysicsEntity(*reference, entities[spring], spring);
            WalkerGravityReferenceNative *ref = new WalkerGravityReferenceNative;
            ref->recorded.body = oracle;
            reference->SetContactListener(ref);
            WalkerGravityProbeNative *actual = new WalkerGravityProbeNative;
            actual->recorded.body = bodies[target];
            physicsWorld->SetContactListener(actual);
            reference->Step(dt, 10, 10);
            tick(dt, false, false);
            bool saw = actual->recorded.seen && ref->recorded.seen;
            contacts &= saw;
            phase &= saw && absf(actual->recorded.first - ref->recorded.first) < .001f;
            launch &= saw && ref->recorded.first == 0 && e.state.vy < 300;
            char row[256];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d\r\n", category,
                      rate == 0   ? 30
                      : rate == 1 ? 120
                                  : 240,
                      actual->recorded.seen, ref->recorded.seen,
                      (int)(actual->recorded.first * 1000), (int)(ref->recorded.first * 1000),
                      (int)(e.state.vy * 1000),
                      (int)(oracle->GetLinearVelocity().y / metres * 1000));
            lstrcatA(rows, row);
            physicsWorld->SetContactListener(originalContacts);
            delete actual;
            delete reference;
            delete ref;
        }
    check(
        contacts,
        "Both actual Wizard walker classes reach the real spring fixture and independent native-gravity reference contacts at30/120/240Hz");
    check(
        phase,
        "Walker Spring PreSolve incoming velocity matches native body-gravity timing before the current solver step");
    check(
        launch,
        "A newly touching walker with zero native incoming fall speed does not receive a premature spring launch");
    startStage(6);
    readyTimer = 0;
    int box = -1;
    for (int i = 0; i < entityCount; i++)
        if (nativeLooseBoxGravity(entities[i])) {
            box = i;
            break;
        }
    if (box < 0)
        ui = false;
    if (box >= 0) {
        Entity copy = entities[box];
        copy.state.x = 500;
        copy.state.y = 400;
        copy.state.vx = 0;
        copy.state.vy = -30;
        UiScene sample = {};
        sample.playfield = &copy;
        sample.playfieldCount = 1;
        b2Body *localBodies[2] = {};
        sample.bodies = localBodies;
        sample.physics = new b2World(actualUiGravity, true);
        localBodies[0] = createPhysicsEntity(*sample.physics, copy, 0);
        float dt = 1 / 120.0f;
        bool nativeGravity = localBodies[0]->GetGravityScale() == 1;
        tickUiPlayfield(sample, dt, false);
        bool before = copy.state.vy == -30;
        localBodies[0]->SetLinearVelocity(b2Vec2(0, copy.state.vy * metres));
        sample.physics->Step(dt, 10, 10);
        float final = localBodies[0]->GetLinearVelocity().y / metres;
        ui = nativeGravity && before && absf(final - (-30 + nativeWorldGravity * dt)) < .001f;
        char detail[256];
        wsprintfA(
            detail,
            "nativeBodyGravity=%d preStepStateVy1000=%d finalBodyVy1000=%d expectedVy1000=%d\r\n",
            nativeGravity, (int)(copy.state.vy * 1000), (int)(final * 1000),
            (int)((-30 + nativeWorldGravity * dt) * 1000));
        atomicWrite("test-output/walker-gravity-ui-observation.txt", detail, lstrlenA(detail));
        delete sample.physics;
    }
    check(
        ui,
        "An authored dynamic crate in an independent UI world receives native body gravity once without a second playfield velocity increment");
    bool crateOff = box >= 0;
    if (box >= 0) {
        Entity &crate = entities[box];
        crate.state.x = -1000;
        crate.state.y = 1000;
        crate.state.vx = 0;
        crate.state.vy = -30;
        restorePhysics();
        bodies[box]->SetGravityScale(0);
        bodies[playerIndex]->SetType(b2_staticBody);
        tick(1 / 120.0f, false, false);
        crateOff = absf(crate.state.vy + 30) < .001f;
    }
    check(
        crateOff,
        "A campaign loose crate with explicitly disabled native body gravity is not accelerated by the fallback integrator");
    int target = prepareWalkPermissionNative(2, 0);
    Entity &actor = entities[target];
    actor.state.x = -1000;
    actor.state.y = 1000;
    actor.state.vx = 0;
    actor.state.vy = -30;
    actor.walkCanWalk = 0;
    restorePhysics();
    bodies[target]->SetGravityScale(0);
    bodies[playerIndex]->SetType(b2_staticBody);
    tick(1 / 120.0f, false, false);
    disabled = absf(actor.state.vy + 30) < .001f;
    char detail[160];
    wsprintfA(detail, "gravityScale1000=%d expectedVy1000=-30000 actualVy1000=%d\r\n",
              (int)(bodies[target]->GetGravityScale() * 1000), (int)(actor.state.vy * 1000));
    atomicWrite("test-output/walker-gravity-disabled-observation.txt", detail, lstrlenA(detail));
    check(
        disabled,
        "Disabling native walking-body gravity is respected instead of silently restoring manual gravity");
    captureCheckpoint(false);
    bodies[target]->SetGravityScale(1);
    restoreCheckpoint();
    bodies[playerIndex]->SetType(b2_staticBody);
    bool savedOff = checkpoint.version == 28 && bodies[target]->GetGravityScale() == 0;
    tick(1 / 120.0f, false, false);
    check(
        savedOff && absf(actor.state.vy + 30) < .001f,
        "Version28 preserves an explicitly disabled walker gravity flag and its actual next-frame velocity after reload");
    captureCheckpoint(false);
    SnapshotV24 *old = (SnapshotV24 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV24));
    memcpy(old, &checkpoint, sizeof(*old));
    old->version = 24;
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    char path[MAX_PATH];
    savePath(path, "legacy-manual-walker-v24.dat");
    bool loaded = atomicWrite(path, old, sizeof(*old)) && readCheckpoint(path);
    if (loaded)
        restoreCheckpoint();
    bodies[playerIndex]->SetType(b2_staticBody);
    bool migrated = loaded && checkpoint.version == 24 && sizeof(*old) == 543392 &&
                    sizeof(Snapshot) == 543392 && bodies[target]->GetGravityScale() == 1;
    float oldVy = actor.state.vy;
    tick(1 / 120.0f, false, false);
    check(
        migrated && absf(actor.state.vy - (oldVy + nativeWorldGravity / 120)) < .001f,
        "Exact version24 files upgrade their old manual-walker gravity marker from authored configuration while keeping the same543392byte layout");
    old->checksum ^= 1;
    savePath(path, "corrupt-manual-walker-v24.dat");
    check(
        atomicWrite(path, old, sizeof(*old)) && !readCheckpoint(path),
        "A corrupt version24 checksum is still rejected before applying the walker-gravity migration");
    HeapFree(GetProcessHeap(), 0, old);
    atomicWrite("test-output/walker-gravity-observations.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/walker-gravity-results.txt", testLog, lstrlenA(testLog));
    lstrcpyA(saveRoot, oldRoot);
    startStage(0);
    readyTimer = 0;
}
