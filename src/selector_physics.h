#pragma once

// Owns the selector background physics world, its contact queues and post-step launches.
#include "trampoline_native.h"

void selectorSpecialPostSolveNative(UiScene &s, b2Contact *contact);
void selectorPinballPreSolveNative(UiScene &s, b2Contact *contact);

// A selector is a distinct original LevelTask. It shares map fixtures and
// fluid equations with a stage, while owning every body, contact and force.
// User-requested impact feedback also covers the native sensor-spike
// affector path. Keep this queue in the owning UI world.
void helperSpikeBeginContactUiNative(b2Contact *contact, UiScene &scene, bool *stopped) {
    b2Fixture *fs[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++) {
        int source = (int)fs[side]->GetBody()->GetUserData() - 1,
            other = (int)fs[1 - side]->GetBody()->GetUserData() - 1;
        if (source < 0 || source >= scene.playfieldCount || other < 0 ||
            other >= scene.playfieldCount || fs[1 - side]->IsSensor())
            continue;
        Entity &spike = scene.playfield[source];
        Entity &bird = scene.playfield[other];
        if (spike.kind == SPIKE && spike.source->has("SpikeGroupBehaviour") && !spike.state.gone &&
            spike.state.hp && bird.kind == HELPER && !bird.state.gone &&
            fs[1 - side]->GetBody() == scene.bodies[other])
            stopped[other] = true;
    }
}

class UiContacts : public b2ContactListener {
  public:
    JumperGroundFixture *jumperGround[513];
    CreaturePressureState<513> creaturePressure;
    SingleCollectableEventNative collectables[1024];
    int collectableCount;
    bool coinPending[513];
    PinballDynamicQueueNative pinballDynamic;
    UiScene *scene;
    SpringIncomingVelocity incoming[1024];
    int incomingCount;
    bool magnetSolved, spikeBeanPending;
    int trampoline;
    float trampolineY;
    float tomIncomingVy, tomSolvedVx;
    Vec tomNormal;
    bool tomNormalFound;
    bool helperSolved[513], helperEnemyPending[513], helperStopPending[513];

    explicit UiContacts(UiScene &value)
        : scene(&value), incomingCount(0), magnetSolved(false), spikeBeanPending(false),
          trampoline(-1), trampolineY(0), tomIncomingVy(0), tomSolvedVx(0), tomNormalFound(false) {
        memset(jumperGround, 0, sizeof(jumperGround));
        resetCreaturePressure(creaturePressure);
        tomNormal = Vec{0, 1};
        memset(helperSolved, 0, sizeof(helperSolved));
        memset(helperEnemyPending, 0, sizeof(helperEnemyPending));
        memset(helperStopPending, 0, sizeof(helperStopPending));
        resetPinballDynamicNative(pinballDynamic);
        collectableCount = 0;
        memset(coinPending, 0, sizeof(coinPending));
    }

    ~UiContacts() {
        clearJumperGroundNative(jumperGround, 513);
    }

    void splash(b2Contact *contact) {
        UiScene &s = *scene;
        if (s.waterStartup)
            return;
        b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
        for (int side = 0; side < 2; side++)
            if (fixtures[side]->GetUserData() == (void *)7) {
                int i = (int)fixtures[side]->GetBody()->GetUserData() - 1;
                if (i >= 0 && i < s.playfieldCount)
                    waterSplash(s.playfield[i], *fixtures[1 - side]->GetBody(), 1);
            }
    }

    void BeginContact(b2Contact *contact) {
        recordCollectableContactNative(contact, scene->playfield, scene->playfieldCount + 1,
                                       scene->playfieldCount, collectables, collectableCount, 1024,
                                       coinPending, false);
        helperPerchBeginContactNative(contact, scene->playfield, scene->playfieldCount + 1,
                                      scene->playfieldCount);
        beginOneWayContact(contact);
        helperSpikeBeginContactUiNative(contact, *scene, helperStopPending);
        splash(contact);
        nativeWaterContact(contact, true);
        b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
        for (int side = 0; side < 2; side++) {
            int source = (int)fixtures[side]->GetBody()->GetUserData() - 1;
            int target = (int)fixtures[1 - side]->GetBody()->GetUserData() - 1;
            if (source >= 0 && source < scene->playfieldCount && target == scene->playfieldCount &&
                !fixtures[1 - side]->IsSensor()) {
                Entity &spike = scene->playfield[source];
                if (spike.kind == SPIKE && !spike.state.gone && spike.state.hp)
                    spikeBeanPending = true;
            }
        }
    }

