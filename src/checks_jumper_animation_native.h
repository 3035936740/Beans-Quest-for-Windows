#pragma once

// Regression coverage for jumper animation native. Runs only in the isolated BQ_ENABLE_TESTS executable.
float distinctJumperFrameAgeNative(Entity &e, bool jumping) {
    float &age = jumping ? e.jumperAnimation.jumpingAge : e.jumperAnimation.standingAge;
    age = 0;
    u32 initial = enemyAnimationBoundaryPixels(e);
    Animation *a = animation(e.art, jumping ? "jumping" : "standing");
    int ticks = 0;
    for (int i = 0; i < a->count; i++) {
        age = (ticks + a->frames[i].duration * .5f) / a->duration;
        if (enemyAnimationBoundaryPixels(e) != initial)
            return age;
        ticks += a->frames[i].duration;
    }
    return -1;
}

int prepareJumperAnimationNative() {
    startStage(0);
    readyTimer = deathTimer = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex)
            entities[i].state.gone = 1;
    Object *purple = 0;
    for (int i = 0; i < classCount; i++)
        if (classes[i].has("EnemyJumperBehaviour") &&
            animation(actor(classes[i].id), "jumping")->count > 1) {
            purple = classes + i;
            break;
        }
    if (!purple)
        return -1;
    int index = entityCount;
    addEntity(*purple, Rect{240, 112, 272, 144});
    Entity &e = entities[index];
    e.state.active = 1;
    initialState[index] = e.state;
    initialMotion[index] = MotionState{e.originX, e.originY, e.turnCooldown, 1, e.lastLaunchSpeed};
    static Vec points[] = {{0, 80}, {0, 96}, {4096, 96}, {4096, 80}};
    static Geometry ground = {};
    ground.kind = 1;
    ground.flags = 1;
    ground.count = 4;
    ground.points = points;
    current.geometry = &ground;
    current.geometryCount = 1;
    entities[playerIndex].state.x = 1000;
    entities[playerIndex].state.y = 1000;
    entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
    rebuildPhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    return index;
}

