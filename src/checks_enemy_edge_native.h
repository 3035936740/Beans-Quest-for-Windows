#pragma once

// Regression coverage for enemy edge native. Runs only in the isolated BQ_ENABLE_TESTS executable.

struct EnemyEdgeRestoreProbeNative : b2ContactListener {
    int target;
    bool seen;
    float incomingX;

    EnemyEdgeRestoreProbeNative(int index) : target(index), seen(false), incomingX(0) {}

    void BeginContact(b2Contact *c) {
        originalContacts->BeginContact(c);
    }

    void EndContact(b2Contact *c) {
        originalContacts->EndContact(c);
    }

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        if (!seen && c->GetManifold()->pointCount && !c->GetFixtureA()->IsSensor() &&
            !c->GetFixtureB()->IsSensor()) {
            b2Body *a = c->GetFixtureA()->GetBody();
            b2Body *b = c->GetFixtureB()->GetBody();
            b2Body *own = (int)a->GetUserData() - 1 == target   ? a
                          : (int)b->GetUserData() - 1 == target ? b
                                                                : 0;
            if (own) {
                seen = true;
                incomingX = own->GetLinearVelocity().x / metres;
            }
        }
        originalContacts->PreSolve(c, old);
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        originalContacts->PostSolve(c, impulse);
    }
};

// Diagnostic contexts, not an assertion of Android equivalence. Keep the
// intact-edge failure below visible until native descent is explained.
void enemyEdgeContextNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/enemy-edge-context-saves");
    CreateDirectoryA(saveRoot, 0);
    interfaceEnabled = false;
    debugMode = true;
    musicEnabled = soundEnabled = false;
    char *rows = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 600000);
    lstrcpyA(rows, "class,side,mode,step,x100,y100,vx100,vy100,ground,left,right,spring,burst\r\n");
    bool intactTurns = true, burstLaunch = true, directLaunch = true, realBursts = true;
    for (int category = 2; category <= 8; category += 6)
        for (int side = 0; side < 2; side++)
            for (int mode = 0; mode < 3; mode++) {
                int selected[3], boss = wizardDismissPrepareNative(1, category, selected),
                                 target = selected[0], cloud = -1;
                for (int i = 0; i < entityCount; i++)
                    if (i != playerIndex && entities[i].kind == ENEMY)
                        entities[i].state.gone = 1;
                entities[boss].state.gone = 1;
                float cloudX = side ? 4304.0f : 4208.0f;
                for (int i = 0; i < entityCount; i++) {
                    Rect r = hitRect(entities[i]);
                    if (entities[i].crumble && r.l < cloudX && r.r > cloudX && r.t > 79 &&
                        r.t < 81) {
                        cloud = i;
                        break;
                    }
                }
                State &bean = entities[playerIndex].state;
                bool burst = false;
                if (mode == 1 && cloud >= 0) {
                    bean.x = cloudX;
                    bean.y = hitRect(entities[cloud]).t - entities[playerIndex].hit.b + 1;
                    bean.vx = 0;
                    bean.vy = -120;
                    restorePhysics();
                    for (int n = 0; n < 30 && entities[cloud].state.active != 2; n++)
                        tick(1 / 120.0f, false, false);
                    burst = entities[cloud].state.active == 2 && !bodies[cloud]->IsActive();
                    realBursts &= burst;
                } else if (mode == 1)
                    realBursts = false;
                bean.x = 4400;
                bean.y = 350;
                bean.vx = bean.vy = 0;
                Entity &e = entities[target];
                e.state.gone = 0;
                e.state.active = 1;
                e.state.x = mode == 2   ? 4256.0f
                            : mode == 1 ? (side ? 4298.0f : 4214.0f)
                                        : (side ? 4320.0f : 4192.0f);
                e.state.y = 110;
                e.state.vx = 0;
                e.state.vy = 0;
                nativeWalkDirection(e, side != 0, true);
                e.walkCanWalk = 0;
                restorePhysics();
                bodies[playerIndex]->SetType(b2_staticBody);
                bool launch = false, turn = false;
                for (int step = 0; step < 240; step++) {
                    tick(1 / 120.0f, false, false);
                    bool contact = false;
                    for (int n = 0; n < springIncomingCount; n++)
                        if (springIncoming[n].target == target && springIncoming[n].solved)
                            contact = true;
                    launch |= contact && e.state.vy > 300;
                    turn |= side ? e.state.aux > 0 : e.state.aux < 0;
                    char row[192];
                    wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", category, side,
                              mode, step, (int)(e.state.x * 100), (int)(e.state.y * 100),
                              (int)(e.state.vx * 100), (int)(e.state.vy * 100),
                              enemyGrounded(target), walkDetector(target, 5),
                              walkDetector(target, 6), contact, burst);
                    lstrcatA(rows, row);
                    if (launch)
                        break;
                }
                if (mode == 0)
                    intactTurns &= launch;
                else if (mode == 1)
                    burstLaunch &= launch;
                else
                    directLaunch &= launch;
            }
    check(
        intactTurns,
        "Both actual Wizard walkers leave intact cloud lips and launch from the lower spring from either side");
    check(
        realBursts,
        "Diagnostic: both adjacent cloud contexts are burst by actual Bean solver contacts before walker placement");
    check(
        burstLaunch,
        "Diagnostic: both Wizard walkers reach and launch from the spring after the adjacent cloud has really burst");
    check(
        directLaunch,
        "Diagnostic: both Wizard walker classes launch when falling directly onto the actual spring");
    int selected[3], boss = wizardDismissPrepareNative(1, 2, selected), target = selected[0];
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && i != target && entities[i].kind == ENEMY)
            entities[i].state.gone = 1;
    entities[boss].state.gone = 1;
    State &bean = entities[playerIndex].state;
    bean.x = 4400;
    bean.y = 350;
    bean.vx = bean.vy = 0;
    Entity &walker = entities[target];
    walker.state.x = 4192;
    walker.state.y = 110;
    walker.state.vx = walker.state.vy = 0;
    nativeWalkDirection(walker, false, true);
    walker.walkCanWalk = 0;
    restorePhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    for (int n = 0; n < 120 && !enemyGrounded(target); n++)
        tick(1 / 120.0f, false, false);
    bool grounded = enemyGrounded(target);
    captureCheckpoint(false);
    EnemyEdgeRestoreProbeNative *probe = new EnemyEdgeRestoreProbeNative(target);
    physicsWorld->SetContactListener(probe);
    tick(1 / 120.0f, false, false);
    float uninterrupted = walker.state.vx;
    bool beforeSeen = probe->seen;
    int beforeVx = (int)(walker.state.vx * 10000), beforeGone = walker.state.gone;
    physicsWorld->SetContactListener(originalContacts);
    restoreCheckpoint();
    bodies[playerIndex]->SetType(b2_staticBody);
    probe->seen = false;
    probe->incomingX = 0;
    physicsWorld->SetContactListener(probe);
    tick(1 / 120.0f, false, false);
    float restored = walker.state.vx;
    char detail[384];
    wsprintfA(
        detail,
        "grounded=%d seenBefore=%d seenAfter=%d uninterruptedVx10000=%d restoredVx10000=%d postVxBefore=%d postVxAfter=%d goneBefore=%d goneAfter=%d active=%d y100=%d\r\n",
        grounded, beforeSeen, probe->seen, (int)(uninterrupted * 10000), (int)(restored * 10000),
        beforeVx, (int)(walker.state.vx * 10000), beforeGone, walker.state.gone,
        walker.state.active, (int)(walker.state.y * 100));
    atomicWrite("test-output/enemy-edge-restore-observation.txt", detail, lstrlenA(detail));
    physicsWorld->SetContactListener(originalContacts);
    delete probe;
    check(grounded && absf(uninterrupted - restored) < .001f,
          "Diagnostic: restoring a grounded walker preserves its next-step horizontal speed");
    atomicWrite("test-output/enemy-edge-context-observations.csv", rows, lstrlenA(rows));
    HeapFree(GetProcessHeap(), 0, rows);
    atomicWrite("test-output/enemy-edge-context-results.txt", testLog, lstrlenA(testLog));
    lstrcpyA(saveRoot, oldRoot);
    startStage(0);
    readyTimer = 0;
}

void enemyEdgeNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/enemy-edge-saves");
    CreateDirectoryA(saveRoot, 0);
    interfaceEnabled = false;
    debugMode = true;
    musicEnabled = soundEnabled = false;
    char *rows = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 500000);
    lstrcpyA(rows, "class,side,step,x100,y100,vx100,vy100,direction,ground,left,right,spring\r\n");
    bool reached = true;
    for (int category = 2; category <= 8; category += 6)
        for (int side = 0; side < 6; side++) {
            int selected[3], boss = wizardDismissPrepareNative(1, category, selected),
                             target = selected[0];
            for (int i = 0; i < entityCount; i++)
                if (i != playerIndex && i != target && entities[i].kind == ENEMY)
                    entities[i].state.gone = 1;
            entities[boss].state.gone = 1;
            State &bean = entities[playerIndex].state;
            bean.x = 4400;
            bean.y = 350;
            bean.vx = bean.vy = 0;
            Entity &e = entities[target];
            e.state.x = (side % 2) ? 4320 : 4192;
            e.state.y = 110;
            e.state.vx = 0;
            e.state.vy = 0;
            nativeWalkDirection(e, (side % 2) != 0, true);
            e.walkCanWalk = 0;
            restorePhysics();
            bodies[playerIndex]->SetType(b2_staticBody);
            bool spring = false;
            for (int step = 0; step < 480; step++) {
                tick(side < 2 ? 1 / 120.0f : side < 4 ? 1 / 30.0f : 1 / 240.0f, false, false);
                bool contact = false;
                for (b2ContactEdge *edge = bodies[target]->GetContactList(); edge;
                     edge = edge->next) {
                    int j = (int)edge->other->GetUserData() - 1;
                    if (j >= 0 && j < entityCount && entities[j].kind == SPRING &&
                        edge->contact->IsTouching() && edge->contact->IsEnabled() &&
                        edge->contact->GetManifold()->pointCount)
                        contact = true;
                }
                spring |= contact && e.state.vy > 300;
                char row[180];
                wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", category, side, step,
                          (int)(e.state.x * 100), (int)(e.state.y * 100), (int)(e.state.vx * 100),
                          (int)(e.state.vy * 100), e.state.aux < 0, enemyGrounded(target),
                          walkDetector(target, 5), walkDetector(target, 6), contact);
                lstrcatA(rows, row);
            }
            reached &= spring;
        }
    atomicWrite("test-output/enemy-edge-observations.csv", rows, lstrlenA(rows));
    HeapFree(GetProcessHeap(), 0, rows);
    check(
        reached,
        "Both actual Wizard walker classes independently descend from both intact cloud lips onto the actual spring and launch at30/120/240Hz");
    atomicWrite("test-output/enemy-edge-results.txt", testLog, lstrlenA(testLog));
    lstrcpyA(saveRoot, oldRoot);
    startStage(0);
    readyTimer = 0;
}
