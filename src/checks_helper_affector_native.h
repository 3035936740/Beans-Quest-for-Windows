#pragma once

// Regression coverage for helper affector native. Runs only in the isolated BQ_ENABLE_TESTS executable.

class HelperAffectorObserverNative : public OriginalContacts {
  public:
    int bird, enemy, seen, solved;
    bool stomp;

    HelperAffectorObserverNative(int b, int e)
        : bird(b), enemy(e), seen(0), solved(0), stomp(false) {}

    bool pair(b2Contact *c) {
        b2Body *a = c->GetFixtureA()->GetBody();
        b2Body *b = c->GetFixtureB()->GetBody();
        return a == bodies[bird] && b == bodies[enemy] || a == bodies[enemy] && b == bodies[bird];
    }

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        if (pair(c) && !c->GetFixtureA()->IsSensor() && !c->GetFixtureB()->IsSensor() &&
            c->GetManifold()->pointCount) {
            seen++;
            b2WorldManifold m;
            c->GetWorldManifold(&m);
            Entity &target = entities[enemy];
            float low =
                target.state.y + target.hit.t -
                target.source->behavior("KillableEnemyBehaviour")->num("vulnerable-size", 8);
            bool high = true;
            for (int i = 0; i < c->GetManifold()->pointCount; i++)
                high &= m.points[i].y / metres >= low;
            float vy = bodies[bird]->GetLinearVelocity().y,
                  enemyVy = bodies[enemy]->GetLinearVelocity().y;
            stomp |= high && (vy < 0 || vy - enemyVy < 0);
        }
        OriginalContacts::PreSolve(c, old);
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        if (pair(c))
            solved++;
        OriginalContacts::PostSolve(c, impulse);
    }
};

int helperAffectorPrepareNative(int mode, int &enemy) {
    interfaceEnabled = musicEnabled = soundEnabled = debugMode = false;
    startStage(stageById(641851699177ULL));
    readyTimer = 0;
    int bird = -1;
    enemy = -1;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].source->id == 776999843797ULL)
            bird = i;
        if (entities[i].source->id == 60786170106ULL)
            enemy = i;
    }
    if (bird < 0 || enemy < 0)
        return -1;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && i != bird && i != enemy)
            entities[i].state.gone = 1;
    Entity &target = entities[enemy];
    target.walk = 0;
    target.dynamic = 1;
    target.state.x = 500;
    target.state.y = 400;
    target.state.vx = 0;
    target.state.vy = mode == 0 ? 120 : 0;
    Entity &helper = entities[bird];
    helper.state.active = mode == 2 ? 0 : 1;
    helper.state.timer = .2f;
    helper.state.x = mode == 0 ? 500 : target.state.x + target.hit.r - helper.hit.l - .1f;
    helper.state.y = mode == 0 ? target.state.y + target.hit.t - helper.hit.b + .1f : 400;
    helper.state.vx = helper.state.vy = 0;
    State &bean = entities[playerIndex].state;
    bean.x = 800;
    bean.y = 800;
    bean.vx = bean.vy = 0;
    jumps = 7;
    rebuildPhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    bodies[enemy]->SetGravityScale(0);
    if (mode == 3)
        for (b2Fixture *f = bodies[bird]->GetFixtureList(); f; f = f->GetNext())
            f->SetSensor(true);
    stopSounds();
    return bird;
}

