#pragma once

// Regression coverage for water save native. Runs only in the isolated BQ_ENABLE_TESTS executable.

struct WaterSaveObservationNative {
    bool voice, particles, random, wet;
    float force;
};

int prepareWaterSaveNative() {
    interfaceEnabled = debugMode = musicEnabled = false;
    soundEnabled = true;
    startStage(stageById(31661610418ULL));
    readyTimer = 0;
    int water = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == WATER && water < 0)
            water = i;
    if (water < 0)
        return -1;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && i != water)
            entities[i].state.gone = 1;
    Rect r = entityRect(entities[water]);
    State &p = entities[playerIndex].state;
    p.x = (r.l + r.r) * .5f;
    p.y = r.t - entities[playerIndex].hit.b + 64;
    p.vx = p.vy = 0;
    rebuildPhysics();
    bodies[playerIndex]->SetGravityScale(0);
    physicsWorld->Step(1 / 120.0f, 10, 10);
    physicsWorld->ClearForces();
    waterPostStep();
    clearEffects(0);
    stopMusic();
    stopSounds();
    resetBurstRandomNative(0, 5489);
    return water;
}

void placeWaterSaveBeanNative(int water, bool inside, float speed) {
    Rect r = entityRect(entities[water]);
    Entity &bean = entities[playerIndex];
    bean.state.x = (r.l + r.r) * .5f;
    bean.state.y = r.t - bean.hit.b + (inside ? -2 : 64);
    bean.state.vx = 0;
    bean.state.vy = speed;
    bodies[playerIndex]->SetTransform(b2Vec2(bean.state.x * metres, bean.state.y * metres), 0);
    bodies[playerIndex]->SetLinearVelocity(b2Vec2(0, speed * metres));
    bodies[playerIndex]->SetAwake(true);
}

WaterSaveObservationNative observeWaterSaveNative() {
    clearEffects(0);
    stopSounds();
    u32 effectsBefore = emittedParticles;
    NativeParticleRng &random = burstRandomForNative(0).engine;
    u32 before = hashBytes(&random, sizeof(random));
    physicsWorld->ClearForces();
    physicsWorld->Step(1 / 120.0f, 10, 10);
    physicsWorld->ClearForces();
    waterPostStep();
    WaterSaveObservationNative r = {waterVoiceNative() > 0, emittedParticles > effectsBefore,
                                    hashBytes(&random, sizeof(random)) != before, playerInWater,
                                    bodies[playerIndex]->GetCheckpointForce().y};
    return r;
}

bool waterSaveQuietNative(WaterSaveObservationNative r) {
    return !r.voice && !r.particles && !r.random && r.wet && r.force > 0;
}

bool waterSaveAudibleNative(WaterSaveObservationNative r) {
    return r.voice && r.particles && r.random && r.wet && r.force > 0;
}

