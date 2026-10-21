#pragma once

// Regression coverage for fish cloud native. Runs only in the isolated BQ_ENABLE_TESTS executable.

class FishCloudContactsNative : public OriginalContacts {
  public:
    int target, cloudContacts;

    FishCloudContactsNative(int index) : target(index), cloudContacts(0) {}

    void BeginContact(b2Contact *c) {
        OriginalContacts::BeginContact(c);
        b2Fixture *f[] = {c->GetFixtureA(), c->GetFixtureB()};
        for (int side = 0; side < 2; side++)
            if (f[side]->GetBody() == oscillatorProxies[target]) {
                int i = (int)f[1 - side]->GetBody()->GetUserData() - 1;
                if (i >= 0 && i < entityCount && entities[i].crumble)
                    cloudContacts++;
            }
    }
};

bool fishSurfaceFilterNative(Object &source, b2Fixture *detector, b2World &world) {
    Entity platform;
    initializeEntity(platform, source, source.rect);
    b2Body *body = createPhysicsEntity(world, platform, 1);
    if (!body)
        return false;
    const Props *p = source.behavior("BoxPhysicsBehaviour");
    if (!p)
        p = source.behavior("CrumblingPlatformBehaviour");
    unsigned category = 1, mask = 0xffff;
    if (p && p->flag("static-position", false))
        category |= 2;
    if (p && !p->flag("collide-player", true))
        mask &= ~8;
    if (p && !p->flag("collide-oscillating", true))
        mask &= ~4;
    if (p && !p->flag("collide-other", true))
        mask &= 0xc;
    bool expected = (category & 6) && (mask & 4);
    b2ContactFilter filter;
    bool result = filter.ShouldCollide(detector, body->GetFixtureList()) == expected;
    world.DestroyBody(body);
    return result;
}

class FishSurfaceProbeNative : public b2ContactListener {
  public:
    Entity *fish;
    DiagonalRuntimeState direction;
    int disabledByPlatform;

    FishSurfaceProbeNative(Entity *actor) : fish(actor), disabledByPlatform(0) {
        memset(&direction, 0, sizeof(direction));
        direction.directionY = 1;
        direction.effectiveSpeed = 100;
        direction.selected = 1;
    }

    void BeginContact(b2Contact *c) {
        beginOneWayContact(c);
    }

    void EndContact(b2Contact *c) {
        forgetOneWayContact(c);
    }

    void PreSolve(b2Contact *c, const b2Manifold *) {
        oneWayContacts(c, fish, 1);
        if (!c->IsEnabled())
            disabledByPlatform++;
        nativeMotionProxyPreSolve(c, fish, 1, &direction);
    }
};

bool fishSurfacePathNative(int mode) {
    Object *source = classById(706166303646ULL);
    if (!source)
        return false;
    Entity fish;
    initializeEntity(fish, *source, source->rect);
    fish.state.x = 0;
    fish.state.y = -30;
    fish.state.active = 1;
    b2World *world = new b2World(b2Vec2(0, 0), true);
    FishSurfaceProbeNative probe(&fish);
    world->SetContactListener(&probe);
    b2Body *proxy = 0;
    b2Body *body = createPhysicsEntity(*world, fish, 0, 0, &proxy);
    b2BodyDef bd;
    b2Body *surface = world->CreateBody(&bd);
    b2PolygonShape box;
    if (mode == 1)
        terrainOneWayPolygon(box, Vec{-96, 0}, Vec{96, 0});
    else
        box.SetAsBox(96 * metres, 4 * metres);
    b2FixtureDef fd;
    fd.shape = &box;
    fd.filter.categoryBits = 1;
    fd.userData = mode == 1 ? (void *)1 : 0;
    fd.isSensor = mode == 2;
    if (mode == 3)
        fd.filter.maskBits &= ~32;
    surface->CreateFixture(&fd);
    bool reflected = false;
    for (int n = 0; n < 120; n++) {
        body->SetLinearVelocity(b2Vec2(0, probe.direction.directionY * 100 * metres));
        world->Step(1 / 120.0f, 10, 10);
        reflected |= probe.direction.directionY < 0;
    }
    bool crossed = body->GetPosition().y / metres > 20;
    bool result = mode == 0   ? reflected
                  : mode == 1 ? (reflected && probe.disabledByPlatform > 0)
                              : (!reflected && crossed);
    clearOneWayWorld(world);
    delete world;
    return result;
}

