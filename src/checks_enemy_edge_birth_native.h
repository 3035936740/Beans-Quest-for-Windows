#pragma once

// Regression coverage for enemy edge birth native. Runs only in the isolated BQ_ENABLE_TESTS executable.

// Diagnostic uses genuine materialization poses and velocities. No walker
// teleport, speed override, cloud removal, or route manipulation is applied.
class EnemyEdgeBirthProbeNative : public WalkCallbackProbeNative {
  public:
    int target[3];
    bool cloud[3], everCloud[3], afterCloud[3];
    int springEvents[3], firstSpring[3], firstCloud[3], step, stepDowns;

    EnemyEdgeBirthProbeNative(int *selected, int member)
        : WalkCallbackProbeNative(selected[member], entities[selected[member]].walkRight != 0),
          step(0), stepDowns(0) {
        memset(cloud, 0, sizeof(cloud));
        memset(everCloud, 0, sizeof(everCloud));
        memset(afterCloud, 0, sizeof(afterCloud));
        memset(springEvents, 0, sizeof(springEvents));
        for (int n = 0; n < 3; n++) {
            target[n] = selected[n];
            firstSpring[n] = firstCloud[n] = -1;
        }
    }

    void BeginContact(b2Contact *c) {
        int side = ownSide(c);
        if (side >= 0) {
            b2Fixture *own = side ? c->GetFixtureB() : c->GetFixtureA();
            b2Fixture *other = side ? c->GetFixtureA() : c->GetFixtureB();
            int tag = (int)own->GetUserData();
            if (tag >= 3 && tag <= 6 && !other->IsSensor()) {
                int slot = 0;
                while (slot < used && tracked[slot])
                    slot++;
                if (slot < 128) {
                    if (slot == used)
                        used++;
                    tracked[slot] = c;
                    tags[slot] = tag;
                    others[slot] = other;
                }
            }
        }
        OriginalContacts::BeginContact(c);
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        WalkCallbackProbeNative::PostSolve(c, impulse);
        if (!c->IsEnabled() || !c->GetManifold()->pointCount || c->GetFixtureA()->IsSensor() ||
            c->GetFixtureB()->IsSensor())
            return;
        b2Fixture *f[] = {c->GetFixtureA(), c->GetFixtureB()};
        b2WorldManifold m;
        c->GetWorldManifold(&m);
        for (int side = 0; side < 2; side++) {
            int own = (int)f[side]->GetBody()->GetUserData() - 1,
                other = (int)f[1 - side]->GetBody()->GetUserData() - 1;
            if (other < 0 || other >= entityCount || (side ? m.normal.y : -m.normal.y) <= .95f)
                continue;
            for (int n = 0; n < 3; n++)
                if (own == target[n] && entities[other].crumble) {
                    cloud[n] = everCloud[n] = true;
                    if (firstCloud[n] < 0)
                        firstCloud[n] = step;
                }
        }
    }

    void postPhysics() {
        Entity planned = entities[WalkCallbackProbeNative::target];
        planned.walkRight = right;
        bool left = contains(5), supportRight = contains(6);
        if (walkCloudSpringStepNative(planned, bodies[WalkCallbackProbeNative::target], entities,
                                      entityCount, left, supportRight)) {
            stepDowns++;
            return;
        }
        WalkCallbackProbeNative::postPhysics();
    }

    void recordSprings() {
        for (int n = 0; n < 3; n++)
            for (int i = 0; i < springIncomingCount; i++) {
                SpringIncomingVelocity &hit = springIncoming[i];
                if (hit.target == target[n] && hit.solved && hit.solvedY < 0 &&
                    entities[target[n]].state.vy > 300) {
                    springEvents[n]++;
                    if (firstSpring[n] < 0)
                        firstSpring[n] = step;
                    if (everCloud[n])
                        afterCloud[n] = true;
                }
            }
    }
};

