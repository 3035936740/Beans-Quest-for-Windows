#pragma once

// Regression coverage for continuous restore native. Runs only in the isolated BQ_ENABLE_TESTS executable.

NativeContinuousPool *particleRestoreFindNative(Object &source) {
    for (int i = 0; i < 64; i++)
        if (nativeContinuousPools[i].configured && nativeContinuousPools[i].source == &source &&
            nativeContinuousPools[i].id == source.id)
            return nativeContinuousPools + i;
    return 0;
}

u32 particleRestorePayloadNative(const NativeContinuousPool &p) {
    u32 h = hashBytes(p.particles, p.capacity * sizeof(NativeContinuousParticle));
    h = h * 33 + hashBytes(&p.random, sizeof(p.random));
    h = h * 33 + hashBytes(&p.budget, sizeof(p.budget));
    h = h * 33 + p.births;
    h = h * 33 + p.deaths;
    for (int i = 0; i < p.definitionCount; i++) {
        h = h * 33 + hashBytes(&p.definitions[i].lifeNormal, sizeof(NativeParticleNormal));
        h = h * 33 + hashBytes(&p.definitions[i].velocityNormal, sizeof(NativeParticleNormal));
    }
    return h;
}

void continuousRestoreNativeChecks() {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    stopMusic();
    stopSounds();
    bool preserved = true, cursors = true, retry = true, fresh = true;
    int emitters = 0, scenes = 0;
    for (int stage = 0; stage < stageCount; stage++) {
        startStage(stage);
        readyTimer = 0;
        int count = 0;
        for (int i = 0; i < current.objectCount; i++)
            if (current.objects[i].base == 5)
                count++;
        if (!count)
            continue;
        scenes++;
        emitters += count;
        bodies[playerIndex]->SetType(b2_staticBody);
        for (int frame = 0; frame < 120; frame++)
            tick(1.0f / 120, false, false);
        captureCheckpoint(false);
        for (int frame = 0; frame < 120; frame++)
            tick(1.0f / 120, false, false);
        u32 hashes[MAX_ENTITIES] = {};
        for (int i = 0; i < current.objectCount; i++)
            if (current.objects[i].base == 5) {
                NativeContinuousPool *p = particleRestoreFindNative(current.objects[i]);
                if (p)
                    hashes[i] = particleRestorePayloadNative(*p);
                else
                    preserved = false;
            }
        restoreCheckpoint();
        for (int i = 0; i < current.objectCount; i++)
            if (current.objects[i].base == 5) {
                NativeContinuousPool *p = particleRestoreFindNative(current.objects[i]);
                preserved &= p && hashes[i] == particleRestorePayloadNative(*p);
                cursors &= p && p->updated && p->lastTime == elapsed;
            }
        for (int frame = 0; frame < 60; frame++)
            tick(1.0f / 120, false, false);
        for (int i = 0; i < current.objectCount; i++)
            if (current.objects[i].base == 5) {
                NativeContinuousPool *p = particleRestoreFindNative(current.objects[i]);
                if (p)
                    hashes[i] = particleRestorePayloadNative(*p);
            }
        restartStage();
        for (int i = 0; i < current.objectCount; i++)
            if (current.objects[i].base == 5) {
                NativeContinuousPool *p = particleRestoreFindNative(current.objects[i]);
                retry &= p && hashes[i] == particleRestorePayloadNative(*p) && p->lastTime == 0;
            }
        startStage(stage);
        readyTimer = 0;
        for (int i = 0; i < current.objectCount; i++)
            if (current.objects[i].base == 5) {
                NativeContinuousPool *p = particleRestoreFindNative(current.objects[i]);
                fresh &= p && p->births == 0 && p->deaths == 0 && p->lastTime == 0;
            }
    }
    check(
        emitters == 62 && scenes == 23,
        "The actual original campaign particle-load matrix covers62 persistent emitters in23stages");
    check(
        preserved,
        "Checkpoint loading preserves current live particle slots, ages, birth budget and RNG caches instead of rewinding or clearing them");
    check(
        cursors,
        "Checkpoint loading rebases only emitter clock cursors to restored elapsed time so drawing cannot fast-forward or restart a pool");
    check(
        retry,
        "Native reset-state loading retains the current particle population and random stream while restarting the gameplay clock");
    check(
        fresh,
        "A newly instantiated original stage still starts fresh particle pools rather than inheriting a previous scene population");

    startStage(stageById(13052395652604067555ULL));
    readyTimer = 0;
    bodies[playerIndex]->SetType(b2_staticBody);
    Object *source = 0;
    for (int i = 0; i < current.objectCount; i++)
        if (current.objects[i].id == 5405347451742818522ULL)
            source = &current.objects[i];
    bool image = false, advance = false, pause = false, isolation = false;
    if (source) {
        for (int f = 0; f < 120; f++)
            tick(1.0f / 120, false, false);
        captureCheckpoint(false);
        for (int f = 0; f < 60; f++)
            tick(1.0f / 120, false, false);
        NativeContinuousPool *p = particleRestoreFindNative(*source);
        Rect r = shifted(source->rect, 256 - center(source->rect).x, 192 - center(source->rect).y);
        cloudRenderBegin();
        drawParticles(*source, actor(source->klass), r, elapsed, 1, 1);
        u32 before = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
        cloudRenderBegin();
        u32 empty = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
        restoreCheckpoint();
        cloudRenderBegin();
        drawParticles(*source, actor(source->klass), r, elapsed, 1, 1);
        image = before != empty && before == cloudRenderPixels(Rect{0, 0, 512, 384}, false);
        p = particleRestoreFindNative(*source);
        int slot = -1;
        if (p)
            for (int i = 0; i < p->capacity; i++)
                if (p->particles[i].definition >= 0 &&
                    p->particles[i].life - p->particles[i].age > 1) {
                    slot = i;
                    break;
                }
        if (slot >= 0) {
            float age = p->particles[slot].age;
            u32 generation = p->particles[slot].generation;
            tick(1.0f / 120, false, false);
            advance = p->particles[slot].generation == generation &&
                      absf(p->particles[slot].age - age - 1.0f / 120) < .00001f;
        }
        u32 content = p ? particleRestorePayloadNative(*p) : 0;
        screen = PAUSE;
        tick(.1f, false, false);
        pause = p && content == particleRestorePayloadNative(*p);
        screen = PLAY;
        navigate(799282380888ULL);
        nativeContinuousAdvanceScene(page.level, .5f, 1, 1);
        Object *selector = 0;
        for (int i = 0; i < page.level.objectCount; i++)
            if (page.level.objects[i].base == 5) {
                selector = &page.level.objects[i];
                break;
            }
        NativeContinuousPool *other = selector ? particleRestoreFindNative(*selector) : 0;
        u32 otherHash = other ? particleRestorePayloadNative(*other) : 0;
        restoreCheckpoint();
        isolation =
            other && otherHash == particleRestorePayloadNative(*other) && other->lastTime == 1;
    }
    check(
        image,
        "Actual original star particles retain identical nonempty OpenGL output across checkpoint loading");
    check(
        advance,
        "The first physical tick after loading advances a retained real particle by exactly one dt");
    check(pause, "Pausing after particle cursor rebasing still freezes the retained population");
    check(
        isolation,
        "Campaign checkpoint loading leaves the playable-selector particle pool and clock untouched");
    check(
        sizeof(SnapshotV22) == 510592 && sizeof(SnapshotV23) == 510624 &&
            sizeof(Snapshot) == 543392 && checkpoint.version == 28,
        "Particle load fidelity needs no rewind payload alongside the independent version28 ordinary-request extension");
    startStage(0);
    readyTimer = 0;
}
