#pragma once

// Regression coverage for crumbling cloud native. Runs only in the isolated BQ_ENABLE_TESTS executable.

class CrumblingCloudNativeProbe : public OriginalContacts {
  public:
    int solved[MAX_ENTITIES];

    CrumblingCloudNativeProbe() {
        memset(solved, 0, sizeof(solved));
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        b2Fixture *f[] = {c->GetFixtureA(), c->GetFixtureB()};
        for (int side = 0; side < 2; side++) {
            int index = (int)f[side]->GetBody()->GetUserData() - 1;
            if (index >= 0 && index < entityCount && entities[index].crumble &&
                f[1 - side]->GetBody() == bodies[playerIndex])
                solved[index]++;
        }
        OriginalContacts::PostSolve(c, impulse);
    }
};

int crumblingCloudNativeIndex(u64 id) {
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->id == id)
            return i;
    return -1;
}

int crumblingCloudNativeVoices() {
    int n = 0;
    AudioClip *clip = 0;
    for (int i = 0; i < soundClipCount; i++)
        if (eq(soundClips[i].name, "james-cloud"))
            clip = &soundClips[i].clip;
    for (int i = 0; i < 32; i++)
        if (clip && audioVoices[i].clip == clip)
            n++;
    return n;
}

bool crumblingCloudNativePrepare(int &cloud, int &adjacent, bool pair) {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    debugMode = false;
    startStage(stageById(5100914154076244783ULL));
    readyTimer = deathTimer = 0;
    stopSounds();
    soundEnabled = true;
    cloud = crumblingCloudNativeIndex(pair ? 14765388418456001666ULL : 12019496131362594377ULL);
    adjacent = pair ? crumblingCloudNativeIndex(16690355334315047886ULL) : -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == ENEMY)
            entities[i].state.gone = 1;
    return cloud >= 0 && (!pair || adjacent >= 0) && entities[cloud].crumble;
}

