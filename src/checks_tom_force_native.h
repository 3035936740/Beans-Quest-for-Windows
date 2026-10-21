#pragma once

// Regression coverage for tom force native. Runs only in the isolated BQ_ENABLE_TESTS executable.

struct TomForceCaseNative {
    float vy, expectedVy, y, expectedY, preVy, expectedPreVy, mass;
    bool ok;
};

char tomForceRowsNative[28000];

struct TomForceContactObserverNative : OriginalContacts {
    int source, calls;
    float firstIncoming, lastIncoming;

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        int a = (int)c->GetFixtureA()->GetBody()->GetUserData() - 1,
            b = (int)c->GetFixtureB()->GetBody()->GetUserData() - 1;
        if ((a == source && b == playerIndex) || (b == source && a == playerIndex)) {
            float v = bodies[playerIndex]->GetLinearVelocity().y / metres;
            if (!calls)
                firstIncoming = v;
            lastIncoming = v;
            calls++;
        }
        OriginalContacts::PreSolve(c, old);
    }
};

TomForceCaseNative tomForceCaseNative(int mode, float dt, float massScale = 1, int flight = 0,
                                      bool wet = false) {
    startStage(0);
    readyTimer = deathTimer = 0;
    readyOpening.showing = false;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex)
            entities[i].state.gone = 1;
    int bird = -1;
    if (flight) {
        for (int i = 0; i < classCount; i++)
            if (classes[i].has("HelperBirdBehaviour")) {
                bird = entityCount;
                addEntity(classes[i], Rect{600, 500, 632, 532});
                entities[bird].state.active = 1;
                break;
            }
    }
    Entity &bean = entities[playerIndex];
    bean.state.x = 500;
    bean.state.y = 400;
    bean.state.vx = 20;
    bean.state.vy = flight == 2 ? 60.0f : flight == 3 ? 100.0f : flight ? 0.0f : -30.0f;
    bean.state.hp = 1;
    bean.state.timer = .2f;
    jumps = 0;
    rebuildPhysics();
    b2MassData mass;
    bodies[playerIndex]->GetMassData(&mass);
    mass.mass *= massScale;
    mass.I *= massScale;
    bodies[playerIndex]->SetMassData(&mass);
    b2World *reference = new b2World(b2Vec2(0, 0), true);
    b2Body *oracle = createPhysicsEntity(*reference, bean, playerIndex);
    oracle->SetMassData(&mass);
    float vy = bean.state.vy,
          vx = approach(bean.state.vx, bean.physics->num("max-velocity", 200),
                        bean.physics->num("move-acceleration", 500) * dt),
          g = bean.physics->num("gravity", -400), flyMax = 60;
    if (bird >= 0) {
        const Props &fly = *entities[bird].source->behavior("HelperBirdBehaviour");
        flyMax = fly.num("max-fly-speed", 60);
        if (vy <= flyMax)
            vy = approach(vy, flyMax, fly.num("fly-acceleration", 1000) * dt);
    }
    if (mode == 1 || mode == 2) {
        Vec normal = mode == 1 ? Vec{0, 1} : Vec{1, 0};
        nativeMegaJumpRequest(playerIndex, normal, g, bean.physics->num("jump-height", 74), 0, 200);
        vx += pinballPendingNative.speed * normal.x;
        vy = pinballPendingNative.speed * normal.y;
    }
    if (mode == 3) {
        trampolineStateNative.pending = 1;
        trampolineStateNative.source = playerIndex;
        trampolineStateNative.apex = 600;
        trampolineStateNative.speed = root(160000);
        vy = trampolineStateNative.speed;
    }
    float expectedPre = vy;
    oracle->SetLinearVelocity(b2Vec2(vx * metres, vy * metres));
    if (bird < 0 || vy >= flyMax)
        oracle->ApplyForce(b2Vec2(0, mass.mass * g * metres), oracle->GetPosition());
    bool foundWater = !wet;
    if (wet) {
        for (int i = 0; i < classCount; i++)
            if (classes[i].has("WaterAreaBehaviour")) {
                const Props &props = *classes[i].behavior("WaterAreaBehaviour");
                WaterSample fluid =
                    sampleWater(*bodies[playerIndex], true, props, (bean.state.y + 5) * metres);
                bodies[playerIndex]->ApplyForce(fluid.buoyancy, fluid.massCentre);
                bodies[playerIndex]->ApplyForce(fluid.drag, fluid.areaCentre);
                oracle->ApplyForce(fluid.buoyancy, fluid.massCentre);
                oracle->ApplyForce(fluid.drag, fluid.areaCentre);
                foundWater = fluid.area > 0;
                break;
            }
    }
    reference->Step(dt, 10, 10);
    tick(dt, false, true);
    b2Vec2 v = oracle->GetLinearVelocity(), p = oracle->GetPosition();
    TomForceCaseNative result = {bean.state.vy, v.y / metres, bean.state.y, p.y / metres,
                                 preStepVy,     expectedPre,  mass.mass,    false};
    result.ok =
        foundWater && (!flight || bird >= 0) && absf(result.vy - result.expectedVy) < .003f &&
        absf(result.y - result.expectedY) < .003f && absf(bean.state.vx - v.x / metres) < .003f &&
        absf(bean.state.x - p.x / metres) < .003f &&
        absf(result.preVy - result.expectedPreVy) < .003f;
    char row[430];
    wsprintfA(
        row,
        "mode=%d dt_us=%d mass10000=%d flight=%d wet=%d pre100=%d nativePre100=%d vy100=%d nativeVy100=%d y10000=%d nativeY10000=%d ok=%d\r\n",
        mode, (int)(dt * 1000000), (int)(result.mass * 10000), flight, wet,
        (int)(result.preVy * 100), (int)(result.expectedPreVy * 100), (int)(result.vy * 100),
        (int)(result.expectedVy * 100), (int)(result.y * 10000), (int)(result.expectedY * 10000),
        result.ok);
    lstrcatA(tomForceRowsNative, row);
    delete reference;
    return result;
}

void tomForceNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/tom-force-saves");
    CreateDirectoryA(saveRoot, 0);
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldSound = soundEnabled,
         oldMusic = musicEnabled;
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    stopMusic();
    tomForceRowsNative[0] = 0;
    const float steps[] = {1.0f / 60, 1.0f / 120, 1.0f / 240}, masses[] = {.5f, 1, 3};
    const char *labels[] = {
        "Ordinary airborne Tom matches native force integration and pre-force incoming velocity at60/120/240Hz and three masses",
        "Vertical MegaJump preserves this step's mass-scaled gravity after selecting launch velocity at three rates and masses",
        "Sideways Pinball MegaJump still receives vertical gravity in its launch step at three rates and masses",
        "Trampoline receives native body-force gravity in its launch step without overwriting the retained height request"};
    for (int mode = 0; mode < 4; mode++) {
        bool ok = true;
        for (int dt = 0; dt < 3; dt++)
            for (int mass = 0; mass < 3; mass++)
                ok = tomForceCaseNative(mode, steps[dt], masses[mass]).ok && ok;
        check(ok, labels[mode]);
    }
    bool under = true, boundary = true, above = true, launch = true;
    for (int dt = 0; dt < 3; dt++) {
        under = tomForceCaseNative(0, steps[dt], 1, 1).ok && under;
        boundary = tomForceCaseNative(0, steps[dt], 1, 2).ok && boundary;
        above = tomForceCaseNative(0, steps[dt], 1, 3).ok && above;
        launch = tomForceCaseNative(1, steps[dt], 1, 1).ok && launch;
    }
    check(
        under,
        "Helper below its authored flight cap suppresses Tom gravity after lift and before the solver");
    check(
        boundary,
        "Helper at the exact flight cap still receives native gravity because only strictly lower final velocity suppresses it");
    check(
        above,
        "Helper above the cap retains normal force-based fall acceleration without pre-integrating incoming velocity");
    check(
        launch,
        "A queued upward MegaJump while flying receives gravity according to its final selected velocity");
    bool water = true;
    for (int mode = 0; mode < 4; mode++)
        for (int mass = 1; mass < 3; mass++)
            water = tomForceCaseNative(mode, steps[1], masses[mass], 0, true).ok && water;
    check(
        water,
        "Native gravity adds to already accumulated authored water buoyancy and drag instead of replacing or duplicating them");
    int pad = prepareSpringPhaseNative();
    entities[playerIndex].state.vy = 0;
    restorePhysics();
    TomForceContactObserverNative listener;
    listener.source = pad;
    listener.calls = 0;
    listener.firstIncoming = listener.lastIncoming = 999;
    physicsWorld->SetContactListener(&listener);
    tick(steps[1], false, false);
    physicsWorld->SetContactListener(originalContacts);
    check(
        listener.calls > 0 && listener.firstIncoming == 0,
        "The first real Spring PreSolve observes selected velocity before gravity integration, while later continuous-collision callbacks retain their own actual velocities");
    char contactRow[200];
    wsprintfA(contactRow,
              "stationarySpring callbacks=%d firstIncoming100=%d lastIncoming100=%d pending=%d\r\n",
              listener.calls, (int)(listener.firstIncoming * 100),
              (int)(listener.lastIncoming * 100), pinballPendingNative.pending);
    lstrcatA(tomForceRowsNative, contactRow);
    atomicWrite("test-output/tom-force-comparison.txt", tomForceRowsNative,
                lstrlenA(tomForceRowsNative));
    startStage(0);
    readyTimer = 0;
    lstrcpyA(saveRoot, oldRoot);
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
}
