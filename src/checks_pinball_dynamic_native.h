#pragma once

// Regression coverage for pinball dynamic native. Runs only in the isolated BQ_ENABLE_TESTS executable.

class PinballDynamicObserverNative : public OriginalContacts {
  public:
    int pin, target, events, solved;
    bool invalidate, block, allPins;
    Vec normals[32];
    bool soundAtContact;
    float solvedVx, solvedVy;

    PinballDynamicObserverNative()
        : pin(-1), target(-1), events(0), solved(0), invalidate(false), block(false),
          allPins(false), soundAtContact(false), solvedVx(0), solvedVy(0) {}

    bool match(int a, int b) {
        return ((a == pin ||
                 (allPins && a >= 0 && a < entityCount && entities[a].kind == BOUNCER)) &&
                b == target) ||
               ((b == pin ||
                 (allPins && b >= 0 && b < entityCount && entities[b].kind == BOUNCER)) &&
                a == target);
    }

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        int a = (int)c->GetFixtureA()->GetBody()->GetUserData() - 1,
            b = (int)c->GetFixtureB()->GetBody()->GetUserData() - 1;
        bool accepted = match(a, b);
        if (accepted && block)
            c->SetEnabled(false);
        OriginalContacts::PreSolve(c, old);
        if (!accepted || !c->IsEnabled() || c->GetFixtureA()->IsSensor() ||
            c->GetFixtureB()->IsSensor() || !c->GetManifold()->pointCount ||
            bodies[target]->GetType() != b2_dynamicBody)
            return;
        b2WorldManifold m;
        c->GetWorldManifold(&m);
        if (events < 32)
            normals[events++] =
                Vec{a != target ? m.normal.x : -m.normal.x, a != target ? m.normal.y : -m.normal.y};
        soundAtContact |= pinballDetailVoiceCount("pinball") > 0;
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        OriginalContacts::PostSolve(c, impulse);
        int a = (int)c->GetFixtureA()->GetBody()->GetUserData() - 1,
            b = (int)c->GetFixtureB()->GetBody()->GetUserData() - 1;
        if (!match(a, b))
            return;
        solved++;
        solvedVx = bodies[target]->GetLinearVelocity().x / metres;
        solvedVy = bodies[target]->GetLinearVelocity().y / metres;
        if (invalidate)
            c->SetEnabled(false);
    }
};

int preparePinballDynamicNative(int &target) {
    int pin = preparePinballFixtureNative(80);
    target = -1;
    if (pin < 0)
        return pin;
    target = entityCount++;
    Object &source = *classById(14441582509727370349ULL);
    Entity &ball = entities[target];
    initializeEntity(ball, source, source.rect);
    ball.state.x = entities[pin].state.x + 8;
    ball.state.y = entities[pin].state.y + 32 - ball.hit.b - 1.0f;
    ball.state.vx = 45;
    ball.state.vy = -200;
    rebuildPhysics();
    bodies[target]->SetGravityScale(0);
    stopSounds();
    return pin;
}

bool pinballDynamicExpectedNative(PinballDynamicObserverNative &observer, float &x, float &y) {
    if (!observer.events || !observer.solved)
        return false;
    x = observer.solvedVx;
    y = observer.solvedVy;
    float speed = root(1000 * entities[observer.pin]
                                  .source->behavior("PinballBouncerBehaviour")
                                  ->num("physics-power", 200));
    for (int i = 0; i < observer.events; i++) {
        x += speed * observer.normals[i].x;
        y = speed * observer.normals[i].y;
    }
    return true;
}

