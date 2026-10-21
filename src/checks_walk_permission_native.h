#pragma once

// Regression coverage for walk permission native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int prepareWalkPermissionNative(int category, int side) {
    int selected[3], boss = wizardDismissPrepareNative(1, category, selected), target = selected[0];
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && i != target && entities[i].kind == ENEMY)
            entities[i].state.gone = 1;
    entities[boss].state.gone = 1;
    Entity &e = entities[target];
    e.state.x = side ? 4320.0f : 4192.0f;
    e.state.y = 110;
    e.state.vx = e.state.vy = 0;
    e.walkCanWalk = 0;
    nativeWalkDirection(e, side != 0, true);
    State &bean = entities[playerIndex].state;
    bean.x = 4400;
    bean.y = 350;
    bean.vx = bean.vy = 0;
    restorePhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    return target;
}

void walkPermissionNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/walk-permission-saves");
    CreateDirectoryA(saveRoot, 0);
    interfaceEnabled = musicEnabled = soundEnabled = false;
    bool poses = true, firstContact = true;
    int cases = 0;
    char rows[4096] = "class,side,fps,permission,savedVx1000,nextVx1000,restoredVx1000\r\n";
    for (int category = 2; category <= 8; category += 6)
        for (int side = 0; side < 2; side++)
            for (int rate = 0; rate < 2; rate++) {
                int target = prepareWalkPermissionNative(category, side);
                Entity &e = entities[target];
                float dt = rate ? 1 / 30.0f : 1 / 120.0f;
                for (int n = 0; n < 120 && !e.walkCanWalk; n++)
                    tick(dt, false, false);
                firstContact &= e.walkCanWalk == 1;
                captureCheckpoint(false);
                State saved = e.state;
                int permission = e.walkCanWalk;
                if (category == 2 && side == 0 && rate == 0) {
                    char path[MAX_PATH];
                    savePath(path, "first-landing.dat");
                    atomicWrite(path, &checkpoint, sizeof(checkpoint));
                }
                tick(dt, false, false);
                float vx = e.state.vx;
                e.walkCanWalk = 0;
                e.speed = 17;
                e.reverseEdge = 0;
                e.walkRight = !e.walkRight;
                restoreCheckpoint();
                bodies[playerIndex]->SetType(b2_staticBody);
                bool restored =
                    e.walkCanWalk == permission && e.state.x == saved.x && e.state.y == saved.y;
                tick(dt, false, false);
                poses &= restored && absf(e.state.vx - vx) < .001f;
                cases++;
                char row[220];
                wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d\r\n", category, side, rate ? 30 : 120,
                          permission, (int)(saved.vx * 1000), (int)(vx * 1000),
                          (int)(e.state.vx * 1000));
                lstrcatA(rows, row);
            }
    check(
        firstContact && cases == 8,
        "Both actual Wizard walker classes retain their first solved support permission from both sides at30and120Hz");
    check(
        poses,
        "All8first-landing checkpoints preserve the next-frame horizontal velocity and exact restored pose after clearing live walking state");
    int target = prepareWalkPermissionNative(2, 0);
    Entity &e = entities[target];
    e.walkCanWalk = 1;
    e.state.vx = e.speed + 1;
    walkPrePhysicsNative(e);
    bool impulse = e.walkCanWalk == 1 && e.state.vx == e.speed + 1;
    e.state.vx = -e.speed;
    walkPrePhysicsNative(e);
    check(
        impulse && !e.walkCanWalk && e.state.vx == e.speed,
        "Native walking permission survives an outside horizontal impulse and is consumed at the exact eligible speed boundary");
    e.walkCanWalk = 1;
    e.speed = 0;
    nativeWalkDirection(e, true, true);
    captureCheckpoint(false);
    e.speed = 30;
    e.walkRight = 1;
    e.reverseEdge = 0;
    e.walkCanWalk = 0;
    restoreCheckpoint();
    check(
        e.speed == 0 && e.walkRight == 0 && e.reverseEdge == 1 && e.walkCanWalk == 1,
        "Walking snapshots retain speed, reverse-at-edge, explicit zero-speed direction and permission as four independent native fields");
    target = prepareWalkPermissionNative(2, 0);
    for (int n = 0; n < 120 && !entities[target].walkCanWalk; n++)
        tick(1 / 120.0f, false, false);
    captureCheckpoint(false);
    Snapshot *current = (Snapshot *)HeapAlloc(GetProcessHeap(), 0, sizeof(Snapshot));
    *current = checkpoint;
    SnapshotV23 *old = (SnapshotV23 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV23));
    memcpy(old, current, sizeof(*old) - 4);
    old->version = 23;
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    char path[MAX_PATH];
    savePath(path, "legacy-v23.dat");
    bool read = atomicWrite(path, old, sizeof(*old)) && readCheckpoint(path);
    if (read)
        restoreCheckpoint();
    check(
        read && sizeof(SnapshotV23) == 510624 && sizeof(Snapshot) == 543392 &&
            checkpoint.version == 23 && validSnapshot(checkpoint) &&
            entities[target].walkCanWalk == 0 &&
            checkpoint.ordinaryJump.pending == current->ordinaryJump.pending,
        "Exact510624byte version23 files migrate with their original pending jump and no invented historical walking contact");
    bool reject = true;
    for (int kind = 0; kind < 4; kind++) {
        checkpoint = *current;
        if (kind == 0)
            checkpoint.walking[target].speed = -1;
        if (kind == 1)
            checkpoint.walking[target].reverseEdge = 2;
        if (kind == 2)
            checkpoint.walking[target].right = 2;
        if (kind == 3)
            checkpoint.walking[target].canWalk = 2;
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        reject &= !validSnapshot(checkpoint);
    }
    old->checksum ^= 1;
    savePath(path, "corrupt-v23.dat");
    reject &= atomicWrite(path, old, sizeof(*old)) && !readCheckpoint(path);
    check(
        reject,
        "Walking-state validation rejects invalid flags/speed and a corrupt historical version23 checksum");
    checkpoint = *current;
    HeapFree(GetProcessHeap(), 0, current);
    HeapFree(GetProcessHeap(), 0, old);
    restartStage();
    bool restart = true;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].walk)
            restart &= entities[i].walkCanWalk == 0 &&
                       entities[i].walkRight ==
                           !entities[i].source->behavior("WalkBehaviour")->flag("walk-left");
    check(
        restart && checkpoint.version == 28 && validSnapshot(checkpoint),
        "A real stage restart clears every walking permission and restores each authored initial direction before saving version28");
    interfaceEnabled = true;
    navigate(137502285691ULL);
    int walker = -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].walk && page.bodies[i]) {
            walker = i;
            break;
        }
    bool ui = false;
    if (walker >= 0) {
        for (int i = 0; i < page.playfieldCount; i++)
            if (page.bodies[i])
                page.bodies[i]->SetActive(i == walker);
        Entity &actor = page.playfield[walker];
        actor.walkCanWalk = 0;
        b2Body *body = page.bodies[walker];
        body->SetTransform(b2Vec2(100 * metres, 100 * metres), 0);
        body->SetLinearVelocity(b2Vec2(0, -60 * metres));
        b2BodyDef bd;
        bd.position.Set(100 * metres, 60 * metres);
        b2Body *floor = page.physics->CreateBody(&bd);
        b2PolygonShape shape;
        shape.SetAsBox(200 * metres, 10 * metres);
        floor->CreateFixture(&shape, 0);
        for (int n = 0; n < 90 && !actor.walkCanWalk; n++)
            page.physics->Step(1 / 120.0f, 10, 10);
        ui = actor.walkCanWalk == 1;
        actor.state.vx = 0;
        walkPrePhysicsNative(actor);
        ui &= !actor.walkCanWalk && absf(actor.state.vx) == actor.speed;
        page.physics->DestroyBody(floor);
    }
    check(
        ui,
        "The actual credits/selector contact listener records walker support and consumes the same native permission through a real solver contact");
    atomicWrite("test-output/walk-permission-observations.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/walk-permission-results.txt", testLog, lstrlenA(testLog));
    lstrcpyA(saveRoot, oldRoot);
    interfaceEnabled = false;
    startStage(0);
    readyTimer = 0;
}

bool walkPermissionDiskReloadNative() {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    lstrcpyA(saveRoot, "test-output/walk-permission-saves");
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    char path[MAX_PATH];
    savePath(path, "first-landing.dat");
    bool read = readCheckpoint(path);
    if (read)
        restoreCheckpoint();
    int target = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].bossSpawn == 2) {
            target = i;
            break;
        }
    bool ok = read && checkpoint.version == 28 && target >= 0 &&
              entities[target].walkCanWalk == 1 && entities[target].state.vx == 0;
    float speed = target >= 0 ? entities[target].speed : 0;
    tick(1 / 120.0f, false, false);
    ok &= target >= 0 && absf(entities[target].state.vx - speed) < .001f;
    const char *result = ok ? "PASS fresh-process walking permission disk restoration\r\n"
                            : "FAIL fresh-process walking permission disk restoration\r\n";
    atomicWrite("test-output/walk-permission-fresh-process.txt", result, lstrlenA(result));
    return ok;
}
