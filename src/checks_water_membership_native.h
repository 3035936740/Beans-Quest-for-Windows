#pragma once

// Regression coverage for water membership native. Runs only in the isolated BQ_ENABLE_TESTS executable.

struct WaterMembershipReferenceNative {
    b2Body *water, *other;
    bool member;
    int begins, ends;

    WaterMembershipReferenceNative(b2Body *w, b2Body *o)
        : water(w), other(o), member(false), begins(0), ends(0) {}

    void contact(b2Contact *c, bool begin) {
        b2Body *a = c->GetFixtureA()->GetBody();
        b2Body *b = c->GetFixtureB()->GetBody();
        if (!((a == water && b == other) || (a == other && b == water)))
            return;
        member = begin;
        if (begin)
            begins++;
        else
            ends++;
    }
};

class WaterStageReferenceNative : public OriginalContacts {
  public:
    WaterMembershipReferenceNative reference;

    WaterStageReferenceNative(b2Body *w, b2Body *o) : reference(w, o) {}

    void BeginContact(b2Contact *c) {
        reference.contact(c, true);
        OriginalContacts::BeginContact(c);
    }

    void EndContact(b2Contact *c) {
        reference.contact(c, false);
        OriginalContacts::EndContact(c);
    }
};

class WaterUiReferenceNative : public UiContacts {
  public:
    WaterMembershipReferenceNative reference;

    WaterUiReferenceNative(UiScene &s) : UiContacts(s), reference(s.bodies[0], s.bodies[1]) {}

    void BeginContact(b2Contact *c) {
        reference.contact(c, true);
        UiContacts::BeginContact(c);
    }

    void EndContact(b2Contact *c) {
        reference.contact(c, false);
        UiContacts::EndContact(c);
    }
};

int waterMembershipTouchingNative(b2Body *water, b2Body *other) {
    int count = 0;
    for (b2ContactEdge *edge = water->GetContactList(); edge; edge = edge->next)
        if (edge->other == other && edge->contact->IsTouching())
            count++;
    return count;
}

