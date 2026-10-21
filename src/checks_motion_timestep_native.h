#pragma once

// Regression coverage for motion timestep native. Runs only in the isolated BQ_ENABLE_TESTS executable.
struct MotionTimestepSample {
    float x, y, clock;
};

void motionTimestepNativeChecks() {
    char oldSaveRoot[MAX_PATH];
    lstrcpyA(oldSaveRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/motion-timestep-saves");
    CreateDirectoryA(saveRoot, 0);
    Progress oldProgress = progress;
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldMusic = musicEnabled,
         oldSound = soundEnabled;
    float oldRenderFPS = nativeMotionRenderFPS;
    interfaceEnabled = false;
    debugMode = true;
    musicEnabled = soundEnabled = false;
    const float dt = 1.0f / 120;
    const u64 stages[] = {8315980062923458623ULL, 5687107557475985288ULL};
    const u64 ids[] = {14840038685926996665ULL, 12540492440314510417ULL};
    const float base[] = {100, 90}, slow[] = {99, 89};
    check(
        absf(nativeMotionPhysicsFPS(dt) - 120) < .001f &&
            absf(nativeMotionPhysicsFPS(1.0f / 30) - 30) < .001f,
        "Motion performance selection derives native per-physics-frame frequency from its actual timestep rather than renderer timing");
    MotionTimestepSample *reference = (MotionTimestepSample *)HeapAlloc(
        GetProcessHeap(), HEAP_ZERO_MEMORY, 720 * sizeof(MotionTimestepSample));
    bool foundAll = true, initial = true, same = true, checkpointExact = true, slowBranch = true,
         oldDifferent = true;
    char trace[12000] = {};
    lstrcpyA(trace,
             "scene,mode,renderFPS,step,x1000,y1000,clock100000,speed1000,selected,active\r\n");
    for (int scenario = 0; scenario < 2; scenario++)
        for (int mode = 0; mode < 3; mode++) {
            nativeMotionRenderFPS = mode == 1 ? 60.0f : 30.0f;
            startStage(stageById(stages[scenario]));
            readyTimer = 0;
            int index = -1;
            for (int i = 0; i < entityCount; i++) {
                if (entities[i].source->id == ids[scenario])
                    index = i;
                else if (entities[i].kind == ENEMY)
                    entities[i].state.gone = 1;
            }
            if (index < 0) {
                foundAll = false;
                continue;
            }
            Entity &e = entities[index];
            State &bean = entities[playerIndex].state;
            bean.x = 64;
            bean.y = 300;
            bean.vx = bean.vy = 0;
            restorePhysics();
            bodies[playerIndex]->SetType(b2_staticBody);
            float initialClock = oscillatorRuntime[index].clock;
            // Historical raw-render caller is explicitly emulated only in mode2.
            if (mode == 2)
                selectNativeMotionSpeed(*e.move, 30, oscillatorRuntime[index].effectiveSpeed,
                                        oscillatorRuntime[index].selected);
            tick(dt, false, false);
            {
                char first[256];
                wsprintfA(
                    first,
                    "initial,%d,%d,selected=%d,active=%d,clock1000000=%d,beforeClock1000000=%d,speed1000=%d,expectedSpeed1000=%d\r\n",
                    scenario, mode, oscillatorRuntime[index].selected, e.state.active,
                    (int)(oscillatorRuntime[index].clock * 1000000), (int)(initialClock * 1000000),
                    (int)(oscillatorRuntime[index].effectiveSpeed * 1000),
                    (int)((mode == 2 ? slow[scenario] : base[scenario]) * 1000));
                lstrcatA(trace, first);
            }
            initial &= oscillatorRuntime[index].selected && e.state.active == 1 &&
                       !e.source->paused &&
                       absf(oscillatorRuntime[index].clock - (initialClock + dt)) < .00001f &&
                       absf(oscillatorRuntime[index].effectiveSpeed -
                            (mode == 2 ? slow[scenario] : base[scenario])) < .001f;
            captureCheckpoint(false);
            OscillatorRuntimeState selected = oscillatorRuntime[index];
            oscillatorRuntime[index].selected = 0;
            oscillatorRuntime[index].effectiveSpeed = 1;
            oscillatorRuntime[index].clock = 0;
            restoreCheckpoint();
            checkpointExact &= oscillatorRuntime[index].selected == selected.selected &&
                               oscillatorRuntime[index].effectiveSpeed == selected.effectiveSpeed &&
                               oscillatorRuntime[index].clock == selected.clock;
            bean.x = e.originX + (scenario == 0 ? 100 : 0);
            bean.y = e.originY + (scenario == 0 ? 0 : 80);
            bean.vx = bean.vy = 0;
            restorePhysics();
            float largestLegacyDelta = 0;
            for (int step = 0; step < 720; step++) {
                tick(dt, false, false);
                MotionTimestepSample got = {e.state.x, e.state.y, oscillatorRuntime[index].clock};
                if (mode == 0)
                    reference[step] = got;
                else if (mode == 1)
                    same &= absf(got.x - reference[step].x) < .001f &&
                            absf(got.y - reference[step].y) < .001f &&
                            absf(got.clock - reference[step].clock) < .00001f;
                else
                    largestLegacyDelta =
                        maxf(largestLegacyDelta,
                             absf(got.x - reference[step].x) + absf(got.y - reference[step].y));
                if (step % 60 == 0 || step == 719) {
                    char sceneId[24];
                    idstr(stages[scenario], sceneId);
                    char row[256];
                    wsprintfA(row, "%s,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", sceneId, mode,
                              (int)nativeMotionRenderFPS, step, (int)(got.x * 1000),
                              (int)(got.y * 1000), (int)(got.clock * 100000),
                              (int)(oscillatorRuntime[index].effectiveSpeed * 1000),
                              oscillatorRuntime[index].selected, e.state.active);
                    if (lstrlenA(trace) + lstrlenA(row) < 11000)
                        lstrcatA(trace, row);
                }
            }
            if (mode == 2)
                oldDifferent &= largestLegacyDelta > .5f;
            if (mode == 0) {
                nativeMotionRenderFPS = 10;
                tick(1.0f / 30, false, false);
                checkpointExact &=
                    oscillatorRuntime[index].selected &&
                    absf(oscillatorRuntime[index].effectiveSpeed - base[scenario]) < .001f;
            }
            if (mode == 2) {
                startStage(stageById(stages[scenario]));
                readyTimer = 0;
                index = -1;
                for (int i = 0; i < entityCount; i++)
                    if (entities[i].source->id == ids[scenario])
                        index = i;
                if (index < 0) {
                    slowBranch = false;
                    continue;
                }
                bean = entities[playerIndex].state;
                bean.x = 64;
                bean.y = 300;
                bean.vx = bean.vy = 0;
                restorePhysics();
                bodies[playerIndex]->SetType(b2_staticBody);
                nativeMotionRenderFPS = 120;
                tick(1.0f / 30, false, false);
                slowBranch &=
                    oscillatorRuntime[index].selected &&
                    absf(oscillatorRuntime[index].effectiveSpeed - slow[scenario]) < .001f;
            }
        }
    HeapFree(GetProcessHeap(), 0, reference);
    atomicWrite("test-output/motion-timestep-native.csv", trace, lstrlenA(trace));
    check(
        foundAll,
        "Both original nonempty3g-speed platform instances resolve in Clifftop Tricks and The Tunnels");
    check(
        initial,
        "A30Hz renderer with120Hz physics selects original100/90 while both authored-unpaused platforms retain their native first-step clock advance");
    check(
        same,
        "Both actual authored platforms follow identical six-second positions and phases at30/60 renderer inputs when the physics timestep is the same");
    check(
        checkpointExact,
        "Checkpoint reload preserves each platform's once-only speed decision and phase, and later slower physical steps do not reselect it");
    check(
        slowBranch,
        "Fresh30Hz physical frames still select the original99/89 slow-device overrides even with a120Hz renderer input");
    check(
        oldDifferent,
        "The historical raw-render30Hz selection reproduces a measurable phase/position discrepancy in both authored platforms");
    startStage(0);
    readyTimer = 0;
    lstrcpyA(saveRoot, oldSaveRoot);
    progress = oldProgress;
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    musicEnabled = oldMusic;
    soundEnabled = oldSound;
    nativeMotionRenderFPS = oldRenderFPS;
}
