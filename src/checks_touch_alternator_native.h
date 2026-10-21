#pragma once

// Regression coverage for touch alternator native. Runs only in the isolated BQ_ENABLE_TESTS executable.

class TouchAlternatorNativeObserver : public OriginalContacts {
  public:
    int target, solved;
    float normalY;

    void PostSolve(b2Contact *contact, const b2ContactImpulse *impulse) {
        OriginalContacts::PostSolve(contact, impulse);
        int a = (int)contact->GetFixtureA()->GetBody()->GetUserData() - 1,
            b = (int)contact->GetFixtureB()->GetBody()->GetUserData() - 1;
        if (!((a == target && b == playerIndex) || (b == target && a == playerIndex)))
            return;
        b2WorldManifold m;
        contact->GetWorldManifold(&m);
        solved++;
        normalY = maxf(normalY, a == target ? m.normal.y : -m.normal.y);
    }
};

int touchAlternatorNativeFixture(bool isolate = true) {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    startStage(stageById(1966799885083198ULL));
    readyTimer = 0;
    int target = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch && entities[i].alternation) {
            target = i;
            break;
        }
    if (target < 0)
        return target;
    if (isolate) {
        current.geometryCount = 0;
        for (int i = 0; i < entityCount; i++)
            if (i != target && i != playerIndex)
                entities[i].state.gone = 1;
    }
    rebuildPhysics();
    return target;
}

void touchAlternatorNativePlace(int target, float vy) {
    Entity &bean = entities[playerIndex];
    Entity &block = entities[target];
    bean.state.x = block.state.x;
    bean.state.y = hitRect(block).t - bean.hit.b - 0.5f;
    bean.state.vx = 0;
    bean.state.vy = vy;
    bean.state.timer = .2f;
    restorePhysics();
}

