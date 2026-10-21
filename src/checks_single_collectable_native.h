#pragma once

// Regression coverage for single collectable native. Runs only in the isolated BQ_ENABLE_TESTS executable.

bool singleCollectableFixtureNative(Entity &e, b2Body *body) {
    b2Fixture *f = body ? body->GetFixtureList() : 0;
    const char *type =
        e.kind == SALAMANDER ? "CollectableSalamanderBehaviour" : "UnbeanPotionBehaviour";
    return body && body->GetType() == b2_kinematicBody && !body->IsFixedRotation() && e.circle &&
           e.physics == e.source->behavior(type) &&
           circleFixtureGeometryMatchesNative(body, e.hit) &&
           circleFixtureDefaultMaterialNative(body) && f->IsSensor() &&
           f->GetFilterData().categoryBits == 4 && f->GetFilterData().maskBits == 0xffff;
}

int prepareSingleCollectableNative(u64 klass, int mode) {
    interfaceEnabled = debugMode = musicEnabled = false;
    soundEnabled = true;
    stopMusic();
    startStage(0);
    readyTimer = 0;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex)
            entities[i].state.gone = 1;
    Entity &p = entities[playerIndex];
    p.state.x = p.state.y = 128;
    p.state.vx = p.state.vy = 0;
    p.state.timer = .2f;
    Object &source = *classById(klass);
    Rect r = source.rect;
    Vec c = center(r);
    int index = entityCount;
    addEntity(source, shifted(r, 150 - c.x, (mode ? 152 : 128) - c.y));
    rebuildPhysics();
    stopSounds();
    clearEffects();
    return index;
}

