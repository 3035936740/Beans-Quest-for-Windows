#pragma once

// Regression coverage for av. Runs only in the isolated BQ_ENABLE_TESTS executable.

int activeAudioVoices() {
    int n = 0;
    for (int i = 0; i < 32; i++)
        if (audioVoices[i].clip)
            n++;
    return n;
}

void captureCase(const char *file) {
    transitionTime = 0;
    render();
    capture(file);
}

void audiovisualChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = false;
    musicEnabled = false;
    soundEnabled = false;
    debugMode = false;
    progress.unlocked = stageCount - 1;
    check(soundClipCount == 21 && victoryClip.frames > 0,
          "All 21 original effects and victory music preload as PCM");
    soundEnabled = true;
    stopSounds();
    u32 reads = audioFileReads;
    sound("coin");
    sound("jump");
    short chunk[1024];
    mixAudio(chunk, 512);
    u32 coinPosition = audioVoices[0].position;
    sound("enemyexplosion");
    check(activeAudioVoices() == 3 && audioVoices[0].position == coinPosition &&
              audioFileReads == reads,
          "Overlapping sounds preserve existing voices and perform no file IO");
    for (int i = 0; i < 64; i++)
        mixAudio(chunk, 512);
    check(activeAudioVoices() == 0 && audioNonzeroSamples > 0,
          "Effects mix nonzero PCM and each voice reaches its own end");
    sound("bean-win");
    for (int i = 0; i < 24; i++) {
        sound("jump");
        mixAudio(chunk, 512);
    }
    check(audioVoices[0].clip && audioVoices[0].position == 24 * 512,
          "Long effect remains playing through repeated short effects");
    stopSounds();
    musicEnabled = true;
    playMusic(3080012186760326ULL);
    sound("coin");
    sound("jump");
    const int frames = 22050;
    u8 *wav = (u8 *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 44 + frames * 4);
    *(u32 *)wav = 0x46464952;
    *(u32 *)(wav + 4) = 36 + frames * 4;
    *(u32 *)(wav + 8) = 0x45564157;
    *(u32 *)(wav + 12) = 0x20746d66;
    *(u32 *)(wav + 16) = 16;
    *(WORD *)(wav + 20) = 1;
    *(WORD *)(wav + 22) = 2;
    *(u32 *)(wav + 24) = 22050;
    *(u32 *)(wav + 28) = 88200;
    *(WORD *)(wav + 32) = 4;
    *(WORD *)(wav + 34) = 16;
    *(u32 *)(wav + 36) = 0x61746164;
    *(u32 *)(wav + 40) = frames * 4;
    mixAudio((short *)(wav + 44), frames);
    atomicWrite("test-output/audio-mix.wav", wav, 44 + frames * 4);
    HeapFree(GetProcessHeap(), 0, wav);
    check(playingMusic == 3080012186760326ULL && musicPosition == (u32)frames,
          "Music and effects share one mixer without restarting music");
    u32 beforeVictoryReads = audioFileReads;
    playMusic(16755430903372978986ULL);
    check(playingMusic == 16755430903372978986ULL && audioFileReads == beforeVictoryReads,
          "Victory music switches to its preloaded buffer");
    musicEnabled = soundEnabled = false;
    stopSounds();
    stopMusic();

    startStage(0);
    int coin = -1, axolotl = -1, enemy = -1;
    for (int i = 0; i < entityCount; i++) {
        if (coin < 0 && entities[i].kind == COIN)
            coin = i;
        if (entities[i].kind == SALAMANDER)
            axolotl = i;
        if (enemy < 0 && entities[i].kind == ENEMY && entities[i].walk)
            enemy = i;
    }
    if (coin >= 0) {
        State &s = entities[playerIndex].state;
        s.x = entities[coin].state.x;
        s.y = entities[coin].state.y;
        s.vx = s.vy = 0;
        tick(dt, false, false);
        check(coins > 0 && liveEffects() > 0,
              "Diamond contact emits the original bling and sparkles");
        tickEffects(0.1f);
        updateCamera(true);
        captureCase("test-output/coin-particles.bmp");
    }
    startStage(0);
    axolotl = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == SALAMANDER)
            axolotl = i;
    if (axolotl >= 0) {
        State &s = entities[playerIndex].state;
        s.x = entities[axolotl].state.x;
        s.y = entities[axolotl].state.y;
        s.vx = s.vy = 0;
        tick(dt, false, false);
        bool whee = false, woohoo = false;
        for (int i = 0; i < MAX_EFFECT_PARTICLES; i++)
            if (effectParticles[i].definition) {
                const char *a = effectParticles[i].definition->get("animation");
                whee |= eq(a, "whee");
                woohoo |= eq(a, "woohoo");
            }
        check(salamander && whee && woohoo,
              "Axolotl contact starts its whee spin and woohoo animation");
        tickEffects(0.2f);
        updateCamera(true);
        captureCase("test-output/axolotl-animation.bmp");
    }
    startStage(0);
    u32 cachedActors = actorCount;
    die();
    b2World *retainedWorld = physicsWorld;
    b2Body *retainedBody = bodies[playerIndex];
    tick(0.2f, false, false);
    captureCase("test-output/death-early.bmp");
    tick(0.4f, false, false);
    captureCase("test-output/death-late.bmp");
    check(deathAge > 0.5f && liveEffects() > 0 && (u32)actorCount == cachedActors,
          "Death particles continue animating using preloaded assets");
    for (int i = 0; i < 270; i++)
        tick(dt, false, false);
    check(deathTimer <= 0 && physicsWorld == retainedWorld && bodies[playerIndex] == retainedBody,
          "Respawn reuses existing bodies without rebuilding the physics world");

    startStage(0);
    for (int i = 0; i < 30; i++)
        tick(dt, true, false);
    for (int i = 0; i < 60; i++)
        tick(dt, false, false);
    check(entities[playerIndex].state.aux < 0,
          "Bean retains its left-facing direction after stopping");
    for (int i = 0; i < 30; i++)
        tick(dt, false, true);
    check(entities[playerIndex].state.aux > 0, "Moving right restores the right-facing direction");
    startStage(0);
    enemy = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == ENEMY && entities[i].walk) {
            enemy = i;
            break;
        }
    if (enemy >= 0) {
        for (int i = 0; i < 600; i++)
            tick(dt, false, false);
        check(
            entities[enemy].state.active && !entities[enemy].source->paused &&
                !entities[enemy].state.gone && absf(entities[enemy].state.vy) < 20,
            "An authored unpaused distant enemy stays active and grounded instead of being forced asleep");
        Entity &e = entities[enemy];
        entities[playerIndex].state.x = e.state.x - 80;
        entities[playerIndex].state.y = e.state.y + 40;
        entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
        tick(dt, false, false);
        check(e.state.active && !e.state.gone && absf(e.state.vy) < 20,
              "Enemy activates inside its authored proximity range");
        updateCamera(true);
        captureCase("test-output/enemy-visible.bmp");
    }

    // Isolate an authored cloud fixture to exercise both contact directions.
    startStage(0);
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex)
            entities[i].state.gone = 1;
    current.geometryCount = 0;
    int cloud = entityCount;
    addEntity(*classById(6181314796472045350ULL), Rect{480, 190, 544, 222});
    entities[cloud].move = 0;
    entities[playerIndex].state.x = 512;
    entities[playerIndex].state.y = 150;
    entities[playerIndex].state.vx = 0;
    rebuildPhysics();
    for (int i = 0; i < 45; i++) {
        entities[playerIndex].state.vy = 300;
        simulateBodies(dt);
    }
    check(entities[playerIndex].state.y > 240 && entities[playerIndex].state.vy > 250,
          "Cloud platform permits jumping through from below");
    entities[playerIndex].state.y = 282;
    entities[playerIndex].state.vy = -140;
    bool landed = false;
    for (int i = 0; i < 80 && !landed; i++) {
        simulateBodies(dt);
        landed = onGround && supportEntity == cloud;
    }
    check(landed, "The same cloud platform supports a descending Bean from above");
    int fragile = entityCount;
    addEntity(*classById(14815823507569313204ULL), Rect{550, 190, 614, 222});
    entities[fragile].state.active = 1;
    entities[fragile].state.timer = 0.66f;
    rebuildPhysics();
    float cloudY = entities[fragile].state.y;
    tick(0.02f, false, false);
    check(entities[fragile].state.active == 2 && !bodies[fragile]->IsActive() &&
              entities[fragile].state.y == cloudY && liveEffects() > 0,
          "Fragile cloud bursts and disables collision without falling");

    startStage(7);
    bool blockStates = true;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == ALTERNATOR && entities[i].source->has("AlternatorBlockBehaviour"))
            blockStates &= activeBlock(entities[i]) == (entities[i].alternation != 0);
    check(blockStates, "Alternator initial artwork state matches enabled collision");
    int sw = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == SWITCH) {
            sw = i;
            break;
        }
    if (sw >= 0) {
        Entity &e = entities[sw];
        for (int n = 0; n < 2; n++) {
            entities[playerIndex].state.x = e.state.x;
            entities[playerIndex].state.y = e.state.y;
            entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
            e.state.timer = 1;
            tick(dt, false, false);
        }
        check((alternationBits & e.mask) == 0,
              "Two separate switch activations restore the original block state");
    }

    interfaceEnabled = true;
    defaultBindings();
    navigate(12192614360109026946ULL);
    int before = page.focus;
    keyPressed(VK_UP);
    check(screen == TITLE && page.focus != before,
          "Up selects another title button instead of confirming it");
    navigate(worldScenes[0]);
    page.minimized = true;
    int mini = -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].kind == COIN) {
            mini = i;
            break;
        }
    u32 progressBefore = hashBytes(&progress, sizeof(progress));
    if (mini >= 0) {
        Entity &item = page.playfield[mini];
        page.bean.state.x = item.state.x;
        page.bean.state.y = item.state.y;
        page.bean.state.vx = page.bean.state.vy = 0;
        tickInterface(dt);
        check(page.miniCoins > 0 && item.state.gone && liveEffects(1) > 0,
              "Selector Bean collects individual background gems with particles");
        check(hashBytes(&progress, sizeof(progress)) == progressBefore,
              "Selector collectibles do not alter campaign records");
        captureCase("test-output/select-playable.bmp");
    }
    page.minimized = false;
    page.cameraX = page.homeX;
    page.cameraY = page.homeY;
    page.age = 3;
    captureCase("test-output/select-focus-fixed.bmp");
    startStage(0);
    readyTimer = 0;
    entities[playerIndex].state.x = 700;
    coins = 3;
    captureCheckpoint(true);
    navigate(levels[stageList[0]].id);
    check(entities[playerIndex].state.x == 256 && coins == 0 &&
              checkpoint.state[playerIndex].x == 256,
          "Entering from stage selection starts fresh instead of resuming a checkpoint");
    readyTimer = 1;
    keyPressed(VK_UP);
    check(readyTimer > 0, "Up preserves the source Ready timeline inside a gameplay stage");

    interfaceEnabled = false;
    startStage(0);
    int goal = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == EXIT)
            goal = i;
    if (goal >= 0) {
        Entity &e = entities[goal];
        State &p = entities[playerIndex].state;
        p.x = e.state.x;
        p.y = hitRect(e).t - entities[playerIndex].hit.b + 2;
        p.vx = 0;
        p.vy = -60;
        for (int i = 0; i < 30 && !complete; i++)
            tick(dt, false, false);
        check(
            bodies[goal] && complete && screen == COMPLETE && goalLaunch.active &&
                goalLaunch.jumpVelocity > 200,
            "Goal has a collision fixture and launches Bean while the original result banner begins");
        updateCamera(true);
        tick(0.1f, false, false);
        captureCase("test-output/goal-launch.bmp");
        for (int i = 0; i < 150; i++)
            tick(dt, false, false);
        check(screen == COMPLETE,
              "Original result banner remains active during the goal launch animation");
        musicEnabled = soundEnabled = true;
        playMusic(16755430903372978986ULL);
        sound("bean-win");
        restartStage();
        check(
            playingMusic == idnum(current.props.get("music")) && activeAudioVoices() == 1,
            "Retry replaces victory BGM while the independent victory effect finishes without truncation");
        musicEnabled = soundEnabled = false;
        stopMusic();
    }

    interfaceEnabled = true;
    progress.unlocked = 0;
    writeProgress();
    char debugPath[MAX_PATH];
    savePath(debugPath, "progress.dat");
    Progress *beforeDebug = (Progress *)HeapAlloc(GetProcessHeap(), 0, sizeof(Progress));
    Progress *afterDebug = (Progress *)HeapAlloc(GetProcessHeap(), 0, sizeof(Progress));
    bool beforeRead = exactRead(debugPath, beforeDebug, sizeof(Progress));
    savePath(debugPath, "checkpoint.dat");
    Snapshot *checkpointBeforeDebug = (Snapshot *)HeapAlloc(GetProcessHeap(), 0, sizeof(Snapshot));
    bool checkpointRead = exactRead(debugPath, checkpointBeforeDebug, sizeof(Snapshot));
    u32 checkpointHash = hashBytes(checkpointBeforeDebug, sizeof(Snapshot));
    navigate(12192614360109026946ULL);
    const char *code = "SKELETONKEY";
    for (int i = 0; code[i]; i++)
        keyPressed(code[i]);
    check(!debugMode && progress.unlocked == stageCount - 1 && worldOpen(6),
          "Typing SKELETONKEY persists every world unlock without entering DEBUG mode");
    navigate(worldScenes[4]);
    bool unlocked = true;
    for (int i = 0; i < page.level.objectCount; i++)
        if (page.nodes[i].stage >= 0)
            unlocked &= !page.nodes[i].disabled;
    check(unlocked, "Persistent SKELETONKEY unlock enables all individual stage buttons");
    interfaceEnabled = false;
    startStage(0);
    for (int i = 0; i < 360; i++)
        tick(dt, false, i < 90);
    check(jumps > 0, "Persistent unlock keeps normal jump counting through repeated bounces");
    captureCheckpoint(true);
    finish();
    writeProgress();
    savePath(debugPath, "progress.dat");
    bool afterRead = exactRead(debugPath, afterDebug, sizeof(Progress));
    savePath(debugPath, "checkpoint.dat");
    bool checkpointUnchanged = exactRead(debugPath, checkpointBeforeDebug, sizeof(Snapshot)) &&
                               checkpointHash == hashBytes(checkpointBeforeDebug, sizeof(Snapshot));
    check(beforeRead && afterRead && checkpointRead && !checkpointUnchanged &&
              afterDebug->unlocked == stageCount - 1 && afterDebug->cleared[0],
          "Persistent unlock retains normal checkpoint, completion and progress saving");
    HeapFree(GetProcessHeap(), 0, beforeDebug);
    HeapFree(GetProcessHeap(), 0, afterDebug);
    HeapFree(GetProcessHeap(), 0, checkpointBeforeDebug);
    debugMode = false;
    startStage(4);
    bool commentHidden = true;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->id == 15558680146931544709ULL)
            commentHidden = false;
    check(commentHidden, "Stage 1-05 hides the developer comment's black backing");
    entities[playerIndex].state.x = 544;
    entities[playerIndex].state.y = 540;
    updateCamera(true);
    captureCase("test-output/stage-1-05-background.bmp");
    startStage(5);
    entities[playerIndex].state.x = 256;
    entities[playerIndex].state.y = 1410;
    updateCamera(true);
    captureCase("test-output/stage-1-06-background.bmp");
    startStage(2);
    int spring = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == SPRING) {
            spring = i;
            break;
        }
    if (spring >= 0) {
        Entity &e = entities[spring];
        State &p = entities[playerIndex].state;
        p.x = e.state.x;
        p.y = hitRect(e).t - entities[playerIndex].hit.b + 2;
        p.vx = 0;
        p.vy = -60;
        for (int i = 0; i < 30 && e.state.active != 2; i++)
            tick(dt, false, false);
        bool contactAnimated = e.state.active == 2;
        for (int i = 0; i < 30 && p.vy <= entities[playerIndex].jumpPower; i++)
            tick(dt, false, false);
        check(
            contactAnimated && e.state.active == 2 && pinballLaunchedThisStepNative &&
                pinballConsumedNative.source == spring && p.vy > entities[playerIndex].jumpPower,
            "Spring contact animates immediately and its queued stronger launch follows on an eligible later physics update");
        tick(0.08f, false, false);
        updateCamera(true);
        captureCase("test-output/spring-animation.bmp");
    }
    // An actual walking enemy against a controlled wall must turn once and leave.
    startStage(6);
    enemy = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == ENEMY && entities[i].walk) {
            enemy = i;
            break;
        }
    if (enemy >= 0) {
        for (int i = 0; i < entityCount; i++)
            if (i != playerIndex && i != enemy)
                entities[i].state.gone = 1;
        current.geometryCount = 0;
        Entity &e = entities[enemy];
        e.state.x = 500;
        e.state.y = 100;
        e.state.active = 1;
        e.state.aux = e.state.vx = 75;
        e.state.vy = 0;
        e.reverseEdge = 0;
        entities[playerIndex].state.x = 360;
        entities[playerIndex].state.y = 300;
        entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
        rebuildPhysics();
        b2BodyDef bd;
        bd.position.Set(500 * metres, 60 * metres);
        b2Body *floor = physicsWorld->CreateBody(&bd);
        b2PolygonShape shape;
        shape.SetAsBox(200 * metres, 10 * metres);
        floor->CreateFixture(&shape, 0);
        bd.position.Set(540 * metres, 130 * metres);
        b2Body *wall = physicsWorld->CreateBody(&bd);
        shape.SetAsBox(10 * metres, 80 * metres);
        wall->CreateFixture(&shape, 0);
        float peak = e.state.x;
        for (int i = 0; i < 90; i++) {
            tick(dt, false, false);
            peak = maxf(peak, e.state.x);
        }
        check(e.state.aux < 0 && e.state.x < peak - 8,
              "Walking enemy turns away from a wall without alternating direction in place");
        Object *spike = levelArena.array<Object>(1);
        *spike = *classById(17084625859160385508ULL);
        spike->rect = shifted(Rect{-8, -8, 8, 8}, e.state.x, e.state.y);
        addEntity(*spike, spike->rect);
        rebuildPhysics();
        tick(dt, false, false);
        bool deferred = killableDeathVisibleNative(e);
        for (int n = 0; n < 14; n++)
            tick(1 / 120.0f, false, false);
        check(
            deferred && e.state.gone == 1 && liveEffects() > 0,
            "A killable enemy entering the actual SpikeGroup sensor squashes before its delayed explosion effect");
    }
    interfaceEnabled = true;
    progress.unlocked = 0;
    navigate(18006219643615701355ULL);
    int locked = -1;
    for (int i = 0; i < page.level.objectCount; i++)
        if (page.nodes[i].world == 4)
            locked = i;
    if (locked >= 0) {
        page.focus = locked;
        page.cameraX = page.targetX = center(page.nodes[locked].rect).x - 256;
        confirm();
        check(screen == WORLD && page.focus == locked && !worldOpen(4) &&
                  page.nodes[locked].disabled,
              "Locked world stays locked and focused when confirmed");
        pointerHeld = true;
        pointerDragged = false;
        pointerNode = locked;
        pointerScene = page.level.id;
        captureCase("test-output/locked-world-pressed.bmp");
        check(page.nodes[locked].disabled,
              "Pressing a locked world cannot replace its lock with an unlocked button frame");
        pointerHeld = false;
        captureCase("test-output/locked-world-focus.bmp");
    }
    navigate(worldScenes[0]);
    page.minimized = true;
    float miniX = page.bean.state.x;
    keys[VK_RIGHT] = true;
    for (int i = 0; i < 30; i++)
        tickInterface(dt);
    keys[VK_RIGHT] = false;
    check(page.bean.state.x > miniX + 3,
          "Selector Bean responds to held right movement while the menu is minimized");
    navigate(9521867969610021ULL);
    focusId(page, 1266841032746010ULL);
    confirm();
    keyPressed('W');
    navigate(12192614360109026946ULL);
    int initialFocus = page.focus;
    keyPressed('W');
    check(screen == TITLE && page.focus != initialFocus,
          "Remapped Up key performs menu selection instead of being ignored");
    defaultBindings();
    writeBindings();
    interfaceEnabled = false;
    startStage(3);
    float cover = 1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == SECRET)
            cover = minf(cover, 1 - entities[i].state.aux);
    check(cover == 1, "Secret foreground cover begins fully opaque before entering its trigger");
    startStage(6);
    int moving = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].move) {
            moving = i;
            break;
        }
    if (moving >= 0) {
        Entity &e = entities[moving];
        float origin = e.state.x;
        for (int i = 0; i < 120; i++)
            tick(dt, false, false);
        check(!e.state.gone && absf(e.state.x - origin) > 5,
              "Stage 1-07 moving object remains present and follows its authored path");
    }
    startStage(0);
    int cloudBackground = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->has("BackgroundBehaviour")) {
            cloudBackground = i;
            break;
        }
    if (cloudBackground >= 0) {
        Entity &e = entities[cloudBackground];
        bool continuous = true, wrapped = false;
        Rect previous = backgroundRect(*e.source, entityRect(e), 0, 0, 0);
        float speed = absf(e.source->behavior("BackgroundBehaviour")->num("speed", 30));
        for (int i = 1; i < 1800; i++) {
            Rect next = backgroundRect(*e.source, entityRect(e), i * 0.05f, 0, 0);
            bool beforeVisible = previous.r > 0 && previous.l < 512,
                 afterVisible = next.r > 0 && next.l < 512;
            if (absf(next.l - previous.l) > speed * 0.05f + 0.1f) {
                wrapped = true;
                if (beforeVisible || afterVisible)
                    continuous = false;
            }
            previous = next;
        }
        check(wrapped && continuous, "Cloud looping wraps only after leaving the visible viewport");
    }
    startStage(42);
    int turret = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == TURRET) {
            turret = i;
            break;
        }
    if (turret >= 0) {
        shoot(entities[turret], *entities[turret].source->behavior("BulletSource"));
        simulateBodies(dt);
        b2Body *reused = shotBodies[0];
        bullets[0].life = 0;
        entities[turret].state.x += 500;
        shoot(entities[turret], *entities[turret].source->behavior("BulletSource"));
        float x = bullets[0].state.x, y = bullets[0].state.y;
        simulateBodies(dt);
        check(shotBodies[0] == reused &&
                  absf(bullets[0].state.x - x) + absf(bullets[0].state.y - y) < 10,
              "Reused projectile slot begins at its new muzzle position");
    }
}

