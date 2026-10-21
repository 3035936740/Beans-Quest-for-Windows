#pragma once

// Regression coverage for circle fixture native. Runs only in the isolated BQ_ENABLE_TESTS executable.

bool circleFixtureGeometryMatchesNative(b2Body *body, Rect adjusted) {
    if (!body)
        return false;
    b2Fixture *f = body->GetFixtureList();
    if (!f || f->GetShape()->GetType() != b2Shape::e_circle)
        return false;
    const b2CircleShape &s = *(const b2CircleShape *)f->GetShape();
    return absf(s.m_radius / metres - (adjusted.r - adjusted.l) * .5f) < .0001f &&
           absf(s.m_p.x / metres - (adjusted.l + adjusted.r) * .5f) < .0001f &&
           absf(s.m_p.y / metres - (adjusted.b + adjusted.t) * .5f) < .0001f;
}

bool circleFixtureDefaultMaterialNative(b2Body *body) {
    b2Fixture *f = body ? body->GetFixtureList() : 0;
    return f && absf(f->GetDensity() - 2.5f) < .0001f && absf(f->GetRestitution() - .5f) < .0001f &&
           absf(f->GetFriction() - .2f) < .0001f;
}

bool circleFixtureBodyDefaultsNative() {
    Object variant = *classById(14441582509727370349ULL);
    Props props = {};
    Behavior behavior = {0, (char *)"CirclePhysicsBehaviour", props};
    variant.behaviorCount = 1;
    variant.behaviors = &behavior;
    b2World *world = new b2World(b2Vec2(0, -500 * metres), true);
    bool result = true;
    for (int mode = 0; mode < 2; mode++) {
        behavior.type = (char *)(mode ? "BoxPhysicsBehaviour" : "CirclePhysicsBehaviour");
        Entity e;
        initializeEntity(e, variant, variant.rect);
        b2Body *body = createPhysicsEntity(*world, e, 0);
        result &= body && body->GetType() == b2_dynamicBody && body->GetGravityScale() == 1 &&
                  circleFixtureDefaultMaterialNative(body);
        if (body)
            world->DestroyBody(body);
    }
    delete world;
    return result;
}

bool circleFixtureRotationDefaultsNative() {
    int index = (int)(classById(6430600862785566ULL) - classes);
    Object original = classes[index];
    bool result = true;
    for (int mode = 0; mode < 3; mode++) {
        killableProjectileFixture(6430600862785566ULL, 280, 200, 0, 0);
        Pair fixed = {(char *)"fixed-rotation", (char *)(mode == 2 ? "1" : "0")};
        Behavior behavior = {0, (char *)"CirclePhysicsBehaviour", Props{mode ? 1 : 0, &fixed}};
        Object variant = original;
        variant.behaviorCount = 1;
        variant.behaviors = &behavior;
        classes[index] = variant;
        b2World *world = new b2World(b2Vec2(0, 0), true);
        Entity e;
        initializeEntity(e, variant, variant.rect);
        for (int shape = 0; shape < 2; shape++) {
            behavior.type = (char *)(shape ? "BoxPhysicsBehaviour" : "CirclePhysicsBehaviour");
            initializeEntity(e, variant, variant.rect);
            b2Body *b = createPhysicsEntity(*world, e, 0);
            if (!b) {
                result = false;
                continue;
            }
            b->ApplyAngularImpulse(.2f);
            result &= b->IsFixedRotation() == (mode == 2) &&
                      (mode == 2 ? b->GetAngularVelocity() == 0 : b->GetAngularVelocity() > 0);
            world->DestroyBody(b);
        }
        delete world;
        behavior.type = (char *)"CirclePhysicsBehaviour";
        initializeEntity(e, variant, variant.rect);
        if (shotBodies[0]) {
            physicsWorld->DestroyBody(shotBodies[0]);
            shotBodies[0] = 0;
        }
        shotBodies[0] = createShotBody(0);
        b2Body *b = shotBodies[0];
        b->ApplyAngularImpulse(.2f);
        result &= b->IsFixedRotation() == (mode == 2) &&
                  (mode == 2 ? b->GetAngularVelocity() == 0 : b->GetAngularVelocity() > 0);
        UiScene scene = {};
        scene.physics = new b2World(b2Vec2(0, 0), true);
        b = creditsCreateShotBody(scene, e, 0);
        b->ApplyAngularImpulse(.2f);
        result &= b->IsFixedRotation() == (mode == 2) &&
                  (mode == 2 ? b->GetAngularVelocity() == 0 : b->GetAngularVelocity() > 0);
        delete scene.physics;
        classes[index] = original;
    }
    return result;
}

