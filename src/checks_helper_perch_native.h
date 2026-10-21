#pragma once

// Regression coverage for helper perch native. Runs only in the isolated BQ_ENABLE_TESTS executable.
int helperPerchPrepareNative(int &bird) {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    startStage(stageById(669939580618ULL));
    readyTimer = 0;
    bird = -1;
    int perch = -1;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].kind == HELPER)
            bird = i;
        if (entities[i].kind == PERCH)
            perch = i;
    }
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != bird && i != perch && i != playerIndex)
            entities[i].state.gone = 1;
    State &bean = entities[playerIndex].state;
    bean.x = current.size.x * .5f;
    bean.y = 1000;
    bean.vx = bean.vy = 0;
    rebuildPhysics();
    // Isolate the perch's real sensor callback from collecting the bird
    // sitting directly above it, which owns a different sound and state change.
    if (bird >= 0)
        for (b2Fixture *f = bodies[bird]->GetFixtureList(); f; f = f->GetNext()) {
            b2Filter filter = f->GetFilterData();
            filter.maskBits = 0;
            f->SetFilterData(filter);
        }
    return perch;
}

void helperPerchPlacePlayerNative(int perch) {
    State &bean = entities[playerIndex].state;
    bean.x = entities[perch].state.x;
    bean.y = entities[perch].state.y;
    bean.vx = bean.vy = 0;
    restorePhysics();
}