void singleCollectableNativeChecks() {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    int count = 0, axolotls = 0, potions = 0;
    bool fixtures = true;
    b2World *world = new b2World(b2Vec2(0, 0), true);
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scene, memory);
        for (int i = 0; i < scene.objectCount; i++) {
            Object &o = scene.objects[i];
            if (!o.has("CollectableSalamanderBehaviour") && !o.has("UnbeanPotionBehaviour"))
                continue;
            Entity e;
            initializeEntity(e, o, o.rect);
            count++;
            axolotls += e.kind == SALAMANDER;
            potions += e.kind == POTION;
            b2Body *b = createPhysicsEntity(*world, e, 0);
            fixtures &= singleCollectableFixtureNative(e, b);
            if (b)
                world->DestroyBody(b);
        }
    }
    delete world;
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        count == 53 && axolotls == 52 && potions == 1 && fixtures,
        "All52authored axolotls and the original potion inherit centred width-derived Circle fixtures, native material and configuration, with kinematic sensor activation");
    int item = prepareSingleCollectableNative(13485834099348442826ULL, 1);
    tick(1 / 240.0f, false, false);
    check(
        !entities[item].state.gone && salamander == 0,
        "Actual Bean overlapping an axolotl sprite-box corner outside the native circle cannot collect it");
    item = prepareSingleCollectableNative(13485834099348442826ULL, 0);
    u32 voices = audioVoicesStarted;
    tick(1 / 240.0f, false, false);
    bool collected = entities[item].state.gone && salamander == 1 &&
                     audioVoicesStarted == voices + 1 && liveEffects() > 0;
    tick(1 / 240.0f, false, false);
    check(
        collected && salamander == 1 && audioVoicesStarted == voices + 1,
        "A real circular side contact still collects the axolotl, marks the stage item and emits exactly one original sound/effect");
    bool potion = true;
    for (int mode = 0; mode < 2; mode++) {
        item = prepareSingleCollectableNative(8048939671444015ULL, mode);
        tick(1 / 240.0f, false, false);
        potion &= mode ? !entities[item].state.gone && !playerWalkMode
                       : entities[item].state.gone && playerWalkMode;
    }
    check(
        potion,
        "The original32by40potion retains its16pixel circle: side contact transforms Bean while a sprite corner does not");
    bool overrides = true;
    for (int kind = 0; kind < 2; kind++) {
        Object source = *classById(kind ? 8048939671444015ULL : 13485834099348442826ULL);
        source.rect = Rect{-24, -9, 24, 9};
        Pair pairs[] = {
            {(char *)"adjust-left", (char *)"3"},   {(char *)"adjust-right", (char *)"-7"},
            {(char *)"adjust-bottom", (char *)"2"}, {(char *)"adjust-top", (char *)"4"},
            {(char *)"density", (char *)"4"},       {(char *)"restitution", (char *)"0.25"},
            {(char *)"friction", (char *)"0.1"},    {(char *)"sensor", (char *)"0"},
            {(char *)"fixed-rotation", (char *)"1"}};
        Behavior behavior = {
            0, (char *)(kind ? "UnbeanPotionBehaviour" : "CollectableSalamanderBehaviour"),
            Props{9, pairs}};
        source.behaviorCount = 1;
        source.behaviors = &behavior;
        Entity e;
        initializeEntity(e, source, source.rect);
        world = new b2World(b2Vec2(0, 0), true);
        b2Body *body = createPhysicsEntity(*world, e, 0);
        b2Fixture *f = body ? body->GetFixtureList() : 0;
        overrides &= body && body->GetType() == b2_kinematicBody && body->IsFixedRotation() &&
                     circleFixtureGeometryMatchesNative(body, Rect{-21, -7, 17, 13}) &&
                     f->IsSensor() && absf(f->GetDensity() - 4) < .001f &&
                     absf(f->GetRestitution() - .25f) < .001f &&
                     absf(f->GetFriction() - .1f) < .001f;
        delete world;
    }
    check(
        overrides,
        "Controlled inherited Circle properties honor asymmetric bounds/material/rotation while native single-collectable activation overrides sensor0 and body type");
    bool helper = true;
    for (int kind = 0; kind < 2; kind++) {
        item =
            prepareSingleCollectableNative(kind ? 8048939671444015ULL : 13485834099348442826ULL, 0);
        Entity &p = entities[playerIndex];
        p.state.x = 1000;
        p.state.y = 1000;
        Object &source = *classById(937947234821ULL);
        int bird = entityCount;
        addEntity(source,
                  shifted(source.rect, 150 - center(source.rect).x, 128 - center(source.rect).y));
        entities[bird].state.active = 0;
        rebuildPhysics();
        bool contact = false;
        for (int frame = 0; frame < 2; frame++) {
            tick(1 / 240.0f, false, false);
            contact |= sensorBodyContact(bodies[item], bodies[bird]);
        }
        helper &= contact && !entities[item].state.gone && !salamander && !playerWalkMode;
    }
    check(
        helper,
        "Actual helper-body sensor contacts cannot collect single axolotls or potions on behalf of Tom, preserving the native type check");
    item = prepareSingleCollectableNative(13485834099348442826ULL, 0);
    tick(1 / 240.0f, false, false);
    captureCheckpoint(false);
    char path[MAX_PATH];
    savePath(path, "single-collectable-checkpoint.dat");
    bool disk = entities[item].state.gone && atomicWrite(path, &checkpoint, sizeof(checkpoint));
    rebuildPhysics();
    disk &= readCheckpoint(path);
    if (disk)
        restoreCheckpoint();
    check(
        disk && entities[item].state.gone && salamander == 1 &&
            singleCollectableFixtureNative(entities[item], bodies[item]) &&
            !bodies[item]->IsActive() && checkpoint.version == 28 && sizeof(Snapshot) == 543392,
        "Private disk reload and world reconstruction retain the circular fixture and collected axolotl state without changing version28 checkpoint bytes");
    u32 campaign = hashBytes(entities, sizeof(Entity) * entityCount),
        saved = hashBytes(&checkpoint, sizeof(checkpoint));
    navigate(137502285691ULL);
    int ending = 0;
    bool independent = true;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].kind == SALAMANDER) {
            ending++;
            independent &= singleCollectableFixtureNative(page.playfield[i], page.bodies[i]);
        }
    check(
        ending == 3 && independent &&
            campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
            saved == hashBytes(&checkpoint, sizeof(checkpoint)),
        "All three actual ending axolotls use native circular kinematic sensor fixtures in their separate world without mutating campaign state");
    atomicWrite("test-output/single-collectable-results.txt", testLog, lstrlenA(testLog));
    soundEnabled = false;
    stopSounds();
    startStage(0);
    readyTimer = 0;
}
