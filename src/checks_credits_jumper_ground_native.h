#pragma once

// Regression coverage for credits jumper ground native. Runs only in the isolated BQ_ENABLE_TESTS executable.

class CreditsJumperGroundObserverNative : public CreditsContactsNative {
  public:
    int target, count;
    bool block;
    float preVy;
    int preHp, seen;
    b2Fixture *members[64];

    CreditsJumperGroundObserverNative(UiScene &s, int index)
        : CreditsContactsNative(s), target(index), count(0), block(false), preVy(0), preHp(0),
          seen(0) {
        memset(members, 0, sizeof(members));
    }

    b2Fixture *other(b2Contact *c) {
        if (c->GetFixtureA()->GetBody() == scene->bodies[target])
            return c->GetFixtureB();
        if (c->GetFixtureB()->GetBody() == scene->bodies[target])
            return c->GetFixtureA();
        return 0;
    }

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        CreditsContactsNative::PreSolve(c, old);
        if (other(c)) {
            if (!seen) {
                preVy = scene->playfield[target].state.vy;
                preHp = scene->playfield[target].state.hp;
            }
            seen++;
            if (block)
                c->SetEnabled(false);
        }
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        b2Fixture *f = other(c);
        if (f) {
            b2WorldManifold m;
            c->GetWorldManifold(&m);
            float ny =
                c->GetFixtureA()->GetBody() == scene->bodies[target] ? -m.normal.y : m.normal.y;
            if (ny > .95f) {
                bool exists = false;
                for (int i = 0; i < count; i++)
                    exists |= members[i] == f;
                if (!exists && count < 64)
                    members[count++] = f;
            }
        }
        CreditsContactsNative::PostSolve(c, impulse);
    }

    void EndContact(b2Contact *c) {
        b2Fixture *f = other(c);
        if (f)
            for (int i = 0; i < count; i++)
                if (members[i] == f) {
                    members[i] = members[--count];
                    break;
                }
        CreditsContactsNative::EndContact(c);
    }
};

int creditsJumperPrepareNative(u64 id, CreditsJumperGroundObserverNative *&observer,
                               b2Body *&floor) {
    interfaceEnabled = true;
    musicEnabled = soundEnabled = debugMode = false;
    navigate(137502285691ULL);
    int target = creditsEntityById(page, id);
    if (target < 0)
        return -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (i != target)
            page.playfield[i].state.gone = 1;
    Entity &e = page.playfield[target];
    e.state.gone = 0;
    e.state.active = 1;
    e.state.x = 2000;
    e.state.y = 4500;
    e.state.vx = e.state.vy = 0;
    e.state.aux = 0;
    e.state.hp = 0;
    page.bean.state.x = 1000;
    page.bean.state.y = 5000;
    page.bean.state.vx = page.bean.state.vy = 0;
    resetUiPhysics(page);
    page.bodies[page.playfieldCount]->SetType(b2_staticBody);
    observer = new CreditsJumperGroundObserverNative(page, target);
    delete page.contacts;
    page.contacts = observer;
    page.physics->SetContactListener(observer);
    b2BodyDef def;
    def.position.Set(e.state.x * metres, (e.state.y + e.hit.b - 8 + .25f) * metres);
    floor = page.physics->CreateBody(&def);
    b2PolygonShape shape;
    shape.SetAsBox(200 * metres, 8 * metres);
    floor->CreateFixture(&shape, 0);
    stepUiPhysics(page, 1 / 120.0f);
    return target;
}

