#pragma once

// Regression coverage for rotation native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int rotationOwner(u64 id) {
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->id == id)
            return i;
    return -1;
}

void rotationNativeChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    const float dt = 1.0f / 120.0f;
    Props empty = {};
    check(
        absf(nativeRotatingMotorSpeed(empty) + b2_pi * 0.5f) < 0.00001f &&
            nativeRotatingMotorPower(empty) == 1,
        "Native rotating defaults are90degrees per second and power1, with motor sign compensated for the reversed port joint order");
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    int scenes = 0, stages = 0, rotors = 0, seesaws = 0, positive = 0, negative = 0, preRotated = 0;
    bool directions = true, materials = true, correction = true, anchors = true;
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scene, memory);
        scenes++;
        if (scene.props.num("bean-world") > 0)
            stages++;
        for (int i = 0; i < scene.objectCount; i++) {
            Object &o = scene.objects[i];
            if (!o.has("RotatingBehaviour") && !o.has("SeesawBehaviour"))
                continue;
            Entity e;
            initializeEntity(e, o, o.rect);
            b2World *world = new b2World(b2Vec2(0, 0), true);
            b2RevoluteJoint *joint = 0;
            b2Body *body = createPhysicsEntity(*world, e, 0, &joint);
            if (!body || !joint) {
                directions = materials = correction = false;
                delete world;
                continue;
            }
            materials &= body->GetType() == b2_dynamicBody && !body->IsFixedRotation() &&
                         body->GetAngularDamping() == 0 && joint->IsMotorEnabled() &&
                         !joint->IsLimitEnabled();
            Vec c = center(o.rect);
            b2Vec2 a = joint->GetAnchorA(), b = joint->GetAnchorB();
            anchors &= absf(a.x / metres - c.x) < 0.01f && absf(a.y / metres - c.y) < 0.01f &&
                       absf(a.x - b.x) < 0.00001f && absf(a.y - b.y) < 0.00001f;
            if (e.rotate == 1) {
                rotors++;
                const Props &p = *o.behavior("RotatingBehaviour");
                float speed = p.num("rotation-speed", 90);
                positive += speed > 0;
                negative += speed < 0;
                preRotated += o.angle != 0;
                float before = body->GetAngle();
                for (int step = 0; step < 30; step++)
                    world->Step(dt, 10, 10);
                float delta = body->GetAngle() - before;
                directions &= delta * speed < 0 &&
                              absf(body->GetAngularVelocity() + speed * 0.017453293f) < 0.001f &&
                              absf(joint->GetMotorSpeed() + speed * 0.017453293f) < 0.001f;
                materials &=
                    absf(joint->GetMotorTorque(1 / dt)) <= p.num("rotation-power", 1) + 0.001f;
            } else {
                seesaws++;
                const float degrees[] = {20, -20, 170, -170, 0.05f, -0.05f};
                const float expected[] = {-1.1888889f, 1.1888889f, 0.6444444f, -0.6444444f, 0, 0};
                for (int n = 0; n < 6; n++) {
                    body->SetTransform(b2Vec2(c.x * metres, c.y * metres),
                                       degrees[n] * 0.017453293f);
                    body->SetAngularVelocity(0);
                    updateSeesaw(e, *body, *joint);
                    correction &= absf(joint->GetMotorSpeed() - expected[n]) < 0.0001f;
                }
                body->SetTransform(b2Vec2(c.x * metres, c.y * metres), 20 * 0.017453293f);
                body->SetAngularVelocity(0);
                for (int step = 0; step < 30; step++) {
                    updateSeesaw(e, *body, *joint);
                    world->Step(dt, 10, 10);
                }
                correction &= body->GetAngle() > 0 && body->GetAngle() < 20 * 0.017453293f;
                materials &= absf(joint->GetMotorTorque(1 / dt)) <=
                             o.behavior("SeesawBehaviour")->num("rotational-friction", 1) + 0.001f;
            }
            delete world;
        }
    }
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        scenes == 74 && stages == 50 && rotors == 12 && seesaws == 18 && positive == 10 &&
            negative == 2 && preRotated == 2,
        "All74 scenes and50 stages are inspected:12 motor rotors,18 passive seesaws,10 positive speeds,2 negative speeds and2 initial90degree boards");
    check(
        directions && anchors,
        "Every original motor-driven board's real Box2D angular velocity follows the native joint direction and retains its original centre pivot");
    check(
        correction && materials,
        "All18 original seesaws keep their correct restoring directions in four quadrants, native dead zone and original motor torque budgets");
    // Actual reported Sewer scene, both original boards and full geometry /
    // water. The stationary observation Bean cannot impart a platform torque.
    startStage(stageById(8458860001749117ULL));
    readyTimer = 0;
    int a = rotationOwner(7258720689966995ULL), b = rotationOwner(8063845108398762ULL);
    check(a >= 0 && b >= 0, "Reported5-09 Sewer resolves both actual authored rotating boards");
    bool sewer = false, restored = false;
    if (a >= 0 && b >= 0) {
        State &bean = entities[playerIndex].state;
        bean.x = 400;
        bean.y = 300;
        bean.vx = bean.vy = 0;
        restorePhysics();
        bodies[playerIndex]->SetType(b2_staticBody);
        float beforeA = entities[a].state.angle, beforeB = entities[b].state.angle;
        for (int step = 0; step < 60; step++)
            tick(dt, false, false);
        float deltaA = entities[a].state.angle - beforeA,
              deltaB = entities[b].state.angle - beforeB;
        sewer = deltaA < -20 && deltaB < -20 && bodies[a]->GetAngularVelocity() < 0 &&
                bodies[b]->GetAngularVelocity() < 0;
        captureCheckpoint(false);
        float saved = entities[a].state.angle;
        for (int step = 0; step < 20; step++)
            tick(dt, false, false);
        restoreCheckpoint();
        restored =
            absf(entities[a].state.angle - saved) < 0.001f && bodies[a]->GetAngularVelocity() < 0;
        for (int step = 0; step < 30; step++)
            tick(dt, false, false);
        restored &= entities[a].state.angle < saved - 10;
        char trace[256];
        wsprintfA(
            trace,
            "scene=8458860001749117 first=7258720689966995 second=8063845108398762 deltaA-times100=%d deltaB-times100=%d speedA-times1000=%d speedB-times1000=%d restored=%d\r\n",
            (int)(deltaA * 100), (int)(deltaB * 100), (int)(bodies[a]->GetAngularVelocity() * 1000),
            (int)(bodies[b]->GetAngularVelocity() * 1000), restored);
        atomicWrite("test-output/sewer-rotation-native.txt", trace, lstrlenA(trace));
    }
    check(
        sewer && restored,
        "Both real Sewer boards rotate in the original negative angular direction, including after an actual checkpoint restore");
    startStage(stageById(1071369106910786ULL));
    readyTimer = 0;
    int negativeBoard = rotationOwner(9799072667983811ULL),
        positiveBoard = rotationOwner(1606928437095812ULL);
    bool opposite = false;
    if (negativeBoard >= 0 && positiveBoard >= 0) {
        State &bean = entities[playerIndex].state;
        bean.x = 64;
        bean.y = 700;
        bean.vx = bean.vy = 0;
        restorePhysics();
        bodies[playerIndex]->SetType(b2_staticBody);
        float beforeN = entities[negativeBoard].state.angle,
              beforeP = entities[positiveBoard].state.angle;
        for (int step = 0; step < 30; step++)
            tick(dt, false, false);
        opposite = entities[negativeBoard].state.angle > beforeN + 10 &&
                   entities[positiveBoard].state.angle < beforeP - 10;
    }
    check(
        opposite,
        "Original Cherry Garden oppositely configured minus90 and plus90 boards rotate in opposite native directions with the real stage intact");
    startStage(0);
}
