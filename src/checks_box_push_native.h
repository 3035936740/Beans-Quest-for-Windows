#pragma once

// Regression coverage for box push native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void boxPushNativeChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    check(
        b2MixRestitution(-1, .5f) == 0 && b2MixRestitution(.5f, -1) == 0 &&
            b2MixRestitution(.25f, .5f) == .5f,
        "Native restitution sentinel disables Bean impacts in either fixture order without removing ordinary box restitution");
    {
        b2World *world = new b2World(b2Vec2(0, 0), true);
        world->SetContinuousPhysics(false);
        b2BodyDef d;
        d.type = b2_dynamicBody;
        d.fixedRotation = true;
        b2CircleShape shape;
        shape.m_radius = .5f;
        b2FixtureDef f;
        f.shape = &shape;
        f.density = 1;
        b2Body *a = world->CreateBody(&d);
        a->CreateFixture(&f);
        d.position.x = .9f;
        f.density = 10;
        b2Body *b = world->CreateBody(&d);
        b->CreateFixture(&f);
        world->Step(.001f, 1, 1);
        float da = -a->GetPosition().x, db = b->GetPosition().x - .9f;
        check(
            da > .001f && absf(da - db) < .00001f,
            "A real unequal-mass contact separates by equal native position corrections while keeping its ten-to-one physical mass ratio");
        delete world;
    }
    startStage(stageById(13151310487044325505ULL));
    readyTimer = 0;
    int boxes[64], count = 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->klass == 2913132245228912724ULL)
            boxes[count++] = i;
    bool material = count == 27;
    for (int k = 0; k < count; k++) {
        int i = boxes[k];
        b2Body *b = bodies[i];
        const b2PolygonShape *shape = (const b2PolygonShape *)b->GetFixtureList()->GetShape();
        material &= b->GetType() == b2_dynamicBody && b->GetGravityScale() == 1 &&
                    absf(b->GetMass() - .375f) < .00001f &&
                    absf(shape->m_vertices[1].x - shape->m_vertices[0].x - .5f) < .00001f &&
                    b->GetFixtureList()->GetFriction() == .2f &&
                    b->GetFixtureList()->GetRestitution() == .5f;
    }
    check(
        material,
        "All27 real1-7 crates retain authored32px cores, density1.5, friction0.2 and restitution0.5 with native world gravity");
    char *log = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1000000);
    lstrcpyA(log, "frame,index,x1000,y1000,vx1000,vy1000,awake\r\n");
    float settled[64] = {};
    bool stable = true;
    for (int frame = 0; frame <= 1200; frame++) {
        if (frame == 600)
            for (int k = 0; k < count; k++)
                settled[k] = entities[boxes[k]].state.y;
        if (frame == 1200)
            for (int k = 0; k < count; k++)
                stable &= absf(entities[boxes[k]].state.y - settled[k]) < .02f;
        if (frame % 30 == 0)
            for (int k = 0; k < count; k++) {
                int i = boxes[k];
                State &s = entities[i].state;
                char line[180];
                wsprintfA(line, "%d,%d,%d,%d,%d,%d,%d\r\n", frame, i, (int)(s.x * 1000),
                          (int)(s.y * 1000), (int)(s.vx * 1000), (int)(s.vy * 1000),
                          bodies[i]->IsAwake());
                lstrcatA(log, line);
            }
        tick(1.0f / 120, false, false);
    }
    check(
        stable,
        "Real1-7 untouched stacks stop drifting after settling while Bean continues automatic jumps");
    atomicWrite("test-output/box-push-native.csv", log, lstrlenA(log));
    HeapFree(GetProcessHeap(), 0, log);
    startStage(stageById(13151310487044325505ULL));
    readyTimer = 0;
    int box = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->klass == 2913132245228912724ULL) {
            box = i;
            break;
        }
    bool slope = false;
    if (box >= 0) {
        current.geometryCount = 0;
        for (int i = 0; i < entityCount; i++)
            if (i != box && i != playerIndex)
                entities[i].state.gone = 1;
        Entity &crate = entities[box];
        crate.state.x = 300;
        crate.state.y = 100;
        crate.state.angle = 20;
        crate.state.vx = crate.state.vy = 0;
        Entity &bean = entities[playerIndex];
        bean.state.x = 300;
        bean.state.y = 131;
        bean.state.vx = 0;
        bean.state.vy = -60;
        restorePhysics();
        bodies[box]->SetType(b2_staticBody);
        for (int t = 0; t < 30; t++) {
            float beforeVx = bean.state.vx;
            tick(1.0f / 120, false, false);
            if (ordinaryJumpLaunchedNative) {
                float nx = tomOrdinaryNormalNative.x;
                float expectedX =
                    approach(beforeVx, 0, bean.physics->num("stop-acceleration", 200) / 120) +
                    bean.jumpPower * nx * .2f;
                float expectedY = bean.jumpPower * root(1 - nx * nx * .04f) +
                                  bean.physics->num("gravity", -400) / 120;
                slope = absf(nx) > .3f && absf(bean.state.vx - expectedX) < .002f &&
                        absf(bean.state.vy - expectedY) < .002f;
                break;
            }
        }
    }
    check(
        slope,
        "An actual tilted1-7 crate contact applies the native horizontal jump kick and normalized vertical launch in production gameplay");
    startStage(0);
}

void boxPushProbeNative() {
    boxPushNativeChecks();
}
