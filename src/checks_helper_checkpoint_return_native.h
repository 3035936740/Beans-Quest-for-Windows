#pragma once

// Regression coverage for helper checkpoint return native. Runs only in the isolated BQ_ENABLE_TESTS executable.

class HelperCheckpointReturnObserverNative : public OriginalContacts {
  public:
    int bird, solved;
    char rows[16000];

    explicit HelperCheckpointReturnObserverNative(int index) : bird(index), solved(0) {
        lstrcpyA(rows, "callback,other,tag,sensor,enabled,normal_x1000,normal_y1000\r\n");
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        b2Fixture *fs[] = {c->GetFixtureA(), c->GetFixtureB()};
        for (int i = 0; i < 2; i++)
            if (fs[i]->GetBody() == bodies[bird]) {
                b2WorldManifold m;
                c->GetWorldManifold(&m);
                char row[180];
                wsprintfA(row, "PostSolve,%d,%d,%d,%d,%d,%d\r\n",
                          (int)fs[1 - i]->GetBody()->GetUserData() - 1,
                          (int)fs[1 - i]->GetUserData(), fs[1 - i]->IsSensor(), c->IsEnabled(),
                          (int)(m.normal.x * 1000), (int)(m.normal.y * 1000));
                if (lstrlenA(rows) < 15500)
                    lstrcatA(rows, row);
                solved++;
            }
        OriginalContacts::PostSolve(c, impulse);
    }
};