void creditsJumperGroundNativeChecks() {
    const float dt = 1 / 120.0f;
    const u64 actors[] = {7390099274039660ULL, 7675190002759765ULL, 6071696873400895ULL,
                          8056460997797086ULL};
    u32 campaign = hashBytes(entities, sizeof(Entity) * entityCount),
        saved = hashBytes(&checkpoint, sizeof(checkpoint)),
        campaignGround = hashBytes(jumperGroundFixtures, sizeof(jumperGroundFixtures));
    bool fixtures = true, before = true, equal = true, after = true, upward = true, persist = true,
         end = true, reset = true, retained = true, pause = true;
    int cases = 0;
    char trace[4096] = "actor,case,callbacks_ground,phase_millis,hp,pre_vy\r\n";
    for (int a = 0; a < 4; a++)
        for (int mode = 0; mode < 4; mode++) {
            CreditsJumperGroundObserverNative *observer = 0;
            b2Body *floor = 0;
            int index = creditsJumperPrepareNative(actors[a], observer, floor);
            if (index < 0) {
                fixtures = false;
                continue;
            }
            Entity &e = page.playfield[index];
            bool paired = a < 2;
            float delay = paired ? creditsRuntime.frogPeriod
                                 : e.source->behavior("EnemyJumperBehaviour")->num("jump-delay", 1);
            fixtures &= observer->count > 0 && creditsGrounded(page, index);
            float old = delay + (mode == 0 ? -dt * .5f : mode == 2 ? dt * .5f : 0);
            e.state.aux = old;
            e.state.hp = 1;
            e.state.vx = 0;
            e.state.vy = mode == 3 ? 40 : 0;
            page.bodies[index]->SetLinearVelocity(b2Vec2(0, e.state.vy * metres));
            observer->seen = 0;
            tickCreditsRuntime(page, dt);
            bool phase = absf(e.state.aux - (old + dt)) < .00001f;
            if (mode == 0)
                before &= phase && e.state.hp == 0;
            if (mode == 1)
                equal &= phase && e.state.hp == 0;
            if (mode == 2)
                after &= phase && e.state.hp == 1 && observer->seen > 0 && observer->preVy > 200;
            if (mode == 3)
                upward &= phase && e.state.hp == 0;
            char row[180];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d\r\n", a, mode, observer->count,
                      (int)(e.state.aux * 1000), e.state.hp, (int)observer->preVy);
            lstrcatA(trace, row);
            cases++;
            page.physics->DestroyBody(floor);
        }
    check(
        fixtures && cases == 16,
        "All four original ending jumpers generate independently observed upward solved contacts for sixteen physical clock/velocity boundary cases");
    check(
        before,
        "Ending jumpers wait when the old clock is just below delay even if adding this tick would cross it");
    check(equal,
          "The exact ending jump-delay boundary remains waiting and advances its phase once");
    check(
        after,
        "All four ending jumpers launch only above the old delay and retain the advanced phase through the launch step");
    check(
        upward,
        "An actual upward push does not erase a retained native ground contact or skip its waiting branch");
    for (int a = 0; a < 4; a++) {
        CreditsJumperGroundObserverNative *observer = 0;
        b2Body *floor = 0;
        int index = creditsJumperPrepareNative(actors[a], observer, floor);
        if (index < 0) {
            persist = end = reset = false;
            continue;
        }
        Entity &e = page.playfield[index];
        e.state.aux = 0;
        e.state.hp = 1;
        e.state.vx = e.state.vy = 0;
        page.bodies[index]->SetLinearVelocity(b2Vec2(0, 0));
        observer->block = true;
        tickCreditsRuntime(page, dt);
        persist &= observer->count > 0 && creditsGrounded(page, index) && e.state.hp == 0 &&
                   absf(e.state.aux - dt) < .00001f;
        page.physics->DestroyBody(floor);
        end &= observer->count == 0 && !creditsGrounded(page, index);
        // A fresh scene supplies a real supported body before testing explicit reset.
        index = creditsJumperPrepareNative(actors[a], observer, floor);
        bool had = observer->count > 0;
        resetUiPhysics(page);
        reset &= had && !creditsGrounded(page, index);
        page.physics->DestroyBody(floor);
    }
    check(
        persist,
        "Disabling an existing ending contact preserves its native solved-fixture membership until EndContact instead of querying its current enabled flag");
    check(
        end,
        "Destroying the actual support fixture removes every ending jumper ground membership through EndContact");
    check(
        reset,
        "Explicit ending physics reset clears transient support pointers before any new solved callback");
    for (int a = 0; a < 2; a++) {
        CreditsJumperGroundObserverNative *observer = 0;
        b2Body *floor = 0;
        int index = creditsJumperPrepareNative(actors[a], observer, floor);
        if (index < 0) {
            retained = pause = false;
            continue;
        }
        Entity &e = page.playfield[index];
        page.playfield[creditsRuntime.frogBall].state.hp = 0;
        e.state.aux = .75f;
        e.state.hp = 1;
        e.state.vx = e.state.vy = 0;
        page.bodies[index]->SetLinearVelocity(b2Vec2(0, 0));
        tickCreditsRuntime(page, dt);
        retained &= e.state.hp == 0 && absf(e.state.aux - (.75f + dt)) < .00001f;
        float phase = e.state.aux;
        tickCreditsRuntime(page, 0);
        pause &= e.state.aux == phase;
        page.physics->DestroyBody(floor);
    }
    check(
        retained,
        "Both original ending frogs retain the special two-second delay after cooperation ends instead of reverting to the authored half-second delay");
    check(
        pause && campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
            saved == hashBytes(&checkpoint, sizeof(checkpoint)) &&
            campaignGround == hashBytes(jumperGroundFixtures, sizeof(jumperGroundFixtures)),
        "Ending jumper updates respect zero elapsed time and never borrow campaign actors, checkpoint or ground-fixture membership");
    atomicWrite("test-output/credits-jumper-ground-observed.csv", trace, lstrlenA(trace));
    interfaceEnabled = false;
    startStage(0);
    readyTimer = 0;
    atomicWrite("test-output/credits-jumper-ground-results.txt", testLog, lstrlenA(testLog));
}
