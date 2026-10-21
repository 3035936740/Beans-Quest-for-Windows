#pragma once

// Regression coverage for motion. Runs only in the isolated BQ_ENABLE_TESTS executable.

int motionStage(int world, int number) {
    for (int i = 0; i < stageCount; i++)
        if (levels[stageList[i]].props.num("bean-world") == world &&
            levels[stageList[i]].props.num("bean-level") == number)
            return i;
    return -1;
}

void motionChecks() {
    const float dt = 1.0f / 120.0f;
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    int stage = motionStage(2, 6);
    check(stage >= 0, "Original Tunnels stage exists for moving block proximity and rail checks");
    if (stage >= 0) {
        startStage(stage);
        readyTimer = 0;
        int block = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].source->id == 12540492440314510417ULL)
                block = i;
        check(block >= 0, "The original horizontal Tunnels moving block is present");
        if (block >= 0) {
            Entity &e = entities[block];
            State &bean = entities[playerIndex].state;
            float x = e.state.x, y = e.state.y;
            for (int step = 0; step < 120; step++) {
                bean.x = 64;
                bean.y = 350;
                bean.vx = bean.vy = 0;
                tick(dt, false, false);
            }
            check(e.state.active != 0 && bodies[block]->IsActive() && !e.source->paused,
                  "Unpaused authored moving block already runs outside its proximity rectangle");
            bean.x = x;
            bean.y = y + 90;
            bean.vx = bean.vy = 0;
            tick(dt, false, false);
            bool activated = e.state.active != 0;
            float minX = e.state.x, maxX = e.state.x, minY = e.state.y, maxY = e.state.y;
            bool finite = true;
            for (int step = 0; step < 720; step++) {
                bean.x = 64;
                bean.y = 350;
                bean.vx = bean.vy = 0;
                tick(dt, false, false);
                minX = minf(minX, e.state.x);
                maxX = maxf(maxX, e.state.x);
                minY = minf(minY, e.state.y);
                maxY = maxf(maxY, e.state.y);
                finite &= e.state.x == e.state.x && e.state.y == e.state.y;
            }
            check(activated && e.state.active && maxX - minX > 30,
                  "An initially unpaused moving block remains active and traverses its rail");
            check(
                finite && absf(minY - y) < 1.5f && absf(maxY - y) < 1.5f &&
                    minX >= e.originX + e.lo - 2 && maxX <= e.originX + e.hi + 2,
                "The original Tunnels moving block stays on its horizontal rail throughout six seconds of actual simulation");
        }
    }
    stage = motionStage(3, 2);
    check(stage >= 0, "Original A New Friend stage exists for the checkpoint flight regression");
    if (stage >= 0) {
        startStage(stage);
        readyTimer = 0;
        int bird = -1, checkpointIndex = -1;
        for (int i = 0; i < entityCount; i++) {
            if (entities[i].kind == HELPER)
                bird = i;
            if (entities[i].source->id == 987869988781ULL)
                checkpointIndex = i;
        }
        if (bird >= 0 && checkpointIndex >= 0) {
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
            bool flying = true;
            for (int step = 0; step < 240; step++) {
                tick(dt, false, false);
                flying &= helper.state.active == 1;
            }
            check(
                flying && bean.y > 950 && entities[checkpointIndex].state.hp == 2,
                "Helper and Bean ascend through the original one-way checkpoint platform without falsely returning to the roost");
            check(
                helper.originX == 306 && helper.originY == 967 && helper.state.active == 1,
                "Reaching the new authored perch updates the helper roost while flight continues");
            camX = 0;
            camY = 760;
            captureCase("test-output/helper-checkpoint-ascent.bmp");
        } else
            check(false, "Authored helper and first checkpoint resolve in A New Friend");
    }
    startStage(0);
    readyTimer = 0;
}