void touchAlternatorNativeChecks() {
    const float dt = 1.0f / 120;
    TouchAlternatorNativeObserver observer;
    int target = touchAlternatorNativeFixture(false), count = 0, off = 0, on = 0;
    bool initialized = true;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch) {
            count++;
            off += !entities[i].alternation;
            on += entities[i].alternation != 0;
            initialized &= absf(entities[i].state.timer - .1f) < .00001f && bodies[i] &&
                           bodies[i]->GetType() == b2_kinematicBody;
        }
    check(
        target >= 0 && count == 10 && off == 5 && on == 5 && initialized,
        "All ten authored Forcefields touch blocks initialize native0.1 age and kinematic bodies");
    if (target < 0)
        return;
    target = touchAlternatorNativeFixture();
    touchAlternatorNativePlace(target, 50);
    observer.target = target;
    observer.solved = 0;
    observer.normalY = -2;
    physicsWorld->SetContactListener(&observer);
    int before = alternationBits;
    tick(dt, false, false);
    bool upward = observer.solved && observer.normalY > .9f &&
                  alternationBits == (before ^ entities[target].mask) &&
                  entities[playerIndex].state.vy > 0 && jumps == 0;
    physicsWorld->SetContactListener(originalContacts);
    check(
        upward,
        "A real solved upward-settling Tom top contact toggles once without inventing an automatic jump");
    target = touchAlternatorNativeFixture(false);
    touchAlternatorNativePlace(target, -30);
    before = alternationBits;
    tick(dt, false, false);
    tick(dt, false, false);
    check(
        alternationBits == (before ^ entities[target].mask) && jumps == 1 &&
            entities[playerIndex].state.vy > 200,
        "Normal descending contact on the original geometry keeps its single jump and alternation");
    target = touchAlternatorNativeFixture();
    touchAlternatorNativePlace(target, -30);
    int mask = entities[target].mask;
    alternateScene(mask);
    alternateScene(mask);
    before = alternationBits;
    observer.target = target;
    observer.solved = 0;
    observer.normalY = -2;
    physicsWorld->SetContactListener(&observer);
    tick(dt, false, false);
    bool guarded = observer.solved && observer.normalY > .9f && alternationBits == before &&
                   jumps == 0 && ordinaryJumpStateNative.pending;
    physicsWorld->SetContactListener(originalContacts);
    for (int i = 0; i < 15; i++)
        tick(dt, false, false);
    check(
        guarded && alternationBits == before,
        "A real contact inside the post-alternation guard is rejected and its latch cannot fire later without a new contact");
    bool matching = true;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch)
            entities[i].state.timer = .75f;
    alternateScene(1);
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch)
            matching &= entities[i].state.timer == .75f;
    alternateScene(2);
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch)
            matching &= entities[i].state.timer == 0;
    check(
        matching,
        "Only an overlapping alternation mask resets every matching touch age, including inactive targets");
    target = touchAlternatorNativeFixture();
    touchAlternatorNativePlace(target, -30);
    entities[target].state.timer = .1f;
    touchAlternatorPending[target] = true;
    before = alternationBits;
    touchAlternatorPostStepNative(0);
    bool strict = alternationBits == before && !touchAlternatorPending[target];
    touchAlternatorPostStepNative(.02f);
    strict &= alternationBits == before;
    check(
        strict,
        "The exact0.1 endpoint is strict and rejected pending state is consumed, rather than delayed");
    target = touchAlternatorNativeFixture();
    touchAlternatorNativePlace(target, -30);
    alternateScene(2);
    entities[target].state.timer = .012f;
    captureCheckpoint(false);
    int savedBits = checkpoint.bits;
    bool saved = checkpoint.version == 28 && validSnapshot(checkpoint);
    alternateScene(2);
    touchAlternatorPending[target] = true;
    float currentAges[MAX_ENTITIES];
    bool currentPending[MAX_ENTITIES];
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch) {
            currentAges[i] = entities[i].state.timer;
            currentPending[i] = touchAlternatorPending[i];
        }
    restoreCheckpoint();
    bool restored = saved && alternationBits == savedBits;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch)
            restored &= entities[i].state.timer == currentAges[i] &&
                        touchAlternatorPending[i] == currentPending[i] &&
                        (!bodies[i] || bodies[i]->IsActive() == activeBlock(entities[i]));
    check(
        restored,
        "Version28 checkpoint restores touch on/off parity while retaining current native unsaved cooldown and pending contact");
    // Fixture-intrinsic rotation yields a genuine corner/sloped manifold;
    // changing only Actor angle would exercise the wrong matrix-angle path.
    target = touchAlternatorNativeFixture();
    touchAlternatorNativePlace(target, -30);
    Entity &block = entities[target];
    Entity &bean = entities[playerIndex];
    b2Body *body = bodies[target];
    b2FixtureDef fd;
    fd.friction = .2f;
    b2PolygonShape shape;
    Rect r = block.hit;
    float angle = 35 * .017453293f;
    shape.SetAsBox((r.r - r.l) * .5f * metres, (r.t - r.b) * .5f * metres, b2Vec2(0, 0), angle);
    fd.shape = &shape;
    while (body->GetFixtureList())
        body->DestroyFixture(body->GetFixtureList());
    body->CreateFixture(&fd);
    float radius = (r.t - r.b) * .5f - bean.hit.b - .5f;
    bean.state.x = block.state.x - sine(angle) * radius;
    bean.state.y = block.state.y + cosine(angle) * radius;
    bodies[playerIndex]->SetTransform(b2Vec2(bean.state.x * metres, bean.state.y * metres), 0);
    observer.target = target;
    observer.solved = 0;
    observer.normalY = -2;
    physicsWorld->SetContactListener(&observer);
    before = alternationBits;
    tick(dt, false, false);
    physicsWorld->SetContactListener(originalContacts);
    check(
        observer.solved && observer.normalY > .5f && observer.normalY < .9f &&
            alternationBits == before,
        "A real sloped manifold below native0.9 cannot toggle merely because Tom classifies it as ground");
}