    void EndContact(b2Contact *contact) {
        endJumperGroundNative(contact, scene->bodies, jumperGround, scene->playfieldCount + 1);
        splash(contact);
        nativeWaterContact(contact, false);
        forgetOneWayContact(contact);
    }

    void PreSolve(b2Contact *contact, const b2Manifold *) {
        tomPreSolve(contact, scene->playfield, scene->playfieldCount + 1, &tomIncomingVy);
        oneWayContacts(contact, scene->playfield, scene->playfieldCount + 1);
        recordHelperEnemyContactNative(contact, scene->playfield, scene->playfieldCount + 1,
                                       scene->bodies, helperEnemyPending, helperStopPending);
        oscillatorProxyContacts(contact);
        nativeMotionProxyPreSolve(contact, scene->playfield, scene->playfieldCount + 1,
                                  scene->diagonal, scene->oscillator);
        recordSpringIncoming(contact, scene->playfield, scene->playfieldCount + 1, incoming,
                             incomingCount, 1024);
        recordPinballDynamicNative(contact, scene->playfield, scene->playfieldCount + 1,
                                   scene->playfieldCount, pinballDynamic);
        selectorPinballPreSolveNative(*scene, contact);
    }

    void PostSolve(b2Contact *contact, const b2ContactImpulse *impulse) {
        creaturePressurePostSolve(contact, impulse, scene->playfield, scene->playfieldCount + 1,
                                  scene->bodies, creaturePressure, helperEnemyPending);
        oneWaySupportPostSolveNative(contact);
        recordJumperGroundNative(contact, scene->playfield, scene->playfieldCount + 1,
                                 scene->bodies, jumperGround);
        walkPostSolveNative(contact, scene->playfield, scene->playfieldCount);
        recordHelperSolvedContactNative(contact, scene->playfield, scene->playfieldCount + 1,
                                        helperSolved);
        tomOrdinaryPostSolveNative(contact, scene->bodies[scene->playfieldCount], tomIncomingVy,
                                   tomNormal, tomNormalFound, tomSolvedVx,
                                   &scene->specialJump.ordinary, &scene->bean);
        int pad =
            nativeTrampolineSolvedContact(contact, scene->playfield, scene->playfieldCount + 1,
                                          scene->playfieldCount, trampolineY);
        if (pad >= 0)
            trampoline = pad;
        recordSpringPostSolve(contact, incoming, incomingCount);
        selectorSpecialPostSolveNative(*scene, contact);
        crumblingCloudPostSolve(contact, scene->playfield, scene->playfieldCount + 1,
                                scene->playfieldCount);
        magnetSolved |= magnetPostSolve(contact, scene->playfield, scene->playfieldCount + 1,
                                        scene->playfieldCount);
    }
};

