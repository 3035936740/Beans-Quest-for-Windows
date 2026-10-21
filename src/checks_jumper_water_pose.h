#pragma once

// Regression coverage for jumper water pose. Runs only in the isolated BQ_ENABLE_TESTS executable.

int prepareJumperWaterPoseCheck(bool settled = false) {
    startStage(stageById(5770908237787719ULL));
    readyTimer = 0;
    int index = frogLinks[0].right;
    for (int i = 0; i < entityCount; i++)
        if ((entities[i].kind == ENEMY && i != index) || i == frogLinks[0].ball)
            entities[i].state.gone = 1;
    entities[frogLinks[0].ball].state.hp = 0;
    int water = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == WATER && entityRect(entities[i]).l < 1408 &&
            entityRect(entities[i]).r > 1408)
            water = i;
    Entity &e = entities[index];
    float surface = entityRect(entities[water]).t;
    e.state.x = 1408;
    e.state.y = settled
                    ? surface - e.hit.b -
                          (e.hit.t - e.hit.b) * e.physics->num("density") /
                              entities[water].source->behavior("WaterAreaBehaviour")->num("density")
                    : surface + 70;
    e.state.vx = 0;
    e.state.vy = settled ? 0 : -100;
    e.state.hp = 1;
    e.state.aux = 0;
    State &observer = entities[playerIndex].state;
    observer.x = 128;
    observer.y = 350;
    observer.vx = observer.vy = 0;
    restorePhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    for (b2Fixture *f = bodies[playerIndex]->GetFixtureList(); f; f = f->GetNext()) {
        b2Filter filter = f->GetFilterData();
        filter.maskBits = 0;
        f->SetFilterData(filter);
    }
    if (settled) {
        simulateBodies(1.0f / 120);
        waterPostStep();
        e.state.vx = e.state.vy = 0;
        bodies[index]->SetLinearVelocity(b2Vec2(0, 0));
    }
    return index;
}

