#pragma once

// Regression coverage for pinball fixture native. Runs only in the isolated BQ_ENABLE_TESTS executable.

bool pinballFixtureMatchesNative(Entity &e, b2Body *body) {
    b2Fixture *f = body ? body->GetFixtureList() : 0;
    if (!f || f->GetShape()->GetType() != b2Shape::e_circle)
        return false;
    const b2CircleShape &shape = *(const b2CircleShape *)f->GetShape();
    return body->GetType() == b2_staticBody &&
           e.physics == e.source->behavior("PinballBouncerBehaviour") &&
           absf(shape.m_radius / metres - 32) < .0001f && shape.m_p.x == 0 && shape.m_p.y == 0 &&
           f->GetDensity() == 0 && f->GetFriction() == 0 && f->GetRestitution() == 1 &&
           !f->IsSensor() && f->GetFilterData().categoryBits == 4 &&
           f->GetFilterData().maskBits == 0xffff;
}

int preparePinballFixtureNative(float edge) {
    int pin = preparePinballDetailNative(0);
    if (pin < 0)
        return pin;
    Entity &p = entities[playerIndex];
    Entity &b = entities[pin];
    b.state.x = b.originX = 256;
    b.state.y = b.originY = 128;
    p.state.x = b.state.x - edge - p.hit.r;
    p.state.y = b.state.y - (p.hit.b + p.hit.t) * .5f;
    p.state.vx = p.state.vy = 0;
    p.state.timer = .2f;
    restorePhysics();
    stopSounds();
    return pin;
}

class PinballFixtureSolverNative : public b2ContactListener {
  public:
    int solved;
    float restitution;

    PinballFixtureSolverNative() : solved(0), restitution(-1) {}

    void PostSolve(b2Contact *c, const b2ContactImpulse *) {
        solved++;
        restitution = c->GetRestitution();
    }
};