void pinballDynamicNativeChecks() {
    interfaceEnabled = debugMode = musicEnabled = false;
    soundEnabled = true;
    stopMusic();
    bool retained = true, contactSound = true, normal = true;
    char rows[2500] =
        "fps,invalidated,preEvents,postEvents,expectedVx1000,expectedVy1000,actualVx1000,actualVy1000,soundDuringPre\r\n";
    for (int invalid = 0; invalid < 2; invalid++)
        for (int rate = 0; rate < 3; rate++) {
            int target = -1, pin = preparePinballDynamicNative(target);
            if (pin < 0) {
                retained = normal = contactSound = false;
                continue;
            }
            PinballDynamicObserverNative observer;
            observer.pin = pin;
            observer.target = target;
            observer.invalidate = invalid != 0;
            physicsWorld->SetContactListener(&observer);
            float dt = 1.0f / (rate == 0 ? 30 : rate == 1 ? 120 : 240);
            simulateBodies(dt);
            float x = 0, y = 0;
            bool expected = pinballDynamicExpectedNative(observer, x, y);
            bool before = absf(entities[target].state.vy - observer.solvedVy) < .01f;
            mechanismsPostStep(dt);
            bool correct = expected && before && absf(entities[target].state.vx - x) < .03f &&
                           absf(entities[target].state.vy - y) < .03f;
            if (invalid)
                retained &= correct;
            else
                normal &= correct;
            contactSound &= observer.events > 0 && observer.soundAtContact;
            char row[240];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
                      rate == 0   ? 30
                      : rate == 1 ? 120
                                  : 240,
                      invalid, observer.events, observer.solved, (int)(x * 1000), (int)(y * 1000),
                      (int)(entities[target].state.vx * 1000),
                      (int)(entities[target].state.vy * 1000), observer.soundAtContact);
            lstrcatA(rows, row);
            physicsWorld->SetContactListener(originalContacts);
        }
    check(
        normal,
        "Actual non-Tom Pinball contacts retain post-solver horizontal addition and assigned vertical power at30/120/240Hz, using each recorded manifold normal");
    check(
        retained,
        "A real Pinball PreSolve remains scheduled when the same solved contact is disabled later in the step, rather than disappearing from final-contact polling");
    check(
        contactSound,
        "Each real ordinary-body Pinball contact plays its dedicated sound during PreSolve before post-physics launch dispatch");
    bool order = true, once = true;
    for (int variant = 0; variant < 2; variant++) {
        int target = -1, pin = preparePinballDynamicNative(target);
        Entity &first = entities[pin];
        first.state.x = 256 - 48;
        first.state.y = 128;
        int second = entityCount++;
        initializeEntity(entities[second], *first.source, first.source->rect);
        entities[second].state.x = 256 + 48;
        entities[second].state.y = 128;
        Entity &ball = entities[target];
        ball.state.x = 256;
        ball.state.y = 160;
        ball.state.vx = 30;
        ball.state.vy = -100;
        if (variant) {
            float x = first.state.x;
            first.state.x = entities[second].state.x;
            entities[second].state.x = x;
        }
        rebuildPhysics();
        bodies[target]->SetGravityScale(0);
        PinballDynamicObserverNative listener;
        listener.pin = pin;
        listener.target = target;
        listener.allPins = true;
        physicsWorld->SetContactListener(&listener);
        simulateBodies(1 / 120.0f);
        float x = 0, y = 0;
        bool expected = pinballDynamicExpectedNative(listener, x, y);
        mechanismsPostStep(1 / 120.0f);
        order &= expected && listener.events >= 2 && absf(entities[target].state.vx - x) < .03f &&
                 absf(entities[target].state.vy - y) < .03f;
        float vx = entities[target].state.vx, vy = entities[target].state.vy;
        u32 sounds = audioVoicesStarted;
        mechanismsPostStep(1 / 120.0f);
        once &= absf(entities[target].state.vx - vx) < .001f &&
                absf(entities[target].state.vy - vy) < .001f && audioVoicesStarted == sounds;
        physicsWorld->SetContactListener(originalContacts);
    }
    check(
        order,
        "Two actual simultaneous Pinball contacts dispatch captured normals in callback order under both source-entity arrangements");
    check(
        once,
        "Dispatching an already drained physical-contact queue cannot repeat ordinary-body velocity additions or replay sound");
    bool rejected = true;
    for (int mode = 0; mode < 4; mode++) {
        int target = -1, pin = preparePinballDynamicNative(target);
        PinballDynamicObserverNative observer;
        observer.pin = pin;
        observer.target = target;
        observer.block = mode == 1;
        if (mode == 0)
            bodies[target]->GetFixtureList()->SetSensor(true);
        if (mode == 2)
            bodies[target]->SetType(b2_staticBody);
        if (mode == 3)
            bodies[target]->SetType(b2_kinematicBody);
        physicsWorld->SetContactListener(&observer);
        simulateBodies(1 / 120.0f);
        mechanismsPostStep(1 / 120.0f);
        rejected &= observer.events == 0 && pinballDetailVoiceCount("pinball") == 0;
        physicsWorld->SetContactListener(originalContacts);
    }
    check(
        rejected,
        "Sensors, disabled PreSolve, static targets and kinematic targets cannot schedule ordinary-body Pinball sound or launch");
    int target = -1, pin = preparePinballDynamicNative(target);
    PinballDynamicObserverNative observer;
    observer.pin = pin;
    observer.target = target;
    physicsWorld->SetContactListener(&observer);
    simulateBodies(1 / 120.0f);
    physicsWorld->SetContactListener(originalContacts);
    bool had = observer.events > 0;
    restorePhysics();
    float vx = entities[target].state.vx, vy = entities[target].state.vy;
    mechanismsPostStep(1 / 120.0f);
    check(
        had && absf(entities[target].state.vx - vx) < .001f &&
            absf(entities[target].state.vy - vy) < .001f,
        "Restoring physics after an actual accepted contact discards its unsaved ordinary-body callback instead of launching stale state");
    navigate(137502285691ULL);
    int endingPin = -1, ball = -1;
    for (int i = 0; i < page.playfieldCount; i++) {
        if (page.playfield[i].kind == BOUNCER && endingPin < 0)
            endingPin = i;
        if (page.playfield[i].source->klass == 2079096017ULL && ball < 0)
            ball = i;
    }
    bool ui = false, isolated = false;
    if (endingPin >= 0 && ball >= 0) {
        u32 stageHash = hashBytes(entities, sizeof(Entity) * entityCount),
            savedHash = hashBytes(&checkpoint, sizeof(checkpoint));
        destroyUiPhysics(page);
        page.level.geometryCount = 0;
        for (int i = 0; i < page.playfieldCount; i++)
            if (i != endingPin && i != ball)
                page.playfield[i].state.gone = 1;
        Entity &pad = page.playfield[endingPin];
        Entity &falling = page.playfield[ball];
        falling.state.x = pad.state.x;
        falling.state.y = pad.state.y + 32 - falling.hit.b - .25f;
        falling.state.vx = 0;
        falling.state.vy = -200;
        page.bean.state.x = 1000;
        page.bean.state.y = 10000;
        createUiPhysics(page);
        page.bodies[ball]->SetGravityScale(0);
        stopSounds();
        stepUiPhysics(page, 1 / 120.0f);
        ui = absf(page.playfield[ball].state.vy - root(75000)) < .03f &&
             pinballDetailVoiceCount("pinball") > 0;
        isolated = stageHash == hashBytes(entities, sizeof(Entity) * entityCount) &&
                   savedHash == hashBytes(&checkpoint, sizeof(checkpoint));
    }
    check(
        ui,
        "The actual ending world launches its real original beach ball from a real Pinball contact through the shared UI post-physics path exactly once");
    check(
        isolated,
        "The ending world's ordinary-body Pinball callback leaves campaign entities and checkpoint bytes untouched");
    atomicWrite("test-output/pinball-dynamic-contact.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/pinball-dynamic-results.txt", testLog, lstrlenA(testLog));
    soundEnabled = false;
    stopSounds();
    startStage(0);
    readyTimer = 0;
}