void helperCheckpointReturnNativeChecks() {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    int stage = -1;
    for (int i = 0; i < stageCount; i++)
        if (levels[stageList[i]].props.num("bean-world") == 3 &&
            levels[stageList[i]].props.num("bean-level") == 2)
            stage = i;
    check(stage >= 0, "Actual A New Friend resolves for the reported checkpoint return collision");
    if (stage < 0)
        return;
    bool saved = true, baseline = true, position = true, continued = true;
    char trace[14000] =
        "mode,step,bean_x1000,bean_y1000,bird_x1000,bird_y1000,bird_mode,roost_x1000,roost_y1000,solved\r\n";
    for (int mode = 0; mode < 3; mode++) {
        startStage(stage);
        readyTimer = 0;
        int bird = -1, point = -1;
        for (int i = 0; i < entityCount; i++) {
            if (entities[i].kind == HELPER)
                bird = i;
            if (entities[i].source->id == 987869988781ULL)
                point = i;
        }
        if (bird < 0 || point < 0) {
            saved = false;
            continue;
        }
        Entity &helper = entities[bird];
        State &bean = entities[playerIndex].state;
        bean.x = 272;
        bean.y = 850;
        bean.vx = 0;
        bean.vy = 60;
        helper.state.active = 1;
        helper.state.timer = 1;
        helper.state.x = bean.x;
        helper.state.y = bean.y + 34;
        restorePhysics();
        bool captured = false;
        for (int step = 0; step < 240; step++) {
            tick(1.0f / 120, false, false);
            if (checkpoint.state[point].hp == 2 && checkpoint.state[bird].active == 1) {
                captured = true;
                break;
            }
        }
        saved &= captured;
        if (!captured)
            continue;
        State birdSave = checkpoint.state[bird];
        float roostX = checkpoint.motion[bird].originX, roostY = checkpoint.motion[bird].originY;
        if (mode == 1)
            restoreCheckpoint();
        if (mode == 2) {
            for (int step = 0; step < 240; step++)
                tick(1.0f / 120, false, false);
            bean.x = 1200;
            bean.y = 600;
            helper.state.x = 1200;
            helper.state.y = 634;
            helper.state.active = 2;
            restorePhysics();
            simulateBodies(1.0f / 120);
            restoreCheckpoint();
        }
        if (mode)
            position &= helper.state.x == birdSave.x && helper.state.y == birdSave.y &&
                        helper.state.active == 1 && helper.originX == roostX &&
                        helper.originY == roostY;
        HelperCheckpointReturnObserverNative observer(bird);
        physicsWorld->SetContactListener(&observer);
        for (int step = 0; step < 240; step++) {
            tick(1.0f / 120, false, false);
            if (!mode)
                baseline &= helper.state.active == 1;
            else
                continued &= helper.state.active == 1;
            char row[220];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", mode, step, (int)(bean.x * 1000),
                      (int)(bean.y * 1000), (int)(helper.state.x * 1000),
                      (int)(helper.state.y * 1000), helper.state.active,
                      (int)(helper.originX * 1000), (int)(helper.originY * 1000), observer.solved);
            if (lstrlenA(trace) < 13400)
                lstrcatA(trace, row);
        }
        char path[120];
        wsprintfA(path, "test-output/helper-checkpoint-return-%d-contacts.csv", mode);
        atomicWrite(path, observer.rows, lstrlenA(observer.rows));
        physicsWorld->SetContactListener(originalContacts);
    }
    check(
        saved && baseline,
        "Uninterrupted actual checkpoint ascent captures a following helper and stays airborne through the original platform");
    check(
        position,
        "Immediate and later checkpoint restores retain the saved helper position mode and roost before the first physical step");
    check(
        continued,
        "Immediate and later checkpoint restores cannot turn the saved one-way traversal into a new helper wall impact");
    // Cover every point along the real platform ascent, not only the first
    // sensor entry. This finds restoration contacts born within a top face.
    bool scan = true;
    char scanRows[20000] = "phase,pre_mode,post_mode,solved,bean_y1000,bird_y1000\r\n";
    for (int phase = 0; phase < 180; phase++) {
        startStage(stage);
        readyTimer = 0;
        int bird = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == HELPER)
                bird = i;
        if (bird < 0) {
            scan = false;
            break;
        }
        Entity &helper = entities[bird];
        State &bean = entities[playerIndex].state;
        bean.x = 272;
        bean.y = 850;
        bean.vx = 0;
        bean.vy = 60;
        helper.state.active = 1;
        helper.state.timer = 1;
        helper.state.x = bean.x;
        helper.state.y = bean.y + 34;
        restorePhysics();
        for (int step = 0; step < phase; step++)
            tick(1.0f / 120, false, false);
        int pre = helper.state.active;
        captureCheckpoint(false);
        bean.x = 1200;
        bean.y = 600;
        helper.state.x = 1200;
        helper.state.y = 634;
        helper.state.active = 2;
        restorePhysics();
        simulateBodies(1.0f / 120);
        restoreCheckpoint();
        HelperCheckpointReturnObserverNative observer(bird);
        physicsWorld->SetContactListener(&observer);
        tick(1.0f / 120, false, false);
        scan &= helper.state.active == pre;
        char row[180];
        wsprintfA(row, "%d,%d,%d,%d,%d,%d\r\n", phase, pre, helper.state.active, observer.solved,
                  (int)(bean.y * 1000), (int)(helper.state.y * 1000));
        lstrcatA(scanRows, row);
        if (observer.solved) {
            char path[120];
            wsprintfA(path, "test-output/helper-checkpoint-impact-phase-%d.csv", phase);
            atomicWrite(path, observer.rows, lstrlenA(observer.rows));
        }
        physicsWorld->SetContactListener(originalContacts);
    }
    check(
        scan,
        "Restoring every phase of the real platform ascent does not invent an extra helper collision");
    atomicWrite("test-output/helper-checkpoint-return-scan.csv", scanRows, lstrlenA(scanRows));
    bool autoSave = true;
    char matrix[14000] = "fps,x1000,saved_y1000,saved_bird_y1000,restored_mode,solved\r\n";
    for (int rate = 0; rate < 4; rate++)
        for (int lane = 0; lane < 6; lane++) {
            int fps = rate == 0 ? 30 : rate == 1 ? 60 : rate == 2 ? 120 : 240;
            float dt = 1.0f / fps;
            startStage(stage);
            readyTimer = 0;
            int bird = -1, point = -1;
            for (int i = 0; i < entityCount; i++) {
                if (entities[i].kind == HELPER)
                    bird = i;
                if (entities[i].source->id == 987869988781ULL)
                    point = i;
            }
            Entity &helper = entities[bird];
            State &bean = entities[playerIndex].state;
            bean.x = 224 + lane * 9.6f;
            bean.y = 850;
            bean.vx = 0;
            bean.vy = 60;
            helper.state.active = 1;
            helper.state.timer = 1;
            helper.state.x = bean.x;
            helper.state.y = bean.y + 34;
            restorePhysics();
            bool captured = false;
            for (int step = 0; step < fps * 3; step++) {
                tick(dt, false, false);
                if (checkpoint.state[point].hp == 2 && checkpoint.state[bird].active == 1) {
                    captured = true;
                    break;
                }
            }
            if (!captured)
                continue;
            float y = checkpoint.state[playerIndex].y, by = checkpoint.state[bird].y;
            bean.x = 1200;
            bean.y = 600;
            helper.state.x = 1200;
            helper.state.y = 634;
            helper.state.active = 2;
            restorePhysics();
            simulateBodies(dt);
            restoreCheckpoint();
            HelperCheckpointReturnObserverNative observer(bird);
            physicsWorld->SetContactListener(&observer);
            for (int step = 0; step < 3; step++)
                tick(dt, false, false);
            autoSave &= helper.state.active == 1;
            char row[180];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d\r\n", fps, (int)(bean.x * 1000), (int)(y * 1000),
                      (int)(by * 1000), helper.state.active, observer.solved);
            lstrcatA(matrix, row);
            physicsWorld->SetContactListener(originalContacts);
        }
    check(
        autoSave,
        "Actual automatic checkpoint entries retain helper flight on reload across24frame-rate and approach-lane combinations");
    atomicWrite("test-output/helper-checkpoint-return-matrix.csv", matrix, lstrlenA(matrix));
    atomicWrite("test-output/helper-checkpoint-return-trace.csv", trace, lstrlenA(trace));
    soundEnabled = musicEnabled = false;
    startStage(0);
}
