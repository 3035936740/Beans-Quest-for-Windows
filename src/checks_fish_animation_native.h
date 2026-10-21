#pragma once

// Regression coverage for fish animation native. Runs only in the isolated BQ_ENABLE_TESTS executable.
int prepareFishAnimationNative() {
    startStage(0);
    readyTimer = deathTimer = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex)
            entities[i].state.gone = 1;
    int index = entityCount;
    addEntity(*classById(706166303646ULL), Rect{240, 112, 272, 144});
    Entity &e = entities[index];
    e.state.active = 1;
    resetNativeMotion(index);
    initialState[index] = e.state;
    initialMotion[index] = MotionState{e.originX, e.originY, e.turnCooldown, 1, e.lastLaunchSpeed};
    current.geometryCount = 0;
    entities[playerIndex].state.x = 800;
    entities[playerIndex].state.y = 800;
    entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
    rebuildPhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    b2BodyDef def;
    def.position.Set(192 * metres, 128 * metres);
    b2Body *wall = physicsWorld->CreateBody(&def);
    b2PolygonShape shape;
    shape.SetAsBox(8 * metres, 2000 * metres);
    wall->CreateFixture(&shape, 0);
    def.position.x = 320 * metres;
    wall = physicsWorld->CreateBody(&def);
    wall->CreateFixture(&shape, 0);
    return index;
}

float distinctFishFrameNative(Entity &e, int selected) {
    e.fishAnimation.selected = selected;
    float &age = selected == FISH_LEFT_NATIVE ? e.fishAnimation.leftAge : e.fishAnimation.rightAge;
    age = 0;
    u32 first = enemyAnimationBoundaryPixels(e);
    Animation *a = animation(e.art, fishAnimationNameNative(e));
    int ticks = 0;
    for (int i = 0; i < a->count; i++) {
        age = (ticks + a->frames[i].duration * .5f) / a->duration;
        if (enemyAnimationBoundaryPixels(e) != first)
            return age;
        ticks += a->frames[i].duration;
    }
    return -1;
}

