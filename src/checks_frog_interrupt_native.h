#pragma once

// Regression coverage for frog interrupt native. Runs only in the isolated BQ_ENABLE_TESTS executable.

class FrogInterruptSolvedObserverNative : public OriginalContacts {
  public:
    int ball, solved, owner;

    explicit FrogInterruptSolvedObserverNative(int index) : ball(index), solved(0), owner(-2) {}

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        b2Body *a = c->GetFixtureA()->GetBody();
        b2Body *b = c->GetFixtureB()->GetBody();
        if (a == bodies[ball] || b == bodies[ball]) {
            solved++;
            owner = (int)(a == bodies[ball] ? b : a)->GetUserData() - 1;
        }
        OriginalContacts::PostSolve(c, impulse);
    }
};

void prepareFrogInterruptNativeScene() {
    startStage(stageById(5770908237787719ULL));
    readyTimer = 0;
    if (frogLinkCount != 1)
        return;
    FrogLink &link = frogLinks[0];
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == ENEMY && i != link.left && i != link.right)
            entities[i].state.gone = 1;
    State &observer = entities[playerIndex].state;
    observer.x = 128;
    observer.y = 350;
    observer.vx = observer.vy = 0;
    restorePhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    for (b2Fixture *f = bodies[playerIndex]->GetFixtureList(); f; f = f->GetNext()) {
        b2Filter filter = f->GetFilterData();
        filter.maskBits = 0;
        f->SetFilterData(filter);
    }
}