void enemyEdgeBirthNativeChecks(bool requireDescent = true) {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/enemy-edge-birth-saves");
    CreateDirectoryA(saveRoot, 0);
    interfaceEnabled = musicEnabled = soundEnabled = false;
    debugMode = false;
    char *rows = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 2000000);
    lstrcpyA(
        rows,
        "class,fps,mode,member,step,x100,y100,vx100,vy100,gone,cloud,everCloud,springs,afterCloud,left,right,canWalk\r\n");
    char callbackRows[50000] =
        "class,fps,mode,step,actualRight,referenceRight,canWalk,referenceChanges,mainContacts,left,right,refLeft,refRight,x100,y100\r\n";
    char summary[12000] =
        "class,fps,mode,member,birthX100,birthY100,firstCloud,firstSpring,springEvents,afterCloud,finalGone\r\n";
    bool born = true, cloudsIntact = true, postCloud = true;
    int cases = 0, members = 0, mismatches = 0, saturated = 0, stepDowns = 0;
    for (int category = 2; category <= 8; category += 6)
        for (int rate = 0; rate < 2; rate++)
            for (int mode = 0; mode < 4; mode++) {
                int selected[3], boss = prepareWizardWalkSpawnNative(1, category, selected);
                if (boss < 0) {
                    born = false;
                    continue;
                }
                float birthX[3], birthY[3];
                bool observed[3] = {};
                for (int n = 0; n < 3; n++) {
                    Entity &e = entities[selected[n]];
                    birthX[n] = e.state.x;
                    birthY[n] = e.state.y;
                    born &= e.state.vx == 0 && e.state.vy == 0 && !e.walkCanWalk;
                    observed[n] = mode == 0 || mode == n + 1;
                    if (!observed[n]) {
                        e.state.gone = 1;
                        bodies[selected[n]]->SetActive(false);
                    } else
                        members++;
                }
                for (int i = 0; i < entityCount; i++)
                    if (entities[i].crumble)
                        cloudsIntact &= entities[i].state.active != 2;
                EnemyEdgeBirthProbeNative *probe =
                    new EnemyEdgeBirthProbeNative(selected, mode ? mode - 1 : 0);
                physicsWorld->SetContactListener(probe);
                int fps = rate ? 30 : 120;
                for (int step = 0; step < fps * 20; step++) {
                    probe->step = step;
                    memset(probe->cloud, 0, sizeof(probe->cloud));
                    tick(1.0f / fps, false, false);
                    probe->postPhysics();
                    if (probe->used < 128) {
                        Entity &own = entities[probe->WalkCallbackProbeNative::target];
                        bool mismatch = probe->right != (own.walkRight != 0);
                        if (mismatch && lstrlenA(callbackRows) < 48000) {
                            char row[220];
                            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
                                      category, fps, mode, step, own.walkRight, probe->right,
                                      own.walkCanWalk, probe->changes, probe->eligible,
                                      walkDetector(probe->WalkCallbackProbeNative::target, 5),
                                      walkDetector(probe->WalkCallbackProbeNative::target, 6),
                                      probe->contains(5), probe->contains(6),
                                      (int)(own.state.x * 100), (int)(own.state.y * 100));
                            lstrcatA(callbackRows, row);
                        }
                        mismatches += mismatch;
                    } else
                        saturated++;
                    probe->recordSprings();
                    if (step % 10 == 0)
                        for (int n = 0; n < 3; n++)
                            if (observed[n]) {
                                Entity &e = entities[selected[n]];
                                char row[240];
                                wsprintfA(row,
                                          "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
                                          category, fps, mode, n, step, (int)(e.state.x * 100),
                                          (int)(e.state.y * 100), (int)(e.state.vx * 100),
                                          (int)(e.state.vy * 100), e.state.gone, probe->cloud[n],
                                          probe->everCloud[n], probe->springEvents[n],
                                          probe->afterCloud[n], walkDetector(selected[n], 5),
                                          walkDetector(selected[n], 6), e.walkCanWalk);
                                lstrcatA(rows, row);
                            }
                }
                for (int n = 0; n < 3; n++)
                    if (observed[n]) {
                        char row[200];
                        wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", category, fps, mode,
                                  n, (int)(birthX[n] * 100), (int)(birthY[n] * 100),
                                  probe->firstCloud[n], probe->firstSpring[n],
                                  probe->springEvents[n], probe->afterCloud[n],
                                  entities[selected[n]].state.gone);
                        lstrcatA(summary, row);
                        if (mode && category == 2)
                            postCloud &= probe->afterCloud[n];
                    }
                for (int i = 0; i < entityCount; i++)
                    if (entities[i].crumble)
                        cloudsIntact &= entities[i].state.active != 2;
                stepDowns += probe->stepDowns;
                physicsWorld->SetContactListener(originalContacts);
                delete probe;
                cases++;
            }
    check(
        born && cloudsIntact && cases == 16 && members == 24,
        "Diagnostic:16actual materialization contexts keep native birth poses, zero velocity and intact clouds for both Wizard walkers at30/120Hz");
    char callbackResult[180];
    wsprintfA(callbackResult,
              "direction_mismatches=%d saturated_steps=%d cloud_spring_step_downs=%d\r\n",
              mismatches, saturated, stepDowns);
    atomicWrite("test-output/enemy-edge-birth-callbacks.txt", callbackResult,
                lstrlenA(callbackResult));
    atomicWrite("test-output/enemy-edge-birth-callback-mismatches.csv", callbackRows,
                lstrlenA(callbackRows));
    check(
        mismatches == 0 && saturated == 0 && stepDowns > 0,
        "Every actual materialization frame preserves native side-turn ordering plus the observed-original cloud-to-spring step-down contract");
    if (requireDescent)
        check(
            postCloud,
            "All6isolated actual blue Wizard walkers descend to and launch from the spring AFTER landing on an intact cloud at30/120Hz without Bean or another summon pushing them");
    atomicWrite("test-output/enemy-edge-birth-observations.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/enemy-edge-birth-summary.csv", summary, lstrlenA(summary));
    atomicWrite(requireDescent ? "test-output/enemy-edge-birth-results.txt"
                               : "test-output/walk-side-callback-results.txt",
                testLog, lstrlenA(testLog));
    HeapFree(GetProcessHeap(), 0, rows);
    lstrcpyA(saveRoot, oldRoot);
    startStage(0);
    readyTimer = 0;
}