void fishAnimationNativeChecks() {
    bool oldInterface = interfaceEnabled, oldSound = soundEnabled, oldMusic = musicEnabled,
         oldDebug = debugMode;
    interfaceEnabled = soundEnabled = musicEnabled = debugMode = false;
    stopMusic();
    stopSounds();
    Arena memory;
    memory.init(32 * 1024 * 1024);
    Level scan = {};
    int count = 0, paused = 0;
    bool clips = true, retained = true;
    for (int scene = 0; scene < levelCount; scene++) {
        readLevelData(levels[scene].file, scan, memory);
        for (int i = 0; i < scan.objectCount; i++) {
            Object &o = scan.objects[i];
            if (!o.has("DiagonalBehaviour"))
                continue;
            Entity e;
            initializeEntity(e, o, o.rect);
            count++;
            paused += o.paused != 0;
            clips &=
                o.klass == 706166303646ULL && e.fishAnimation.selected == FISH_DEFAULT_NATIVE &&
                animation(e.art, "")->count == 1 && animationLength(animation(e.art, "")) == 0 &&
                animation(e.art, "left")->count == 4 && animation(e.art, "right")->count == 4;
            e.fishAnimation = FishAnimationNative{.13f, .07f, FISH_LEFT_NATIVE};
            State before = e.state;
            selectFishAnimationNative(e, 1);
            advanceFishAnimationNative(e, .01f);
            retained &=
                e.fishAnimation.leftAge == .13f && absf(e.fishAnimation.rightAge - .08f) < .000001f;
            selectFishAnimationNative(e, -1);
            selectFishAnimationNative(e, -1);
            retained &= e.fishAnimation.leftAge == .13f &&
                        hashBytes(&before, sizeof(before)) == hashBytes(&e.state, sizeof(e.state));
        }
    }
    check(
        count == 9 && paused == 8 && clips,
        "All74 original scenes contain exactly9diagonal fish with8initially paused, a static default pose and separate four-frame left/right clips");
    check(
        retained,
        "Every authored fish retains both directional instances and advances only the selected one without changing physical state");
    Entity fish;
    initializeEntity(fish, *classById(706166303646ULL), Rect{240, 112, 272, 144});
    float leftAge = distinctFishFrameNative(fish, FISH_LEFT_NATIVE);
    u32 leftPixels = enemyAnimationBoundaryPixels(fish);
    float rightAge = distinctFishFrameNative(fish, FISH_RIGHT_NATIVE);
    u32 rightPixels = enemyAnimationBoundaryPixels(fish);
    selectFishAnimationNative(fish, -1);
    fish.state.vx = 100;
    bool leftRetained = enemyAnimationBoundaryPixels(fish) == leftPixels;
    fish.fishAnimation.leftAge = 0;
    bool leftDifferent = enemyAnimationBoundaryPixels(fish) != leftPixels;
    fish.fishAnimation.leftAge = leftAge;
    selectFishAnimationNative(fish, 1);
    fish.state.vx = -100;
    bool rightRetained = enemyAnimationBoundaryPixels(fish) == rightPixels;
    fish.fishAnimation.rightAge = 0;
    check(
        leftAge > 0 && rightAge > 0 && leftRetained && leftDifferent && rightRetained &&
            enemyAnimationBoundaryPixels(fish) != rightPixels,
        "Actual OpenGL fish frames retain each directional phase and obey selected animation rather than opposite solver velocity");
    DiagonalRuntimeState d = {-1, 0, 100, 1};
    OscillatorRuntimeState oscillator = {};
    fish.state.active = 1;
    fish.fishAnimation = FishAnimationNative{.13f, .07f, FISH_LEFT_NATIVE};
    nativeDiagonalOnHit(d, 1, 0);
    bool deferred = fish.fishAnimation.selected == FISH_LEFT_NATIVE;
    advanceNativeDiagonalState(fish, d, 120);
    bool right = fish.fishAnimation.selected == FISH_RIGHT_NATIVE &&
                 fish.fishAnimation.leftAge == .13f && fish.fishAnimation.rightAge == .07f;
    d.directionX = 0;
    d.directionY = -1;
    advanceNativeDiagonalState(fish, d, 120);
    nativeDiagonalOnHit(d, 0, 1);
    advanceNativeDiagonalState(fish, d, 120);
    check(
        deferred && right && fish.fishAnimation.selected == FISH_RIGHT_NATIVE && d.directionY > 0,
        "Diagonal contact changes direction first, selects its retained clip on the next PrePhysics pass, treats zeroX as right and keeps facing across vertical-only reflection");
    int index = prepareFishAnimationNative();
    Entity &e = entities[index];
    e.fishAnimation = FishAnimationNative{.13f, .07f, FISH_LEFT_NATIVE};
    int turns = 0;
    bool continuity = true, callbackOrder = true;
    for (int i = 0; i < 1800 && turns < 6; i++) {
        FishAnimationNative before = e.fishAnimation;
        float direction = diagonalRuntime[index].directionX;
        tick(1 / 120.0f, false, false);
        int expected = direction < 0 ? FISH_LEFT_NATIVE : FISH_RIGHT_NATIVE;
        callbackOrder &= e.fishAnimation.selected == expected;
        if (before.selected != e.fishAnimation.selected) {
            turns++;
            continuity &= fishAnimationAgeNative(e) ==
                          (expected == FISH_LEFT_NATIVE ? before.leftAge : before.rightAge);
        }
    }
    check(
        turns >= 6 && continuity && callbackOrder && deathTimer <= 0 &&
            absf(root(e.state.vx * e.state.vx + e.state.vy * e.state.vy) - 100) < .001f,
        "Six actual Box2D fish wall reflections preserve directional ages and native callback order without changing original100px-per-second motion");
    e.fishAnimation = FishAnimationNative{.13f, .07f, FISH_RIGHT_NATIVE};
    float x = e.state.x, y = e.state.y;
    screen = PAUSE;
    tick(.01f, false, false);
    screen = PLAY;
    check(
        e.fishAnimation.leftAge == .13f && e.fishAnimation.rightAge > .07f && e.state.x == x &&
            e.state.y == y,
        "Pause advances only an activated fish's selected opted-in clip while its carrier and detector stay stationary");
    e.fishAnimation = FishAnimationNative{.13f, .07f, FISH_RIGHT_NATIVE};
    captureCheckpoint(false);
    char path[MAX_PATH];
    savePath(path, "fish-v16.dat");
    bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    e.fishAnimation = FishAnimationNative{};
    bool loaded = written && readCheckpoint(path);
    restoreCheckpoint();
    check(
        loaded && checkpoint.version == 28 && sizeof(SnapshotV15) == 319868 &&
            sizeof(SnapshotV16) == 344444 && sizeof(SnapshotV17) == 364156 &&
            sizeof(SnapshotV18) == 366720 && sizeof(SnapshotV19) == 401536 &&
            sizeof(SnapshotV21) == 486016 && sizeof(SnapshotV22) == 510592 &&
            sizeof(SnapshotV23) == 510624 && sizeof(Snapshot) == 543392 &&
            validSnapshot(checkpoint) && e.fishAnimation.leftAge == .13f &&
            e.fishAnimation.rightAge == .07f && e.fishAnimation.selected == FISH_RIGHT_NATIVE,
        "Actual version28 disk restoration preserves fish selection and both ages while retaining the exact legacy version15 layout");
    SnapshotV15 *old = (SnapshotV15 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV15));
    memcpy(old, &checkpoint, sizeof(*old) - 4);
    old->version = 15;
    old->state[index].vx = -100;
    old->state[index].timer = .03f;
    old->jumperAnimations[index] = JumperAnimationNative{.11f, .06f, 1};
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    savePath(path, "fish-legacy-v15.dat");
    written = atomicWrite(path, old, sizeof(*old));
    HeapFree(GetProcessHeap(), 0, old);
    loaded = written && readCheckpoint(path);
    restoreCheckpoint();
    check(
        loaded && checkpoint.version == 15 && validSnapshot(checkpoint) &&
            e.fishAnimation.selected == FISH_LEFT_NATIVE && e.fishAnimation.leftAge == .03f &&
            e.fishAnimation.rightAge == 0 && e.jumperAnimation.standingAge == .11f &&
            e.jumperAnimation.jumpingAge == .06f,
        "Actual version15 bytes preserve the prior jumper records and migrate the fish's known visible phase without inventing its unsaved opposite age");
    captureCheckpoint(false);
    checkpoint.fishAnimations[index].selected = 3;
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    bool badSelection = !validSnapshot(checkpoint);
    checkpoint.fishAnimations[index].selected = FISH_LEFT_NATIVE;
    checkpoint.fishAnimations[index].rightAge = -1;
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    check(
        badSelection && !validSnapshot(checkpoint),
        "Invalid fish selection and negative retained age fail validation even when the saved checksum is recomputed");
    e.fishAnimation = FishAnimationNative{.13f, .07f, FISH_RIGHT_NATIVE};
    killEnemy(e);
    advanceKillableDeathNative(e, .01f);
    bool dying = killableDeathVisibleNative(e) && e.fishAnimation.leftAge == .13f &&
                 e.fishAnimation.rightAge > .07f;
    restartStage();
    check(
        dying && e.fishAnimation.selected == FISH_DEFAULT_NATIVE && e.fishAnimation.leftAge == 0 &&
            e.fishAnimation.rightAge == 0,
        "Fish death squash keeps the selected directional instance progressing while a fresh retry restores the native default pose and zero ages");
    startStage(0);
    u32 campaignHash = hashBytes(entities, sizeof(Entity) * entityCount);
    navigate(137502285691ULL);
    int ending = -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (hasFishAnimationNative(page.playfield[i]))
            ending = i;
    bool endingFrames = false;
    if (ending >= 0) {
        Entity &display = page.playfield[ending];
        tickCreditsRuntime(page, .01f);
        FishAnimationNative before = display.fishAnimation;
        tickCreditsRuntime(page, .01f);
        endingFrames = display.fishAnimation.selected == FISH_LEFT_NATIVE &&
                       display.fishAnimation.leftAge > before.leftAge &&
                       display.fishAnimation.rightAge == before.rightAge;
    }
    check(
        endingFrames && campaignHash == hashBytes(entities, sizeof(Entity) * entityCount),
        "The real ending fish advances its scene-owned directional instance and leaves suspended campaign actors untouched");
    startStage(stageById(176011402805ULL));
    readyTimer = 0;
    int fishCount = 0;
    bool dormant = true;
    u32 initialPixels[2] = {};
    int ids[2] = {-1, -1};
    for (int i = 0; i < entityCount; i++)
        if (hasFishAnimationNative(entities[i]) && fishCount < 2) {
            ids[fishCount] = i;
            initialPixels[fishCount] = enemyAnimationBoundaryPixels(entities[i]);
            fishCount++;
        }
    for (int i = 0; i < 30; i++)
        tick(1 / 120.0f, false, false);
    for (int i = 0; i < fishCount; i++) {
        Entity &dormantFish = entities[ids[i]];
        dormant &= !dormantFish.state.active &&
                   dormantFish.fishAnimation.selected == FISH_DEFAULT_NATIVE &&
                   dormantFish.fishAnimation.leftAge == 0 &&
                   dormantFish.fishAnimation.rightAge == 0 && !bodies[ids[i]]->IsActive() &&
                   initialPixels[i] == enemyAnimationBoundaryPixels(dormantFish);
    }
    check(
        fishCount == 2 && dormant,
        "Both untouched original2-4 fish keep their static default artwork and inactive physics before the authored proximity activation");
    startStage(0);
    interfaceEnabled = oldInterface;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
    debugMode = oldDebug;
}
