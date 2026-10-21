#pragma once

// Regression coverage for creature crush matrix. Runs only in the isolated BQ_ENABLE_TESTS executable.

class CreatureCrushMatrixContacts : public b2ContactListener {
  public:
    Entity *actors;
    b2Body *bodies[2];
    b2Body *proxy;
    DiagonalRuntimeState diagonal[2];
    CreaturePressureState<2> pressure;
    bool pending[2];
    int solidPairs;

    explicit CreatureCrushMatrixContacts(Entity *things) : actors(things), proxy(0), solidPairs(0) {
        memset(bodies, 0, sizeof(bodies));
        memset(diagonal, 0, sizeof(diagonal));
        memset(pending, 0, sizeof(pending));
        resetCreaturePressure(pressure);
    }

    void PreSolve(b2Contact *c, const b2Manifold *) {
        oscillatorProxyContacts(c);
        nativeMotionProxyPreSolve(c, actors, 2, diagonal);
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        creaturePressurePostSolve(c, impulse, actors, 2, bodies, pressure, pending);
        b2Body *a = c->GetFixtureA()->GetBody();
        b2Body *b = c->GetFixtureB()->GetBody();
        if ((a == bodies[0] && b == bodies[1]) || (b == bodies[0] && a == bodies[1]))
            solidPairs++;
    }
};