bool circleFixtureCreditsGravityNative() {
    const u64 ids[] = {12978618438667112518ULL, 537747635566ULL, 6430600862785566ULL,
                       14441582509727370349ULL};
    bool result = true;
    for (int i = 0; i < 4; i++) {
        UiScene s = {};
        s.physics = new b2World(b2Vec2(0, -500 * metres), true);
        Entity e;
        Object &o = *classById(ids[i]);
        initializeEntity(e, o, o.rect);
        e.state.x = 280;
        e.state.y = 200;
        b2Body *body = creditsCreateShotBody(s, e, 0);
        body->SetLinearVelocity(b2Vec2(120 * metres, 80 * metres));
        for (int n = 0; n < 12; n++)
            s.physics->Step(1 / 120.0f, 10, 10);
        b2Vec2 v = body->GetLinearVelocity(), p = body->GetPosition();
        result &=
            absf(v.x / metres - 120) < .001f && absf(v.y / metres - (i == 3 ? 30 : 80)) < .001f;
        if (i < 3)
            result &= absf(p.x / metres - 292) < .001f && absf(p.y / metres - 208) < .001f;
        delete s.physics;
    }
    return result;
}

bool plasmaMarginCaseNative(float offset) {
    killableProjectileFixture(12978618438667112518ULL, 280, 200, 0, 0);
    Entity &bean = entities[playerIndex];
    bean.state.x = 256;
    bean.state.y = 200;
    bean.state.vx = bean.state.vy = 0;
    // Start separated so the original first-frame launch-ignore set cannot
    // conceal the impact. Approach at60px/s until the requested face offset.
    bullets[0].state.x = bean.state.x + bean.hit.r + 12;
    bullets[0].state.y = bean.state.y + (bean.hit.b + bean.hit.t) * .5f;
    bullets[0].state.vx = -60;
    restorePhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    bool hit = false;
    for (int n = 0; n < (int)((12 - offset) * 2 + .1f); n++) {
        tick(1 / 120.0f, false, false);
        hit |= deathTimer > 0 || respawnBusy(respawnTransition);
        if (hit)
            break;
    }
    return hit;
}

