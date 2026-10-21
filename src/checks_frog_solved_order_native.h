#pragma once

// Regression coverage for frog solved order native. Runs only in the isolated BQ_ENABLE_TESTS executable.

// Independent callback observers: the native PostSolve clears +0x70 in the
// callback itself, while an already scheduled DoRecalculate still executes.
class FrogSolvedOrderObserverNative : public OriginalContacts {
  public:
    int ball, partner, linked, solved;
    bool immediate;

    FrogSolvedOrderObserverNative(int b, int p)
        : ball(b), partner(p), linked(0), solved(0), immediate(true) {}

    bool contains(b2Contact *c, b2Body *b) {
        return c->GetFixtureA()->GetBody() == b || c->GetFixtureB()->GetBody() == b;
    }

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        OriginalContacts::PreSolve(c, old);
        if (contains(c, bodies[ball]) && contains(c, bodies[partner]) && !c->IsEnabled())
            linked++;
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        OriginalContacts::PostSolve(c, impulse);
        if (contains(c, bodies[ball])) {
            solved++;
            immediate &= entities[ball].state.hp == 0;
        }
    }
};

b2Body *frogSolvedOrderWallNative(b2World &world, Entity &ball, b2Body &body, bool sensor) {
    float radius = 0;
    for (b2Fixture *f = body.GetFixtureList(); f; f = f->GetNext())
        if (!f->IsSensor() && f->GetShape()->GetType() == b2Shape::e_circle)
            radius = ((b2CircleShape *)f->GetShape())->m_radius / metres;
    b2BodyDef def;
    def.position.Set((ball.state.x + radius + 2 - .25f) * metres, ball.state.y * metres);
    b2Body *wall = world.CreateBody(&def);
    b2PolygonShape shape;
    shape.SetAsBox(2 * metres, 40 * metres);
    b2FixtureDef fixture;
    fixture.shape = &shape;
    fixture.isSensor = sensor;
    wall->CreateFixture(&fixture);
    return wall;
}

int frogSolvedOrderPrepareNative() {
    interfaceEnabled = musicEnabled = soundEnabled = debugMode = false;
    startStage(stageById(5770908237787719ULL));
    readyTimer = 0;
    if (frogLinkCount != 1)
        return -1;
    FrogLink &link = frogLinks[0];
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && i != link.ball && i != link.left && i != link.right)
            entities[i].state.gone = 1;
    Entity &left = entities[link.left];
    Entity &right = entities[link.right];
    Entity &ball = entities[link.ball];
    left.state.x = 500;
    left.state.y = 500;
    right.state.x = 800;
    right.state.y = 500;
    left.state.active = right.state.active = 1;
    left.state.vx = left.state.vy = right.state.vx = right.state.vy = 0;
    ball.state.x = 530;
    ball.state.y = 530;
    ball.state.vx = ball.state.vy = ball.state.aux = 0;
    ball.state.hp = 1;
    ball.state.active = 0;
    ball.state.timer = .8f;
    entities[playerIndex].state.x = 100;
    entities[playerIndex].state.y = 1000;
    entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
    rebuildPhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    resetBurstRandomNative(0, 5489);
    return link.ball;
}

class CreditsFrogSolvedOrderObserverNative : public CreditsContactsNative {
  public:
    int linked, solved;
    bool immediate;
    float phase;

    explicit CreditsFrogSolvedOrderObserverNative(UiScene &s)
        : CreditsContactsNative(s), linked(0), solved(0), immediate(true), phase(0) {}

    bool contains(b2Contact *c, int i) {
        return c->GetFixtureA()->GetBody() == scene->bodies[i] ||
               c->GetFixtureB()->GetBody() == scene->bodies[i];
    }

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        CreditsContactsNative::PreSolve(c, old);
        if (contains(c, creditsRuntime.frogBall) && contains(c, creditsRuntime.frogLeft) &&
            !c->IsEnabled())
            linked++;
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        CreditsContactsNative::PostSolve(c, impulse);
        if (contains(c, creditsRuntime.frogBall)) {
            solved++;
            immediate &= scene->playfield[creditsRuntime.frogBall].state.hp == 0;
            phase = scene->playfield[creditsRuntime.frogBall].state.timer;
        }
    }
};

