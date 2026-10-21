#pragma once

// Regression coverage for fixture filter native. Runs only in the isolated BQ_ENABLE_TESTS executable.

unsigned short fixtureFilterReferenceMaskNative(const Props &p) {
    unsigned short m = 0xffff;
    if (!p.flag("collide-player", true))
        m &= ~8;
    if (!p.flag("collide-oscillating", true))
        m &= ~4;
    if (!p.flag("collide-other", true))
        m &= 12;
    return m;
}

unsigned short fixtureFilterProbeMaskNative(unsigned m) {
    return (unsigned short)((m & 1 ? (4 | 16) : 0) | (m & 2 ? 1 : 0) | (m & 4 ? (8 | 32) : 0) |
                            (m & 8 ? 2 : 0));
}

class FixtureFilterPairProbeNative : public b2ContactListener {
  public:
    int contacts;

    FixtureFilterPairProbeNative() : contacts(0) {}

    void BeginContact(b2Contact *) {
        contacts++;
    }
};

bool fixtureFilterCarrierContactNative(bool allowOther) {
    Entity carrier;
    Object *source = classById(6181314796472045350ULL);
    initializeEntity(carrier, *source, source->rect);
    carrier.state.x = carrier.state.y = 0;
    b2World *world = new b2World(b2Vec2(0, 0), true);
    FixtureFilterPairProbeNative listener;
    world->SetContactListener(&listener);
    b2Body *main = createPhysicsEntity(*world, carrier, 1);
    b2BodyDef bd;
    bd.type = b2_dynamicBody;
    b2Body *other = world->CreateBody(&bd);
    b2CircleShape circle;
    circle.m_radius = 4 * metres;
    b2FixtureDef f;
    f.shape = &circle;
    f.density = 1;
    f.filter.categoryBits = 4;
    f.filter.maskBits = fixtureFilterProbeMaskNative(allowOther ? 15 : 12);
    other->CreateFixture(&f);
    world->Step(1 / 120.0f, 10, 10);
    bool result = main && (allowOther ? listener.contacts > 0 : listener.contacts == 0);
    delete world;
    return result;
}

void fixtureFilterNativeChecks() {
    interfaceEnabled = musicEnabled = soundEnabled = debugMode = false;
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    b2World *world = new b2World(b2Vec2(0, 0), true);
    b2ContactFilter filter;
    b2BodyDef bd;
    b2Body *probes = world->CreateBody(&bd);
    b2CircleShape shape;
    shape.m_radius = metres;
    b2Fixture *test[4][8];
    unsigned nativeMasks[8];
    const unsigned portCategories[] = {4, 1, 32, 2};
    for (int flags = 0; flags < 8; flags++) {
        unsigned m = 15;
        if (!(flags & 1))
            m &= ~8;
        if (!(flags & 2))
            m &= ~4;
        if (!(flags & 4))
            m &= 12;
        nativeMasks[flags] = m;
        for (int group = 0; group < 4; group++) {
            b2FixtureDef f;
            f.shape = &shape;
            f.filter.categoryBits = (unsigned short)portCategories[group];
            f.filter.maskBits = fixtureFilterProbeMaskNative(m);
            test[group][flags] = probes->CreateFixture(&f);
        }
    }
    int instances = 0, pairs = 0, mismatches = 0, scenes = 0;
    char rows[10000] = "id,nativeCategory,portCategory,probeCategory,probeMask,expected,actual\r\n";
    for (int n = 0; n < levelCount; n++) {
        readLevelData(levels[n].file, scene, memory);
        scenes++;
        for (int i = 0; i < scene.objectCount; i++) {
            Object &o = scene.objects[i];
            const Props *p = o.behavior("BoxPhysicsBehaviour");
            if (!p)
                p = o.behavior("CirclePhysicsBehaviour");
            if (!p)
                continue;
            Entity e;
            initializeEntity(e, o, o.rect);
            b2Body *body = createPhysicsEntity(*world, e, 1);
            if (!body)
                continue;
            b2Fixture *main = body->GetFixtureList();
            for (b2Fixture *f = main; f; f = f->GetNext())
                if (!f->IsSensor())
                    main = f;
            if (!main) {
                world->DestroyBody(body);
                continue;
            }
            instances++;
            unsigned category = 1 | (p->flag("static-position", false) ? 2 : 0),
                     mask = fixtureFilterReferenceMaskNative(*p);
            for (int group = 0; group < 4; group++)
                for (int flags = 0; flags < 8; flags++) {
                    bool expected = (category & nativeMasks[flags]) && (mask & (1 << group));
                    bool actual = filter.ShouldCollide(main, test[group][flags]);
                    pairs++;
                    if (expected != actual) {
                        mismatches++;
                        if (lstrlenA(rows) < 9700) {
                            char id[24], row[160];
                            idstr(o.id, id);
                            wsprintfA(row, "%s,%d,%d,%d,%d,%d,%d\r\n", id, category,
                                      main->GetFilterData().categoryBits, 1 << group,
                                      nativeMasks[flags], expected, actual);
                            lstrcatA(rows, row);
                        }
                    }
                }
            world->DestroyBody(body);
        }
    }
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    delete world;
    check(
        scenes == 74 && instances > 0 && pairs == instances * 32 && mismatches == 0,
        "All74scenes:actual explicit Box/Circle main fixtures match independent native categories across four peer categories and eight collision-flag combinations");
    check(
        fixtureFilterCarrierContactNative(false),
        "A native loose actor admitting oscillators but excluding other actors cannot contact the moving cloud carrier's native category3 main fixture");
    check(
        fixtureFilterCarrierContactNative(true),
        "Admitting native other actors still creates an actual contact with the same moving cloud carrier");
    char summary[220];
    wsprintfA(summary, "scenes=%d instances=%d pairs=%d mismatches=%d\r\n", scenes, instances,
              pairs, mismatches);
    atomicWrite("test-output/fixture-filter-summary.txt", summary, lstrlenA(summary));
    atomicWrite("test-output/fixture-filter-mismatches.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/fixture-filter-results.txt", testLog, lstrlenA(testLog));
    startStage(0);
    readyTimer = 0;
}