void waterSaveNativeChecks() {
    int water = prepareWaterSaveNative();
    if (water < 0) {
        check(false, "Original water intro is available for save/splash lifecycle probes");
        return;
    }
    placeWaterSaveBeanNative(water, true, -200);
    check(
        waterSaveAudibleNative(observeWaterSaveNative()),
        "An ordinary real water BeginContact at200px/s produces native sound, particles and RNG draws while buoyancy remains active");
    water = prepareWaterSaveNative();
    placeWaterSaveBeanNative(water, true, -200);
    captureCheckpoint(false);
    bool memory = waterSaveQuietNative(observeWaterSaveNative());
    placeWaterSaveBeanNative(water, false, 0);
    observeWaterSaveNative();
    placeWaterSaveBeanNative(water, true, -200);
    memory &= waterSaveAudibleNative(observeWaterSaveNative());
    check(
        memory,
        "Actual in-memory capture suppresses only the next physics water splash, preserving flotation and allowing the following unsaved re-entry sound");
    water = prepareWaterSaveNative();
    placeWaterSaveBeanNative(water, true, -200);
    captureCheckpoint(true);
    bool disk = waterSaveQuietNative(observeWaterSaveNative());
    char path[MAX_PATH];
    savePath(path, "checkpoint.dat");
    bool read = readCheckpoint(path);
    placeWaterSaveBeanNative(water, false, 0);
    observeWaterSaveNative();
    if (read)
        restoreCheckpoint();
    disk &= read && checkpoint.version == 28 && sizeof(Snapshot) == 543392 &&
            waterSaveQuietNative(observeWaterSaveNative());
    check(
        disk,
        "Real version28 disk capture and subsequent restore suppress their next water sound/effect/RNG event without changing the543392byte layout or buoyancy");
    water = prepareWaterSaveNative();
    checkpointSavePendingNative = true;
    placeWaterSaveBeanNative(water, true, -200);
    bool deferred = waterSaveAudibleNative(observeWaterSaveNative()) && checkpointSavePendingNative;
    placeWaterSaveBeanNative(water, false, 0);
    observeWaterSaveNative();
    placeWaterSaveBeanNative(water, true, -200);
    checkpointFlushSaveNative();
    deferred &= !checkpointSavePendingNative && waterSaveQuietNative(observeWaterSaveNative());
    check(
        deferred,
        "A queued checkpoint request leaves the current splash audible; only the actual next-task capture starts native one-step splash suppression");
    water = prepareWaterSaveNative();
    placeWaterSaveBeanNative(water, true, -200);
    screen = PAUSE;
    captureCheckpoint(false);
    for (int i = 0; i < 3; i++)
        tick(1 / 120.0f, false, false);
    screen = PLAY;
    check(
        waterSaveQuietNative(observeWaterSaveNative()),
        "Paused presentation ticks cannot consume save-side water suppression before the first resumed physics callback");
    water = prepareWaterSaveNative();
    placeWaterSaveBeanNative(water, true, -200);
    soundEnabled = false;
    captureCheckpoint(false);
    check(
        waterSaveQuietNative(observeWaterSaveNative()),
        "Save-side suppression also blocks particles and RNG draws with sound disabled, rather than merely muting the audio device");
    soundEnabled = true;
    water = prepareWaterSaveNative();
    navigate(799282380888ULL);
    int wet = -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].kind == WATER)
            wet = i;
    bool isolated = false;
    if (wet >= 0) {
        UiScene scene = {};
        scene.memory.init(1024 * 1024);
        scene.playfield = scene.memory.array<Entity>(2);
        scene.playfieldCount = 1;
        scene.hasBean = true;
        scene.playfield[0] = page.playfield[wet];
        scene.bean = page.bean;
        Rect r = entityRect(scene.playfield[0]);
        scene.bean.state.x = (r.l + r.r) * .5f;
        scene.bean.state.y = r.t - scene.bean.hit.b + 64;
        scene.bean.state.vx = scene.bean.state.vy = 0;
        createUiPhysics(scene);
        scene.bodies[1]->SetGravityScale(0);
        stepUiPhysics(scene, 1 / 120.0f);
        clearEffects(0);
        clearEffects(1);
        stopSounds();
        resetBurstRandomNative(0, 5489);
        resetBurstRandomNative(1, 5489);
        captureCheckpoint(false);
        u32 campaign = hashBytes(&burstRandomForNative(0).engine, sizeof(NativeParticleRng));
        u32 ui = hashBytes(&burstRandomForNative(1).engine, sizeof(NativeParticleRng));
        u32 particles = emittedParticles;
        scene.bean.state.y = r.t - scene.bean.hit.b - 2;
        scene.bean.state.vy = -200;
        stepUiPhysics(scene, 1 / 120.0f);
        isolated =
            waterVoiceNative() > 0 && emittedParticles > particles && scene.inWater &&
            scene.bodies[1]->GetCheckpointForce().y > 0 &&
            campaign == hashBytes(&burstRandomForNative(0).engine, sizeof(NativeParticleRng)) &&
            ui != hashBytes(&burstRandomForNative(1).engine, sizeof(NativeParticleRng));
        destroyUiPhysics(scene);
        VirtualFree(scene.memory.data, 0, MEM_RELEASE);
    }
    check(
        isolated,
        "Saving the campaign leaves an independent selector world's ordinary splash, buoyancy and scene-local sound/particle RNG active");
    atomicWrite("test-output/water-save-results.txt", testLog, lstrlenA(testLog));
    clearEffects(0);
    clearEffects(1);
    stopSounds();
    soundEnabled = false;
    startStage(0);
    readyTimer = 0;
}