void fishPassThroughInventoryNativeChecks() {
    Entity fish;
    Object *source = classById(706166303646ULL);
    initializeEntity(fish, *source, source->rect);
    fish.state.active = 1;
    b2World *world = new b2World(b2Vec2(0, 0), true);
    b2Body *proxy = 0;
    createPhysicsEntity(*world, fish, 0, 0, &proxy);
    b2Fixture *detector = proxy->GetFixtureList();
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    int scenes = 0, clouds = 0, platforms = 0, onewaySegments = 0;
    bool filters = true;
    for (int n = 0; n < levelCount; n++) {
        readLevelData(levels[n].file, scene, memory);
        scenes++;
        for (int j = 0; j < scene.geometryCount; j++)
            if (scene.geometry[j].kind == 2)
                onewaySegments += scene.geometry[j].count - 1;
        for (int i = 0; i < scene.objectCount; i++) {
            Object &o = scene.objects[i];
            if (!o.has("CrumblingPlatformBehaviour") && !o.has("PlatformBehaviour"))
                continue;
            clouds += o.has("CrumblingPlatformBehaviour");
            platforms += o.has("PlatformBehaviour");
            filters &= fishSurfaceFilterNative(o, detector, *world);
        }
    }
    Object *safe = classById(14843448166857854273ULL);
    filters &= safe && fishSurfaceFilterNative(*safe, detector, *world);
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    delete world;
    check(
        scenes == 74 && clouds == 121 && platforms == 15 && onewaySegments > 0 && filters,
        "All74scenes:121crumbling clouds,15moving one-way platforms and the unused Safe Platform match independently decoded native detector masks");
    check(
        fishSurfacePathNative(0) && fishSurfacePathNative(1),
        "Native-admitted solid and one-way terrain still reflect fish; disabling the one-way solver contact does not suppress its independent detector callback");
    check(
        fishSurfacePathNative(2) && fishSurfacePathNative(3),
        "Real sensor and collision-masked surfaces let fish pass without reflection while admitted solid walls remain detectable");
    char summary[200];
    wsprintfA(summary,
              "scenes=%d clouds=%d movingPlatforms=%d oneWaySegments=%d filterMatch=%d\r\n", scenes,
              clouds, platforms, onewaySegments, filters);
    atomicWrite("test-output/fish-pass-through-inventory.txt", summary, lstrlenA(summary));
}

void fishCloudNativeChecks() {
    interfaceEnabled = musicEnabled = soundEnabled = debugMode = false;
    bool filters = true, below = true, above = true, contactFree = true, speed = true;
    int cases = 0;
    char rows[10000] = "fps,side,entry,crossed,cloudContacts,minY1000,maxY1000,alive\r\n";
    fishPassThroughInventoryNativeChecks();
    const int rates[] = {30, 120, 240};
    for (int rate = 0; rate < 3; rate++)
        for (int side = 0; side < 2; side++)
            for (int entry = 0; entry < 2; entry++) {
                int selected[3], boss = prepareWizardWalkSpawnNative(1, 4, selected);
                if (boss < 0) {
                    filters = false;
                    continue;
                }
                int target = selected[1];
                for (int i = 0; i < entityCount; i++)
                    if (i != target && i != playerIndex && entities[i].kind == ENEMY)
                        entities[i].state.gone = 1;
                Entity &fish = entities[target];
                fish.state.x = 4416;
                fish.state.y = entry ? 112.0f : 48.0f;
                fish.state.vx = fish.state.vy = 0;
                fish.state.gone = 0;
                fish.state.active = 1;
                DiagonalRuntimeState &d = diagonalRuntime[target];
                d.directionX = (side ? -1 : 1) * .707106781f;
                d.directionY = (entry ? -1 : 1) * .707106781f;
                d.effectiveSpeed = 100;
                d.selected = 1;
                restorePhysics();
                bodies[playerIndex]->SetType(b2_staticBody);
                b2Fixture *proxy = oscillatorProxies[target]->GetFixtureList();
                b2ContactFilter filter;
                int clouds = 0;
                for (int i = 0; i < entityCount; i++)
                    if (entities[i].crumble && bodies[i]) {
                        Rect r = hitRect(entities[i]);
                        if (r.l >= 4288 && r.r <= 4576 && r.t > 79.9f && r.t < 80.1f) {
                            clouds++;
                            filters &= !filter.ShouldCollide(proxy, bodies[i]->GetFixtureList());
                        }
                    }
                filters &= clouds == 9 && proxy->GetFilterData().maskBits == (1 | 8 | 32);
                FishCloudContactsNative *probe = new FishCloudContactsNative(target);
                physicsWorld->SetContactListener(probe);
                bool crossed = false;
                float minY = fish.state.y, maxY = fish.state.y;
                for (int step = 0; step < rates[rate] * 1.1f; step++) {
                    tick(1.0f / rates[rate], false, false);
                    minY = minf(minY, fish.state.y);
                    maxY = maxf(maxY, fish.state.y);
                    crossed |= entry ? fish.state.y < 62 : fish.state.y > 92;
                    speed &= absf(fish.state.vx * fish.state.vx + fish.state.vy * fish.state.vy -
                                  10000) < 1 &&
                             !fish.state.gone;
                }
                if (entry)
                    above &= crossed;
                else
                    below &= crossed;
                contactFree &= probe->cloudContacts == 0;
                char row[160];
                wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d\r\n", rates[rate], side, entry, crossed,
                          probe->cloudContacts, (int)(minY * 1000), (int)(maxY * 1000),
                          !fish.state.gone);
                lstrcatA(rows, row);
                physicsWorld->SetContactListener(originalContacts);
                delete probe;
                cases++;
            }
    check(
        cases == 12 && filters,
        "Actual Wizard fish detector excludes all nine native loose-category clouds at the fixture filter before any callback");
    check(
        below,
        "Actual Wizard fish passes upward through the complete cloud row from below in both horizontal directions at30/120/240Hz");
    check(
        above,
        "Actual Wizard fish also passes downward through the cloud row instead of reflecting from a category it does not detect");
    check(
        contactFree && speed,
        "All12actual fish-cloud paths generate zero cloud detector contacts and preserve the original100px/s speed and live state");
    atomicWrite("test-output/fish-cloud-observations.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/fish-cloud-results.txt", testLog, lstrlenA(testLog));
    startStage(0);
    readyTimer = 0;
}
