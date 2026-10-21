#pragma once

// Regression coverage for cloud spring native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int prepareCloudSpringEdgeNative(int side = 0) {
    int target = prepareWalkPermissionNative(2, side);
    if (target < 0)
        return -1;
    for (int n = 0; n < 240; n++) {
        tick(1 / 120.0f, false, false);
        Entity &e = entities[target];
        if (walkCloudSpringStepNative(e, bodies[target], entities, entityCount,
                                      walkDetector(target, 5), walkDetector(target, 6)))
            return target;
    }
    return -1;
}

bool cloudSpringAcceptedNative(int target) {
    return target >= 0 &&
           walkCloudSpringStepNative(entities[target], bodies[target], entities, entityCount,
                                     walkDetector(target, 5), walkDetector(target, 6));
}

bool cloudSpringActualLaunchNative(int target, int steps = 480) {
    for (int n = 0; n < steps; n++) {
        tick(1 / 120.0f, false, false);
        for (int i = 0; i < springIncomingCount; i++)
            if (springIncoming[i].target == target && springIncoming[i].solved &&
                springIncoming[i].solvedY < 0 && entities[target].state.vy > 300)
                return true;
    }
    return false;
}

void cloudSpringNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/cloud-spring-saves");
    CreateDirectoryA(saveRoot, 0);
    int target = prepareCloudSpringEdgeNative(), spring = -1;
    bool ready = target >= 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == SPRING && bodies[i]) {
            Rect r = shifted(boxGroupMatrixFixture(entities[i]), entities[i].state.x,
                             entities[i].state.y);
            if (r.l < 4256 && r.r > 4256 && r.t > 63 && r.t < 65) {
                spring = i;
                break;
            }
        }
    ready &= spring >= 0;
    check(
        ready,
        "Actual blue-walker cloud lip resolves a reachable spring from real contact geometry without an arena-coordinate override");
    bool absent = false, distant = false, filtered = false, blocked = false, ordinary = false,
         permission = false, ui = false, replay = false;
    if (ready) {
        Entity &e = entities[target];
        b2Body *body = bodies[target];
        bodies[spring]->SetActive(false);
        absent = !cloudSpringAcceptedNative(target);
        bodies[spring]->SetActive(true);
        b2Vec2 original = bodies[spring]->GetPosition();
        bodies[spring]->SetTransform(original - b2Vec2(0, 64 * metres), 0);
        distant = !cloudSpringAcceptedNative(target);
        bodies[spring]->SetTransform(original, 0);
        b2Fixture *main = 0, *front = 0;
        for (b2Fixture *f = body->GetFixtureList(); f; f = f->GetNext()) {
            if (!f->IsSensor())
                main = f;
            else if (f->GetUserData() == (void *)6)
                front = f;
        }
        b2Filter originalFilter = main->GetFilterData(), off = originalFilter;
        off.maskBits = 0;
        main->SetFilterData(off);
        filtered = !cloudSpringAcceptedNative(target);
        main->SetFilterData(originalFilter);
        b2Vec2 from = body->GetWorldPoint(((const b2PolygonShape *)front->GetShape())->m_centroid);
        b2BodyDef bd;
        bd.position = from - b2Vec2(0, 8 * metres);
        b2Body *wall = physicsWorld->CreateBody(&bd);
        b2PolygonShape shape;
        shape.SetAsBox(8 * metres, 1 * metres);
        wall->CreateFixture(&shape, 0);
        blocked = !cloudSpringAcceptedNative(target);
        physicsWorld->DestroyBody(wall);
        e.walkCanWalk = 0;
        permission = !cloudSpringAcceptedNative(target);
        e.walkCanWalk = 1;
        bool touched = false;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].crumble) {
                entities[i].crumble = 0;
                touched = true;
            }
        ordinary = touched && !cloudSpringAcceptedNative(target);
        for (int i = 0; i < entityCount; i++)
            if (entities[i].source->has("CrumblingPlatformBehaviour"))
                entities[i].crumble = 1;
        UiScene sample = {};
        sample.playfield = entities;
        sample.playfieldCount = entityCount;
        sample.bodies = bodies;
        sample.physics = physicsWorld;
        bool facing = e.walkRight != 0;
        creditsWalkContacts(sample, target);
        ui = cloudSpringAcceptedNative(target) && e.walkRight == facing;
        bodies[spring]->SetActive(false);
        updateWalkContacts(target);
        absent &= e.walkRight == 0;
        bodies[spring]->SetActive(true);
    }
    check(
        absent && distant,
        "Missing disabled or too-distant lower springs preserve ordinary cliff reversal instead of permitting a fall");
    check(
        filtered && blocked,
        "Masked-out springs and intervening solid obstacles never count as reachable step-down support");
    check(
        ordinary && permission,
        "Ordinary non-cloud ledges and walkers without solved-support permission retain the existing edge rules");
    check(
        ui,
        "The shared selector and credits walking path permits the same actual cloud-to-spring descent");
    target = prepareCloudSpringEdgeNative();
    if (target >= 0) {
        captureCheckpoint(false);
        char path[MAX_PATH];
        savePath(path, "edge.dat");
        bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
        bool uninterrupted = cloudSpringActualLaunchNative(target);
        bool loaded = written && readCheckpoint(path);
        if (loaded)
            restoreCheckpoint();
        bodies[playerIndex]->SetType(b2_staticBody);
        replay = loaded && uninterrupted && entities[target].reverseEdge == 1 &&
                 cloudSpringActualLaunchNative(target);
    }
    check(
        replay,
        "A real checkpoint at the intact cloud lip still descends and launches after disk restore with authored reverse-at-edge enabled");
    atomicWrite("test-output/cloud-spring-results.txt", testLog, lstrlenA(testLog));
    lstrcpyA(saveRoot, oldRoot);
    startStage(0);
    readyTimer = 0;
}