void selectorPinballPreSolveNative(UiScene &s, b2Contact *contact) {
    if (!contact->IsEnabled() || !contact->GetManifold()->pointCount)
        return;
    b2Fixture *fs[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    if (fs[0]->IsSensor() || fs[1]->IsSensor())
        return;
    for (int side = 0; side < 2; side++) {
        int source = (int)fs[side]->GetBody()->GetUserData() - 1,
            target = (int)fs[1 - side]->GetBody()->GetUserData() - 1;
        if (source < 0 || source >= s.playfieldCount || target != s.playfieldCount ||
            s.playfield[source].kind != BOUNCER || s.playfield[source].state.gone)
            continue;
        b2WorldManifold manifold;
        contact->GetWorldManifold(&manifold);
        Vec normal = {side ? -manifold.normal.x : manifold.normal.x,
                      side ? -manifold.normal.y : manifold.normal.y};
        Entity &pad = s.playfield[source];
        const Props &p = *s.bean.physics;
        s.specialJump.mega = PinballPendingNative{
            1, source, nativePinballNormal(normal),
            nativePinballJumpSpeed(
                p.num("gravity", -200), p.num("jump-height", 37), 0,
                pad.source->behavior("PinballBouncerBehaviour")->num("jump-power", 200))};
        s.specialJump.contact = true;
        pad.state.active = 2;
        pad.state.timer = 0;
        sound("pinball");
    }
}

void selectorSpecialPostSolveNative(UiScene &s, b2Contact *contact) {
    float y = 0;
    int source = nativeTrampolineSolvedContact(contact, s.playfield, s.playfieldCount + 1,
                                               s.playfieldCount, y);
    const Props &p = *s.bean.physics;
    if (source >= 0) {
        s.specialJump.jelly = TrampolineStateNative{1, source, s.trampolineApex, 0};
        s.specialJump.jelly.speed = nativeTrampolineLaunch(
            y, p.num("gravity", -200), p.num("jump-height", 37), 0, s.trampolineApex);
        s.specialJump.jelly.apex = s.trampolineApex;
        s.specialJump.contact = true;
    }
    if (!contact->IsEnabled() || contact->GetFixtureA()->IsSensor() ||
        contact->GetFixtureB()->IsSensor() || !contact->GetManifold()->pointCount)
        return;
    for (int i = 0; i < s.contacts->incomingCount; i++) {
        SpringIncomingVelocity &impact = s.contacts->incoming[i];
        if (impact.contact != contact || !impact.solved || impact.target != s.playfieldCount ||
            impact.y >= 0)
            continue;
        b2WorldManifold m;
        contact->GetWorldManifold(&m);
        float ny = contact->GetFixtureB() == impact.other ? m.normal.y : -m.normal.y;
        if (ny <= .1f)
            continue;
        Entity &pad = s.playfield[impact.source];
        s.specialJump.mega = PinballPendingNative{1, impact.source, Vec{0, 1},
                                                  nativePinballJumpSpeed(p.num("gravity", -200),
                                                                         p.num("jump-height", 37),
                                                                         0, pad.jumpPower)};
        s.specialJump.contact = true;
        markMatrixContactActlets(s.playfield, s.playfieldCount, pad, m,
                                 contact->GetManifold()->pointCount, true);
        sound("megajump");
    }
}

// Consume the previous solved launch request once; interactive selector play enables ordinary jump audio.
void selectorSpecialConsumeNative(UiScene &s, float dt) {
    SelectorSpecialJumpNative &state = s.specialJump;
    PinballPendingNative mega = state.mega;
    TrampolineStateNative jelly = state.jelly;
    OrdinaryJumpStateNative ordinary = state.ordinary;
    state.ordinary = OrdinaryJumpStateNative{};
    state.mega.pending = state.jelly.pending = 0;
    state.launchedSource = -1;
    state.launchedKind = 0;
    float oldAge = s.bean.state.timer;
    s.bean.state.timer += dt;
    if (oldAge < .05f || s.bean.state.gone || s.miniDeath > 0)
        return;
    float speed = 0;
    Vec normal = {0, 1};
    if (mega.pending) {
        if (!validPinballPendingNative(mega, s.playfieldCount))
            return;
        state.launchedKind = 1;
        state.launchedSource = mega.source;
        speed = mega.speed;
        normal = mega.normal;
    } else if (jelly.pending) {
        if (!validTrampolineStateNative(jelly, s.playfieldCount))
            return;
        state.launchedKind = 2;
        state.launchedSource = jelly.source;
        speed = jelly.speed;
        const Props &p = *s.bean.physics;
        sound("jump", nativeTrampolineSoundRateFromSpeed(speed, p.num("gravity", -200),
                                                         p.num("jump-height", 37)));
    } else if (ordinary.pending && !ordinary.magnet) {
        state.launchedKind = 3;
        speed = ordinary.speed;
        normal = ordinary.direction;
        s.ordinaryJumpNative = true;
        emitEffect(idnum(s.bean.physics->get("dust-actor")), ordinary.point.x, ordinary.point.y,
                   s.bean.layer, .5f, 1);
        nativeTrampolineOrdinaryJump(s.bean.state.y, s.bean.physics->num("jump-height", 37), 0,
                                     s.trampolineApex);
        if (s.selectorLayer < 0 || s.minimized)
            sound("jump");
    }
    if (state.launchedKind) {
        s.bean.state.vx += speed * normal.x;
        s.bean.state.vy = speed * normal.y;
        s.bean.lastLaunchSpeed = speed;
        s.bean.state.timer = 0;
        s.bean.state.active = 1;
    }
}

void selectorGravityNative(UiScene &s) {
    int helper = -1;
    for (int i = 0; i < s.playfieldCount; i++)
        if (s.playfield[i].kind == HELPER && s.playfield[i].state.active == 1)
            helper = i;
    float cap =
        helper >= 0
            ? s.playfield[helper].source->behavior("HelperBirdBehaviour")->num("max-fly-speed", 60)
            : 0;
    nativeTomGravityForce(*s.bodies[s.playfieldCount], s.bean.physics->num("gravity", -200),
                          helper >= 0, s.bean.state.vy, cap);
}

void destroyUiPhysics(UiScene &s) {
    resetSelectorSpecialJumpNative(s.specialJump);
    s.ordinaryJumpNative = false;
    s.waterStartup = true;
    if (s.physics) {
        clearNativeWaterWorld(s.physics);
        clearOneWayWorld(s.physics);
        delete s.physics;
        s.physics = 0;
    }
    if (s.contacts) {
        delete s.contacts;
        s.contacts = 0;
    }
    s.bodies = 0;
    s.inWater = false;
}

// Keep bodies and deferred contacts owned by this UI scene so they cannot modify campaign checkpoints.
void createUiPhysics(UiScene &s) {
    resetSelectorSpecialJumpNative(s.specialJump);
    s.ordinaryJumpNative = false;
    if (!s.hasBean)
        return;
    s.physics = new b2World(b2Vec2(0, nativeWorldGravity * metres), true);
    s.contacts = new UiContacts(s);
    s.physics->SetContactListener(s.contacts);
    s.waterStartup = true;
    s.inWater = false;
    s.bodies = s.memory.array<b2Body *>(s.playfieldCount + 1);
    s.oscillatorProxies = s.memory.array<b2Body *>(s.playfieldCount + 1);
    s.playfield[s.playfieldCount] = s.bean;
    s.diagonal = s.memory.array<DiagonalRuntimeState>(s.playfieldCount + 1);
    s.oscillator = s.memory.array<OscillatorRuntimeState>(s.playfieldCount + 1);
    createPhysicsTerrain(*s.physics, s.level);
    for (int i = 0; i <= s.playfieldCount; i++) {
        resetNativeMotionState(s.playfield[i], s.diagonal[i], s.oscillator[i]);
        s.bodies[i] =
            createPhysicsEntity(*s.physics, s.playfield[i], i, 0, &s.oscillatorProxies[i]);
    }
    restoreCoinGroupActivityNative(s.playfield, s.playfieldCount + 1, s.bodies);
}

void uiGroupCollectablePostStepNative(UiScene &s) {
    int count = s.contacts->collectableCount;
    s.contacts->collectableCount = 0;
    for (int n = 0; n < count; n++) {
        const SingleCollectableEventNative &event = s.contacts->collectables[n];
        int i = event.source;
        if (i < 0 || i >= s.playfieldCount || s.bodies[i] != event.body)
            continue;
        Entity &e = s.playfield[i];
        if (e.kind != COIN)
            continue;
        e.state.gone = 1;
        s.miniCoins++;
        sound("coin");
        Object *base = effectClass(e.source->klass);
        emitEffect(idnum(e.source->behavior("CoinGroupBehaviour")->get("effect")), e.state.x,
                   e.state.y, e.layer,
                   (e.local.r - e.local.l) / maxf(1, base ? base->rect.r - base->rect.l : 32), 1);
        coinGroupActivityNative(e.source, s.playfield, s.playfieldCount + 1, s.bodies,
                                s.contacts->coinPending);
    }
    memset(s.contacts->coinPending, 0, sizeof(s.contacts->coinPending));
}

void resetUiPhysics(UiScene &s) {
    resetSelectorSpecialJumpNative(s.specialJump);
    s.ordinaryJumpNative = false;
    s.waterStartup = true;
    s.inWater = false;
    if (!s.physics)
        return;
    clearJumperGroundNative(s.contacts->jumperGround, 513);
    s.contacts->collectableCount = 0;
    memset(s.contacts->coinPending, 0, sizeof(s.contacts->coinPending));
    resetPinballDynamicNative(s.contacts->pinballDynamic);
    resetCreaturePressure(s.contacts->creaturePressure);
    memset(s.contacts->helperSolved, 0, sizeof(s.contacts->helperSolved));
    memset(s.contacts->helperEnemyPending, 0, sizeof(s.contacts->helperEnemyPending));
    memset(s.contacts->helperStopPending, 0, sizeof(s.contacts->helperStopPending));
    s.physics->ClearForces();
    clearOneWayWorld(s.physics);
    s.playfield[s.playfieldCount] = s.bean;
    for (int i = 0; i <= s.playfieldCount; i++)
        if (s.bodies[i]) {
            Entity &e = s.playfield[i];
            b2Body &body = *s.bodies[i];
            if (boxGroupMatrix(e))
                e.state.angle = e.source->angle;
            body.SetTransform(b2Vec2(e.state.x * metres, e.state.y * metres),
                              nativePhysicsAngle(e));
            body.SetLinearVelocity(b2Vec2(e.state.vx * metres, e.state.vy * metres));
            body.SetAwake(true);
        }
    for (int i = 0; i <= s.playfieldCount; i++)
        if (s.oscillatorProxies[i]) {
            s.oscillatorProxies[i]->SetTransform(s.bodies[i]->GetPosition(),
                                                 s.bodies[i]->GetAngle());
            s.oscillatorProxies[i]->SetLinearVelocity(s.bodies[i]->GetLinearVelocity());
        }
}

// Run one selector simulation step with scene-local queues, then copy solved motion back to UI actors.
bool stepUiPhysics(UiScene &s, float dt) {
    s.ordinaryJumpNative = false;
    if (!s.physics || !s.bodies[s.playfieldCount] || dt <= 0)
        return false;
    selectorSpecialConsumeNative(s, dt);
    s.playfield[s.playfieldCount] = s.bean;
    nativeTrampolineObserve(s.bean.state.y, s.trampolineApex);
    for (int i = 0; i < s.playfieldCount; i++)
        if (s.playfield[i].kind == MAGNET)
            s.playfield[i].state.active = 1;
    for (int i = 0; i <= s.playfieldCount; i++)
        if (s.bodies[i]) {
            Entity &e = s.playfield[i];
            b2Body &body = *s.bodies[i];
            body.SetActive(
                (e.kind == COIN
                     ? coinGroupAvailableNative(e.source, s.playfield, s.playfieldCount + 1)
                     : !e.state.gone) &&
                !(e.crumble && e.state.active == 2) && (!proximityControlled(e) || e.state.active));
            if (s.oscillatorProxies[i])
                s.oscillatorProxies[i]->SetActive(body.IsActive());
            if (!body.IsActive())
                continue;
            b2Vec2 target(e.state.x * metres, e.state.y * metres);
            float angle = nativePhysicsAngle(e);
            if (body.GetType() == b2_kinematicBody) {
                if ((e.move || e.source->has("DiagonalBehaviour")) &&
                    (target - body.GetPosition()).LengthSquared() >
                        (0.5f * metres) * (0.5f * metres)) {
                    body.SetTransform(target, angle);
                    if (s.oscillatorProxies[i])
                        s.oscillatorProxies[i]->SetTransform(target, angle);
                }
                body.SetLinearVelocity((e.move || e.source->has("DiagonalBehaviour"))
                                           ? b2Vec2(e.state.vx * metres, e.state.vy * metres)
                                           : (1 / dt) * (target - body.GetPosition()));
                body.SetAngularVelocity((angle - body.GetAngle()) / dt);
            } else if (body.GetType() == b2_dynamicBody) {
                if ((target - body.GetPosition()).LengthSquared() >
                    (0.5f * metres) * (0.5f * metres))
                    body.SetTransform(target, angle);
                body.SetLinearVelocity(b2Vec2(e.state.vx * metres, e.state.vy * metres));
            } else
                body.SetTransform(target, angle);
        }
    memset(s.contacts->helperSolved, 0, sizeof(s.contacts->helperSolved));
    memset(s.contacts->helperEnemyPending, 0, sizeof(s.contacts->helperEnemyPending));
    memset(s.contacts->helperStopPending, 0, sizeof(s.contacts->helperStopPending));
    s.contacts->incomingCount = 0;
    s.contacts->collectableCount = 0;
    memset(s.contacts->coinPending, 0, sizeof(s.contacts->coinPending));
    resetPinballDynamicNative(s.contacts->pinballDynamic);
    s.contacts->trampoline = -1;
    s.contacts->magnetSolved = false;
    s.contacts->spikeBeanPending = false;
    s.contacts->tomNormalFound = false;
    s.specialJump.contact = false;
    resetCreaturePressure(s.contacts->creaturePressure);
    selectorGravityNative(s);
    s.physics->Step(dt, 10, 10);
    s.physics->ClearForces();
    for (int i = 0; i < s.playfieldCount; i++)
        if (s.contacts->helperEnemyPending[i]) {
            s.contacts->helperEnemyPending[i] = false;
            killEnemy(s.playfield[i]);
            if (s.bodies[i])
                s.bodies[i]->SetActive(false);
            if (s.oscillatorProxies[i])
                s.oscillatorProxies[i]->SetActive(false);
            for (int j = 0; j < s.playfieldCount; j++)
                if (s.playfield[j].parent == i) {
                    s.playfield[j].state.gone = s.playfield[i].state.gone;
                    if (s.bodies[j])
                        s.bodies[j]->SetActive(false);
                }
        }
    for (int i = 0; i <= s.playfieldCount; i++)
        if (s.bodies[i] && s.bodies[i]->IsActive()) {
            Entity &e = s.playfield[i];
            b2Body &body = *s.bodies[i];
            b2Vec2 position = body.GetPosition(), velocity = body.GetLinearVelocity();
            e.state.x = position.x / metres;
            e.state.y = position.y / metres;
            e.state.vx = velocity.x / metres;
            e.state.vy = velocity.y / metres;
            if (!boxGroupMatrix(e))
                e.state.angle = body.GetAngle() * 57.29578f;
        }
    b2Body *bean = s.bodies[s.playfieldCount];
    bool grounded = false, magnetContact = s.contacts->magnetSolved;
    for (b2ContactEdge *edge = bean->GetContactList(); edge; edge = edge->next) {
        b2Contact *c = edge->contact;
        if (!c->IsTouching() || !c->IsEnabled() || !c->GetManifold()->pointCount ||
            c->GetFixtureA()->IsSensor() || c->GetFixtureB()->IsSensor())
            continue;
        b2WorldManifold manifold;
        c->GetWorldManifold(&manifold);
        float normalY =
            c->GetFixtureA()->GetBody() == bean ? -manifold.normal.y : manifold.normal.y;
        // Magnetic attachment is latched only by UiContacts::PostSolve.
        if (normalY >= 0.5f)
            grounded = true;
    }
    Entity &player = s.playfield[s.playfieldCount];
    player.state.active = magnetContact ? 2 : 1;
    s.specialJump.ordinary.magnet |= magnetContact ? 1 : 0;
    for (int i = 0; i < s.contacts->incomingCount; i++) {
        SpringIncomingVelocity &impact = s.contacts->incoming[i];
        int target = impact.target;
        if (!impact.solved || target < 0 || target >= s.playfieldCount || !s.bodies[target] ||
            !s.bodies[target]->IsActive() || s.playfield[target].state.gone ||
            s.bodies[target]->GetType() != b2_dynamicBody)
            continue;
        Entity &pad = s.playfield[impact.source];
        if (launchDynamic(pad, s.playfield[target], *s.bodies[target], impact.manifold.normal,
                          pad.source->behavior("SpringBehaviour")->num("physics-power", 200)))
            markMatrixContactActlets(s.playfield, s.playfieldCount, pad, impact.manifold,
                                     impact.points, true);
    }
    for (int i = 0; i < s.playfieldCount; i++)
        if (s.playfield[i].crumble && s.playfield[i].state.active == 1) {
            burstUiCloud(s.playfield[i]);
            if (s.bodies[i])
                s.bodies[i]->SetActive(false);
        }
    dispatchPinballDynamicNative(s.contacts->pinballDynamic, s.playfield, s.playfieldCount + 1,
                                 s.bodies);
    s.inWater = false;
    for (int i = 0; i < s.playfieldCount; i++)
        if (s.playfield[i].kind == WATER && s.bodies[i]) {
            Entity &water = s.playfield[i];
            const Props &props = *water.source->behavior("WaterAreaBehaviour");
            s.inWater |=
                applyNativeWaterMembers(s.bodies[i], bean, props, entityRect(water).t * metres);
        }
    advanceProximityAfterPhysicsNative(s.playfield, s.playfieldCount, player.state, s.bodies,
                                       s.oscillatorProxies);
    uiGroupCollectablePostStepNative(s);
    s.waterStartup = false;
    s.bean = player;
    return grounded;
}