void helperAffectorNativeChecks() {
    bool real = true, stomp = false, flying = false, silent = false, repeated = false,
         resting = false, sensors = false;
    for (int mode = 0; mode < 4; mode++) {
        int enemy = -1, bird = helperAffectorPrepareNative(mode, enemy);
        if (bird < 0) {
            real = false;
            continue;
        }
        HelperAffectorObserverNative observer(bird, enemy);
        physicsWorld->SetContactListener(&observer);
        u32 voices = audioVoicesStarted;
        soundEnabled = true;
        simulateBodies(1 / 120.0f);
        helperPostStep(1 / 120.0f);
        soundEnabled = false;
        if (mode != 3)
            real &= observer.seen > 0;
        if (mode == 0) {
            real &= observer.stomp;
            stomp = entities[enemy].state.gone == 2 && !bodies[enemy]->IsActive() &&
                    observer.solved == 0;
            flying = entities[bird].state.active == 1 && jumps == 7 && deathTimer == 0 &&
                     audioVoicesStarted == voices + 1;
        }
        if (mode == 1) {
            real &= !observer.stomp;
            silent = !entities[enemy].state.gone && entities[bird].state.active == 2 &&
                     observer.solved == 0 && audioVoicesStarted == voices + 1;
            u32 after = audioVoicesStarted;
            soundEnabled = true;
            helperPostStep(0);
            helperPostStep(0);
            soundEnabled = false;
            repeated = entities[bird].state.active == 2 && audioVoicesStarted == after;
        }
        if (mode == 2)
            resting = !entities[enemy].state.gone && entities[bird].state.active == 0 &&
                      audioVoicesStarted == voices;
        if (mode == 3)
            sensors = !entities[enemy].state.gone && entities[bird].state.active == 1 &&
                      audioVoicesStarted == voices;
        physicsWorld->SetContactListener(originalContacts);
    }
    check(
        real,
        "Original Stormy Weather helper and turtle produce actual upper and side contacts with independently observed native vulnerable-strip decisions");
    check(
        stomp,
        "A stationary helper contacted from below by a rising actual turtle kills the enemy through the native affector branch without a solved impact");
    check(
        flying,
        "Helper KilledEnemyFromJump is a no-op: the bird keeps following, Bean gains no jump or death, and only the enemy death voice plays");
    check(
        silent && repeated,
        "User-requested non-stomp helper impact plays one bird chirp and cannot repeat while returning");
    check(
        resting && sensors,
        "A resting helper and an opposite sensor cannot invent a helper collection, enemy death, or collision sound");
    int enemy = -1, bird = helperAffectorPrepareNative(1, enemy);
    bool cleared = false;
    if (bird >= 0) {
        captureCheckpoint(false);
        simulateBodies(1 / 120.0f);
        restoreCheckpoint();
        u32 voices = audioVoicesStarted;
        soundEnabled = true;
        helperPostStep(0);
        soundEnabled = false;
        cleared = entities[bird].state.active == 1 && audioVoicesStarted == voices;
    }
    check(
        cleared,
        "Checkpoint restore clears any unconsumed helper-affector stop event before restored flight resumes");
    u32 campaign = hashBytes(entities, sizeof(Entity) * entityCount),
        saved = hashBytes(&checkpoint, sizeof(checkpoint));
    interfaceEnabled = true;
    navigate(137502285691ULL);
    int helper = -1, target = -1;
    for (int i = 0; i < page.playfieldCount; i++) {
        if (page.playfield[i].kind == HELPER)
            helper = i;
        if (page.playfield[i].source->id == 6949754987661836ULL)
            target = i;
    }
    bool ending = false, domain = false;
    if (helper >= 0 && target >= 0) {
        for (int i = 0; i < page.playfieldCount; i++)
            if (i != helper && i != target)
                page.playfield[i].state.gone = 1;
        Entity &turtle = page.playfield[target];
        Entity &birdActor = page.playfield[helper];
        turtle.walk = 0;
        turtle.dynamic = 1;
        turtle.state.x = 2000;
        turtle.state.y = 4000;
        turtle.state.vx = 0;
        turtle.state.vy = 120;
        birdActor.state.gone = 0;
        birdActor.state.active = 1;
        birdActor.state.timer = .2f;
        birdActor.state.x = turtle.state.x;
        birdActor.state.y = turtle.state.y + turtle.hit.t - birdActor.hit.b + .1f;
        birdActor.state.vx = birdActor.state.vy = 0;
        page.bean.state.x = 3000;
        page.bean.state.y = 4500;
        page.bean.state.vx = page.bean.state.vy = 0;
        resetUiPhysics(page);
        page.bodies[page.playfieldCount]->SetType(b2_staticBody);
        page.bodies[target]->SetGravityScale(0);
        stepUiPhysics(page, 1 / 120.0f);
        creditsHelperPostStep(page, 1 / 120.0f);
        ending = turtle.state.gone == 2 && !page.bodies[target]->IsActive() &&
                 birdActor.state.active == 1;
        clearEffects(1);
        u32 before = emittedParticles;
        tickUiPlayfield(page, .2f, false);
        domain = turtle.state.gone == 1 && emittedParticles > before && liveEffects(1) > 0;
    }
    check(
        ending && campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
            saved == hashBytes(&checkpoint, sizeof(checkpoint)),
        "The actual ending helper and turtle use the same affector branch in their own physics world without changing campaign actors or checkpoint");
    check(
        domain,
        "An ending helper stomp completes enemy death particles in the ending effect domain instead of the campaign pool");
    interfaceEnabled = false;
    startStage(0);
    readyTimer = 0;
    stopSounds();
    atomicWrite("test-output/helper-affector-results.txt", testLog, lstrlenA(testLog));
}