void creatureCrushMatrixChecks() {
    char *rows = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 300000);
    lstrcpyA(
        rows,
        "class_id,kind,body_type,killable,rate,axis,blocked,carrier_mode,solid_pairs,expected_death,actual_deaths,live_escape1000\r\n");
    int classesTested = 0, projectileClasses = 0, cases = 0, expectedDeaths = 0;
    bool deaths = true, once = true, confined = true, freeAlive = true;
    const int rates[] = {30, 120, 240};
    for (int klass = 0; klass < classCount; klass++) {
        Entity prototype;
        initializeEntity(prototype, classes[klass], classes[klass].rect);
        if (prototype.kind != ENEMY && prototype.kind != HELPER)
            continue;
        if (classes[klass].has("EnemyBulletBehaviour") ||
            classes[klass].has("FinalBossBulletBehaviour")) {
            projectileClasses++;
            continue;
        }
        classesTested++;
        for (int rate = 0; rate < 3; rate++)
            for (int axis = 0; axis < 3; axis++)
                for (int blocked = 0; blocked < 2; blocked++)
                    for (int carrierMode = 0; carrierMode < 4; carrierMode++) {
                        Entity actors[2];
                        initializeEntity(actors[0], classes[klass], classes[klass].rect);
                        Object *fish =
                            classById(carrierMode == 1 ? 14264481342587988014ULL : 706166303646ULL);
                        initializeEntity(actors[1], *fish, fish->rect);
                        for (int i = 0; i < 2; i++) {
                            actors[i].state.active = 1;
                            actors[i].state.x = 0;
                            actors[i].state.y = 100;
                            actors[i].state.vx = actors[i].state.vy = 0;
                        }
                        Entity &victim = actors[0];
                        Entity &carrier = actors[1];
                        if (axis == 0)
                            victim.state.y = 100 + carrier.hit.t - victim.hit.b + .65f;
                        else
                            victim.state.x = axis == 1 ? carrier.hit.r - victim.hit.l + .65f
                                                       : carrier.hit.l - victim.hit.r - .65f;
                        b2World *world = new b2World(b2Vec2(0, nativeWorldGravity * metres), true);
                        CreatureCrushMatrixContacts listener(actors);
                        world->SetContactListener(&listener);
                        b2Body *victimProxy = 0;
                        listener.bodies[0] =
                            createPhysicsEntity(*world, victim, 0, 0, &victimProxy);
                        listener.bodies[1] =
                            createPhysicsEntity(*world, carrier, 1, 0, &listener.proxy);
                        b2Body *body = listener.bodies[0];
                        if (!body || !listener.bodies[1]) {
                            deaths = false;
                            delete world;
                            continue;
                        }
                        b2Fixture *main = body->GetFixtureList();
                        while (main && main->IsSensor())
                            main = main->GetNext();
                        b2AABB aabb;
                        main->GetShape()->ComputeAABB(&aabb, body->GetTransform(), 0);
                        float left = -1000, right = 1000, top = 1000,
                              bottom = aabb.lowerBound.y / metres - .65f;
                        if (blocked) {
                            if (axis == 0)
                                top = aabb.upperBound.y / metres + 2.5f;
                            else if (axis == 1)
                                right = aabb.upperBound.x / metres + 2.5f;
                            else
                                left = aabb.lowerBound.x / metres - 2.5f;
                        }
                        if (axis == 0)
                            bottom = -1000;
                        b2BodyDef bd;
                        b2Body *room = world->CreateBody(&bd);
                        b2Vec2 points[] = {b2Vec2(left * metres, bottom * metres),
                                           b2Vec2(right * metres, bottom * metres),
                                           b2Vec2(right * metres, top * metres),
                                           b2Vec2(left * metres, top * metres)};
                        b2ChainShape chain;
                        chain.CreateLoop(points, 4);
                        b2FixtureDef fd;
                        fd.shape = &chain;
                        fd.filter.categoryBits = 1;
                        room->CreateFixture(&fd);
                        bool physical =
                            body->GetType() == b2_dynamicBody &&
                            (!victim.physics || victim.physics->flag("collide-other", true));
                        bool expected = (carrierMode == 0 || carrierMode == 3) && blocked &&
                                        physical && victim.kind == ENEMY &&
                                        victim.source->has("KillableEnemyBehaviour");
                        int died = 0;
                        float escaped = 0;
                        listener.diagonal[1].directionX = axis == 0 ? 0 : axis == 1 ? 1 : -1;
                        listener.diagonal[1].directionY = axis == 0 ? 1 : 0;
                        for (int step = 0; step < rates[rate]; step++) {
                            resetCreaturePressure(listener.pressure);
                            carrier.state.vx =
                                (carrierMode == 2 ? 0 : 100) * listener.diagonal[1].directionX;
                            carrier.state.vy =
                                (carrierMode == 2 ? 0 : 100) * listener.diagonal[1].directionY;
                            listener.bodies[1]->SetLinearVelocity(
                                b2Vec2(carrier.state.vx * metres, carrier.state.vy * metres));
                            if (carrierMode == 3)
                                carrier.state.vx = carrier.state.vy = 0;
                            world->Step(1.0f / rates[rate], 10, 10);
                            if (listener.pending[0] && !victim.state.gone) {
                                killEnemy(victim);
                                body->SetActive(false);
                                if (victimProxy)
                                    victimProxy->SetActive(false);
                                died++;
                            }
                            listener.pending[0] = false;
                            if (body->IsActive()) {
                                b2AABB box;
                                main->GetShape()->ComputeAABB(&box, body->GetTransform(), 0);
                                escaped =
                                    maxf(escaped, maxf(maxf(box.upperBound.y / metres - top,
                                                            box.upperBound.x / metres - right),
                                                       left - box.lowerBound.x / metres));
                                b2Vec2 p = body->GetPosition(), v = body->GetLinearVelocity();
                                victim.state.x = p.x / metres;
                                victim.state.y = p.y / metres;
                                victim.state.vx = v.x / metres;
                                victim.state.vy = v.y / metres;
                            }
                        }
                        deaths &= (died > 0) == expected;
                        once &= died <= 1;
                        confined &= !expected || escaped < 2;
                        freeAlive &= blocked || died == 0;
                        expectedDeaths += expected;
                        cases++;
                        char row[256];
                        wsprintfA(row, "%I64u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
                                  classes[klass].id, victim.kind, body->GetType(),
                                  victim.source->has("KillableEnemyBehaviour"), rates[rate], axis,
                                  blocked, carrierMode, listener.solidPairs, expected, died,
                                  (int)(escaped * 1000));
                        if (lstrlenA(rows) < 299500)
                            lstrcatA(rows, row);
                        delete world;
                    }
    }
    atomicWrite("test-output/creature-crush-class-matrix.csv", rows, lstrlenA(rows));
    HeapFree(GetProcessHeap(), 0, rows);
    check(
        classesTested == 18 && projectileClasses == 3 && cases == 1296,
        "All20enemy definitions and HelperBird are accounted for:18body classes execute1296pressure/control paths;3projectile classes retain their dedicated lifecycle tests");
    check(
        deaths && expectedDeaths > 0,
        "Every dynamic killable creature dies under genuine vertical or left/right carrier compression; native kinematic and unkillable classes retain their authored distinction");
    check(
        once && freeAlive,
        "Compression is one death event; free carrying, ordinary finite-mass stacking and stationary platforms never acquire crush deaths");
    check(
        confined,
        "The all-class pressure matrix leaves no crushed killable creature alive beyond the blocking wall");
}

