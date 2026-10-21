#pragma once

// Regression coverage for selector trampoline native. Runs only in the isolated BQ_ENABLE_TESTS executable.

struct SelectorTrampolineRoute {
    int launches, padLaunches, sourceLaunches, targetLaunches, ordinaryLaunches, overlap;
    float sourceSpeed, targetSpeed, ordinaryApex, minTargetApex, maxTargetApex;
};

void prepareSelectorTrampolineNative(bool fromRight = false) {
    if (hudScene.level.id != 14897698531695279339ULL)
        initInterface();
    interfaceEnabled = true;
    musicEnabled = soundEnabled = false;
    defaultBindings();
    memset(keys, 0, sizeof(keys));
    navigate(12038123762473955738ULL);
    page.minimized = true;
    page.minimizePhase = 1;
    page.bean.state.x = fromRight ? 528.0f : 336.0f;
    page.bean.state.y = 340;
    page.bean.state.vx = page.bean.state.vy = 0;
    resetUiPhysics(page);
}

SelectorTrampolineRoute selectorTrampolineNativeRoute(bool fromRight, bool ordinary = false) {
    prepareSelectorTrampolineNative(fromRight);
    SelectorTrampolineRoute result = {};
    result.minTargetApex = 100000;
    float start = page.bean.state.x, target = ordinary ? 216.0f : fromRight ? 336.0f : 528.0f;
    const float dt = 1.0f / 120;
    bool lastTerrain = false;
    char contactRows[16000] = {};
    for (int step = 0; step < 1650; step++) {
        float oldVy = page.bean.state.vy;
        tickInterface(dt);
        bool launch = page.specialJump.launchedKind != 0 || (oldVy <= 0 && page.bean.state.vy > 30);
        if (launch || page.contacts->trampoline >= 0) {
            char trace[220];
            wsprintfA(trace,
                      "step%d x%d y100%d vy100%d age1000%d pad%d req%d kind%d apex%d death%d\r\n",
                      step, (int)page.bean.state.x, (int)(page.bean.state.y * 100),
                      (int)(page.bean.state.vy * 100), (int)(page.bean.state.timer * 1000),
                      page.contacts->trampoline, page.specialJump.jelly.pending,
                      page.specialJump.launchedKind, (int)page.trampolineApex,
                      (int)(page.miniDeath * 1000));
            if (lstrlenA(contactRows) < 15500)
                lstrcatA(contactRows, trace);
        }
        if (launch) {
            result.launches++;
            int pad = page.specialJump.launchedKind == 2 ? page.specialJump.launchedSource : -1;
            if (pad >= 0) {
                result.padLaunches++;
                float x = page.playfield[pad].state.x;
                if (absf(x - start) < 8) {
                    result.sourceLaunches++;
                    result.sourceSpeed = page.bean.lastLaunchSpeed;
                } else if (absf(x - target) < 8) {
                    result.targetLaunches++;
                    result.targetSpeed = page.bean.lastLaunchSpeed;
                    result.minTargetApex = minf(result.minTargetApex, page.trampolineApex);
                    result.maxTargetApex = maxf(result.maxTargetApex, page.trampolineApex);
                    result.overlap += lastTerrain;
                    if (!fromRight && result.targetLaunches == 1)
                        captureCase("test-output/selector-jelly-right-inherited-launch.bmp");
                }
            } else if (absf(page.bean.state.x - target) < 10) {
                result.ordinaryLaunches++;
                result.ordinaryApex = page.trampolineApex;
            }
        }
        if (page.contacts->trampoline >= 0) {
            lastTerrain = false;
            for (b2ContactEdge *edge = page.bodies[page.playfieldCount]->GetContactList(); edge;
                 edge = edge->next)
                if ((int)edge->other->GetUserData() == 0 && edge->contact->IsTouching() &&
                    edge->contact->IsEnabled() && edge->contact->GetManifold()->pointCount &&
                    !edge->contact->GetFixtureA()->IsSensor() &&
                    !edge->contact->GetFixtureB()->IsSensor()) {
                    b2WorldManifold m;
                    edge->contact->GetWorldManifold(&m);
                    float ny =
                        edge->contact->GetFixtureA()->GetBody() == page.bodies[page.playfieldCount]
                            ? -m.normal.y
                            : m.normal.y;
                    lastTerrain |= ny >= .5f;
                }
        }
        float stop = page.bean.state.vx * page.bean.state.vx / 200;
        keys[VK_RIGHT] = result.launches > 0 && target > start && page.bean.state.x < target - stop;
        keys[VK_LEFT] = result.launches > 0 && target < start && page.bean.state.x > target + stop;
        if (!fromRight && !ordinary && step == 650)
            captureCase("test-output/selector-jelly-right-inherited-flight.bmp");
        if (ordinary && result.ordinaryLaunches > 0)
            break;
    }
    char row[400], file[120];
    wsprintfA(
        row,
        "source=%d target=%d ordinary=%d overlap=%d sourceSpeed100=%d targetSpeed100=%d ordinaryApex100=%d minApex100=%d maxApex100=%d finalX100=%d finalY100=%d\r\n",
        result.sourceLaunches, result.targetLaunches, result.ordinaryLaunches, result.overlap,
        (int)(result.sourceSpeed * 100), (int)(result.targetSpeed * 100),
        (int)(result.ordinaryApex * 100), (int)(result.minTargetApex * 100),
        (int)(result.maxTargetApex * 100), (int)(page.bean.state.x * 100),
        (int)(page.bean.state.y * 100));
    wsprintfA(file, "test-output/selector-jelly-route-%d-%d.txt", fromRight, ordinary);
    atomicWrite(file, row, lstrlenA(row));
    wsprintfA(file, "test-output/selector-jelly-route-contact-%d-%d.txt", fromRight, ordinary);
    atomicWrite(file, contactRows, lstrlenA(contactRows));
    memset(keys, 0, sizeof(keys));
    return result;
}

