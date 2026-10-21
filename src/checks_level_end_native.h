#pragma once

// Regression coverage for level end native. Runs only in the isolated BQ_ENABLE_TESTS executable.

class LevelEndObserverNative : public OriginalContacts {
  public:
    int target, contacts, beanContacts;
    float normalY;
    bool disable;

    LevelEndObserverNative()
        : target(-1), contacts(0), beanContacts(0), normalY(-2), disable(false) {}

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        OriginalContacts::PreSolve(c, old);
        int a = (int)c->GetFixtureA()->GetBody()->GetUserData() - 1,
            b = (int)c->GetFixtureB()->GetBody()->GetUserData() - 1;
        if (disable && (a == target || b == target))
            c->SetEnabled(false);
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        OriginalContacts::PostSolve(c, impulse);
        int a = (int)c->GetFixtureA()->GetBody()->GetUserData() - 1,
            b = (int)c->GetFixtureB()->GetBody()->GetUserData() - 1;
        if (a != target && b != target)
            return;
        contacts++;
        if (a != playerIndex && b != playerIndex)
            return;
        b2WorldManifold m;
        c->GetWorldManifold(&m);
        beanContacts++;
        normalY = maxf(normalY, a == target ? m.normal.y : -m.normal.y);
    }
};

int prepareLevelEndNative() {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    startStage(0);
    readyTimer = 0;
    current.geometryCount = 0;
    int target = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == EXIT) {
            target = i;
            break;
        }
    if (target < 0)
        return -1;
    for (int i = 0; i < entityCount; i++)
        if (i != target && i != playerIndex)
            entities[i].state.gone = 1;
    entities[target].state.x = entities[target].originX = 256;
    entities[target].state.y = entities[target].originY = 128;
    entities[target].state.vx = entities[target].state.vy = 0;
    entities[playerIndex].state.x = 1000;
    entities[playerIndex].state.y = 700;
    entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
    rebuildPhysics();
    return target;
}

void placeLevelEndBeanNative(int target, int face, float vy) {
    Entity &e = entities[target];
    Entity &bean = entities[playerIndex];
    Rect r = hitRect(e);
    bean.state.vx = 0;
    bean.state.vy = vy;
    bean.state.timer = .2f;
    bean.state.x = e.state.x;
    bean.state.y = r.t - bean.hit.b - .25f;
    if (face == 1) {
        bean.state.x = r.r - bean.hit.l - .25f;
        bean.state.y = r.t - bean.hit.b - 4;
    }
    if (face == 2)
        bean.state.y = r.b - bean.hit.t + .25f;
    restorePhysics();
}

bool levelEndBodyNative(Entity &e, b2Body *body) {
    b2Fixture *f = body ? body->GetFixtureList() : 0;
    return e.physics == e.source->behavior("LevelEndBehaviour") && body &&
           body->GetType() == b2_kinematicBody && !body->IsFixedRotation() && f && !f->IsSensor() &&
           f->GetFilterData().categoryBits == 4 && f->GetFilterData().maskBits == 0xffff &&
           circleFixtureDefaultMaterialNative(body);
}