void creatureCrushUiChecks() {
    const u64 jumpers[] = {15190319732989319544ULL, 6425677159578671338ULL, 3692840978977075ULL};
    const int rates[] = {30, 120, 240};
    u32 campaign = hashBytes(entities, sizeof(Entity) * entityCount),
        saved = hashBytes(&checkpoint, sizeof(checkpoint)),
        pressure =
            hashBytes(campaignCreaturePressure.count, sizeof(campaignCreaturePressure.count));
    bool died = true, fresh = true;
    int cases = 0;
    for (int species = 0; species < 3; species++)
        for (int rate = 0; rate < 3; rate++)
            for (int mode = 0; mode < 2; mode++) {
                UiScene *s = new UiScene;
                memset(s, 0, sizeof(*s));
                s->memory.init(2 * 1024 * 1024);
                s->hasBean = true;
                s->playfieldCount = 2;
                s->playfield = s->memory.array<Entity>(3);
                Object *jumper = classById(jumpers[species]);
                Object *fish = classById(706166303646ULL);
                Object *bean = classById(11120285848841938423ULL);
                initializeEntity(s->playfield[0], *jumper, jumper->rect);
                initializeEntity(s->playfield[1], *fish, fish->rect);
                initializeEntity(s->bean, *bean, bean->rect);
                s->bean.state.x = s->bean.state.y = 1000;
                Entity &e = s->playfield[0];
                Entity &carrier = s->playfield[1];
                e.state.active = carrier.state.active = 1;
                carrier.state.x = e.state.x = 0;
                carrier.state.y = mode ? 160.0f : 180.0f;
                e.state.y = carrier.state.y + carrier.hit.t - e.hit.b + .65f;
                createUiPhysics(*s);
                b2BodyDef bd;
                b2Body *room = s->physics->CreateBody(&bd);
                b2PolygonShape roof;
                roof.SetAsBox(200 * metres, 10 * metres, b2Vec2(0, 230 * metres), 0);
                room->CreateFixture(&roof, 0);
                for (int step = 0; step < rates[rate]; step++) {
                    carrier.state.vx = 0;
                    carrier.state.vy = 100;
                    if (mode && step == 1) {
                        carrier.state.gone = 1;
                        e.state.x = 0;
                        e.state.y = 220 - e.hit.t - .2f;
                        e.state.vx = 0;
                        e.state.vy = 100;
                        s->bodies[0]->SetTransform(b2Vec2(e.state.x * metres, e.state.y * metres),
                                                   0);
                    }
                    stepUiPhysics(*s, 1.0f / rates[rate]);
                }
                if (mode)
                    fresh &= e.state.gone == 0;
                else
                    died &= e.state.gone != 0 && !s->bodies[0]->IsActive();
                cases++;
                destroyUiPhysics(*s);
                VirtualFree(s->memory.data, 0, MEM_RELEASE);
                delete s;
            }
    check(
        cases == 18 && died,
        "Independent UI worlds apply the same crush death to all3jumper species through real stepUiPhysics at30/120/240Hz");
    check(
        fresh,
        "UI pressure resets each physics pass: a departed carrier cannot later turn a normal ceiling hit into a stale crush death");
    check(
        campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
            saved == hashBytes(&checkpoint, sizeof(checkpoint)) &&
            pressure ==
                hashBytes(campaignCreaturePressure.count, sizeof(campaignCreaturePressure.count)),
        "UI crush callbacks and cleanup leave campaign actors, pressure membership and checkpoint bytes unchanged");
}