void frogSolvedOrderNativeChecks() {
    const float dt = 1 / 120.0f;
    bool original = true, simultaneous = true, immediate = true, lastThrow = true, removed = true,
         once = true, restored = true, controls = true;
    char trace[4096] = "case,linked,solved,immediate,mode,phase_millis,mt_index\r\n";
    for (int mode = 0; mode < 5; mode++) {
        int index = frogSolvedOrderPrepareNative();
        if (index < 0) {
            original = false;
            continue;
        }
        FrogLink &link = frogLinks[0];
        Entity &ball = entities[index];
        original &= ball.source->id == 5866742188234202ULL;
        FrogSolvedOrderObserverNative observer(index, link.left);
        physicsWorld->SetContactListener(&observer);
        b2Body *wall =
            mode ? frogSolvedOrderWallNative(*physicsWorld, ball, *bodies[index], mode == 4) : 0;
        if (mode == 3)
            captureCheckpoint(false);
        simulateBodies(dt);
        int queued = frogPending[index];
        if (mode == 1 || mode == 2) {
            simultaneous &= observer.linked > 0 && observer.solved > 0 && queued == 1;
            immediate &= observer.immediate;
        }
        if (mode == 2) {
            physicsWorld->DestroyBody(wall);
            wall = 0;
        }
        if (mode == 3) {
            restoreCheckpoint();
            float phase = ball.state.timer;
            frogPostStep();
            restored &= ball.state.hp == 1 && ball.state.timer == phase &&
                        frogPending[index] == 0 && burstRandomNative[0].engine.index == 624;
        } else {
            Vec from = {ball.state.x, ball.state.y};
            float expectedTime = link.period * .5f - (ball.state.timer - link.period * .5f);
            float vx = (link.rightTarget.x - from.x) / expectedTime,
                  vy = (link.rightTarget.y - from.y) / expectedTime -
                       .5f * nativeWorldGravity * expectedTime;
            frogPostStep();
            bool thrown = absf(ball.state.timer + .2f) < .00001f &&
                          absf(ball.state.vx - vx) < .001f && absf(ball.state.vy - vy) < .001f &&
                          burstRandomNative[0].engine.index == 1;
            if (mode == 1)
                lastThrow &= ball.state.hp == 0 && thrown;
            if (mode == 2)
                removed &= ball.state.hp == 0 && thrown;
            if (mode == 0 || mode == 4)
                controls &=
                    observer.linked > 0 && observer.solved == 0 && ball.state.hp == 1 && thrown;
            float phase = ball.state.timer;
            frogPostStep();
            once &= ball.state.timer == phase && burstRandomNative[0].engine.index == 1;
        }
        char row[180];
        wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d\r\n", mode, observer.linked, observer.solved,
                  observer.immediate, ball.state.hp, (int)(ball.state.timer * 1000),
                  burstRandomNative[0].engine.index);
        lstrcatA(trace, row);
        physicsWorld->SetContactListener(originalContacts);
        if (wall)
            physicsWorld->DestroyBody(wall);
    }
    check(
        original && simultaneous,
        "Original Moonlight Oasis actors generate both a disabled linked-frog contact and a real solved wall contact in one physics step");
    check(
        immediate,
        "The actual ball PostSolve ends cooperation inside the callback rather than waiting for a later contact-list scan");
    check(
        lastThrow,
        "A scheduled linked-frog recalculation still executes once after a simultaneous solid impact has ended cooperation");
    check(
        removed,
        "Removing the solved obstacle before post-step cannot erase the native mode termination or the already scheduled final throw");
    check(
        controls && once,
        "Linked-frog-only and opposite-sensor contacts retain cooperation and consume exactly one scheduled throw without replay");
    check(
        restored,
        "Restoring a checkpoint discards an unconsumed frog callback without changing restored phase or drawing a spin");
    u32 campaign = hashBytes(entities, sizeof(Entity) * entityCount);
    interfaceEnabled = true;
    navigate(137502285691ULL);
    int bi = creditsRuntime.frogBall, li = creditsRuntime.frogLeft, ri = creditsRuntime.frogRight;
    bool ending = false;
    if (bi >= 0 && li >= 0 && ri >= 0) {
        for (int i = 0; i < page.playfieldCount; i++)
            if (i != bi && i != li && i != ri)
                page.playfield[i].state.gone = 1;
        Entity &ball = page.playfield[bi];
        Entity &left = page.playfield[li];
        Entity &right = page.playfield[ri];
        left.state.x = 2000;
        left.state.y = 4500;
        right.state.x = 2300;
        right.state.y = 4500;
        left.state.vx = left.state.vy = right.state.vx = right.state.vy = 0;
        left.state.active = right.state.active = 1;
        ball.state.x = 2030;
        ball.state.y = 4530;
        ball.state.vx = ball.state.vy = ball.state.aux = 0;
        ball.state.timer = .8f;
        ball.state.hp = 1;
        ball.state.active = 0;
        page.bean.state.x = 1000;
        page.bean.state.y = 5000;
        page.bean.state.vx = page.bean.state.vy = 0;
        resetUiPhysics(page);
        page.bodies[page.playfieldCount]->SetType(b2_staticBody);
        CreditsFrogSolvedOrderObserverNative *observer =
            new CreditsFrogSolvedOrderObserverNative(page);
        delete page.contacts;
        page.contacts = observer;
        page.physics->SetContactListener(observer);
        b2Body *wall = frogSolvedOrderWallNative(*page.physics, ball, *page.bodies[bi], false);
        clearEffects(1);
        resetBurstRandomNative(1, 5489);
        tickCreditsRuntime(page, dt);
        ending = observer->linked > 0 && observer->solved > 0 && observer->immediate &&
                 ball.state.hp == 0 &&
                 absf(ball.state.timer - (observer->phase - creditsRuntime.frogPeriod * .5f)) <
                     .00001f &&
                 burstRandomNative[1].engine.index == 1;
        page.physics->DestroyBody(wall);
    }
    check(
        ending && campaign == hashBytes(entities, sizeof(Entity) * entityCount),
        "The actual ending frog ball preserves the same solved-event and final-throw ordering in its separate physics and random domain");
    atomicWrite("test-output/frog-solved-order-observed.csv", trace, lstrlenA(trace));
    interfaceEnabled = false;
    startStage(0);
    readyTimer = 0;
    atomicWrite("test-output/frog-solved-order-results.txt", testLog, lstrlenA(testLog));
}
