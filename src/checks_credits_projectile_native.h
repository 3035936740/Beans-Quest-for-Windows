#pragma once

// Regression coverage for credits projectile native. Runs only in the isolated BQ_ENABLE_TESTS executable.
class CreditsImpactObserverNative : public CreditsContactsNative {
  public:
    bool observed;
    Vec point, shotCenter;
    int points;

    explicit CreditsImpactObserverNative(UiScene &s)
        : CreditsContactsNative(s), observed(false), points(0) {}

    void PostSolve(b2Contact *contact, const b2ContactImpulse *impulse) {
        b2Fixture *a = contact->GetFixtureA();
        b2Fixture *b = contact->GetFixtureB();
        b2Body *shot = (int)a->GetBody()->GetUserData() == 1025   ? a->GetBody()
                       : (int)b->GetBody()->GetUserData() == 1025 ? b->GetBody()
                                                                  : 0;
        if (shot && !a->IsSensor() && !b->IsSensor() && contact->IsEnabled() &&
            contact->GetManifold()->pointCount) {
            b2WorldManifold m;
            contact->GetWorldManifold(&m);
            points = contact->GetManifold()->pointCount;
            float x = 0, y = 0;
            for (int i = 0; i < points; i++) {
                x += m.points[i].x;
                y += m.points[i].y;
            }
            point = Vec{x / (points * metres), y / (points * metres)};
            b2Vec2 c = shot->GetPosition();
            shotCenter = Vec{c.x / metres, c.y / metres};
            observed = true;
        }
        CreditsContactsNative::PostSolve(contact, impulse);
    }
};

int prepareCreditsProjectileNative(u64 klass = 6430600862785566ULL) {
    navigate(137502285691ULL);
    transitionTime = endingCreditsFade = 0;
    int owner = -1;
    for (int i = 0; i < page.playfieldCount; i++) {
        const Props *p = page.playfield[i].source->behavior("BulletSource");
        if (p && idnum(p->get("bulletClass")) == klass)
            owner = i;
        page.playfield[i].state.gone = 1;
    }
    if (owner < 0)
        return -1;
    destroyUiPhysics(page);
    page.level.geometryCount = 0;
    page.playfield[owner].state.gone = 0;
    page.playfield[owner].state.active = 1;
    page.playfield[owner].state.x = 256;
    page.playfield[owner].state.y = 128;
    page.bean.state.x = 1200;
    page.bean.state.y = 2000;
    page.bean.state.vx = page.bean.state.vy = 0;
    createUiPhysics(page);
    delete page.contacts;
    page.contacts = new CreditsImpactObserverNative(page);
    page.physics->SetContactListener(page.contacts);
    page.bodies[page.playfieldCount]->SetType(b2_staticBody);
    creditsFire(page, owner);
    CreditsShotNative &shot = creditsRuntime.shots[0];
    shot.body->SetLinearVelocity(b2Vec2(120 * metres, 0));
    clearEffects(0);
    clearEffects(1);
    return owner;
}

b2Fixture *creditsProjectileWallNative(float x, bool sensor = false, int oneWay = 0) {
    b2BodyDef def;
    def.position.Set(x * metres, 128 * metres);
    b2Body *wall = page.physics->CreateBody(&def);
    b2PolygonShape shape;
    shape.SetAsBox(8 * metres, 80 * metres);
    b2FixtureDef f;
    f.shape = &shape;
    f.isSensor = sensor;
    f.userData = (void *)oneWay;
    return wall->CreateFixture(&f);
}

u32 creditsImpactPixelsNative(int layer) {
    cloudRenderBegin();
    drawEffects(1, layer);
    return cloudRenderPixels(Rect{240, 80, 360, 176}, false);
}