void selectorTrampolineNativeChecks() {
    bool oldMusic = musicEnabled, oldSound = soundEnabled, oldDebug = debugMode;
    debugMode = false;
    float bonusApex = 0;
    float bonusSpeed = nativeTrampolineLaunch(100, -200, 37, 20, bonusApex);
    bool bonus = bonusApex == 157 && absf(bonusSpeed - root(22800)) < .01f;
    nativeTrampolineOrdinaryJump(80.25f, 37, 20, bonusApex);
    check(
        bonus && bonusApex == 137.25f && nativeTrampolineRoundedY(-.5f) == -1 &&
            nativeTrampolineRoundedY(.5f) == 1 && nativeTrampolineRoundedY(-.49f) == 0,
        "Native trampoline mathematics keeps Tom's separate pixel-height jump bonus and rounds half-pixel coordinates away from zero");
    prepareSelectorTrampolineNative();
    int pads = 0;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].kind == TRAMPOLINE)
            pads++;
    check(page.level.id == 12038123762473955738ULL && pads == 2 &&
              page.playfield[0].source->id == 6285546912625162820ULL &&
              page.playfield[1].source->id == 12499371799514561398ULL,
          "Third-chapter playable selector uses the two original jelly pads and original geometry");
    u32 checkpointHash = hashBytes(&checkpoint, sizeof(checkpoint)),
        stageHash = hashBytes(entities, sizeof(Entity) * entityCount);
    int stageJumps = jumps;
    SelectorTrampolineRoute high = selectorTrampolineNativeRoute(false);
    check(
        high.sourceLaunches == 1 && high.targetLaunches >= 3 &&
            absf(high.sourceSpeed - root(400 * 254)) < .02f &&
            absf(high.targetSpeed - root(400 * 238)) < .02f,
        "Actual left-to-higher-right jelly route inherits native absolute apex instead of resetting to the basic37pixel jump");
    check(
        high.overlap == high.targetLaunches && high.minTargetApex == 340 &&
            high.maxTargetApex == 340,
        "Every right jelly landing retains apex340 despite simultaneous solved terrain support and repeated pad contacts");
    check(
        checkpointHash == hashBytes(&checkpoint, sizeof(checkpoint)) &&
            stageHash == hashBytes(entities, sizeof(Entity) * entityCount) && stageJumps == jumps,
        "Playable selector trampoline inheritance leaves the running campaign entities, checkpoint and jump count unchanged");
    SelectorTrampolineRoute low = selectorTrampolineNativeRoute(true);
    check(
        low.sourceLaunches == 1 && low.targetLaunches >= 3 &&
            absf(low.sourceSpeed - root(400 * 238)) < .02f &&
            absf(low.targetSpeed - root(400 * 254)) < .02f && low.minTargetApex == 340 &&
            low.maxTargetApex == 340,
        "Actual right-to-lower-left jelly route preserves the same native apex and derives the greater lower-pad launch speed");
    SelectorTrampolineRoute terrain = selectorTrampolineNativeRoute(false, true);
    check(
        terrain.sourceLaunches == 1 && terrain.ordinaryLaunches == 1 &&
            terrain.ordinaryApex > 170 && terrain.ordinaryApex < 172,
        "An actual ordinary-terrain landing after a high jelly bounce resets the inherited apex to its local basic jump");
    prepareSelectorTrampolineNative(true);
    page.trampolineApex = 340;
    page.bean.state.y = 101.5f;
    page.bean.state.vy = 40;
    resetUiPhysics(page);
    tickInterface(1.0f / 120);
    check(
        page.contacts->trampoline >= 0 && page.bean.state.vy > 0 && page.bean.state.vy < 40 &&
            page.bean.state.timer > 0,
        "A real rising jelly contact records a next-step request without flipping velocity in the contact tick");
    page.trampolineApex = 999;
    page.miniDeath = .001f;
    tickInterface(1.0f / 120);
    bool deathReset = page.trampolineApex == page.beanSpawn.y;
    page.trampolineApex = 999;
    navigate(12038123762473955738ULL);
    check(
        deathReset && page.trampolineApex == page.beanSpawn.y,
        "Mini-Bean death/respawn and reopening the selector reset inherited apex to the authored spawn height");
    debugMode = oldDebug;
    musicEnabled = oldMusic;
    soundEnabled = oldSound;
}
