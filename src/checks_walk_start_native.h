#pragma once

// Regression coverage for walk start native. Runs only in the isolated BQ_ENABLE_TESTS executable.

// Native Walk ctor clears grounded (+be); WalkRight selects direction only.
// Horizontal velocity is first assigned by PrePhysics after a real PostSolve.
void walkStartNativeChecks() {
    char oldSaveRoot[MAX_PATH];
    lstrcpyA(oldSaveRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/walk-start-saves");
    CreateDirectoryA(saveRoot, 0);
    Progress oldProgress = progress;
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldMusic = musicEnabled,
         oldSound = soundEnabled;
    interfaceEnabled = false;
    debugMode = true;
    musicEnabled = soundEnabled = false;
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scan = {};
    int walks = 0;
    bool initial = true;
    for (int scene = 0; scene < levelCount; scene++) {
        readLevelData(levels[scene].file, scan, memory);
        for (int i = 0; i < scan.objectCount; i++) {
            Object &o = scan.objects[i];
            const Props *config = o.behavior("WalkBehaviour");
            if (!config)
                continue;
            walks++;
            Entity e;
            initializeEntity(e, o, o.rect);
            float speed = config->num("walk-speed", 75),
                  direction = config->flag("walk-left") ? -1.0f : 1.0f;
            initial &=
                e.walk && e.state.vx == 0 && e.state.aux == speed * direction && e.speed == speed;
        }
    }
    releaseLevel(scan, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        levelCount == 74 && walks == 78 && initial,
        "All74 original scenes initialize all78 walkers at zero horizontal velocity while preserving authored speed and left/right direction");
    startStage(stageById(176011402805ULL));
    readyTimer = 0;
    const u64 ids[] = {211762447926ULL, 657093808498ULL};
    int index[2] = {-1, -1}, first[2] = {-1, -1};
    float origin[2] = {};
    bool held = true, moved = true, restarted = true, restored = true;
    for (int i = 0; i < entityCount; i++)
        for (int n = 0; n < 2; n++)
            if (entities[i].source->id == ids[n]) {
                index[n] = i;
                origin[n] = entities[i].state.x;
            }
    State &bean = entities[playerIndex].state;
    bean.x = 64;
    bean.y = 300;
    bean.vx = bean.vy = 0;
    restorePhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    const float dt = 1.0f / 120;
    char trace[12000] = {};
    lstrcpyA(trace, "step,object,x1000,y1000,vx1000,ground\r\n");
    for (int step = 0; step < 18; step++) {
        tick(dt, false, false);
        for (int n = 0; n < 2; n++) {
            if (index[n] < 0) {
                held = moved = false;
                continue;
            }
            Entity &e = entities[index[n]];
            bool ground = enemyGrounded(index[n]);
            if (first[n] < 0) {
                held &= absf(e.state.x - origin[n]) < .001f && absf(e.state.vx) < .001f;
                if (ground)
                    first[n] = step;
            } else if (step == first[n] + 1)
                moved &= e.state.x > origin[n] + dt * e.speed * .8f && e.state.vx > e.speed - 2 &&
                         e.state.vx <= e.speed + .01f;
            char id[24], row[160];
            idstr(ids[n], id);
            wsprintfA(row, "%d,%s,%d,%d,%d,%d\r\n", step, id, (int)(e.state.x * 1000),
                      (int)(e.state.y * 1000), (int)(e.state.vx * 1000), ground);
            lstrcatA(trace, row);
        }
    }
    check(
        index[0] >= 0 && index[1] >= 0 && first[0] > 0 && first[1] > 0 && held && moved,
        "Both real No Diving walkers wait for first physical ground contact before starting their authored75-pixel walk, with all stage geometry retained");
    captureCheckpoint(false);
    State saved[2] = {};
    for (int n = 0; n < 2; n++)
        if (index[n] >= 0) {
            saved[n] = entities[index[n]].state;
            entities[index[n]].state.vx = 0;
            entities[index[n]].state.aux = -1;
        }
    restoreCheckpoint();
    for (int n = 0; n < 2; n++)
        if (index[n] >= 0) {
            State &state = entities[index[n]].state;
            restored &= saved[n].vx > 70 && state.vx == saved[n].vx && state.aux == saved[n].aux &&
                        state.x == saved[n].x;
        } else
            restored = false;
    check(
        restored,
        "Reloading an in-motion checkpoint restores both walkers' real velocity, position and direction rather than reapplying startup zero velocity");
    restartStage();
    for (int n = 0; n < 2; n++)
        if (index[n] >= 0) {
            State &state = entities[index[n]].state;
            restarted &= state.vx == 0 && state.aux == 75 && state.x == origin[n] &&
                         bodies[index[n]] && bodies[index[n]]->GetLinearVelocity().x == 0;
        } else
            restarted = false;
    check(
        restarted,
        "The ordinary R restart path resets both original walkers and their Box2D bodies to zero horizontal velocity while retaining their75-pixel direction");
    atomicWrite("test-output/walk-start-native.csv", trace, lstrlenA(trace));
    startStage(0);
    readyTimer = 0;
    lstrcpyA(saveRoot, oldSaveRoot);
    progress = oldProgress;
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    musicEnabled = oldMusic;
    soundEnabled = oldSound;
}
