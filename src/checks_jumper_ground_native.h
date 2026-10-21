#pragma once

// Regression coverage for jumper ground native. Runs only in the isolated BQ_ENABLE_TESTS executable.

// Independent callback observer models the native fixture set without using
// the production set, including disabled contacts which have not ended.
class JumperGroundNativeObserver : public OriginalContacts {
  public:
    int actorIndex[2], count[2], ends;
    b2Fixture *fixtures[2][128];

    JumperGroundNativeObserver(int left, int right) {
        actorIndex[0] = left;
        actorIndex[1] = right;
        memset(count, 0, sizeof(count));
        memset(fixtures, 0, sizeof(fixtures));
        ends = 0;
    }

    void EndContact(b2Contact *c) {
        for (int n = 0; n < 2; n++) {
            b2Fixture *other = 0;
            if (c->GetFixtureA()->GetBody() == bodies[actorIndex[n]])
                other = c->GetFixtureB();
            else if (c->GetFixtureB()->GetBody() == bodies[actorIndex[n]])
                other = c->GetFixtureA();
            if (!other)
                continue;
            for (int i = 0; i < count[n]; i++)
                if (fixtures[n][i] == other) {
                    fixtures[n][i] = fixtures[n][--count[n]];
                    ends++;
                    break;
                }
        }
        OriginalContacts::EndContact(c);
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        for (int n = 0; n < 2; n++) {
            b2Body *own = bodies[actorIndex[n]];
            b2Fixture *other = 0;
            if (c->GetFixtureA()->GetBody() == own)
                other = c->GetFixtureB();
            else if (c->GetFixtureB()->GetBody() == own)
                other = c->GetFixtureA();
            if (!other)
                continue;
            b2WorldManifold m;
            c->GetWorldManifold(&m);
            if ((c->GetFixtureA()->GetBody() == own ? -m.normal.y : m.normal.y) <= .95f)
                continue;
            bool found = false;
            for (int i = 0; i < count[n]; i++)
                if (fixtures[n][i] == other)
                    found = true;
            if (!found && count[n] < 128)
                fixtures[n][count[n]++] = other;
        }
        OriginalContacts::PostSolve(c, impulse);
    }
};

void jumperGroundNativePrepare() {
    startStage(stageById(5770908237787719ULL));
    readyTimer = 0;
    FrogLink &link = frogLinks[0];
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == ENEMY && i != link.left && i != link.right)
            entities[i].state.gone = 1;
    State &p = entities[playerIndex].state;
    p.x = 128;
    p.y = 350;
    p.vx = p.vy = 0;
    restorePhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    for (b2Fixture *f = bodies[playerIndex]->GetFixtureList(); f; f = f->GetNext()) {
        b2Filter filter = f->GetFilterData();
        filter.maskBits = 0;
        f->SetFilterData(filter);
    }
}

bool jumperGroundNativeWet(const Entity &e) {
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == WATER) {
            Rect r = entityRect(entities[i]);
            if (e.state.x > r.l && e.state.x < r.r && hitRect(e).b < r.t)
                return true;
        }
    return false;
}