void crumblingCloudNativeChecks() {
    const float dt = 1.0f / 120.0f;
    bool ordinary = true, edge = true, oneVoice = true;
    int actualHits = 0;
    // Reuse the original stage and source geometry. A probe observes actual
    // solver callbacks and forwards every callback to the production listener.
    for (int scenario = 0; scenario < 5; scenario++) {
        int cloud = -1, adjacent = -1;
        if (!crumblingCloudNativePrepare(cloud, adjacent, false)) {
            check(false, "Original Cloud Road crumbling actor resolves for native contact tests");
            return;
        }
        Entity &pad = entities[cloud];
        Rect bounds = hitRect(pad);
        State &bean = entities[playerIndex].state;
        bean.x = scenario == 1 ? bounds.l - entities[playerIndex].hit.r + 3
                               : (bounds.l + bounds.r) * 0.5f;
        bean.y = bounds.t - entities[playerIndex].hit.b + 1;
        bean.vx = 0;
        bean.vy = scenario == 2 ? -1.0f : scenario == 3 ? -1500.0f : -120.0f;
        restorePhysics();
        if (scenario == 4) {
            b2Fixture *main = bodies[playerIndex]->GetFixtureList();
            while (main && main->IsSensor())
                main = main->GetNext();
            b2FixtureDef second;
            second.shape = main->GetShape();
            second.density = main->GetDensity();
            second.friction = main->GetFriction();
            second.restitution = main->GetRestitution();
            second.filter = main->GetFilterData();
            bodies[playerIndex]->CreateFixture(&second);
        }
        CrumblingCloudNativeProbe *probe = new CrumblingCloudNativeProbe;
        physicsWorld->SetContactListener(probe);
        for (int frame = 0; frame < 60 && !probe->solved[cloud]; frame++)
            tick(scenario == 3 ? 1.0f / 30.0f : dt, false, false);
        bool hit = probe->solved[cloud] > 0;
        actualHits += probe->solved[cloud];
        bool burst = hit && pad.state.active == 2 && !bodies[cloud]->IsActive() &&
                     !pad.state.gone && pad.state.timer == 0;
        if (scenario == 1)
            edge &= burst;
        else
            ordinary &= burst;
        oneVoice &= crumblingCloudNativeVoices() == 1;
        char trace[384], path[128];
        wsprintfA(path, "test-output/crumbling-native-contact-%d.txt", scenario);
        wsprintfA(
            trace,
            "source=12019496131362594377 scenario=%d solved=%d active=%d timer1000=%d bodyActive=%d voices=%d grounded=%d support=%d vy100=%d\r\n",
            scenario, probe->solved[cloud], pad.state.active, (int)(pad.state.timer * 1000),
            bodies[cloud]->IsActive(), crumblingCloudNativeVoices(), onGround, supportEntity,
            (int)(bean.vy * 100));
        atomicWrite(path, trace, lstrlenA(trace));
        physicsWorld->SetContactListener(originalContacts);
        delete probe;
    }
    check(
        ordinary && actualHits >= 4,
        "Actual Cloud Road top contact bursts immediately for ordinary, weak, fast-TOI and multiple-Bean-fixture impacts");
    check(
        edge,
        "An actual cloud top-edge solver contact bursts without requiring that cloud to remain the final selected ground support");
    check(
        oneVoice,
        "Every actual crumbling cloud solver event emits exactly one james-cloud voice even with multiple contact fixtures");
    int cloud = -1, adjacent = -1;
    if (!crumblingCloudNativePrepare(cloud, adjacent, true)) {
        check(false, "Original adjacent Cloud Road pair resolves");
        return;
    }
    Rect a = hitRect(entities[cloud]), b = hitRect(entities[adjacent]);
    State &bean = entities[playerIndex].state;
    bean.x = (a.r + b.l) * 0.5f;
    bean.y = a.t - entities[playerIndex].hit.b + 1;
    bean.vx = 0;
    bean.vy = -160;
    restorePhysics();
    CrumblingCloudNativeProbe *probe = new CrumblingCloudNativeProbe;
    physicsWorld->SetContactListener(probe);
    bool allSolvedBurst = true;
    int hitClouds = 0;
    for (int frame = 0; frame < 60; frame++) {
        tick(dt, false, false);
        hitClouds = (probe->solved[cloud] > 0) + (probe->solved[adjacent] > 0);
        for (int n = 0; n < 2; n++) {
            int i = n ? adjacent : cloud;
            if (probe->solved[i])
                allSolvedBurst &= entities[i].state.active == 2 && !bodies[i]->IsActive();
        }
        if (hitClouds >= 2 || entities[cloud].state.active == 2 ||
            entities[adjacent].state.active == 2)
            break;
    }
    check(
        hitClouds == 2 && allSolvedBurst && crumblingCloudNativeVoices() == 2,
        "Both authored adjacent clouds touched in one real solver step burst independently rather than only the last support owner");
    physicsWorld->SetContactListener(originalContacts);
    delete probe;
    bool underside = true, lateral = true;
    for (int scenario = 0; scenario < 2; scenario++) {
        crumblingCloudNativePrepare(cloud, adjacent, false);
        Rect bounds = hitRect(entities[cloud]);
        State &p = entities[playerIndex].state;
        p.x = scenario ? bounds.l - entities[playerIndex].hit.r - 8 : (bounds.l + bounds.r) * 0.5f;
        p.y = scenario ? bounds.t - entities[playerIndex].hit.b - 1
                       : bounds.b - entities[playerIndex].hit.t - 4;
        p.vx = scenario ? 180 : 0;
        p.vy = scenario ? 0 : 200;
        restorePhysics();
        probe = new CrumblingCloudNativeProbe;
        physicsWorld->SetContactListener(probe);
        for (int frame = 0; frame < (scenario ? 60 : 24); frame++)
            tick(dt, false, scenario != 0);
        bool intact = probe->solved[cloud] == 0 && entities[cloud].state.active == 0 &&
                      bodies[cloud]->IsActive() && crumblingCloudNativeVoices() == 0;
        if (scenario)
            lateral = intact && hitRect(entities[playerIndex]).l > bounds.r;
        else
            underside = intact && hitRect(entities[playerIndex]).t > bounds.t;
        physicsWorld->SetContactListener(originalContacts);
        delete probe;
    }
    check(
        underside && lateral,
        "Original one-way underside and lateral traversal never produce a solved Bean contact or falsely burst a cloud");
    // Stay outside the fixture after burst and test the source Time default.
    crumblingCloudNativePrepare(cloud, adjacent, false);
    Rect bounds = hitRect(entities[cloud]);
    State &p = entities[playerIndex].state;
    p.x = (bounds.l + bounds.r) * 0.5f;
    p.y = bounds.t - entities[playerIndex].hit.b + 1;
    p.vx = 0;
    p.vy = -120;
    restorePhysics();
    for (int frame = 0; frame < 60 && entities[cloud].state.active != 2; frame++)
        tick(dt, false, false);
    bool burst = entities[cloud].state.active == 2;
    p.x = bounds.l - 100;
    p.y = bounds.t + 100;
    p.vx = p.vy = 0;
    restorePhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    for (int frame = 0; frame < 348; frame++)
        tick(dt, false, false);
    bool beforeReturn = entities[cloud].state.active == 2;
    for (int frame = 0; frame < 24; frame++)
        tick(dt, false, false);
    check(
        burst && beforeReturn && entities[cloud].state.active == 0 && bodies[cloud]->IsActive() &&
            !entities[cloud].state.gone,
        "An actual burst cloud stays disabled until its native three-second Time interval then reappears without permanent deletion");
    musicEnabled = soundEnabled = false;
    stopSounds();
    startStage(0);
    readyTimer = 0;
}