void levelEndNativeChecks() {
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    bool bodiesMatch = true, geometry = true;
    int scenes = 0, goals = 0;
    b2World *world = new b2World(b2Vec2(0, 0), true);
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scene, memory);
        scenes++;
        for (int i = 0; i < scene.objectCount; i++) {
            Object &source = scene.objects[i];
            if (!source.has("LevelEndBehaviour"))
                continue;
            goals++;
            Entity e;
            initializeEntity(e, source, source.rect);
            b2Body *body = createPhysicsEntity(*world, e, 0);
            bodiesMatch &= levelEndBodyNative(e, body);
            Vec c = center(source.rect);
            Rect expected = shifted(source.rect, -c.x, -c.y);
            expected.t -= 16;
            geometry &= body && absf(e.hit.l - expected.l) < .001f &&
                        absf(e.hit.r - expected.r) < .001f && absf(e.hit.b - expected.b) < .001f &&
                        absf(e.hit.t - expected.t) < .001f;
            if (body) {
                b2PolygonShape &shape = *(b2PolygonShape *)body->GetFixtureList()->GetShape();
                Rect physical = {10000, 10000, -10000, -10000};
                for (int k = 0; k < shape.m_vertexCount; k++) {
                    Vec p = {shape.m_vertices[k].x / metres, shape.m_vertices[k].y / metres};
                    physical.l = minf(physical.l, p.x);
                    physical.r = maxf(physical.r, p.x);
                    physical.b = minf(physical.b, p.y);
                    physical.t = maxf(physical.t, p.y);
                }
                geometry &= absf(physical.l - expected.l) < .001f &&
                            absf(physical.r - expected.r) < .001f &&
                            absf(physical.b - expected.b) < .001f &&
                            absf(physical.t - expected.t) < .001f;
                world->DestroyBody(body);
            }
        }
    }
    delete world;
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        scenes == 74 && goals == 49 && bodiesMatch,
        "All49authored LevelEnds across74scenes bind their inherited configuration and create native kinematic Box fixtures with2.5/.5/.2 material and unchanged filters");
    check(
        goals == 49 && geometry,
        "All49actual LevelEnd polygon fixtures retain their original sixteen-pixel top adjustment exactly once");
    Object variant = *classById(11867101362052293448ULL);
    Pair changes[] = {
        {(char *)"adjust-left", (char *)"5"},   {(char *)"adjust-right", (char *)"-3"},
        {(char *)"adjust-bottom", (char *)"2"}, {(char *)"adjust-top", (char *)"-10"},
        {(char *)"density", (char *)"4"},       {(char *)"restitution", (char *)"0.25"},
        {(char *)"friction", (char *)"0.1"},    {(char *)"fixed-rotation", (char *)"1"}};
    Behavior behavior = {0, (char *)"LevelEndBehaviour", Props{8, changes}};
    variant.behaviorCount = 1;
    variant.behaviors = &behavior;
    Entity custom;
    initializeEntity(custom, variant, variant.rect);
    Vec c = center(variant.rect);
    Rect base = shifted(variant.rect, -c.x, -c.y);
    world = new b2World(b2Vec2(0, 0), true);
    b2Body *b = createPhysicsEntity(*world, custom, 0);
    b2Fixture *f = b ? b->GetFixtureList() : 0;
    check(
        f && b->GetType() == b2_kinematicBody && b->IsFixedRotation() &&
            absf(custom.hit.l - base.l - 5) < .001f && absf(custom.hit.r - base.r + 3) < .001f &&
            absf(custom.hit.b - base.b - 2) < .001f && absf(custom.hit.t - base.t + 10) < .001f &&
            absf(f->GetDensity() - 4) < .001f && absf(f->GetRestitution() - .25f) < .001f &&
            absf(f->GetFriction() - .1f) < .001f,
        "Controlled original-style LevelEnd configuration honors all four adjustments once and explicit material/fixed-rotation overrides");
    delete world;
    bool landing = true;
    char rows[3000] = "case,fps,solved,beanSolved,normalY1000,complete,jumps\r\n";
    for (int rate = 0; rate < 3; rate++) {
        int fps = rate == 0 ? 30 : rate == 1 ? 120 : 240, target = prepareLevelEndNative();
        if (target < 0) {
            landing = false;
            continue;
        }
        placeLevelEndBeanNative(target, 0, -100);
        LevelEndObserverNative observer;
        observer.target = target;
        physicsWorld->SetContactListener(&observer);
        for (int frame = 0; frame < 8 && !complete; frame++)
            tick(1.0f / fps, false, false);
        landing &= observer.beanContacts > 0 && observer.normalY >= .5f && complete &&
                   goalLaunch.active && !bodies[target]->IsActive() &&
                   !bodies[playerIndex]->IsActive();
        physicsWorld->SetContactListener(originalContacts);
        char row[160];
        wsprintfA(row, "top,%d,%d,%d,%d,%d,%d\r\n", fps, observer.contacts, observer.beanContacts,
                  (int)(observer.normalY * 1000), complete, jumps);
        lstrcatA(rows, row);
    }
    check(
        landing,
        "Real solved top landings on the original GOAL finish once and deactivate both bodies at30/120/240Hz");
    bool results[5] = {};
    for (int mode = 0; mode < 5; mode++) {
        int target = prepareLevelEndNative();
        if (target < 0)
            continue;
        placeLevelEndBeanNative(target,
                                mode == 1   ? 1
                                : mode == 2 ? 2
                                            : 0,
                                mode == 0   ? 30
                                : mode == 2 ? 100
                                            : -30);
        if (mode == 3)
            bodies[target]->GetFixtureList()->SetSensor(true);
        LevelEndObserverNative observer;
        observer.target = target;
        observer.disable = mode == 4;
        physicsWorld->SetContactListener(&observer);
        tick(1 / 120.0f, false, false);
        results[mode] =
            mode == 0   ? observer.beanContacts > 0 && observer.normalY >= .5f && complete
            : mode == 1 ? observer.beanContacts > 0 && absf(observer.normalY) < .01f && !complete
            : mode == 2 ? observer.beanContacts > 0 && observer.normalY < -.5f && !complete
                        : observer.beanContacts == 0 && !complete;
        physicsWorld->SetContactListener(originalContacts);
        char row[160];
        wsprintfA(row, "%d,120,%d,%d,%d,%d,%d\r\n", mode, observer.contacts, observer.beanContacts,
                  (int)(observer.normalY * 1000), complete, jumps);
        lstrcatA(rows, row);
    }
    check(
        results[0],
        "An actual upward-settling top PostSolve finishes the GOAL independently of Tom's descending-only ordinary-jump latch");
    check(
        results[1],
        "A real near-top side collision cannot finish the level merely because Bean overlaps the old eight-pixel AABB fallback");
    check(
        results[2],
        "An actual underside PostSolve keeps the level active rather than treating every GOAL contact as completion");
    check(
        results[3],
        "A controlled sensor-only GOAL overlap cannot complete without a solved physical contact");
    check(
        results[4],
        "A controlled disabled physical contact cannot complete through an overlapping-bounds fallback");
    int target = prepareLevelEndNative();
    bool other = false;
    if (target >= 0) {
        int index = entityCount++;
        Object &source = *classById(14441582509727370349ULL);
        initializeEntity(entities[index], source, source.rect);
        Entity &ball = entities[index];
        ball.state.x = entities[target].state.x;
        ball.state.y = hitRect(entities[target]).t - ball.hit.b - .25f;
        ball.state.vx = 0;
        ball.state.vy = -100;
        rebuildPhysics();
        LevelEndObserverNative observer;
        observer.target = target;
        physicsWorld->SetContactListener(&observer);
        tick(1 / 120.0f, false, false);
        other = observer.contacts > 0 && observer.beanContacts == 0 && !complete;
        physicsWorld->SetContactListener(originalContacts);
    }
    check(
        other,
        "A real original beach-ball fixture landing on the GOAL cannot substitute for the native Tom type check");
    target = prepareLevelEndNative();
    bool disk = false;
    if (target >= 0) {
        captureCheckpoint(false);
        char path[MAX_PATH];
        savePath(path, "level-end-checkpoint.dat");
        disk = atomicWrite(path, &checkpoint, sizeof(checkpoint));
        rebuildPhysics();
        disk &= readCheckpoint(path);
        if (disk)
            restoreCheckpoint();
        disk &= levelEndBodyNative(entities[target], bodies[target]) && checkpoint.version == 28 &&
                sizeof(Snapshot) == 543392;
        placeLevelEndBeanNative(target, 0, -100);
        tick(1 / 120.0f, false, false);
        disk &= complete;
    }
    check(
        disk,
        "Private disk checkpoint and world reconstruction retain native GOAL configuration and real top-contact completion without changing version28 layout");
    target = prepareLevelEndNative();
    bool ui = false;
    if (target >= 0) {
        UiScene sample = {};
        sample.memory.init(1024 * 1024);
        sample.playfield = sample.memory.array<Entity>(2);
        sample.playfieldCount = 1;
        sample.hasBean = true;
        sample.playfield[0] = entities[target];
        sample.bean = entities[playerIndex];
        createUiPhysics(sample);
        ui = levelEndBodyNative(sample.playfield[0], sample.bodies[0]);
        destroyUiPhysics(sample);
        VirtualFree(sample.memory.data, 0, MEM_RELEASE);
    }
    check(
        ui,
        "The independent UI factory retains native LevelEnd body/material in a controlled class placement, without claiming an authored selector GOAL");
    atomicWrite("test-output/level-end-contact.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/level-end-results.txt", testLog, lstrlenA(testLog));
    startStage(0);
    readyTimer = 0;
}
