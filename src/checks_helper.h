#pragma once

// Regression coverage for helper. Runs only in the isolated BQ_ENABLE_TESTS executable.

void helperChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    int scenes = 0;
    for (int stage = 0; stage < stageCount; stage++) {
        if (levels[stageList[stage]].props.num("bean-world") != 3)
            continue;
        startStage(stage);
        int bird = -1, perch = -1, other = -1;
        for (int i = 0; i < entityCount; i++) {
            if (entities[i].kind == HELPER)
                bird = i;
            if (entities[i].kind == PERCH) {
                if (perch < 0)
                    perch = i;
                else
                    other = i;
            }
        }
        if (bird < 0)
            continue;
        other = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == PERCH &&
                (absf(entities[i].state.x - entities[bird].originX) > 1 ||
                 absf(
                     entities[i].state.y +
                     entities[i].source->behavior("HelperBirdPerchBehaviour")->num("offset-y", 23) -
                     entities[bird].originY) > 1)) {
                other = i;
                break;
            }
        scenes++;
        readyTimer = 0;
        Entity &b = entities[bird];
        Entity &p = entities[playerIndex];
        check(b.state.active == 0 && bodies[bird] && bodies[bird]->GetType() == b2_dynamicBody,
              "Authored helper bird begins roosting with its original collision body");
        check(perch >= 0 && bodies[perch] && bodies[perch]->GetFixtureList()->IsSensor(),
              "Original bird perch is a sensor rather than a solid wall");
        p.state.x = b.state.x;
        p.state.y = b.state.y;
        p.state.vx = p.state.vy = 0;
        restorePhysics();
        u32 voices = activeAudioVoices(), pickupVoices = audioVoicesStarted;
        soundEnabled = true;
        tick(dt, false, false);
        soundEnabled = false;
        check(audioVoicesStarted == pickupVoices + 1,
              "Authored helper pickup starts exactly one native bird sound");
        check(
            b.state.active == 1 && flyingHelper() == bird && liveEffects() > 0 &&
                activeAudioVoices() >= (int)voices + 1,
            "Actual Bean contact wakes the helper, emits its effect and plays the original bird sound");
        stopSounds();
        // Keep the authored contact test above, then isolate sustained lift
        // from each map's nearby ceilings and hazards.
        current.geometryCount = 0;
        for (int i = 0; i < entityCount; i++)
            if (i != bird && i != playerIndex && entities[i].kind != PERCH)
                entities[i].state.gone = 1;
        rebuildPhysics();
        float startY = p.state.y;
        int before = jumps;
        for (int step = 0; step < 180; step++)
            tick(dt, false, false);
        const Props &follow = *b.source->behavior("HelperBirdBehaviour");
        check(b.state.active == 1 && p.state.y > startY + 8 && p.state.vy > 40 &&
                  p.state.vy <= follow.num("max-fly-speed", 60) + 0.01f && jumps == before,
              "Helper flight lifts Bean toward the authored speed without adding artificial jumps");
        check(
            absf(b.state.x - p.state.x - follow.num("follow-offset-x")) < 0.01f &&
                absf(b.state.y - p.state.y - follow.num("follow-offset-y", 34)) < 0.01f,
            "Flying helper follows Bean at its authored offset after the native 0.1 second transition");
        captureCheckpoint(true);
        float roost = b.originX, savedY = p.state.y, savedTimer = b.state.timer;
        b.state.active = 0;
        b.originX += 100;
        restoreCheckpoint();
        check(b.state.active == 1 && flyingHelper() == bird && b.originX == roost &&
                  p.state.y == savedY && b.state.timer == savedTimer,
              "Checkpoint restores helper flight, return target and animation time");
        updateCamera(true);
        captureCase(scenes == 1 ? "test-output/helper-flight-first.bmp"
                                : "test-output/helper-flight-second.bmp");
        // An actual solid contact above the helper ends flight. The platform
        // is test-only; all remaining physics and callbacks are the real ones.
        int obstruction = entityCount;
        addEntity(*classById(12987116682223282109ULL),
                  Rect{b.state.x - 30, b.state.y + b.hit.t - 1, b.state.x + 30,
                       b.state.y + b.hit.t + 12});
        entities[obstruction].move = 0;
        rebuildPhysics();
        u32 stopVoices = audioVoicesStarted;
        soundEnabled = true;
        simulateBodies(dt);
        helperPostStep(dt);
        soundEnabled = false;
        check(audioVoicesStarted == stopVoices + 1,
              "Actual solid obstruction plays exactly one original helper collision sound");
        u32 afterImpactVoices = audioVoicesStarted;
        soundEnabled = true;
        helperPostStep(dt);
        soundEnabled = false;
        check(
            audioVoicesStarted == afterImpactVoices,
            "Returning helper does not repeat the collision sound while the obstruction contact remains");
        check(b.state.active == 2 && flyingHelper() < 0,
              "Helper contact with a solid surface ends flight and starts returning to its roost");
        entities[obstruction].state.gone = 1;
        b.state.active = 1;
        soundEnabled = true;
        stopHelper(b);
        soundEnabled = false;
        check(b.state.active == 2 && audioVoicesStarted == afterImpactVoices,
              "Non-contact StopFollowing returns the helper without a collision chirp");
        p.state.x = 64;
        p.state.y = 300;
        p.state.vx = p.state.vy = 0;
        float returnX = b.state.x, returnY = b.state.y;
        b.state.x = b.originX + 100;
        b.state.y = b.originY + 100;
        updateHelper(b, dt);
        check(absf(root((b.state.x - b.originX) * (b.state.x - b.originX) +
                        (b.state.y - b.originY) * (b.state.y - b.originY)) -
                   (root(20000.0f) - 150 * dt)) < 0.001f,
              "Returning helper uses the original normalized 150 pixel-per-second movement");
        for (int step = 0; step < 240; step++)
            updateHelper(b, dt);
        check(b.state.active == 0 && b.state.x == b.originX && b.state.y == b.originY,
              "Returning helper arrives exactly at its roost and sleeps again");
        (void)returnX;
        (void)returnY;
        // Native 0x2170d0..0x217130 accepts 100 float representations per
        // component after movement, including a nonzero remaining distance.
        State roostState = b.state;
        bool inside = true, outside = true, diagonalArrival = true;
        for (int axis = 0; axis < 2; axis++)
            for (int side = -1; side <= 1; side += 2)
                for (int edge = 100; edge <= 101; edge++) {
                    float target = axis ? b.originY : b.originX;
                    u32 bits;
                    memcpy(&bits, &target, 4);
                    u32 nextBits = bits + 1;
                    float next;
                    memcpy(&next, &nextBits, 4);
                    float ulp = absf(next - target);
                    b.state = roostState;
                    b.state.active = 2;
                    if (axis)
                        b.state.y = target - side * (1.25f + edge * ulp);
                    else
                        b.state.x = target - side * (1.25f + edge * ulp);
                    updateHelper(b, dt);
                    if (edge == 100)
                        inside = inside && b.state.active == 0 && b.state.x == b.originX &&
                                 b.state.y == b.originY;
                    else
                        outside = outside && b.state.active == 2;
                }
        check(
            inside,
            "Authored helper roost accepts the native inclusive 100-ULP arrival from both axes and directions");
        check(outside, "Authored helper roost remains returning at 101-ULP residual distance");
        // With no elapsed motion, compare both components independently.
        b.state = roostState;
        b.state.active = 2;
        u32 xBits, yBits;
        memcpy(&xBits, &b.originX, 4);
        memcpy(&yBits, &b.originY, 4);
        xBits += 100;
        yBits += 100;
        memcpy(&b.state.x, &xBits, 4);
        memcpy(&b.state.y, &yBits, 4);
        updateHelper(b, 0);
        diagonalArrival = b.state.active == 0 && b.state.x == b.originX && b.state.y == b.originY;
        check(
            diagonalArrival,
            "Authored helper return checks the native tolerance separately on both diagonal components");
        b.state = roostState;
        if (other >= 0) {
            Entity &target = entities[other];
            p.state.x = target.state.x;
            p.state.y = target.state.y;
            p.state.vx = p.state.vy = 0;
            restorePhysics();
            simulateBodies(dt);
            helperPostStep(dt);
            const Props &perchProps = *target.source->behavior("HelperBirdPerchBehaviour");
            check(
                target.state.active == 2 &&
                    b.originX == target.state.x + perchProps.num("offset-x") &&
                    b.originY == target.state.y + perchProps.num("offset-y", 23) &&
                    b.state.active == 2,
                "Touching another authored perch rings it and sends the resting bird to the new roost");
            captureCheckpoint(true);
            float newX = b.originX, newY = b.originY;
            b.originX = b.originY = 0;
            restoreCheckpoint();
            check(b.originX == newX && b.originY == newY && b.state.active == 2,
                  "Checkpoint retains a changed perch and the bird's returning state");
        }
    }
    check(scenes == 2,
          "Both original helper bird stages receive functional helper and perch states");
    startStage(0);
    readyTimer = 0;
    // Previous layout retained all old motion values; v7 adds launch speed.
    SnapshotV6 *old =
        (SnapshotV6 *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SnapshotV6));
    memcpy(old, &checkpoint, sizeof(LegacySnapshot) - 4);
    old->version = 5;
    old->camera = checkpoint.camera;
    for (int i = 0; i < entityCount; i++) {
        MotionState &m = checkpoint.motion[i];
        old->motion[i] = LegacyMotionState{m.originX, m.originY, m.turnCooldown, m.valid};
    }
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    char path[MAX_PATH];
    savePath(path, "native-v5-checkpoint.dat");
    atomicWrite(path, old, sizeof(*old));
    check(
        readCheckpoint(path) && checkpoint.version == 5 && checkpoint.motion[playerIndex].valid &&
            checkpoint.motion[playerIndex].lastLaunchSpeed == 0 && checkpoint.camera.valid,
        "Existing version 5 checkpoints migrate motion and camera while safely defaulting the new launch state");
    old->version = 6;
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    savePath(path, "native-v6-checkpoint.dat");
    atomicWrite(path, old, sizeof(*old));
    check(
        readCheckpoint(path) && checkpoint.version == 6 &&
            checkpoint.motion[playerIndex].lastLaunchSpeed == 0 && checkpoint.camera.valid,
        "Version 6 snapshots retain their existing helper and frog protocol states during layout migration");
    HeapFree(GetProcessHeap(), 0, old);
    startStage(0);
    Entity &bean = entities[playerIndex];
    bean.lastLaunchSpeed = 600;
    captureCheckpoint(false);
    bean.lastLaunchSpeed = 100;
    restoreCheckpoint();
    check(bean.lastLaunchSpeed == 600,
          "Checkpoint preserves effective spring launch speed for Bean's original jump animation");
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex)
            entities[i].state.gone = 1;
    current.geometryCount = 0;
    int blockIndex = entityCount;
    addEntity(*classById(12987116682223282109ULL), Rect{240, 200, 272, 232});
    Entity &block = entities[blockIndex];
    bean.state.x = 256;
    bean.state.y = hitRect(block).b - bean.hit.t - 1;
    bean.state.vx = 0;
    bean.state.vy = 180;
    rebuildPhysics();
    for (int step = 0; step < 12 && !block.state.gone; step++)
        tick(dt, false, false);
    check(
        block.state.gone && block.state.hp <= 0 && block.turnCooldown > 0 &&
            !bodies[blockIndex]->IsActive(),
        "Original three-HP brick uses its authored three-point underside damage and breaks on one upward hit");
    block.state.gone = 0;
    block.state.hp = 3;
    block.turnCooldown = 0;
    bean.state.x = block.state.x - block.hit.r + bean.hit.l - 1;
    bean.state.y = block.state.y;
    bean.state.vx = 180;
    bean.state.vy = 0;
    restorePhysics();
    for (int step = 0; step < 6; step++)
        tick(dt, false, true);
    check(!block.state.gone && block.state.hp == 3,
          "Original brick side contact uses zero other-hit damage instead of damaging its artwork");
    startStage(0);
}