void frogInterruptNativeChecks() {
    u32 incomingSeed = effectSeed;
    const float dt = 1.0f / 120;
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    debugMode = false;
    prepareFrogInterruptNativeScene();
    bool fixture = frogLinkCount == 1 &&
                   entities[frogLinks[0].left].source->id == 9532011916981958ULL &&
                   entities[frogLinks[0].right].source->id == 6586680631485202ULL &&
                   entities[frogLinks[0].ball].source->id == 5866742188234202ULL;
    check(
        fixture,
        "Original2-9 resolves the authored paired frogs, water, geometry and cooperative ball for partner-death checks");
    if (!fixture)
        return;
    int deathSteps[] = {0, 60, 96, 144, 192, 240, 264, 288};
    bool modeHeld = true, live = true, cooperativeSafe = true, legitimateEnd = true,
         disabled = true;
    int cases = 0, contacts = 0;
    char trace[8192] =
        "case,death_step,mode_after_death,launches_after3s,min_y,max_y,final_mode,final_pose,clear_step,clear_owner,solved_callbacks_at_clear,postscan_solids_at_clear,seed,minimum_y_while_cooperative\r\n";
    for (int side = 0; side < 2; side++)
        for (int phase = 0; phase < 8; phase++) {
            prepareFrogInterruptNativeScene();
            FrogLink &link = frogLinks[0];
            int survivor = side ? link.right : link.left;
            Entity &frog = entities[survivor];
            Entity &ball = entities[link.ball];
            int launches = 0, firstMode = -1, clearStep = -1, clearOwner = -2, solidAtClear = 0,
                scanAtClear = 0;
            u32 initialSeed = effectSeed;
            float low = 100000, high = -100000, activeLow = 100000, previousVy = 0;
            FrogInterruptSolvedObserverNative solvedObserver(link.ball);
            physicsWorld->SetContactListener(&solvedObserver);
            for (int step = 0; step < 1440; step++) {
                if (step == deathSteps[phase])
                    killEnemy(entities[side ? link.left : link.right]);
                int priorMode = ball.state.hp;
                solvedObserver.solved = 0;
                solvedObserver.owner = -2;
                tick(dt, false, false);
                if (priorMode && !ball.state.hp) {
                    clearStep = step;
                    solidAtClear = solvedObserver.solved;
                    clearOwner = solvedObserver.owner;
                    for (b2ContactEdge *edge = bodies[link.ball]->GetContactList(); edge;
                         edge = edge->next) {
                        b2Contact *c = edge->contact;
                        if (c->IsEnabled() && c->IsTouching() && c->GetManifold()->pointCount &&
                            !c->GetFixtureA()->IsSensor() && !c->GetFixtureB()->IsSensor())
                            scanAtClear++;
                    }
                    legitimateEnd &= solidAtClear > 0;
                }
                if (step == deathSteps[phase]) {
                    firstMode = ball.state.hp;
                    modeHeld &= firstMode == 1;
                }
                if (step >= 360) {
                    low = minf(low, frog.state.y);
                    high = maxf(high, frog.state.y);
                    if (frog.state.vy > 100 && previousVy <= 100)
                        launches++;
                }
                if (step >= deathSteps[phase] && ball.state.hp) {
                    activeLow = minf(activeLow, frog.state.y);
                    cooperativeSafe &= frog.state.y > 190;
                }
                previousVy = frog.state.vy;
                live &= !frog.state.gone && frog.state.x == frog.state.x &&
                        frog.state.y == frog.state.y;
                if (ball.state.hp && bodies[link.ball])
                    for (b2ContactEdge *edge = bodies[link.ball]->GetContactList(); edge;
                         edge = edge->next)
                        if (edge->other == bodies[survivor] && edge->contact->IsTouching()) {
                            contacts++;
                            disabled &= !edge->contact->IsEnabled();
                        }
                if ((side == 0 && phase == 2 && step == 420) ||
                    (side == 1 && phase == 6 && step == 300)) {
                    camX = 448;
                    camY = 100;
                    captureCase(side ? "test-output/frog-surviving-right-native.bmp"
                                     : "test-output/frog-surviving-left-native.bmp");
                }
            }
            char row[300];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%d\r\n", side * 8 + phase,
                      deathSteps[phase], firstMode, launches, (int)(low * 100), (int)(high * 100),
                      ball.state.hp, frog.state.hp, clearStep, clearOwner, solidAtClear,
                      scanAtClear, initialSeed, (int)(activeLow * 100));
            lstrcatA(trace, row);
            cases++;
            physicsWorld->SetContactListener(originalContacts);
        }
    atomicWrite("test-output/frog-partner-death-native.csv", trace, lstrlenA(trace));
    check(
        cases == 16 && modeHeld,
        "Across16 mirrored deaths including the right frog's rise/top the partner death itself never clears the ball cooperation mode");
    check(
        live && cooperativeSafe && legitimateEnd,
        "Surviving frogs stay above water while cooperation is active and every later mode clear has a real solved solid contact");
    check(
        contacts > 0 && disabled,
        "Actual linked contacts after a partner death stay disabled while the native ball mode remains active");
    prepareFrogInterruptNativeScene();
    FrogLink &normal = frogLinks[0];
    int last = 0, left = 0, right = 0;
    bool continuous = true;
    for (int i = 0; i < 960; i++) {
        tick(dt, false, false);
        State &b = entities[normal.ball].state;
        continuous &=
            b.hp == 1 && !entities[normal.left].state.gone && !entities[normal.right].state.gone;
        if (b.active != last) {
            last = b.active;
            if (last == normal.left + 1)
                left++;
            if (last == normal.right + 1)
                right++;
        }
    }
    check(
        continuous && left >= 2 && right >= 2,
        "The unchanged original pair still exchanges its ball repeatedly for eight seconds without partner deaths");
    prepareFrogInterruptNativeScene();
    FrogLink &saved = frogLinks[0];
    for (int i = 0; i < 270; i++) {
        if (i == 264)
            killEnemy(entities[saved.left]);
        tick(dt, false, false);
    }
    captureCheckpoint(true);
    int mode = entities[saved.ball].state.hp;
    float phase = entities[saved.right].state.aux;
    State originalBall = entities[saved.ball].state;
    for (int i = 0; i < 120; i++)
        tick(dt, false, false);
    State expectedBall = entities[saved.ball].state, expectedFrog = entities[saved.right].state;
    restoreCheckpoint();
    bool restored = entities[saved.left].state.gone && entities[saved.ball].state.hp == mode &&
                    entities[saved.right].state.aux == phase &&
                    entities[saved.ball].state.timer == originalBall.timer;
    for (int i = 0; i < 120; i++)
        tick(dt, false, false);
    State &actualBall = entities[saved.ball].state;
    State &actualFrog = entities[saved.right].state;
    check(
        restored && mode == 1 && frogJumperDelay(saved.right, .5f) == 2 &&
            actualBall.hp == expectedBall.hp && absf(actualBall.x - expectedBall.x) < 4 &&
            absf(actualBall.y - expectedBall.y) < 4 && absf(actualFrog.x - expectedFrog.x) < 4 &&
            absf(actualFrog.y - expectedFrog.y) < 4,
        "Checkpoint restoration retains the partner-death mode, effective two-second delay and bounded ball/frog trajectory");
    // A controlled real solid contact legitimately ends cooperation. Removing
    // the now ordinary ball isolates the surviving jumper's retained delay.
    prepareFrogInterruptNativeScene();
    FrogLink &cadence = frogLinks[0];
    Entity &cb = entities[cadence.ball];
    Entity &cf = entities[cadence.left];
    b2BodyDef floorDef;
    floorDef.position.Set(1500 * metres, 290 * metres);
    b2Body *floor = physicsWorld->CreateBody(&floorDef);
    b2PolygonShape floorShape;
    floorShape.SetAsBox(100 * metres, 10 * metres);
    floor->CreateFixture(&floorShape, 0);
    cb.state.x = 1500;
    cb.state.y = 330;
    cb.state.vx = cb.state.vy = 0;
    restorePhysics();
    simulateBodies(dt);
    frogPostStep();
    bool ended = cb.state.hp == 0;
    cb.state.gone = 1;
    entities[cadence.right].state.gone = 1;
    for (int i = 0; i < 60 && !enemyGrounded(cadence.left); i++)
        tick(dt, false, false);
    bool supported = enemyGrounded(cadence.left);
    cf.state.aux = 0;
    cf.state.hp = 0;
    cf.state.vy = 0;
    bodies[cadence.left]->SetLinearVelocity(b2Vec2(0, 0));
    int firstJump = -1;
    for (int i = 1; i <= 250; i++) {
        tick(dt, false, false);
        if (firstJump < 0 && cf.state.vy > 100)
            firstJump = i;
    }
    check(
        ended && supported && firstJump > 235 && firstJump <= 245 &&
            frogJumperDelay(cadence.left, .5f) == 2 && frogJumperDelay(playerIndex, .5f) == .5f,
        "After a real solid interruption the paired frog waits two seconds before jumping while unlinked authored delays remain unchanged");
    startStage(0);
    readyTimer = 0;
    effectSeed = incomingSeed;
}