void waterMembershipNativeChecks() {
    interfaceEnabled = musicEnabled = soundEnabled = debugMode = false;
    const float dt = 1 / 120.0f;
    startStage(stageById(176011402805ULL));
    readyTimer = 0;
    int wet = -1, walker = -1;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].kind == WATER && wet < 0)
            wet = i;
        if (entities[i].source->klass == 14264481342587988014ULL)
            walker = i;
    }
    if (wet < 0 || walker < 0) {
        check(false, "Original Water Expansion supplies a real water pool and blue beetle");
        return;
    }
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && i != wet && i != walker)
            entities[i].state.gone = 1;
    Rect pool = entityRect(entities[wet]);
    entities[playerIndex].state.x = pool.r + 500;
    entities[playerIndex].state.y = pool.t + 500;
    State &state = entities[walker].state;
    state.x = pool.r - 40;
    state.y = pool.t - 24;
    state.vx = state.vy = state.aux = 0;
    rebuildPhysics();
    b2Body *body = bodies[walker];
    body->SetGravityScale(0);
    WaterStageReferenceNative observer(bodies[wet], body);
    physicsWorld->SetContactListener(&observer);
    bool stage = true, readback = false, fresh = false;
    char rows[2048] = "world,phase,member,begins,ends,touching,forceMilli\r\n";
    for (int phase = 0; phase < 3; phase++) {
        state.x = phase == 1 ? pool.r - 10 : pool.r - 40;
        state.vx = state.vy = 0;
        body->SetTransform(b2Vec2(state.x * metres, state.y * metres), 0);
        body->SetLinearVelocity(b2Vec2(0, 0));
        body->SetAwake(true);
        physicsWorld->ClearForces();
        physicsWorld->Step(dt, 10, 10);
        physicsWorld->ClearForces();
        waterPostStep();
        float force = body->GetCheckpointForce().y;
        int touching = waterMembershipTouchingNative(bodies[wet], body);
        bool expected = phase != 1;
        stage &= observer.reference.member == expected && touching > 0 &&
                 (expected ? force > 0 : force == 0);
        if (phase == 1) {
            WaterSample sample =
                sampleWater(*body, false, *entities[wet].source->behavior("WaterAreaBehaviour"),
                            pool.t * metres);
            stage &= observer.reference.ends > 0 && sample.area > 0;
            captureCheckpoint(false);
            const char *path = "test-output/water-membership-checkpoint.dat";
            bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
            bool loaded = written && readCheckpoint(path);
            if (loaded)
                restoreCheckpoint();
            physicsWorld->ClearForces();
            physicsWorld->Step(dt, 10, 10);
            physicsWorld->ClearForces();
            waterPostStep();
            readback = loaded && !observer.reference.member &&
                       waterMembershipTouchingNative(bodies[wet], body) > 0 &&
                       body->GetCheckpointForce().y == 0;
        }
        char row[160];
        wsprintfA(row, "stage,%d,%d,%d,%d,%d,%d\r\n", phase, observer.reference.member,
                  observer.reference.begins, observer.reference.ends, touching,
                  (int)(force * 1000));
        lstrcatA(rows, row);
    }
    check(
        stage,
        "Real blue beetle's five water contacts follow original unique-object Begin/End membership: one sensor exit removes buoyancy despite remaining contacts, and re-entry restores it");
    check(
        readback,
        "Same-world version28 disk restore preserves the callback-derived removed water membership while the original beetle remains partly touching");
    physicsWorld->SetContactListener(originalContacts);
    rebuildPhysics();
    body = bodies[walker];
    body->SetGravityScale(0);
    physicsWorld->Step(dt, 10, 10);
    physicsWorld->ClearForces();
    waterPostStep();
    fresh = body->GetCheckpointForce().y > 0;
    check(
        fresh,
        "A rebuilt world seeds fresh water membership from suppressed-startup BeginContact callbacks and applies the original positive buoyancy");
    b2Body *campaign = body;
    float campaignForce = body->GetCheckpointForce().y;
    navigate(799282380888ULL);
    int uiWater = -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].kind == WATER)
            uiWater = i;
    bool ui = uiWater >= 0, isolation = false;
    if (ui) {
        UiScene scene = {};
        scene.memory.init(2 * 1024 * 1024);
        scene.playfield = scene.memory.array<Entity>(3);
        scene.playfieldCount = 2;
        scene.hasBean = true;
        scene.playfield[0] = page.playfield[uiWater];
        Rect r = entityRect(scene.playfield[0]);
        Object &source = *classById(14264481342587988014ULL);
        initializeEntity(scene.playfield[1], source, shifted(source.rect, r.r - 40, r.t - 24));
        scene.bean = page.bean;
        scene.bean.state.x = r.r + 500;
        scene.bean.state.y = r.t + 500;
        scene.bean.state.vx = scene.bean.state.vy = 0;
        createUiPhysics(scene);
        delete scene.contacts;
        WaterUiReferenceNative *reference = new WaterUiReferenceNative(scene);
        scene.contacts = reference;
        scene.physics->SetContactListener(reference);
        scene.bodies[1]->SetGravityScale(0);
        for (int phase = 0; phase < 3; phase++) {
            State &s = scene.playfield[1].state;
            s.x = phase == 1 ? r.r - 10 : r.r - 40;
            s.vx = s.vy = 0;
            scene.physics->ClearForces();
            stepUiPhysics(scene, dt);
            float force = scene.bodies[1]->GetCheckpointForce().y;
            int touching = waterMembershipTouchingNative(scene.bodies[0], scene.bodies[1]);
            bool expected = phase != 1;
            ui &= reference->reference.member == expected && touching > 0 &&
                  (expected ? force > 0 : force == 0);
            char row[160];
            wsprintfA(row, "ui,%d,%d,%d,%d,%d,%d\r\n", phase, reference->reference.member,
                      reference->reference.begins, reference->reference.ends, touching,
                      (int)(force * 1000));
            lstrcatA(rows, row);
        }
        destroyUiPhysics(scene);
        VirtualFree(scene.memory.data, 0, MEM_RELEASE);
        isolation = campaign->GetCheckpointForce().y == campaignForce;
        physicsWorld->ClearForces();
        waterPostStep();
        isolation &= campaign->GetCheckpointForce().y > 0;
    }
    check(
        ui,
        "Actual selector water and original beetle fixtures use the same unique-object membership sequence and force gating in the independent UI world");
    check(
        isolation,
        "Destroying the isolated selector water world leaves the campaign membership and queued buoyancy intact");
    physicsWorld->DestroyBody(campaign);
    bodies[walker] = 0;
    entities[walker].state.x = pool.r + 500;
    entities[walker].state.y = pool.t - 24;
    entities[walker].state.vx = entities[walker].state.vy = 0;
    bodies[walker] = createPhysicsEntity(*physicsWorld, entities[walker], walker);
    bodies[walker]->SetGravityScale(0);
    physicsWorld->ClearForces();
    waterPostStep();
    check(
        bodies[walker]->GetCheckpointForce().LengthSquared() == 0,
        "Destroying a wet body clears its membership before a same-owner replacement body can inherit stale buoyancy without any new BeginContact");
    atomicWrite("test-output/water-membership-trace.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/water-membership-results.txt", testLog, lstrlenA(testLog));
    startStage(0);
    readyTimer = 0;
}