void creditsProjectileNativeChecks() {
    bool oldInterface = interfaceEnabled, oldSound = soundEnabled, oldMusic = musicEnabled,
         oldDebug = debugMode;
    interfaceEnabled = true;
    musicEnabled = false;
    soundEnabled = true;
    debugMode = false;
    stopMusic();
    stopSounds();
    startStage(0);
    u32 campaign = hashBytes(entities, sizeof(Entity) * entityCount),
        campaignShots = hashBytes(bullets, sizeof(bullets));
    const float dt = 1 / 120.0f;
    int owner = prepareCreditsProjectileNative();
    const Props *fire = classById(6430600862785566ULL)->behavior("EnemyBulletBehaviour");
    const Props *plasma = classById(12978618438667112518ULL)->behavior("EnemyBulletBehaviour");
    check(
        owner >= 0 && *fire->get("follow-effect") && !*plasma->get("follow-effect") &&
            idnum(fire->get("effect")) == idnum(plasma->get("effect")),
        "The real ending emitter and both original bullet classes distinguish the unused follow-effect resource field from their shared impact explosion");
    u32 before = emittedParticles;
    for (int i = 0; i < 120; i++) {
        page.physics->Step(dt, 10, 10);
        creditsProjectilePostStepNative(page, dt);
    }
    check(
        creditsRuntime.shots[0].live && creditsRuntime.shots[0].entity.state.x > 370 &&
            emittedParticles == before && liveEffects(1) == 0,
        "A genuine ending fireball flies for one second without emitting the extra nonnative resource-only trail");
    CreditsShotNative &shot = creditsRuntime.shots[0];
    shot.body->SetTransform(b2Vec2(-100 * metres, 128 * metres), 0);
    creditsProjectilePostStepNative(page, dt);
    bool left = shot.live;
    shot.body->SetTransform(b2Vec2((page.level.size.x + 100) * metres, 128 * metres), 0);
    creditsProjectilePostStepNative(page, dt);
    for (int i = 0; i < 20; i++)
        creditsFire(page, owner);
    int live = 0;
    for (int i = 0; i < 32; i++)
        live += creditsRuntime.shots[i].live;
    check(
        left && shot.live && live == 10 && !creditsRuntime.shots[10].live &&
            emittedParticles == before,
        "Leaving either world edge preserves an ordinary bullet and its original ten-shot pool occupancy instead of inventing an offscreen expiry");
    owner = prepareCreditsProjectileNative();
    creditsProjectileWallNative(320);
    CreditsImpactObserverNative *observer = (CreditsImpactObserverNative *)page.contacts;
    for (int i = 0; i < 100 && !shot.hit; i++) {
        page.physics->Step(dt, 10, 10);
        if (!shot.hit)
            creditsProjectilePostStepNative(page, dt);
    }
    before = emittedParticles;
    Vec point = observer->point, center = observer->shotCenter;
    check(
        observer->observed && shot.hit && shot.live && shot.body->IsActive() &&
            absf(shot.hitPoint.x - point.x) < .00001f &&
            absf(shot.hitPoint.y - point.y) < .00001f && absf(point.x - center.x) > 1 &&
            liveEffects(1) == 0,
        "Real solved wall contact queues its native surface point while keeping explosion and deactivation deferred until after Box2D unlocks");
    static BurstRandomNative beforeRandom, afterRandom;
    static BurstEmittersNative beforeEmitters, afterEmitters;
    beforeEmitters = burstEmittersNative[1];
    beforeRandom = burstRandomNative[1];
    u32 seed = effectSeed, voices = audioVoicesStarted;
    creditsProjectilePostStepNative(page, dt);
    u32 exploded = emittedParticles, seedAfter = effectSeed;
    afterRandom = burstRandomNative[1];
    afterEmitters = burstEmittersNative[1];
    int producedCount = liveEffects(1);
    EffectParticle *produced =
        (EffectParticle *)HeapAlloc(GetProcessHeap(), 0, sizeof(effectParticles));
    memcpy(produced, effectParticles, sizeof(effectParticles));
    tickEffects(.05f, 1);
    u32 actualPixels = creditsImpactPixelsNative(shot.entity.layer);
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++)
        if (effectParticles[i].definition && effectParticles[i].domain == 1) {
            effectParticles[i].x += center.x - point.x;
            effectParticles[i].y += center.y - point.y;
        }
    u32 oldCenterPixels = creditsImpactPixelsNative(shot.entity.layer);
    clearEffects(1);
    effectSeed = seed;
    burstRandomNative[1] = beforeRandom;
    burstEmittersNative[1] = beforeEmitters;
    emitEffect(idnum(fire->get("effect")), point.x, point.y, shot.entity.layer, 1, 1);
    bool exact = producedCount > 0 && liveEffects(1) == producedCount;
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++)
        if (produced[i].definition && produced[i].domain == 1)
            exact &= hashBytes(produced + i, sizeof(EffectParticle)) ==
                     hashBytes(effectParticles + i, sizeof(EffectParticle));
    HeapFree(GetProcessHeap(), 0, produced);
    effectSeed = seedAfter;
    burstRandomNative[1] = afterRandom;
    burstEmittersNative[1] = afterEmitters;
    emittedParticles = exploded;
    check(
        exact && !shot.live && !shot.body->IsActive() && exploded > before,
        "The actual ending impact explosion matches the original particle definition at the independently measured contact point rather than the bullet center");
    check(
        producedCount > 0 && actualPixels != oldCenterPixels,
        "Real OpenGL explosion pixels distinguish the corrected wall-contact origin from the previous projectile-center origin");
    before = emittedParticles;
    creditsProjectilePostStepNative(page, dt);
    creditsProjectilePostStepNative(page, dt);
    check(
        emittedParticles == before && audioVoicesStarted == voices && soundEnabled,
        "A completed ending impact emits only once and preserves the muted original bullet behavior and user's sound preference");
    bool pass = true;
    for (int kind = 0; kind < 2; kind++) {
        prepareCreditsProjectileNative();
        creditsProjectileWallNative(300, kind == 0, kind == 1 ? 1 : 0);
        before = emittedParticles;
        for (int i = 0; i < 80; i++) {
            page.physics->Step(dt, 10, 10);
            creditsProjectilePostStepNative(page, dt);
        }
        pass &= shot.live && !shot.hit && shot.entity.state.x > 330 && emittedParticles == before;
    }
    check(
        pass,
        "Actual sensor and one-way wall fixtures do not invoke an ending bullet explosion or stop its native flight");
    owner = prepareCreditsProjectileNative();
    page.bodies[owner]->SetActive(true);
    before = emittedParticles;
    for (int i = 0; i < 2; i++) {
        page.physics->Step(dt, 10, 10);
        creditsProjectilePostStepNative(page, dt);
    }
    bool ignored = (shot.ignored[owner / 32] & (1u << (owner % 32))) != 0;
    check(
        ignored && shot.live && !shot.hit && emittedParticles == before,
        "A first-frame overlap with the real emitter is remembered and remains ignored on the following physics pass");
    prepareCreditsProjectileNative(12978618438667112518ULL);
    creditsProjectileWallNative(320);
    observer = (CreditsImpactObserverNative *)page.contacts;
    for (int i = 0; i < 120 && !shot.hit; i++) {
        page.physics->Step(dt, 10, 10);
        if (!shot.hit)
            creditsProjectilePostStepNative(page, dt);
    }
    bool queued = observer->observed && shot.hit;
    creditsProjectilePostStepNative(page, dt);
    check(
        queued && !shot.live && liveEffects(1) > 0 &&
            campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
            campaignShots == hashBytes(bullets, sizeof(bullets)),
        "The second original plasma class shares corrected impact behavior while every isolated ending check leaves campaign actors and projectiles untouched");
    clearEffects(1);
    startStage(0);
    interfaceEnabled = oldInterface;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
    debugMode = oldDebug;
}