void pinballFixtureNativeChecks() {
    interfaceEnabled = debugMode = musicEnabled = false;
    soundEnabled = true;
    stopMusic();
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    int scenes = 0, pins = 0;
    bool shapes = true;
    b2World *world = new b2World(b2Vec2(0, 0), true);
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scene, memory);
        scenes++;
        for (int i = 0; i < scene.objectCount; i++) {
            Object &o = scene.objects[i];
            if (!o.has("PinballBouncerBehaviour"))
                continue;
            Entity e;
            initializeEntity(e, o, o.rect);
            b2Body *body = createPhysicsEntity(*world, e, 0);
            pins++;
            shapes &= pinballFixtureMatchesNative(e, body);
            if (body)
                world->DestroyBody(body);
        }
    }
    delete world;
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        scenes == 74 && pins == 19 && shapes,
        "All19authored Pinballs use their native fixed32pixel centred circle and static zero-density friction0 restitution1 solid fixture, rather than sprite-derived radius34");
    Object variant = *classById(11607860035091918041ULL);
    variant.rect = Rect{-24, -48, 24, 48};
    Pair properties[] = {
        {(char *)"adjust-left", (char *)"100"},   {(char *)"adjust-right", (char *)"-100"},
        {(char *)"density", (char *)"9"},         {(char *)"restitution", (char *)"0.25"},
        {(char *)"friction", (char *)"0.75"},     {(char *)"sensor", (char *)"1"},
        {(char *)"static-position", (char *)"0"}, {(char *)"collide-player", (char *)"0"},
        {(char *)"fixed-rotation", (char *)"1"}};
    Behavior behavior = {0, (char *)"PinballBouncerBehaviour", Props{9, properties}};
    variant.behaviorCount = 1;
    variant.behaviors = &behavior;
    Entity custom;
    initializeEntity(custom, variant, variant.rect);
    world = new b2World(b2Vec2(0, 0), true);
    b2Body *body = createPhysicsEntity(*world, custom, 0);
    check(
        pinballFixtureMatchesNative(custom, body) && body->IsFixedRotation(),
        "Controlled nonsquare Pinball keeps hardcoded native circle/material/filter despite generic fixture overrides or inverted adjusted bounds, while its Circle body definition honors explicit fixed rotation");
    delete world;
    int pin = preparePinballFixtureNative(33.5f);
    bool ring = false;
    if (pin >= 0) {
        bool visual = overlaps(hitRect(entities[playerIndex]), hitRect(entities[pin]));
        tick(1 / 240.0f, false, false);
        ring = visual && !pinballPendingNative.pending && pinballDetailVoiceCount("pinball") == 0 &&
               jumps == 0;
    }
    check(
        ring,
        "Actual Bean inside the sprite-only outer ring cannot queue a Pinball launch or sound before reaching the native32pixel physical radius");
    pin = preparePinballFixtureNative(31.75f);
    bool inner = false;
    PinballPendingNative request = {};
    if (pin >= 0) {
        tick(1 / 240.0f, false, false);
        request = pinballPendingNative;
        bool queued = request.pending && request.source == pin && request.normal.x < -.99f &&
                      pinballDetailVoiceCount("pinball") > 0 && jumps == 0;
        tick(1 / 240.0f, false, false);
        inner = queued && pinballLaunchedThisStepNative && pinballConsumedNative.source == pin &&
                jumps == 1 && entities[playerIndex].state.vx < -300 &&
                pinballDetailVoiceCount("jump") == 0;
    }
    check(
        inner,
        "Actual contact inside the native radius still queues and consumes one next-step side MegaJump with the original dedicated sound and jump count");
    bool bounce = true;
    char rows[1200] = "fps,outgoingVy1000,mixedRestitution1000,solved\r\n";
    for (int rate = 0; rate < 3; rate++) {
        int fps = rate == 0 ? 30 : rate == 1 ? 120 : 240;
        world = new b2World(b2Vec2(0, 0), true);
        PinballFixtureSolverNative observer;
        world->SetContactListener(&observer);
        Object &pad = *classById(11607860035091918041ULL);
        Entity e;
        initializeEntity(e, pad, pad.rect);
        e.state.x = 256;
        e.state.y = 128;
        body = createPhysicsEntity(*world, e, 0);
        Object &source = *classById(14441582509727370349ULL);
        Entity ball;
        initializeEntity(ball, source, source.rect);
        ball.state.x = 256;
        ball.state.y = 160 - ball.hit.b - .1f;
        ball.state.vx = 0;
        ball.state.vy = -200;
        b2Body *falling = createPhysicsEntity(*world, ball, 1);
        falling->SetGravityScale(0);
        world->Step(1.0f / fps, 10, 10);
        float vy = falling->GetLinearVelocity().y / metres;
        bounce &= observer.solved > 0 && absf(vy - 200) < .01f && observer.restitution == 1;
        char row[180];
        wsprintfA(row, "%d,%d,%d,%d\r\n", fps, (int)(vy * 1000), (int)(observer.restitution * 1000),
                  observer.solved);
        lstrcatA(rows, row);
        delete world;
    }
    check(
        bounce,
        "Actual original beach-ball and Pinball fixtures produce the native restitution1 solver rebound from minus200 to plus200px/s at30/120/240Hz before the separate post-physics power callback");
    pin = preparePinballFixtureNative(31.75f);
    bool disk = false;
    if (pin >= 0) {
        tick(1 / 240.0f, false, false);
        request = pinballPendingNative;
        captureCheckpoint(false);
        char path[MAX_PATH];
        savePath(path, "pinball-fixture-checkpoint.dat");
        disk = request.pending && atomicWrite(path, &checkpoint, sizeof(checkpoint));
        rebuildPhysics();
        disk &= readCheckpoint(path);
        if (disk)
            restoreCheckpoint();
        disk &= pinballFixtureMatchesNative(entities[pin], bodies[pin]) &&
                pinballPendingNative.pending && pinballPendingNative.normal.x == request.normal.x &&
                checkpoint.version == 28 && sizeof(Snapshot) == 543392;
        tick(1 / 240.0f, false, false);
        disk &= pinballLaunchedThisStepNative && jumps == 1;
    }
    check(
        disk,
        "Private disk reload plus world reconstruction retains native Pinball geometry/material and the genuine queued MegaJump without changing version28 checkpoint bytes");
    navigate(137502285691ULL);
    int credits = 0;
    bool ending = true;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].kind == BOUNCER) {
            credits++;
            ending &= pinballFixtureMatchesNative(page.playfield[i], page.bodies[i]);
        }
    check(
        credits == 2 && ending,
        "Both actual ending-scene Pinballs use the shared native32pixel geometry and dedicated material in their independent live physics world");
    Object &beach = *classById(14441582509727370349ULL);
    Entity unchanged;
    initializeEntity(unchanged, beach, beach.rect);
    world = new b2World(b2Vec2(0, 0), true);
    body = createPhysicsEntity(*world, unchanged, 0);
    b2Fixture *ordinary = body ? body->GetFixtureList() : 0;
    bool control = circleFixtureGeometryMatchesNative(body, unchanged.hit) && ordinary &&
                   absf(ordinary->GetDensity() - .5f) < .0001f &&
                   absf(ordinary->GetRestitution() - .96f) < .0001f &&
                   absf(ordinary->GetFriction() - .2f) < .0001f;
    delete world;
    check(
        control,
        "The original beach-ball Circle retains authored width-derived geometry and density0.5/restitution0.96/friction0.2 instead of receiving Pinball's specialized constants");
    atomicWrite("test-output/pinball-fixture-solver.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/pinball-fixture-results.txt", testLog, lstrlenA(testLog));
    soundEnabled = false;
    stopSounds();
    startStage(0);
    readyTimer = 0;
}