void jumperAnimationNativeChecks() {
    bool oldInterface = interfaceEnabled, oldSound = soundEnabled, oldMusic = musicEnabled,
         oldDebug = debugMode;
    interfaceEnabled = soundEnabled = musicEnabled = debugMode = false;
    stopMusic();
    stopSounds();
    Arena memory;
    memory.init(32 * 1024 * 1024);
    Level scan = {};
    int count = 0, multiStanding = 0, multiJumping = 0;
    bool retained = true;
    Entity purple = {};
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scan, memory);
        for (int i = 0; i < scan.objectCount; i++) {
            Object &o = scan.objects[i];
            const Props *jump = o.behavior("EnemyJumperBehaviour");
            if (!jump)
                continue;
            Entity e;
            initializeEntity(e, o, o.rect);
            count++;
            multiStanding += animation(e.art, "standing")->count > 1;
            multiJumping += animation(e.art, "jumping")->count > 1;
            e.jumperAnimation = JumperAnimationNative{.13f, .07f, 0};
            float timer = e.state.timer = .25f, delay = jump->num("jump-delay", 1);
            e.state.aux = delay + .1f;
            jumperNativeStepState(e, .01f, delay, false, true, e.state.x - 1);
            retained &= e.jumperAnimation.selected == 1 && e.jumperAnimation.standingAge == .13f &&
                        e.jumperAnimation.jumpingAge == .07f && e.state.timer == timer;
            advanceJumperAnimationNative(e, .01f);
            float jumping = e.jumperAnimation.jumpingAge;
            e.state.aux = 0;
            jumperNativeStepState(e, .01f, delay, false, true, e.state.x - 1);
            retained &= e.jumperAnimation.selected == 0 && e.jumperAnimation.standingAge == .13f &&
                        e.jumperAnimation.jumpingAge == jumping && e.state.timer == timer;
            advanceJumperAnimationNative(e, .01f);
            retained &= e.jumperAnimation.jumpingAge == jumping;
            if (animation(e.art, "jumping")->count > 1) {
                purple = e;
                purple.source = classById(o.klass);
            }
        }
    }
    check(
        count == 31 && multiStanding == 31 && multiJumping == 5,
        "Original74-scene census covers all31jumpers, their31multiframe standing clips and5multiframe purple jumping clips");
    check(
        retained,
        "Every original jumper selects retained standing and jumping instances without restarting either age or its unrelated timer");
    purple.state.gone = 0;
    purple.state.hp = 1;
    selectJumperAnimationNative(purple, true);
    float jumpAge = distinctJumperFrameAgeNative(purple, true);
    u32 jumpingPixels = enemyAnimationBoundaryPixels(purple);
    purple.state.aux = 0;
    jumperNativeStepState(purple, .01f, 1, false, true, purple.state.x);
    float standAge = distinctJumperFrameAgeNative(purple, false);
    u32 standingPixels = enemyAnimationBoundaryPixels(purple);
    purple.state.aux = 2;
    jumperNativeStepState(purple, .01f, 1, false, true, purple.state.x);
    bool jumpRetained = enemyAnimationBoundaryPixels(purple) == jumpingPixels;
    purple.jumperAnimation.jumpingAge = 0;
    bool jumpVisible = enemyAnimationBoundaryPixels(purple) != jumpingPixels;
    purple.jumperAnimation.jumpingAge = jumpAge;
    purple.state.aux = 0;
    jumperNativeStepState(purple, .01f, 1, false, true, purple.state.x);
    bool standRetained = enemyAnimationBoundaryPixels(purple) == standingPixels;
    purple.jumperAnimation.standingAge = 0;
    char gpuTrace[320];
    wsprintfA(
        gpuTrace,
        "jump_retained=%d jump_visible=%d stand_retained=%d stand_visible=%d class_visible=%d jump_hash=%u stand_hash=%u idle0=%u\r\n",
        jumpRetained, jumpVisible, standRetained,
        enemyAnimationBoundaryPixels(purple) != standingPixels, purple.source->visible,
        jumpingPixels, standingPixels, enemyAnimationBoundaryPixels(purple));
    atomicWrite("test-output/jumper-animation-gpu.txt", gpuTrace, lstrlenA(gpuTrace));
    check(
        jumpRetained && jumpVisible && standRetained &&
            enemyAnimationBoundaryPixels(purple) != standingPixels && standAge > 0,
        "Actual OpenGL purple-hopper frames resume both retained clip phases across landing and takeoff rather than reverting to frame0");
    int index = prepareJumperAnimationNative();
    check(
        index >= 0 && sizeof(JumperAnimationNative) == 12 && sizeof(SnapshotV14) == 295292 &&
            sizeof(SnapshotV15) == 319868 && sizeof(SnapshotV16) == 344444 &&
            sizeof(SnapshotV17) == 364156 && sizeof(SnapshotV18) == 366720 &&
            sizeof(SnapshotV19) == 401536 && sizeof(SnapshotV21) == 486016 &&
            sizeof(SnapshotV22) == 510592 && sizeof(SnapshotV23) == 510624 &&
            sizeof(Snapshot) == 543392,
        "Version28 preserves the exact legacy v14 and v15 layouts while retaining explicit12byte jumper instance records");
    if (index >= 0) {
        Entity &e = entities[index];
        e.jumperAnimation = JumperAnimationNative{.13f, .07f, 0};
        int switches = 0;
        bool continuity = true;
        for (int i = 0; i < 1800 && switches < 6; i++) {
            JumperAnimationNative before = e.jumperAnimation;
            tick(1 / 120.0f, false, false);
            if (e.jumperAnimation.selected != before.selected) {
                switches++;
                continuity &= jumperAnimationAgeNative(e, e.jumperAnimation.selected != 0) ==
                              (e.jumperAnimation.selected ? before.jumpingAge : before.standingAge);
            }
        }
        check(
            switches >= 6 && continuity && deathTimer <= 0,
            "Six real Box2D landing/takeoff transitions preserve the inactive instance phase through the production gameplay tick");
        selectJumperAnimationNative(e, true);
        e.jumperAnimation.standingAge = .11f;
        e.jumperAnimation.jumpingAge = .07f;
        float x = e.state.x, y = e.state.y, age = e.jumperAnimation.jumpingAge;
        screen = PAUSE;
        tick(.01f, false, false);
        screen = PLAY;
        check(
            nativeActorAnimateWhilePaused(e) && e.jumperAnimation.jumpingAge > age &&
                e.jumperAnimation.standingAge == .11f && e.state.x == x && e.state.y == y,
            "An authored animate-while-paused hopper advances only its selected independent clip while paused physics remains fixed");
        e.state.hp = 1;
        e.jumperAnimation = JumperAnimationNative{.13f, .07f, 1};
        captureCheckpoint(false);
        char path[MAX_PATH];
        savePath(path, "jumper-v15.dat");
        bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
        e.jumperAnimation = JumperAnimationNative{};
        bool loaded = written && readCheckpoint(path);
        restoreCheckpoint();
        check(
            loaded && checkpoint.version == 28 && validSnapshot(checkpoint) &&
                e.jumperAnimation.standingAge == .13f && e.jumperAnimation.jumpingAge == .07f &&
                e.jumperAnimation.selected == 1,
            "Actual version28 disk restore retains both independent animation ages and the selected jumper instance");
        SnapshotV14 *legacy = (SnapshotV14 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV14));
        memcpy(legacy, &checkpoint, sizeof(*legacy) - 4);
        legacy->version = 14;
        legacy->state[index].hp = 1;
        legacy->state[index].timer = .03f;
        legacy->checksum = hashBytes(legacy, sizeof(*legacy) - 4);
        savePath(path, "jumper-legacy-v14.dat");
        written = atomicWrite(path, legacy, sizeof(*legacy));
        HeapFree(GetProcessHeap(), 0, legacy);
        loaded = written && readCheckpoint(path);
        restoreCheckpoint();
        check(
            loaded && checkpoint.version == 14 && validSnapshot(checkpoint) &&
                e.jumperAnimation.selected == 1 && e.jumperAnimation.jumpingAge == .03f &&
                e.jumperAnimation.standingAge == 0,
            "An actual old version14 file migrates its known selected phase while safely initializing the previously unsaved inactive instance");
        captureCheckpoint(false);
        checkpoint.jumperAnimations[index].selected = 2;
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        bool selectedRejected = !validSnapshot(checkpoint);
        checkpoint.jumperAnimations[index].selected = 1;
        checkpoint.jumperAnimations[index].standingAge = -1;
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        check(
            selectedRejected && !validSnapshot(checkpoint),
            "Invalid saved jumper selection and negative instance age are rejected even with recomputed checksums");
        e.jumperAnimation = JumperAnimationNative{.13f, .07f, 1};
        killEnemy(e);
        advanceKillableDeathNative(e, .01f);
        bool death = killableDeathVisibleNative(e) && e.jumperAnimation.standingAge == .13f &&
                     e.jumperAnimation.jumpingAge > .07f;
        restartStage();
        check(
            death && e.jumperAnimation.selected == 0 && e.jumperAnimation.standingAge == 0 &&
                e.jumperAnimation.jumpingAge == 0,
            "Death squash keeps advancing the selected jumper clip while stage retry resets both original instance ages");
    }
    startStage(0);
    u32 stageHash = hashBytes(entities, sizeof(Entity) * entityCount);
    navigate(137502285691ULL);
    int frog = creditsRuntime.frogLeft;
    bool credits = false;
    if (frog >= 0) {
        Entity &e = page.playfield[frog];
        e.jumperAnimation = JumperAnimationNative{.13f, .07f, 0};
        float inactive = e.jumperAnimation.jumpingAge;
        tickCreditsRuntime(page, .01f);
        credits = e.jumperAnimation.standingAge > .13f && e.jumperAnimation.jumpingAge == inactive;
    }
    check(
        credits && stageHash == hashBytes(entities, sizeof(Entity) * entityCount),
        "The real ending cast advances its scene-owned retained jumper instances without borrowing or modifying campaign state");
    index = prepareJumperWaterPoseCheck(true);
    Entity &frogEntity = entities[index];
    for (int i = 0; i < 1800 && !jumperWaterStanding(frogEntity); i++)
        tick(1 / 120.0f, false, false);
    bool floating = jumperWaterStanding(frogEntity) && frogEntity.jumperAnimation.selected == 0 &&
                    frogEntity.state.hp == 1;
    float waterJumpAge = frogEntity.jumperAnimation.jumpingAge,
          standingAge = frogEntity.jumperAnimation.standingAge;
    for (int i = 0; i < 12; i++)
        tick(1 / 120.0f, false, false);
    check(
        floating && jumperWaterStanding(frogEntity) &&
            frogEntity.jumperAnimation.jumpingAge == waterJumpAge &&
            frogEntity.jumperAnimation.standingAge != standingAge && frogEntity.state.hp == 1,
        "A settled water frog retains the authorized idle animation fix, advancing standing only while preserving its physical jumping mode");
    startStage(0);
    interfaceEnabled = oldInterface;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
    debugMode = oldDebug;
}
