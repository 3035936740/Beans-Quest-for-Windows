#pragma once

// Regression coverage for oneway native. Runs only in the isolated BQ_ENABLE_TESTS executable.

class OneWayNativeTestContacts : public b2ContactListener {
  public:
    Entity *things;
    int count;
    bool inheritedFalse;

    OneWayNativeTestContacts(Entity *entities, int n)
        : things(entities), count(n), inheritedFalse(false) {}

    void BeginContact(b2Contact *contact) {
        beginOneWayContact(contact);
        for (int i = 0; i < oneWayEntryLimit; i++)
            if (oneWayEntries[i].contact == contact && !oneWayEntries[i].above)
                inheritedFalse = true;
    }

    void EndContact(b2Contact *contact) {
        forgetOneWayContact(contact);
    }

    void PreSolve(b2Contact *contact, const b2Manifold *) {
        oneWayContacts(contact, things, count);
    }
};

bool oneWayNativePhysicalCase(u64 klass, int scenario, bool terrain = false) {
    Entity things[2];
    things[0] = entities[playerIndex];
    Object &source = *classById(klass);
    initializeEntity(things[1], source, shifted(source.rect, 272, 188));
    b2World *storage = new b2World(b2Vec2(0, 0), true);
    b2World &world = *storage;
    OneWayNativeTestContacts listener(things, 2);
    world.SetContactListener(&listener);
    b2Body *platform = 0;
    float left = 240, right = 304, top = 196, bottom = 188;
    if (terrain) {
        b2BodyDef d;
        platform = world.CreateBody(&d);
        b2PolygonShape shape;
        terrainOneWayPolygon(shape, Vec{left, top}, Vec{right, top});
        b2FixtureDef f;
        f.shape = &shape;
        f.userData = (void *)1;
        platform->CreateFixture(&f);
    } else {
        Rect r = hitRect(things[1]);
        left = r.l;
        right = r.r;
        top = r.t;
        bottom = r.b;
        platform = createPhysicsEntity(world, things[1], 1);
    }
    if (!platform) {
        clearOneWayWorld(&world);
        delete storage;
        return false;
    }
    State &p = things[0].state;
    p.x = (left + right) * 0.5f;
    p.y = top - things[0].hit.b + 8;
    p.vx = 0;
    p.vy = -120;
    if (scenario == 0 || scenario == 1) {
        p.x = scenario == 0 ? left - things[0].hit.r - 8 : right - things[0].hit.l + 8;
        p.y = top - things[0].hit.b - 1;
        p.vx = scenario == 0 ? 180 : -180;
        p.vy = 0;
    }
    if (scenario == 2) {
        p.y = bottom - things[0].hit.t - 4;
        p.vy = 180;
    }
    if (scenario == 4) {
        p.y = top - things[0].hit.b + 1;
        p.vy = 5;
        platform->SetType(b2_kinematicBody);
        platform->SetLinearVelocity(b2Vec2(0, 20 * metres));
    }
    b2Body *bean = createPhysicsEntity(world, things[0], 0);
    if (!bean) {
        clearOneWayWorld(&world);
        delete storage;
        return false;
    }
    float vx = p.vx;
    bool topContact = false;
    for (int i = 0; i < 60; i++) {
        world.Step(1 / 120.0f, 10, 10);
        for (b2ContactEdge *edge = bean->GetContactList(); edge; edge = edge->next) {
            b2Contact *c = edge->contact;
            if (edge->other != platform || !c->IsTouching() || !c->IsEnabled() ||
                !c->GetManifold()->pointCount)
                continue;
            b2WorldManifold m;
            c->GetWorldManifold(&m);
            float normal = c->GetFixtureA()->GetBody() == bean ? -m.normal.y : m.normal.y;
            topContact |= normal > 0.5f;
        }
    }
    b2Vec2 position = bean->GetPosition(), velocity = bean->GetLinearVelocity();
    bool result = false;
    if (scenario == 0)
        result = position.x / metres > right - things[0].hit.l &&
                 absf(velocity.x / metres - vx) < 0.001f;
    if (scenario == 1)
        result =
            position.x / metres < left - things[0].hit.r && absf(velocity.x / metres - vx) < 0.001f;
    if (scenario == 2)
        result = position.y / metres + things[0].hit.b > top + 10 &&
                 absf(velocity.y / metres - 180) < 0.001f;
    if (scenario == 3)
        result =
            topContact && velocity.y >= 0 && position.y / metres + things[0].hit.b >= top - 0.01f;
    if (scenario == 4)
        result = absf(velocity.y / metres - 5) < 0.001f;
    if (!result) {
        char id[24], path[128], trace[512];
        idstr(klass, id);
        wsprintfA(path, "test-output/oneway-%s-%d-%d.txt", id, scenario, terrain ? 1 : 0);
        wsprintfA(
            trace,
            "source=%s scenario=%d terrain=%d topContact=%d top_x1000=%d left_x1000=%d right_x1000=%d finalX_x1000=%d finalY_x1000=%d vx_x1000=%d vy_x1000=%d beanBottom_x1000=%d\r\n",
            id, scenario, terrain ? 1 : 0, topContact ? 1 : 0, (int)(top * 1000),
            (int)(left * 1000), (int)(right * 1000), (int)(position.x / metres * 1000),
            (int)(position.y / metres * 1000), (int)(velocity.x / metres * 1000),
            (int)(velocity.y / metres * 1000),
            (int)((position.y / metres + things[0].hit.b) * 1000));
        atomicWrite(path, trace, lstrlenA(trace));
    }
    clearOneWayWorld(&world);
    delete storage;
    return result;
}

void oneWayNativeChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    startStage(0);
    readyTimer = deathTimer = 0;
    check(
        nativeOneWayAbove(b2Vec2(0, 1), b2Vec2(2, 1), b2Vec2(1, 1)) &&
            !nativeOneWayAbove(b2Vec2(0, 1), b2Vec2(2, 1), b2Vec2(1, 0.99999f)),
        "Native one-way top cross-product accepts its exact line and rejects a contact point immediately below without a foot tolerance");
    check(!nativeOneWayFace(0, -1) && !nativeOneWayFace(0.00999f, -1) &&
              nativeOneWayFace(0.01f, 0) && nativeOneWayFace(-0.01f, -1),
          "Original one-way normal threshold rejects lateral faces at absolute normalY below0.01");
    check(
        !nativeOneWayFace(1, 0.00001f) && nativeOneWayFace(1, 0),
        "Original one-way gate uses strict absolute upward velocity rather than relative-platform velocity or a positive tolerance");
    Entity authoredCloud;
    Object &authoredSource = *classById(14815823507569313204ULL);
    initializeEntity(authoredCloud, authoredSource, authoredSource.rect);
    check(
        absf(authoredCloud.hit.r - authoredCloud.hit.l - 32) < 0.0001f &&
            absf(authoredCloud.hit.t - authoredCloud.hit.b - 18) < 0.0001f &&
            absf(authoredCloud.hit.t - 9) < 0.0001f,
        "Original crumbling cloud retains its full32 by18 adjusted box and native top rather than a synthetic six-pixel slab");
    b2World *materialWorld = new b2World(b2Vec2(0, 0), true);
    b2Body *cloudBody = createPhysicsEntity(*materialWorld, authoredCloud, 1);
    b2Fixture *material = cloudBody ? cloudBody->GetFixtureList() : 0;
    check(
        cloudBody && cloudBody->GetType() == b2_kinematicBody && material &&
            absf(material->GetDensity() - 2.5f) < 0.0001f &&
            absf(material->GetRestitution() - 0.5f) < 0.0001f &&
            absf(material->GetFriction() - 0.2f) < 0.0001f &&
            material->GetFilterData().categoryBits == 4,
        "Original crumbling platform inherits its native BoxPhysics material defaults and kinematic body definition");
    delete materialWorld;
    const u64 classesToCheck[] = {14815823507569313204ULL, 6181314796472045350ULL,
                                  14843448166857854273ULL};
    const char *names[] = {
        "Actual small crumbling cloud permits left/right entry immediately below its top edge and remains a safe one-way landing surface",
        "Actual smiley moving cloud permits left/right entry immediately below its top edge and remains a safe one-way landing surface",
        "Actual ordinary Platform actor permits left/right entry immediately below its top edge and remains a safe one-way landing surface"};
    for (int i = 0; i < 3; i++) {
        bool pass = true;
        for (int scenario = 0; scenario < 4; scenario++)
            pass &= oneWayNativePhysicalCase(classesToCheck[i], scenario);
        check(pass, names[i]);
    }
    bool terrain = true;
    for (int scenario = 0; scenario < 4; scenario++)
        terrain &= oneWayNativePhysicalCase(classesToCheck[2], scenario, true);
    check(
        terrain,
        "Original eight-pixel one-way terrain permits both side entries and underside traversal while retaining top collision");
    check(
        oneWayNativePhysicalCase(classesToCheck[1], 4),
        "A rising one-way cloud cannot collide with an upward-moving Bean merely because their relative vertical velocity is downward");
    // A real second sensor fixture keeps the body's refused entry alive after
    // its primary fixture ends, as the native per-body refcount requires.
    Entity things[2];
    things[0] = entities[playerIndex];
    Object &source = *classById(classesToCheck[2]);
    initializeEntity(things[1], source, shifted(source.rect, 272, 188));
    Rect r = hitRect(things[1]);
    things[0].state.x = 272;
    things[0].state.y = r.t - things[0].hit.b - 2;
    things[0].state.vx = things[0].state.vy = 0;
    b2World *storage = new b2World(b2Vec2(0, 0), true);
    b2World &world = *storage;
    OneWayNativeTestContacts listener(things, 2);
    world.SetContactListener(&listener);
    b2Body *platform = createPhysicsEntity(world, things[1], 1);
    b2Body *bean = createPhysicsEntity(world, things[0], 0);
    b2Fixture *main = bean->GetFixtureList();
    b2PolygonShape shape = *(const b2PolygonShape *)main->GetShape();
    b2FixtureDef f;
    f.shape = &shape;
    f.isSensor = true;
    bean->CreateFixture(&f);
    world.Step(1 / 120.0f, 10, 10);
    int entries = 0;
    bool refused = true;
    for (int i = 0; i < oneWayEntryLimit; i++)
        if (oneWayEntries[i].contact && oneWayEntries[i].platform->GetBody() == platform &&
            oneWayEntries[i].other == bean) {
            entries++;
            refused &= !oneWayEntries[i].above;
        }
    bean->DestroyFixture(main);
    f.isSensor = false;
    f.density = 1;
    bean->CreateFixture(&f);
    world.Step(1 / 120.0f, 10, 10);
    bool retained = entries == 2 && refused && listener.inheritedFalse;
    bean->SetTransform(b2Vec2(100 * metres, 300 * metres), 0);
    world.Step(1 / 120.0f, 10, 10);
    bool ended = true;
    for (int i = 0; i < oneWayEntryLimit; i++)
        if (oneWayEntries[i].contact && oneWayEntries[i].platform->GetBody() == platform &&
            oneWayEntries[i].other == bean)
            ended = false;
    check(
        retained && ended,
        "Native one-way entry state is shared across a body's real solid/sensor fixtures, survives one fixture ending, and clears after every contact ends");
    clearOneWayWorld(&world);
    delete storage;
    startStage(0);
}