void circleFixtureNativeChecks() {
    interfaceEnabled = musicEnabled = soundEnabled = debugMode = false;
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    int scenes = 0, placed = 0, prototypes = 0;
    bool authored = true, classesMatch = true, wide = true;
    b2World *world = new b2World(b2Vec2(0, 0), true);
    for (int n = 0; n < levelCount; n++) {
        readLevelData(levels[n].file, scene, memory);
        scenes++;
        for (int i = 0; i < scene.objectCount; i++) {
            Object &o = scene.objects[i];
            if (!o.has("CirclePhysicsBehaviour"))
                continue;
            Entity e;
            initializeEntity(e, o, o.rect);
            b2Body *b = createPhysicsEntity(*world, e, 0);
            authored &= circleFixtureGeometryMatchesNative(b, e.hit);
            if (b)
                world->DestroyBody(b);
            placed++;
        }
    }
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    for (int i = 0; i < classCount; i++)
        if (classes[i].has("CirclePhysicsBehaviour")) {
            Entity e;
            initializeEntity(e, classes[i], classes[i].rect);
            b2Body *b = createPhysicsEntity(*world, e, 0);
            classesMatch &= circleFixtureGeometryMatchesNative(b, e.hit);
            if (b)
                world->DestroyBody(b);
            prototypes++;
        }
    Entity sample;
    Object &beach = *classById(14441582509727370349ULL);
    initializeEntity(sample, beach, beach.rect);
    sample.hit = Rect{-19, -7, 25, 9};
    b2Body *test = createPhysicsEntity(*world, sample, 0);
    wide &= circleFixtureGeometryMatchesNative(test, sample.hit);
    if (test)
        world->DestroyBody(test);
    sample.hit = Rect{-5, -20, 7, 22};
    test = createPhysicsEntity(*world, sample, 0);
    wide &= circleFixtureGeometryMatchesNative(test, sample.hit);
    if (test)
        world->DestroyBody(test);
    delete world;
    check(
        scenes == 74 && placed == 23 && authored,
        "All23placed CirclePhysics actors across74scenes retain their native adjusted-width radius and local centre");
    check(
        prototypes == 8 && classesMatch,
        "All8original CirclePhysics prototypes preserve the signed native radius, including the plasma class's inverted adjusted bounds");
    check(
        wide,
        "Wide/short and narrow/tall actual circle fixtures retain the width-derived radius and asymmetric adjusted centre instead of selecting min(width,height)");
    const u64 shots[] = {12978618438667112518ULL, 537747635566ULL, 6430600862785566ULL};
    bool campaign = true, credits = true;
    for (int i = 0; i < 3; i++) {
        Object &o = *classById(shots[i]);
        Entity e;
        initializeEntity(e, o, o.rect);
        killableProjectileFixture(shots[i], 280, 200, 0, 0);
        restorePhysics();
        campaign &= circleFixtureGeometryMatchesNative(shotBodies[0], e.hit);
        UiScene s = {};
        s.physics = new b2World(b2Vec2(0, 0), true);
        b2Body *b = creditsCreateShotBody(s, e, 0);
        credits &= circleFixtureGeometryMatchesNative(b, e.hit);
        delete s.physics;
    }
    check(
        campaign,
        "All3original campaign projectile classes use their actual signed CirclePhysics radius and local centre");
    check(
        credits,
        "The separate ending/credits projectile factory preserves the same original CirclePhysics geometry for all3classes");
    int index = (int)(classById(6430600862785566ULL) - classes);
    Object original = classes[index], variant = original;
    Pair changes[] = {{(char *)"adjust-left", (char *)"5"},
                      {(char *)"adjust-right", (char *)"-9"},
                      {(char *)"adjust-bottom", (char *)"-1"},
                      {(char *)"adjust-top", (char *)"3"}};
    Behavior circle = {0, (char *)"CirclePhysicsBehaviour", Props{4, changes}};
    variant.behaviorCount = 1;
    variant.behaviors = &circle;
    killableProjectileFixture(6430600862785566ULL, 280, 200, 0, 0);
    classes[index] = variant;
    restorePhysics();
    bool shifted = circleFixtureGeometryMatchesNative(shotBodies[0], Rect{-11, -17, 7, 19});
    bool material = circleFixtureDefaultMaterialNative(shotBodies[0]);
    Entity offsetShot;
    initializeEntity(offsetShot, variant, variant.rect);
    UiScene offsetScene = {};
    offsetScene.physics = new b2World(b2Vec2(0, 0), true);
    b2Body *offsetBody = creditsCreateShotBody(offsetScene, offsetShot, 0);
    shifted &= circleFixtureGeometryMatchesNative(offsetBody, Rect{-11, -17, 7, 19});
    material &= circleFixtureDefaultMaterialNative(offsetBody);
    delete offsetScene.physics;
    classes[index] = original;
    check(
        shifted,
        "Both projectile factories retain an asymmetric original-style adjusted circle centre rather than forcing it to zero");
    check(
        material && circleFixtureBodyDefaultsNative(),
        "Explicit Box/Circle and both projectile factories retain native missing-property density2.5/restitution0.5/friction0.2; absent static-position creates a dynamic gravitated body");
    check(
        circleFixtureRotationDefaultsNative(),
        "Actual Box/Circle bodies and both projectile factories allow angular impulses when fixed-rotation is absent or zero, and suppress rotation when explicitly one");
    check(
        circleFixtureCreditsGravityNative(),
        "All3real credits projectile classes preserve nongravitated12step trajectories while an ordinary gravitated Circle retains world gravity");
    bool outside = plasmaMarginCaseNative(1), inside = plasmaMarginCaseNative(-3);
    check(
        !outside && inside,
        "Actual original plasma does not hit one pixel outside Bean's box but still hits after its centre penetrates three pixels, following its native negative radius");
    killableProjectileFixture(12978618438667112518ULL, 280, 200, 0, 0);
    restorePhysics();
    captureCheckpoint(false);
    const char *path = "test-output/circle-fixture-checkpoint.dat";
    bool saved = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    if (shotBodies[0]) {
        physicsWorld->DestroyBody(shotBodies[0]);
        shotBodies[0] = 0;
    }
    bool loaded = saved && readCheckpoint(path);
    if (loaded)
        restoreCheckpoint();
    Entity plasma;
    Object &source = *classById(shots[0]);
    initializeEntity(plasma, source, source.rect);
    check(
        loaded && circleFixtureGeometryMatchesNative(shotBodies[0], plasma.hit),
        "A real checkpoint reload rebuilds an active plasma projectile with native signed geometry rather than retaining the old absolute radius");
    char result[200];
    wsprintfA(result, "scenes=%d placed=%d prototypes=%d outsideHit=%d insideHit=%d\r\n", scenes,
              placed, prototypes, outside, inside);
    atomicWrite("test-output/circle-fixture-summary.txt", result, lstrlenA(result));
    atomicWrite("test-output/circle-fixture-results.txt", testLog, lstrlenA(testLog));
    startStage(0);
    readyTimer = 0;
}
