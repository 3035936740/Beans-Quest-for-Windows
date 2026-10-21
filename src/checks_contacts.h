#pragma once

// Regression coverage for contacts. Runs only in the isolated BQ_ENABLE_TESTS executable.

void isolateContactScene(int keep) {
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != keep && i != playerIndex)
            entities[i].state.gone = 1;
    rebuildPhysics();
    readyTimer = 0;
}

void contactFollowupChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = false;
    debugMode = true;
    musicEnabled = soundEnabled = false;
    const int switchWorlds[] = {3, 4}, switchLevels[] = {6, 10};
    for (int scene = 0; scene < 2; scene++) {
        int stage = motionStage(switchWorlds[scene], switchLevels[scene]);
        if (stage < 0) {
            check(false, "Reported switch stage exists");
            continue;
        }
        startStage(stage);
        int sw = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == SWITCH) {
                sw = i;
                break;
            }
        check(sw >= 0, "Reported switch level retains its original switch");
        if (sw < 0)
            continue;
        isolateContactScene(sw);
        Entity &e = entities[sw];
        State &bean = entities[playerIndex].state;
        int sensors = 0;
        for (b2Fixture *f = bodies[sw]->GetFixtureList(); f; f = f->GetNext())
            if (f->IsSensor())
                sensors++;
        check(sensors == 2, "Original switch has separate upper and lower sensor faces");
        if (switchOn(e))
            alternationBits ^= e.mask;
        int bits = alternationBits;
        bean.x = e.state.x;
        bean.y = e.state.y - 20;
        bean.vx = bean.vy = 0;
        restorePhysics();
        for (int i = 0; i < 3; i++) {
            bean.y = e.state.y - 20;
            bean.vy = 0;
            tick(dt, false, false);
        }
        check(alternationBits == bits, "OFF switch ignores contact with its lower inactive face");
        bean.y = e.state.y + 20;
        bean.vy = 0;
        restorePhysics();
        tick(dt, false, false);
        bool once = switchOn(e);
        int after = alternationBits;
        for (int i = 0; i < 100; i++) {
            bean.y = e.state.y + 20;
            bean.vx = bean.vy = 0;
            tick(dt, false, false);
        }
        check(
            once && switchOn(e) && after == alternationBits,
            "Pressing the upper face flips once and cannot repeatedly toggle while Bean stays on the same side");
        bean.y = e.state.y - 20;
        bean.vy = 0;
        restorePhysics();
        tick(dt, false, false);
        check(!switchOn(e) && (alternationBits & e.mask) == (bits & e.mask),
              "Only the opposite lower face resets an ON switch");
    }
    startStage(0);
    int seesaw = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].rotate == 2) {
            seesaw = i;
            break;
        }
    if (seesaw >= 0) {
        isolateContactScene(seesaw);
        Entity &e = entities[seesaw];
        b2Body &body = *bodies[seesaw];
        b2RevoluteJoint &joint = *hinges[seesaw];
        check(
            !joint.IsLimitEnabled() && body.GetAngularDamping() == 0 && joint.IsMotorEnabled(),
            "Seesaw uses original friction torque without invented angle limits or angular damping");
        State &bean = entities[playerIndex].state;
        bean.x = e.state.x + 50;
        bean.y = hitRect(e).t - entities[playerIndex].hit.b + 18;
        bean.vx = 0;
        bean.vy = -100;
        restorePhysics();
        float peak = 0, torque = 0;
        for (int i = 0; i < 60; i++) {
            tick(dt, false, false);
            peak = maxf(peak, absf(e.state.angle));
            torque = maxf(torque, absf(joint.GetMotorTorque(1 / dt)));
        }
        check(torque <=
                  e.source->behavior("SeesawBehaviour")->num("rotational-friction", 1) + 0.001f,
              "Seesaw correction stays within its original friction torque budget");
        check(peak > 3,
              "A real Bean landing near the seesaw end tips it through the physical joint");
        captureCase("test-output/seesaw-natural-landing.bmp");
    }
    int magnetStage = -1, magnet = -1;
    for (int stage = 0; stage < stageCount && magnet < 0; stage++) {
        if (levels[stageList[stage]].props.num("bean-world") != 5)
            continue;
        startStage(stage);
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == MAGNET) {
                magnet = i;
                magnetStage = stage;
                break;
            }
    }
    check(magnetStage >= 0, "Original fifth-world magnetic floor is present");
    if (magnet >= 0) {
        isolateContactScene(magnet);
        Entity &e = entities[magnet];
        State &bean = entities[playerIndex].state;
        bean.x = e.state.x;
        bean.y = hitRect(e).t - entities[playerIndex].hit.b + 0.1f;
        bean.vx = 0;
        bean.vy = -20;
        restorePhysics();
        int oldJumps = jumps;
        for (int i = 0; i < 60; i++)
            tick(dt, false, false);
        check(
            bean.active == 2 && e.state.active == 2 && jumps == oldJumps &&
                absf(hitRect(entities[playerIndex]).b - hitRect(e).t) < 2,
            "Top magnetic contact keeps Bean grounded and marks the contacted tile and Bean for their original animations");
        captureCase("test-output/magnet-real-contact.bmp");
        bean.x = hitRect(e).r + 32;
        bean.y = hitRect(e).t + 40;
        bean.vy = 0;
        restorePhysics();
        tick(dt, false, false);
        check(bean.active == 1 && e.state.active == 1,
              "Leaving the magnetic tile clears both transient attachment flags");
    }
    startStage(2);
    int spring = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == SPRING) {
            spring = i;
            break;
        }
    if (spring >= 0) {
        isolateContactScene(spring);
        Entity &e = entities[spring];
        const u64 boxes[] = {2913132245228912724ULL, 17501560557495693516ULL};
        for (int n = 0; n < 2; n++) {
            int box = entityCount;
            Object *object = classById(boxes[n]);
            addEntity(*object, shifted(object->rect, e.state.x, e.state.y + 70));
            Entity &target = entities[box];
            target.state.x = e.state.x;
            target.state.y = hitRect(e).t - target.hit.b + 3;
            target.state.vx = 0;
            target.state.vy = -40;
            entities[playerIndex].state.x = e.state.x + 150;
            rebuildPhysics();
            bool launched = false;
            for (int i = 0; i < 30; i++) {
                tick(dt, false, false);
                launched |= target.state.vy > 100;
            }
            check(
                launched,
                "An original dynamic box or spiked block receives a spring launch through its physical contact");
            target.state.gone = 1;
        }
    }
    startStage(0);
    int bouncer = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOUNCER) {
            bouncer = i;
            break;
        }
    if (bouncer < 0) {
        Object *object = 0;
        for (int i = 0; i < classCount; i++)
            if (classes[i].has("PinballBouncerBehaviour")) {
                object = classes + i;
                break;
            }
        if (object) {
            bouncer = entityCount;
            addEntity(*object, shifted(object->rect, 400, 300));
        }
    }
    if (bouncer >= 0) {
        isolateContactScene(bouncer);
        Entity &e = entities[bouncer];
        int box = entityCount;
        Object *object = classById(2913132245228912724ULL);
        addEntity(*object, shifted(object->rect, e.state.x, e.state.y + 50));
        Entity &target = entities[box];
        target.state.x = e.state.x;
        target.state.y = hitRect(e).t - target.hit.b + 2;
        target.state.vy = -40;
        entities[playerIndex].state.x = e.state.x + 150;
        rebuildPhysics();
        bool launched = false;
        for (int i = 0; i < 30; i++) {
            tick(dt, false, false);
            launched |= target.state.vy > 100;
        }
        check(bodies[bouncer] && launched,
              "Pinball bouncer has a real fixture and launches an original dynamic box");
    }
    startStage(0);
    isolateContactScene(-1);
    int wall = entityCount;
    Object *wallObject = classById(12987116682223282109ULL);
    addEntity(*wallObject, Rect{380, 260, 412, 380});
    entities[wall].move = 0;
    rebuildPhysics();
    entities[playerIndex].state.x = 100;
    entities[playerIndex].state.y = 300;
    restorePhysics();
    Bullet &fire = bullets[0];
    memset(&fire, 0, sizeof(fire));
    fire.klass = (int)(classById(6430600862785566ULL) - classes);
    fire.life = 1;
    fire.state.x = 350;
    fire.state.y = 320;
    fire.state.vx = 125;
    bool vanished = false;
    for (int i = 0; i < 60; i++) {
        tick(dt, false, false);
        if (fire.life <= 0) {
            vanished = true;
            break;
        }
    }
    check(vanished && (!shotBodies[0] || !shotBodies[0]->IsActive()),
          "Original gargoyle fireball deactivates on a wall instead of rebounding");
    restartStage();
    isolateContactScene(-1);
    State &bean = entities[playerIndex].state;
    bean.x = 300;
    bean.y = 340;
    bean.vx = 0;
    bean.vy = -200;
    restorePhysics();
    memset(&fire, 0, sizeof(fire));
    fire.klass = (int)(classById(6430600862785566ULL) - classes);
    fire.life = 1;
    fire.state.x = 300;
    fire.state.y = 300;
    fire.state.vx = 0;
    fire.state.vy = 0;
    bool died = false;
    for (int i = 0; i < 30; i++) {
        tick(dt, false, false);
        if (deathTimer > 0) {
            died = true;
            break;
        }
    }
    check(
        died && fire.life <= 0,
        "Bean touching a fireball from above dies and the fireball disappears instead of acting as a trampoline");
    startStage(0);
    int source = entityCount;
    Object *original = classById(303150949257ULL);
    addEntity(*original, shifted(original->rect, 350, 300));
    resetProjectileAnimationsNative();
    Pair values[] = {{(char *)"bulletClass", (char *)"6430600862785566"},
                     {(char *)"targetingMode", (char *)"static"},
                     {(char *)"targetValidAngles", (char *)"180"},
                     {(char *)"bulletPoolSize", (char *)"2"}};
    Props config = {4, values};
    shoot(entities[source], config);
    shoot(entities[source], config);
    shoot(entities[source], config);
    int active = 0;
    bool left = true;
    for (int i = 0; i < 64; i++)
        if (bullets[i].life > 0) {
            active++;
            left &= bullets[i].state.vx < -120;
        }
    check(
        active == 2 && left,
        "Static fireball sources honour allowed left-facing angles and an explicit two-shot test capacity");
    // Preserve the real adjusted fixtures, but construct a bounded squeeze
    // so a solver correction cannot conceal Bean being driven into a wall.
    startStage(0);
    isolateContactScene(-1);
    int mover = entityCount;
    Object *marble = classById(978266912024ULL);
    addEntity(*marble, shifted(marble->rect, 280, 300));
    Entity &moving = entities[mover];
    moving.moveX = 1;
    moving.moveY = 0;
    moving.lo = -80;
    moving.hi = 80;
    moving.speed = 100;
    moving.state.aux = 1;
    int obstacle = entityCount;
    addEntity(*wallObject, Rect{318, 240, 350, 400});
    entities[obstacle].move = 0;
    State &squeezed = entities[playerIndex].state;
    squeezed.x = 300;
    squeezed.y = 300;
    squeezed.vx = squeezed.vy = 0;
    rebuildPhysics();
    bool crushed = false;
    float worstRight = hitRect(entities[playerIndex]).r;
    char squeezeTrace[8192] = {};
    for (int i = 0; i < 50; i++) {
        tick(dt, false, false);
        worstRight = maxf(worstRight, hitRect(entities[playerIndex]).r);
        char row[160];
        wsprintfA(row, "%d Bean=%d,%d block=%d,%d direction=%d death=%d contacts=", i,
                  (int)(squeezed.x * 100), (int)(squeezed.y * 100), (int)(moving.state.x * 100),
                  (int)(moving.state.y * 100), (int)moving.state.aux, (int)(deathTimer * 100));
        lstrcatA(squeezeTrace, row);
        for (b2ContactEdge *edge = bodies[playerIndex]->GetContactList(); edge; edge = edge->next)
            if (edge->contact->IsTouching() && edge->contact->IsEnabled() &&
                edge->contact->GetManifold()->pointCount) {
                b2WorldManifold m;
                edge->contact->GetWorldManifold(&m);
                wsprintfA(row, "%d:(%d,%d) ", (int)edge->other->GetUserData() - 1,
                          (int)(m.normal.x * 100), (int)(m.normal.y * 100));
                lstrcatA(squeezeTrace, row);
            }
        lstrcatA(squeezeTrace, "\r\n");
        if (deathTimer > 0) {
            crushed = true;
            break;
        }
    }
    atomicWrite("test-output/compression-trace.txt", squeezeTrace, lstrlenA(squeezeTrace));
    check(
        crushed && worstRight < 322,
        "An inward moving marble against a fixed wall kills Bean before he is pushed inside the wall");
    check(
        oscillatorProxies[mover] && bodies[mover]->GetType() == b2_kinematicBody &&
            oscillatorProxies[mover]->GetType() == b2_dynamicBody &&
            oscillatorProxies[mover]->GetFixtureList()->GetDensity() == 0,
        "Original oscillator keeps separate kinematic carrier and zero-density dynamic collision proxy");
    startStage(0);
    isolateContactScene(-1);
    State &safe = entities[playerIndex].state;
    safe.x = 300;
    safe.y = 300;
    safe.vx = safe.vy = 0;
    for (int i = 0; i < 2; i++) {
        int wallIndex = entityCount;
        addEntity(*wallObject, Rect{i ? 318.0f : 250.0f, 240, i ? 350.0f : 282.0f, 400});
        entities[wallIndex].move = 0;
    }
    rebuildPhysics();
    for (int i = 0; i < 20; i++)
        tick(dt, false, false);
    check(deathTimer == 0, "Static walls alone do not trigger moving-block compression death");
    startStage(0);
    isolateContactScene(-1);
    Object *magic = 0;
    for (int i = 0; i < classCount; i++)
        if (classes[i].has("FinalBossBulletBehaviour")) {
            magic = classes + i;
            break;
        }
    check(magic != 0, "Original killable wizard magic projectile is present");
    if (magic) {
        State &landing = entities[playerIndex].state;
        landing.x = 300;
        landing.y = 360;
        landing.vx = 0;
        landing.vy = -160;
        restorePhysics();
        Bullet &shot = bullets[0];
        memset(&shot, 0, sizeof(shot));
        shot.klass = (int)(magic - classes);
        shot.life = 1;
        shot.state.x = 300;
        shot.state.y = 300;
        bool destroyed = false, alive = true;
        for (int i = 0; i < 50; i++) {
            tick(dt, false, false);
            alive &= deathTimer == 0;
            if (shot.life <= 0) {
                destroyed = true;
                break;
            }
        }
        check(
            destroyed && alive && entities[playerIndex].state.vy > 0 &&
                (!shotBodies[0] || !shotBodies[0]->IsActive()),
            "Stomping a killable wizard magic projectile removes its body and launches Bean instead of leaving a frozen ball");
    }
    startStage(0);
    isolateContactScene(-1);
    Object *spiderObject = classById(8234400683372717661ULL);
    int spider = entityCount;
    addEntity(*spiderObject, shifted(spiderObject->rect, 300, 300));
    Entity &crawler = entities[spider];
    crawler.state.active = 1;
    crawler.lo = -64;
    crawler.hi = 64;
    for (int side = 0; side < 2; side++) {
        int wallIndex = entityCount;
        addEntity(*wallObject, Rect{260, side ? 352.0f : 216.0f, 340, side ? 384.0f : 248.0f});
        entities[wallIndex].move = 0;
    }
    rebuildPhysics();
    float minY = 300, maxY = 300, maxDrift = 0;
    bool upward = false, downward = false;
    for (int i = 0; i < 720; i++) {
        State &observer = entities[playerIndex].state;
        observer.x = 100;
        observer.y = 300;
        observer.vx = observer.vy = 0;
        tick(dt, false, false);
        minY = minf(minY, crawler.state.y);
        maxY = maxf(maxY, crawler.state.y);
        maxDrift = maxf(maxDrift, absf(crawler.state.x - 300));
        upward |= crawler.state.vy > 10;
        downward |= crawler.state.vy < -10;
    }
    check(
        upward && downward && maxY - minY > 60 && minY > 259 && maxY < 341 && maxDrift < 0.01f,
        "Original vertical spider reverses at actual walls without leaving its rail or drifting sideways");
    camX = 64;
    camY = 120;
    captureCase("test-output/spider-wall-rail.bmp");
    interfaceEnabled = true;
    navigate(worldScenes[4]);
    int railBlock = -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].move) {
            railBlock = i;
            break;
        }
    if (railBlock >= 0) {
        float low = page.playfield[railBlock].state.x, high = low;
        for (int step = 0; step < 720; step++) {
            tickInterface(dt);
            low = minf(low, page.playfield[railBlock].state.x);
            high = maxf(high, page.playfield[railBlock].state.x);
        }
        check(
            low >= 391 - 2 * 100 * dt && high <= 537 + 2 * 100 * dt && high - low > 100 &&
                absf(page.playfield[railBlock].state.y - 232) < 0.01f,
            "Original selector carrier reverses at both walls within the native two-step contact-order allowance and stays on its horizontal rail");
    }
    interfaceEnabled = false;
    debugMode = false;
}