void helperPerchNativeChecks() {
    const float dt = 1.0f / 120;
    int bird = -1, perch = helperPerchPrepareNative(bird);
    if (perch < 0 || bird < 0) {
        check(false, "Original helper/perch stage resolves for native contact lifecycle checks");
        return;
    }
    Entity &target = entities[perch];
    const Props &p = *target.source->behavior("HelperBirdPerchBehaviour");
    float x = target.state.x + p.num("offset-x"), y = target.state.y + p.num("offset-y", 23);
    entities[bird].originX = x - 100;
    entities[bird].originY = y;
    helperPerchPlacePlayerNative(perch);
    soundEnabled = true;
    u32 voices = audioVoicesStarted;
    simulateBodies(dt);
    soundEnabled = false;
    check(
        target.state.active == 2 && target.state.timer == 0 && entities[bird].originX == x &&
            entities[bird].originY == y && audioVoicesStarted == voices + 1,
        "A real authored perch BeginContact retargets its helper and rings immediately before helperPostStep is called");
    helperPostStep(dt);
    stopSounds();
    // A real Begin followed by a real End before polling must not erase the
    // native event. No fake contact or direct callback invocation is used.
    perch = helperPerchPrepareNative(bird);
    Entity &transient = entities[perch];
    const Props &tp = *transient.source->behavior("HelperBirdPerchBehaviour");
    x = transient.state.x + tp.num("offset-x");
    y = transient.state.y + tp.num("offset-y", 23);
    entities[bird].originX = x - 100;
    entities[bird].originY = y;
    helperPerchPlacePlayerNative(perch);
    simulateBodies(dt);
    State &bean = entities[playerIndex].state;
    bean.x = current.size.x * .5f;
    bean.y = 1000;
    restorePhysics();
    simulateBodies(dt);
    helperPostStep(dt);
    check(
        transient.state.active == 2 && entities[bird].originX == x && entities[bird].originY == y,
        "An actual perch contact that ends before post-step polling still retains its already accepted roost change");
    bool inclusive = true, outside = true, modes = true;
    int voicesInside = 0, voicesOutside = 0;
    for (int mode = 0; mode < 3; mode++)
        for (int axis = 0; axis < 2; axis++)
            for (int distance = 100; distance <= 101; distance++) {
                perch = helperPerchPrepareNative(bird);
                Entity &pad = entities[perch];
                const Props &props = *pad.source->behavior("HelperBirdPerchBehaviour");
                x = pad.state.x + props.num("offset-x");
                y = pad.state.y + props.num("offset-y", 23);
                entities[bird].originX = x;
                entities[bird].originY = y;
                entities[bird].state.active = mode;
                entities[bird].state.timer = .35f;
                float &component = axis ? entities[bird].originY : entities[bird].originX;
                u32 bits;
                memcpy(&bits, &component, 4);
                bits += distance;
                memcpy(&component, &bits, 4);
                float oldX = entities[bird].originX, oldY = entities[bird].originY;
                helperPerchPlacePlayerNative(perch);
                soundEnabled = true;
                u32 before = audioVoicesStarted;
                simulateBodies(dt);
                helperPostStep(0);
                soundEnabled = false;
                int added = audioVoicesStarted - before;
                if (distance == 100) {
                    inclusive &= entities[bird].originX == oldX && entities[bird].originY == oldY &&
                                 pad.state.active == 0 && added == 0;
                    voicesInside += added;
                } else {
                    outside &= entities[bird].originX == x && entities[bird].originY == y &&
                               pad.state.active == 2 && added == 1;
                    voicesOutside += added;
                    modes &= entities[bird].state.active == (mode == 0 ? 2 : mode);
                }
                stopSounds();
            }
    check(
        inclusive && voicesInside == 0,
        "Native perch roost equality includes100float representations on both axes without false retargeting or repeated ringing");
    check(
        outside && voicesOutside == 6 && modes,
        "A101ULP roost difference rings once and retargets all three helper modes while only a sleeping helper starts returning");
    perch = helperPerchPrepareNative(bird);
    Entity &pad = entities[perch];
    const Props &props = *pad.source->behavior("HelperBirdPerchBehaviour");
    x = pad.state.x + props.num("offset-x");
    y = pad.state.y + props.num("offset-y", 23);
    entities[bird].originX = x - 100;
    entities[bird].originY = y;
    helperPerchPlacePlayerNative(perch);
    b2Fixture *main = bodies[playerIndex]->GetFixtureList();
    while (main && main->IsSensor())
        main = main->GetNext();
    b2FixtureDef duplicate;
    duplicate.shape = main->GetShape();
    duplicate.density = 0;
    duplicate.filter = main->GetFilterData();
    bodies[playerIndex]->CreateFixture(&duplicate);
    soundEnabled = true;
    voices = audioVoicesStarted;
    simulateBodies(dt);
    helperPostStep(dt);
    u32 after = audioVoicesStarted;
    for (int f = 0; f < 4; f++) {
        simulateBodies(dt);
        helperPostStep(dt);
    }
    soundEnabled = false;
    check(
        after == voices + 1 && audioVoicesStarted == after,
        "Two real Bean fixtures and continued perch overlap emit only one ring because the first event already updates the roost");
    stopSounds();
    perch = helperPerchPrepareNative(bird);
    Entity &bell = entities[perch];
    bell.state.active = 2;
    float length = animationLength(animation(bell.art, "ringing"));
    bell.state.timer = length - dt * .5f;
    tick(dt, false, false);
    bool lastFrame = bell.state.active == 2 && bell.state.timer >= length;
    tick(dt, false, false);
    check(
        lastFrame && bell.state.active == 0,
        "Perch ringing retains its final STOP frame at the length boundary and restarts idle on the next native OnUpdate");
    perch = helperPerchPrepareNative(bird);
    Entity &savedPad = entities[perch];
    const Props &savedProps = *savedPad.source->behavior("HelperBirdPerchBehaviour");
    x = savedPad.state.x + savedProps.num("offset-x");
    y = savedPad.state.y + savedProps.num("offset-y", 23);
    entities[bird].originX = x - 100;
    entities[bird].originY = y;
    helperPerchPlacePlayerNative(perch);
    simulateBodies(dt);
    helperPostStep(dt);
    captureCheckpoint(false);
    entities[playerIndex].state.y = 1000;
    restorePhysics();
    simulateBodies(dt);
    restoreCheckpoint();
    soundEnabled = true;
    voices = audioVoicesStarted;
    simulateBodies(dt);
    helperPostStep(dt);
    soundEnabled = false;
    check(
        entities[bird].originX == x && entities[bird].originY == y && savedPad.state.active == 2 &&
            audioVoicesStarted == voices,
        "Restoring a ringing perch checkpoint into an existing Bean overlap keeps the accepted roost without replaying its sound");
    stopSounds();
    interfaceEnabled = true;
    u32 campaign = hashBytes(entities, sizeof(Entity) * entityCount),
        saved = hashBytes(&checkpoint, sizeof(checkpoint));
    navigate(137502285691ULL);
    int endingPerch = -1, endingBird = -1;
    for (int i = 0; i < page.playfieldCount; i++) {
        if (page.playfield[i].kind == PERCH)
            endingPerch = i;
        else if (page.playfield[i].kind == HELPER)
            endingBird = i;
        else
            page.playfield[i].state.gone = 1;
    }
    bool ending = endingPerch >= 0 && endingBird >= 0;
    if (ending) {
        Entity &ep = page.playfield[endingPerch];
        Entity &eb = page.playfield[endingBird];
        const Props &pp = *ep.source->behavior("HelperBirdPerchBehaviour");
        x = ep.state.x + pp.num("offset-x");
        y = ep.state.y + pp.num("offset-y", 23);
        eb.originX = x - 100;
        eb.originY = y;
        for (b2Fixture *f = page.bodies[endingBird]->GetFixtureList(); f; f = f->GetNext()) {
            b2Filter filter = f->GetFilterData();
            filter.maskBits = 0;
            f->SetFilterData(filter);
        }
        page.bean.state.x = ep.state.x;
        page.bean.state.y = ep.state.y;
        page.bean.state.vx = page.bean.state.vy = 0;
        resetUiPhysics(page);
        stepUiPhysics(page, dt);
        ending &= ep.state.active == 2 && eb.originX == x && eb.originY == y;
    }
    check(
        ending && campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
            saved == hashBytes(&checkpoint, sizeof(checkpoint)),
        "The original ending perch runs the same real BeginContact retargeting in its independent world without touching campaign state");
    interfaceEnabled = false;
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    perch = -1;
    int helpers = 0;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].kind == PERCH)
            perch = i;
        else if (i != playerIndex)
            entities[i].state.gone = 1;
        if (entities[i].kind == HELPER)
            helpers++;
    }
    current.geometryCount = 0;
    if (perch >= 0) {
        State &bean2 = entities[playerIndex].state;
        bean2.x = entities[perch].state.x;
        bean2.y = entities[perch].state.y;
        bean2.vx = bean2.vy = 0;
        rebuildPhysics();
        soundEnabled = true;
        voices = audioVoicesStarted;
        simulateBodies(dt);
        helperPostStep(dt);
        soundEnabled = false;
    }
    check(
        perch >= 0 && helpers == 0 && entities[perch].state.active == 0 &&
            audioVoicesStarted == voices,
        "The actual orphan Wizard perch remains silent and idle when there is no bound helper to retarget");
    stopSounds();
    Arena memory;
    memory.init(32 * 1024 * 1024);
    Level scan = {};
    int perches = 0, helperCount = 0, scenes = 0;
    bool single = true;
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scan, memory);
        int localHelpers = 0, localPerches = 0;
        for (int i = 0; i < scan.objectCount; i++) {
            localHelpers += scan.objects[i].has("HelperBirdBehaviour");
            localPerches += scan.objects[i].has("HelperBirdPerchBehaviour");
        }
        helperCount += localHelpers;
        perches += localPerches;
        if (localHelpers || localPerches)
            scenes++;
        single &= localHelpers <= 1;
    }
    check(
        perches == 6 && helperCount == 3 && scenes == 4 && single,
        "All74 original scenes contain exactly6perches and3helpers across4scenes with at most one bound helper per world");
    startStage(0);
    readyTimer = 0;
}
