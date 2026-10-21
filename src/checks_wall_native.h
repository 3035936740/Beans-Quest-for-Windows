#pragma once

// Regression coverage for wall native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int wallMatrixOwner(u64 object) {
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->id == object && boxGroupMatrixOwner(entities[i]))
            return i;
    return -1;
}

void isolateWallMatrix(int owner) {
    Object *source = entities[owner].source;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && entities[i].source != source)
            entities[i].state.gone = 1;
    rebuildPhysics();
    readyTimer = 0;
}

void wallNativeChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    int stage = motionStage(4, 3);
    check(stage >= 0, "Original cage-wall stage exists");
    if (stage >= 0) {
        startStage(stage);
        int owner = wallMatrixOwner(5112607920136829ULL);
        check(owner >= 0, "Original five-cell blue/yellow Cage Stop Other wall exists");
        if (owner >= 0) {
            Object *source = entities[owner].source;
            isolateWallMatrix(owner);
            int cells = 0, count = 0;
            for (int i = 0; i < entityCount; i++)
                if (entities[i].source == source) {
                    cells++;
                    if (bodies[i])
                        count++;
                }
            check(
                cells == 5 && count == 1,
                "Tall blue/yellow matrix has one original full-height collider without four internal horizontal ledges");
            b2Fixture *fixture = bodies[owner]->GetFixtureList();
            check(
                (fixture->GetFilterData().maskBits & 2) == 0,
                "Cage Stop Other retains original collide-player=false on its whole-matrix fixture");
            State &bean = entities[playerIndex].state;
            bean.x = 70;
            bean.y = 752;
            bean.vx = 200;
            bean.vy = 0;
            restorePhysics();
            int initialJumps = jumps;
            for (int frame = 0; frame < 65; frame++)
                tick(dt, false, true);
            check(
                bean.x > 150 && jumps == initialJumps,
                "Bean passes through the authored non-player cage wall without invisible obstruction or a phantom wall jump");
        }
    }
    stage = motionStage(6, 1);
    check(stage >= 0, "Original boss-stage player-only cage wall exists");
    if (stage >= 0) {
        startStage(stage);
        int owner = wallMatrixOwner(1184107877261977ULL);
        check(owner >= 0, "Original two-cell player-blocking matrix exists");
        if (owner >= 0) {
            Object *source = entities[owner].source;
            isolateWallMatrix(owner);
            int count = 0;
            for (int i = 0; i < entityCount; i++)
                if (entities[i].source == source && bodies[i])
                    count++;
            check(
                count == 1 && (bodies[owner]->GetFixtureList()->GetFilterData().maskBits & 2) != 0,
                "Player-only cage matrix keeps its real outer wall while removing the internal cell ledge");
            State &bean = entities[playerIndex].state;
            bean.x = 6880 - entities[playerIndex].hit.r - 1;
            bean.y = 124;
            bean.vx = 200;
            bean.vy = 120;
            restorePhysics();
            int initialJumps = jumps;
            float peak = bean.y, maxVelocity = bean.vy;
            for (int frame = 0; frame < 65; frame++) {
                tick(dt, false, true);
                peak = maxf(peak, bean.y);
                maxVelocity = maxf(maxVelocity, bean.vy);
            }
            check(
                jumps == initialJumps && maxVelocity <= 120.1f && peak < 147,
                "Pressing into a vertical matrix side cannot bounce off the eliminated internal 128-pixel seam");
            check(bean.x < 6880 - entities[playerIndex].hit.r + 1,
                  "Removing internal seams still preserves the original solid outer wall");
        }
    }
    stage = motionStage(1, 4);
    if (stage >= 0) {
        startStage(stage);
        int owner = wallMatrixOwner(7741631691555791504ULL);
        if (owner >= 0) {
            isolateWallMatrix(owner);
            State &bean = entities[playerIndex].state;
            bean.x = 1872;
            bean.y = hitRect(entities[owner]).t - entities[playerIndex].hit.b + 0.1f;
            bean.vx = 0;
            bean.vy = -30;
            restorePhysics();
            char probe[8000] = {};
            for (int i = 0; i < 30; i++) {
                tick(dt, false, false);
                char row[240];
                wsprintfA(row,
                          "tick%d feet%d vy%d preVy%d spring%d incoming%d support%d active%d\r\n",
                          i, (int)hitRect(entities[playerIndex]).b, (int)bean.vy, (int)preStepVy,
                          playerSpringContact, springIncomingCount, supportEntity, bean.active);
                lstrcatA(probe, row);
                if (pinballLaunchedThisStepNative && pinballConsumedNative.source == owner)
                    break;
            }
            atomicWrite("test-output/spring-rightmost-native-contact.txt", probe, lstrlenA(probe));
            int active = 0;
            bool first = false;
            for (int i = 0; i < entityCount; i++)
                if (entities[i].source == entities[owner].source && entities[i].state.active == 2) {
                    active++;
                    first |= i == owner;
                }
            check(
                pinballLaunchedThisStepNative && pinballConsumedNative.source == owner &&
                    bean.vy > 100 && active >= 1 && active <= 2 && !first,
                "Whole-matrix spring collider preserves the eligible deferred launch and animates the contacted rightmost spring rather than its owner cell");
        }
    }
}