u32 jumperWaterPosePixels(Entity &e, const char *reference = 0) {
    glViewport(viewportX, viewportY, viewportW, viewportH);
    nativeViewportProjection();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();
    glTranslatef(128 - e.state.x, 128 - e.state.y, 0);
    if (reference)
        drawActor(e.art, entityRect(e), reference,
                  jumperAnimationAgeNative(e, eq(reference, "jumping")), e.state.angle, false, -1,
                  secretLayerAlpha(e.layer));
    else
        drawEntity(e);
    glPopMatrix();
    glFinish();
    float scale = viewportW / viewW;
    int width = (int)(96 * scale), size = width * width * 4;
    u8 *data = (u8 *)HeapAlloc(GetProcessHeap(), 0, size);
    glReadBuffer(GL_BACK);
    glReadPixels(viewportX + (int)(80 * scale), viewportY + (int)(80 * scale), width, width,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    u32 result = hashBytes(data, size);
    HeapFree(GetProcessHeap(), 0, data);
    return result;
}

void jumperWaterPoseChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    debugMode = false;
    const float dt = 1.0f / 120;
    int index = prepareJumperWaterPoseCheck();
    Entity &frog = entities[index];
    bool early = false, floating = false, changed = false, standingReference = true;
    u32 initial = 0;
    unsigned frames = 0;
    int first = -1, standingSamples = 0, longStandingSamples = 0;
    for (int step = 0; step < 4800; step++) {
        tick(dt, false, false);
        bool standing = jumperWaterStanding(frog);
        if (step < 60)
            early |= standing;
        if (step >= 1200 && standing)
            longStandingSamples++;
        if (standing) {
            floating = true;
            standingSamples++;
            if (first < 0) {
                first = step;
                initial = jumperWaterPosePixels(frog);
                camX = 1152;
                camY = 50;
                captureCase("test-output/jumper-water-standing-frame-a.bmp");
            }
            if (step == first + 18) {
                camX = 1152;
                camY = 50;
                captureCase("test-output/jumper-water-standing-frame-b.bmp");
            }
            unsigned frame = animationFrame(animation(frog.art, "standing"),
                                            jumperAnimationAgeNative(frog, false));
            frames |= 1u << frame;
            if (step % 6 == 0) {
                u32 actual = jumperWaterPosePixels(frog);
                changed |= initial != actual;
                standingReference &= actual == jumperWaterPosePixels(frog, "standing");
            }
        }
    }
    check(
        !early && floating && first >= 60 && frog.state.hp == 1,
        "An actual launched frog falls into original water and gains only a visual standing pose after floating settles, without clearing its jumping state");
    char poseTrace[250];
    wsprintfA(
        poseTrace,
        "first=%d standing_samples=%d steady_last30s=%d frames_mask=%u pixels_changed=%d initial_hash=%u hp=%d\r\n",
        first, standingSamples, longStandingSamples, frames, changed, initial, frog.state.hp);
    atomicWrite("test-output/jumper-water-pose-trace.txt", poseTrace, lstrlenA(poseTrace));
    check(
        changed && frames == 15 && longStandingSamples == 3600 && standingReference,
        "Actual stage rendering matches all four standing frames and preserves floating standing for every frame of the final thirty seconds");
    camX = 1152;
    camY = 50;
    captureCase("test-output/jumper-water-standing-native.bmp");
    captureCheckpoint(true);
    restoreCheckpoint();
    bool cleared = !jumperWaterStanding(entities[index]);
    for (int i = 0; i < 1200 && !jumperWaterStanding(entities[index]); i++)
        tick(dt, false, false);
    check(
        cleared && jumperWaterStanding(entities[index]) && entities[index].state.hp == 1,
        "Loading a floating checkpoint resets visual eligibility and naturally reacquires standing without modifying the saved jumping state");

    index = prepareJumperWaterPoseCheck(true);
    Entity &e = entities[index];
    b2Body *body = bodies[index];
    const b2Body *water = 0;
    bool valid = jumperWaterPoseContact(index, &water);
    State before = e.state;
    u32 bodyHash = hashBytes(body, sizeof(b2Body)),
        stateHash = hashBytes(entities, sizeof(Entity) * entityCount),
        bulletsHash = hashBytes(bullets, sizeof(bullets));
    advanceJumperWaterPose(.25f);
    bool firstHalf = !jumperWaterStanding(e) && jumperWaterPose[index].stableAge == .25f;
    screen = PAUSE;
    advanceJumperWaterPose(10);
    bool paused = !jumperWaterStanding(e) && jumperWaterPose[index].stableAge == .25f;
    screen = PLAY;
    advanceJumperWaterPose(.25f);
    check(
        valid && firstHalf && paused && jumperWaterStanding(e),
        "Real partial-submersion contact needs half a second below the floating visual entry velocity and paused time cannot complete that wait");
    check(
        hashBytes(&before, sizeof(before)) == hashBytes(&e.state, sizeof(e.state)) &&
            stateHash == hashBytes(entities, sizeof(Entity) * entityCount) &&
            bodyHash == hashBytes(body, sizeof(b2Body)) &&
            bulletsHash == hashBytes(bullets, sizeof(bullets)),
        "Water-pose updates leave every gameplay entity field, physical body including force storage, and projectile untouched");
    body->SetLinearVelocity(b2Vec2(jumperWaterPoseEnterSpeed * 1.5f, 0));
    advanceJumperWaterPose(.5f);
    bool smallMotion = jumperWaterStanding(e);
    resetJumperWaterPose();
    advanceJumperWaterPose(1);
    bool cannotEnter = !jumperWaterStanding(e) && jumperWaterPose[index].stableAge == 0;
    body->SetLinearVelocity(b2Vec2(0, 0));
    advanceJumperWaterPose(.5f);
    check(
        smallMotion && cannotEnter && jumperWaterStanding(e),
        "Visual hysteresis preserves established standing during small floating motion but that same motion cannot acquire standing");
    body->SetLinearVelocity(b2Vec2(.2f, 0));
    bool pushed = !jumperWaterStanding(e);
    advanceJumperWaterPose(dt);
    pushed &= jumperWaterPose[index].stableAge == 0;
    body->SetLinearVelocity(b2Vec2(0, .2f));
    advanceJumperWaterPose(1);
    bool rising = !jumperWaterStanding(e) && jumperWaterPose[index].stableAge == 0;
    body->SetLinearVelocity(b2Vec2(0, -.2f));
    advanceJumperWaterPose(1);
    bool sinking = !jumperWaterStanding(e) && jumperWaterPose[index].stableAge == 0;
    check(
        pushed && rising && sinking,
        "Horizontal pushes and upward or downward motion immediately revoke the visual standing qualification");
    body->SetLinearVelocity(b2Vec2(0, 0));
    advanceJumperWaterPose(.5f);
    bool wasStanding = jumperWaterStanding(e);
    body->SetTransform(b2Vec2(1408 * metres, 260 * metres), 0);
    bool leftImmediately = !jumperWaterStanding(e);
    advanceJumperWaterPose(1);
    bool apex = !jumperWaterStanding(e);
    body->SetTransform(b2Vec2(1408 * metres, 80 * metres), 0);
    advanceJumperWaterPose(1);
    bool submerged = !jumperWaterStanding(e);
    check(
        wasStanding && leftImmediately && apex && submerged &&
            jumperWaterPosePixels(e) == jumperWaterPosePixels(e, "jumping"),
        "Leaving water immediately returns to the jumping image; zero velocity at an airborne apex or full submersion cannot regain standing");
    index = prepareJumperWaterPoseCheck(true);
    Entity &moving = entities[index];
    const b2Body *w = 0;
    if (!jumperWaterPoseContact(index, &w) || !w) {
        check(false,
              "The settled moving-water test fixture has a valid partial-submersion contact");
        startStage(0);
        readyTimer = 0;
        return;
    }
    b2Body *movingWater = (b2Body *)w;
    movingWater->SetType(b2_kinematicBody);
    movingWater->SetLinearVelocity(b2Vec2(.2f, 0));
    bodies[index]->SetLinearVelocity(b2Vec2(.2f, 0));
    advanceJumperWaterPose(.5f);
    bool relative = jumperWaterStanding(moving);
    movingWater->SetLinearVelocity(b2Vec2(0, 0));
    bool mismatch = !jumperWaterStanding(moving);
    check(relative && mismatch,
          "Floating qualification uses velocity relative to the actual touching water body");
    bodies[index]->SetLinearVelocity(b2Vec2(0, 0));
    advanceJumperWaterPose(.5f);
    moving.state.gone = 1;
    bool dead = !jumperWaterStanding(moving);
    advanceJumperWaterPose(dt);
    moving.state.gone = 0;
    advanceJumperWaterPose(.5f);
    resetJumperWaterPose();
    bool reset = !jumperWaterStanding(moving);
    advanceJumperWaterPose(.5f);
    rebuildPhysics();
    bool rebuilt = !jumperWaterStanding(entities[index]);
    check(
        dead && reset && rebuilt,
        "Death, explicit reset and a rebuilt physics world clear stale floating-pose eligibility");
    startStage(0);
    readyTimer = 0;
}