u32 jumperGroundNativePixels(Entity &e) {
    glViewport(viewportX, viewportY, viewportW, viewportH);
    nativeViewportProjection();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();
    glTranslatef(128 - e.state.x, 128 - e.state.y, 0);
    drawEntity(e, &current, 0, 0, 0);
    glPopMatrix();
    glFinish();
    float scale = viewportW / viewW;
    int width = (int)(64 * scale), size = width * width * 4;
    u8 *pixels = (u8 *)HeapAlloc(GetProcessHeap(), 0, size);
    glReadBuffer(GL_BACK);
    glReadPixels(viewportX + (int)(96 * scale), viewportY + (int)(96 * scale), width, width,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    u32 hash = hashBytes(pixels, size);
    HeapFree(GetProcessHeap(), 0, pixels);
    return hash;
}

void jumperGroundNativeChecks() {
    char oldSaveRoot[MAX_PATH];
    lstrcpyA(oldSaveRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/jumper-ground-native-saves");
    CreateDirectoryA(saveRoot, 0);
    Progress oldProgress = progress;
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldMusic = musicEnabled,
         oldSound = soundEnabled;
    u32 incomingSeed = effectSeed;
    interfaceEnabled = false;
    debugMode = true;
    musicEnabled = soundEnabled = false;
    const float dt = 1.0f / 120;
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    int jumpers = 0;
    bool clocks = true;
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scene, memory);
        for (int i = 0; i < scene.objectCount; i++) {
            Object &o = scene.objects[i];
            const Props *config = o.behavior("EnemyJumperBehaviour");
            if (!config)
                continue;
            jumpers++;
            Entity e;
            initializeEntity(e, o, o.rect);
            float delay = config->num("jump-delay", 1);
            e.state.hp = 1;
            e.state.aux = delay;
            e.state.vy = 100;
            jumperNativeStepState(e, dt, delay, false, true, e.state.x - 1);
            clocks &=
                e.state.hp == 0 && e.state.vy == 100 && absf(e.state.aux - (delay + dt)) < .00001f;
            jumperNativeStepState(e, dt, delay, false, true, e.state.x - 1);
            clocks &= e.state.hp == 1 && e.state.vy > 0 && e.state.aux > delay + dt;
            float launchVy = e.state.vy;
            jumperNativeStepState(e, dt, delay, false, false, e.state.x - 1);
            clocks &= e.state.hp == 1 && e.state.aux == 0 && e.state.vy == launchVy;
            e.state.aux = delay;
            jumperNativeStepState(e, dt, delay, true, false, e.state.x);
            clocks &= absf(e.state.aux - dt) < .00001f && e.state.hp == 1;
        }
    }
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        levelCount == 74 && jumpers == 31 && clocks,
        "All31 original jumpers use the native strict old-clock boundary, retain launch age, wait despite upward pushes and wrap only special airborne clocks");
    bool sceneFound = true, contactsMatch = true, sourceMatches = true, caseStanding = false,
         gpuAnimated = false, caseRises = false;
    int cases = 0, sourceStanding = 0, sourceJumping = 0;
    char trace[12000] =
        "case,side,offset,dy,impact,final_hp,wet,source_clock_match,upward_standing\r\n";
    // Original right/left frog, both displacement signs, and both low/high
    // pressure. An independent source oracle checks every real physics tick;
    // underwater jumping is permitted when no source standing event occurred.
    const int sides[] = {0, 1, 0, 1}, offsets[] = {-36, -36, -24, 36}, heights[] = {0, 15, 15, 0};
    for (int scenario = 0; scenario < 4; scenario++) {
        effectSeed = 0x12345678;
        jumperGroundNativePrepare();
        if (frogLinkCount != 1) {
            sceneFound = false;
            continue;
        }
        FrogLink &link = frogLinks[0];
        int target = sides[scenario] ? link.left : link.right, which = sides[scenario] ? 0 : 1;
        Entity &e = entities[target];
        Entity &ball = entities[link.ball];
        JumperGroundNativeObserver *observer =
            new JumperGroundNativeObserver(link.left, link.right);
        physicsWorld->SetContactListener(observer);
        b2BodyDef floorDef;
        floorDef.position.Set(1500 * metres, 290 * metres);
        b2Body *floor = physicsWorld->CreateBody(&floorDef);
        b2PolygonShape shape;
        shape.SetAsBox(100 * metres, 10 * metres);
        floor->CreateFixture(&shape, 0);
        ball.state.x = 1500;
        ball.state.y = 330;
        ball.state.vx = ball.state.vy = 0;
        bodies[link.ball]->SetTransform(b2Vec2(1500 * metres, 330 * metres), 0);
        bodies[link.ball]->SetLinearVelocity(b2Vec2(0, 0));
        simulateBodies(dt);
        frogPostStep();
        sceneFound &= ball.state.hp == 0;
        physicsWorld->DestroyBody(floor);
        killEnemy(entities[sides[scenario] ? link.right : link.left]);
        int impact = -1;
        float previousVy = e.state.vy;
        bool upwardStanding = false, localSource = true;
        for (int step = 0; step < 1800; step++) {
            State before = e.state;
            bool grounded = observer->count[which] > 0;
            contactsMatch &= jumperNativeGrounded(target) == grounded;
            float delay = link.period;
            int expectedHp = grounded ? (before.aux > delay ? 1 : 0) : before.hp;
            float expectedAge = grounded ? before.aux + dt : 0;
            tick(dt, false, false);
            localSource &= e.state.hp == expectedHp && absf(e.state.aux - expectedAge) < .00002f;
            if (before.hp && e.state.hp == 0 && before.vy > 10)
                upwardStanding = true;
            contactsMatch &= jumperNativeGrounded(target) == (observer->count[which] > 0);
            if (impact < 0 && e.state.vy > 100 && previousVy <= 100) {
                impact = step;
                ball.state.x = e.state.x + offsets[scenario];
                ball.state.y = e.state.y + heights[scenario];
                ball.state.vx = offsets[scenario] < 0 ? 100 : -100;
                ball.state.vy = heights[scenario] == 0    ? -60
                                : heights[scenario] == 15 ? -130
                                                          : -200;
                bodies[link.ball]->SetTransform(
                    b2Vec2(ball.state.x * metres, ball.state.y * metres),
                    bodies[link.ball]->GetAngle());
                bodies[link.ball]->SetLinearVelocity(
                    b2Vec2(ball.state.vx * metres, ball.state.vy * metres));
                bodies[link.ball]->SetAwake(true);
            }
            previousVy = e.state.vy;
        }
        sourceMatches &= localSource;
        cases++;
        if (jumperGroundNativeWet(e) && !e.state.hp)
            sourceStanding++;
        if (jumperGroundNativeWet(e) && e.state.hp)
            sourceJumping++;
        if (scenario == 0) {
            caseStanding =
                impact > 0 && upwardStanding && jumperGroundNativeWet(e) && e.state.hp == 0;
            caseRises = impact > 0;
            float age = e.jumperAnimation.standingAge;
            e.jumperAnimation.standingAge = 0;
            u32 first = jumperGroundNativePixels(e);
            e.jumperAnimation.standingAge = .125f;
            u32 second = jumperGroundNativePixels(e);
            gpuAnimated = first != second;
            e.jumperAnimation.standingAge = age;
        }
        char row[240];
        wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", scenario, sides[scenario],
                  offsets[scenario], heights[scenario], impact, e.state.hp,
                  jumperGroundNativeWet(e), localSource, upwardStanding);
        lstrcatA(trace, row);
        physicsWorld->SetContactListener(originalContacts);
        delete observer;
    }
    atomicWrite("test-output/jumper-ground-native.csv", trace, lstrlenA(trace));
    check(
        sceneFound && cases == 4 && contactsMatch && sourceMatches,
        "Real mirrored ball-pressure routes match an independent native fixture-set and old-clock oracle on all7200 physical ticks");
    check(
        caseRises && caseStanding && gpuAnimated,
        "The original right frog really jumps, is pushed by its original ball and later floats on an animated standing pose after the native upward-contact waiting event");
    check(
        sourceStanding > 0 && sourceJumping > 0,
        "Pressure regressions retain both source outcomes: some water entries stand while entries lacking the source standing event keep jumping");
    jumperGroundNativePrepare();
    FrogLink &link = frogLinks[0];
    int target = link.right;
    for (int i = 0; i < 30; i++)
        tick(dt, false, false);
    bool supported = jumperNativeGrounded(target);
    captureCheckpoint(false);
    float savedPhase = entities[target].state.aux;
    entities[target].state.x = 1408;
    entities[target].state.y = 300;
    entities[target].state.vx = entities[target].state.vy = 0;
    simulateBodies(dt);
    bool ended = !jumperNativeGrounded(target);
    restoreCheckpoint();
    bool cleared = !jumperNativeGrounded(target) && entities[target].state.aux == savedPhase;
    tick(dt, false, false);
    bool rebuilt = jumperNativeGrounded(target);
    rebuildPhysics();
    bool reset = !jumperNativeGrounded(target);
    char lifecycle[240];
    wsprintfA(lifecycle, "supported=%d ended=%d cleared=%d rebuilt=%d reset=%d\r\n", supported,
              ended, cleared, rebuilt, reset);
    atomicWrite("test-output/jumper-ground-native-lifecycle.txt", lifecycle, lstrlenA(lifecycle));
    check(
        supported && ended && cleared && rebuilt && reset,
        "Real EndContact removes native support, checkpoint reload rebuilds transient contacts without saving fixture pointers, and world rebuild clears the set");
    startStage(0);
    readyTimer = 0;
    lstrcpyA(saveRoot, oldSaveRoot);
    progress = oldProgress;
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    musicEnabled = oldMusic;
    soundEnabled = oldSound;
    effectSeed = incomingSeed;
}