bool finaleRouteDeviceSmoke();

int audioDeviceSmoke() {
    CreateDirectoryA("test-output", 0);
    initAudio(soundFiles, true);
    musicEnabled = soundEnabled = true;
    playMusic(3080012186760326ULL);
    sound("coin");
    for (int i = 0; i < 16; i++) {
        if (i == 4)
            sound("jump");
        if (i == 8)
            sound("axolotl");
        Sleep(50);
    }
    MMTIME position = {};
    position.wType = TIME_SAMPLES;
    MMRESULT query = musicDevice ? waveOutGetPosition(musicDevice, &position, sizeof(position))
                                 : MMSYSERR_BADDEVICEID;
    char report[512];
    wsprintfA(
        report,
        "Devices: %u\r\nCached effects: %d\r\nwaveOut error: %u\r\nPosition query: %u\r\nPosition type: %u\r\nPosition: %u\r\nMixed frames: %u\r\nNonzero frames: %u\r\nEffect voices started: %u\r\n",
        waveOutGetNumDevs(), soundClipCount, audioError, query, position.wType, position.u.sample,
        audioSamplesMixed, audioNonzeroSamples, audioVoicesStarted);
    atomicWrite("test-output/audio-device.txt", report, lstrlenA(report));
    bool ok = musicDevice && audioError == 0 && query == 0 && position.u.sample > 0 &&
              audioNonzeroSamples > 0 && audioVoicesStarted == 3;
    bool route = finaleRouteDeviceSmoke();
    const char *routeStatus =
        route
            ? "PASS real post-Wizard normal/DEBUG staircase routes with waveOut worker\r\n"
            : "FAIL real post-Wizard normal/DEBUG staircase routes with waveOut worker; inspect route CSVs\r\n";
    atomicWrite("test-output/audio-device-route-status.txt", routeStatus, lstrlenA(routeStatus));
    shutdownAudio();
    return ok && route ? 0 : 1;
}
