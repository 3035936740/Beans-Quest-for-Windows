#pragma once

// Regression coverage for killable death native. Runs only in the isolated BQ_ENABLE_TESTS executable.
int killableDeathEffectCountNative(const Entity &e) {
    Object *effect =
        effectClass(idnum(e.source->behavior("KillableEnemyBehaviour")->get("explosion-actor")));
    int count = 0;
    if (!effect)
        return count;
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++)
        if (effectParticles[i].definition && effectParticles[i].domain == 0)
            for (int n = 0; n < effect->behaviorCount; n++)
                if (effectParticles[i].definition == &effect->behaviors[n].props) {
                    count++;
                    break;
                }
    return count;
}

int prepareKillableDeathNative() {
    startStage(0);
    readyTimer = deathTimer = 0;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex)
            entities[i].state.gone = 1;
    int index = entityCount;
    addEntity(*classById(14264481342587988014ULL), Rect{240, 112, 272, 144});
    Entity &e = entities[index];
    e.walk = e.dynamic = 0;
    e.state.active = 1;
    e.state.vx = e.state.vy = 0;
    initialState[index] = e.state;
    initialMotion[index] = MotionState{e.originX, e.originY, 0, 1, e.lastLaunchSpeed};
    State &bean = entities[playerIndex].state;
    bean.x = 256;
    bean.y = hitRect(e).t - entities[playerIndex].hit.b + 1;
    bean.vx = 0;
    bean.vy = -60;
    bean.timer = .2f;
    rebuildPhysics();
    clearEffects();
    return index;
}

void killableDeathNativeChecks() {
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldSound = soundEnabled,
         oldMusic = musicEnabled;
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = false;
    soundEnabled = true;
    stopMusic();
    stopSounds();
    int index = prepareKillableDeathNative();
    u32 sounds = audioVoicesStarted;
    tick(1 / 120.0f, false, false);
    Entity &e = entities[index];
    check(
        killableDeathVisibleNative(e) && killableDeathAgeNative(e) == 0 &&
            !bodies[index]->IsActive() && killableDeathEffectCountNative(e) == 0 &&
            audioVoicesStarted == sounds + 1,
        "A real stomp immediately disables the enemy body and plays its death sound before the deferred Tom launch while retaining the full-size enemy without an early explosion");
    Rect original = entityRect(e);
    float animationAge = e.state.timer;
    advanceKillableDeathNative(e, .05f);
    Rect half = killableDeathRectNative(e, entityRect(e));
    check(
        absf(half.r - half.l - (original.r - original.l) * .75f) < .001f &&
            absf(half.t - half.b - (original.t - original.b) * .5f) < .001f &&
            center(half).x == center(original).x && center(half).y == center(original).y &&
            absf(e.state.timer - animationAge - .05f) < .0001f,
        "The native death midpoint keeps its actor center, scales width to75percent and height to50percent, and continues the selected animation clock");
    Entity pose = e;
    pose.state.gone = 0;
    u32 alivePixels = enemyAnimationBoundaryPixels(pose),
        squashedPixels = enemyAnimationBoundaryPixels(e);
    pose.state.gone = 1;
    u32 hiddenPixels = enemyAnimationBoundaryPixels(pose);
    check(
        alivePixels != squashedPixels && squashedPixels != hiddenPixels,
        "Real OpenGL readback distinguishes the visible squashed enemy from both its full-size pose and the hidden completed death");
    e.turnCooldown = 0;
    advanceKillableDeathNative(e, .1f);
    Rect terminal = killableDeathRectNative(e, entityRect(e));
    check(
        killableDeathVisibleNative(e) && killableDeathEffectCountNative(e) == 0 &&
            absf(terminal.r - terminal.l - (original.r - original.l) * .5f) < .001f &&
            terminal.t == terminal.b,
        "At exactly the original100ms die-time the terminal half-width zero-height pose remains visible and the explosion has not fired");
    u32 before = emittedParticles, voices = audioVoicesStarted;
    advanceKillableDeathNative(e, .00001f);
    u32 exploded = emittedParticles;
    advanceKillableDeathNative(e, 1);
    killEnemy(e);
    check(
        e.state.gone == 1 && !killableDeathVisibleNative(e) && exploded > before &&
            emittedParticles == exploded && audioVoicesStarted == voices,
        "The first update beyond die-time hides the enemy and emits one explosion without a second sound or repeat death");
    index = prepareKillableDeathNative();
    killEnemy(entities[index]);
    advanceKillableDeathNative(entities[index], .04f);
    float age = killableDeathAgeNative(entities[index]);
    screen = PAUSE;
    tick(.5f, false, false);
    screen = PLAY;
    check(
        killableDeathAgeNative(entities[index]) == age &&
            killableDeathVisibleNative(entities[index]),
        "Pausing gameplay freezes the enemy death transition rather than completing it behind the pause menu");
    captureCheckpoint(false);
    char path[MAX_PATH];
    savePath(path, "killable-death-v14.dat");
    bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    advanceKillableDeathNative(entities[index], .2f);
    bool loaded = written && readCheckpoint(path);
    stopSounds();
    voices = audioVoicesStarted;
    restoreCheckpoint();
    check(
        loaded && checkpoint.version == 28 && sizeof(SnapshotV14) == 295292 &&
            sizeof(SnapshotV15) == 319868 && sizeof(SnapshotV16) == 344444 &&
            sizeof(SnapshotV17) == 364156 && sizeof(SnapshotV18) == 366720 &&
            sizeof(SnapshotV19) == 401536 && sizeof(SnapshotV21) == 486016 &&
            sizeof(SnapshotV22) == 510592 && sizeof(SnapshotV23) == 510624 &&
            sizeof(Snapshot) == 543392 && validSnapshot(checkpoint) &&
            killableDeathVisibleNative(entities[index]) &&
            absf(killableDeathAgeNative(entities[index]) - age) < .0001f &&
            !bodies[index]->IsActive() && audioVoicesStarted == voices,
        "A version28 disk checkpoint restores saved death age and inactive body while retaining the exact legacy v14 prefix or replaying the death sound");
    clearEffects();
    before = emittedParticles;
    for (int n = 0; n < 10; n++)
        tick(1 / 120.0f, false, false);
    exploded = emittedParticles;
    for (int n = 0; n < 10; n++)
        tick(1 / 120.0f, false, false);
    check(
        entities[index].state.gone == 1 && exploded > before && emittedParticles == exploded,
        "A restored halfway death finishes once through the production tick loop and never emits a duplicate explosion");
    captureCheckpoint(false);
    restoreCheckpoint();
    bool oldDead = !killableDeathVisibleNative(entities[index]);
    before = emittedParticles;
    tick(.15f, false, false);
    restartStage();
    check(
        oldDead && emittedParticles == before &&
            entities[index].state.gone == initialState[index].gone &&
            entities[index].turnCooldown == initialMotion[index].turnCooldown,
        "Finished legacy-compatible deaths stay hidden after restore while retry resets the original entity state and visual clock");
    int classesChecked = 0;
    bool all = true;
    for (int i = 0; i < classCount; i++)
        if (classes[i].has("KillableEnemyBehaviour")) {
            Entity actor;
            initializeEntity(actor, classes[i], Rect{240, 112, 272, 144});
            actor.state.timer = .375f;
            actor.state.aux = -75;
            actor.state.vx = -40;
            actor.state.vy = 20;
            killEnemy(actor);
            advanceKillableDeathNative(actor, .05f);
            Rect r = killableDeathRectNative(actor, entityRect(actor));
            all &= killableDeathVisibleNative(actor) && absf(r.r - r.l - 24) < .001f &&
                   absf(r.t - r.b - 16) < .001f && actor.state.aux == -75 &&
                   actor.state.vx == -40 && actor.state.vy == 20 && !actor.breakable;
            classesChecked++;
        }
    check(
        all && classesChecked == 12,
        "All twelve authored Killable classes retain facing and velocity while sharing the correct visual squash without colliding with a breakable cooldown");
    index = prepareKillableDeathNative();
    Entity &altered = entities[index];
    Object custom = *altered.source;
    Behavior behaviors[24];
    Pair pairs[64];
    memcpy(behaviors, custom.behaviors, sizeof(Behavior) * custom.behaviorCount);
    custom.behaviors = behaviors;
    for (int i = 0; i < custom.behaviorCount; i++)
        if (eq(behaviors[i].type, "KillableEnemyBehaviour")) {
            Props &props = behaviors[i].props;
            memcpy(pairs, props.pairs, sizeof(Pair) * props.count);
            props.pairs = pairs;
            for (int n = 0; n < props.count; n++) {
                if (eq(pairs[n].key, "alternate-on-death"))
                    pairs[n].value = (char *)"1";
                if (eq(pairs[n].key, "alternation-mask"))
                    pairs[n].value = (char *)"2";
            }
            pairs[props.count++] = Pair{(char *)"die-time", (char *)"0.2"};
        }
    altered.source = &custom;
    int bits = alternationBits;
    killEnemy(altered);
    advanceKillableDeathNative(altered, .15f);
    bool delayed = killableDeathVisibleNative(altered) && alternationBits == bits;
    advanceKillableDeathNative(altered, .06f);
    check(
        delayed && altered.state.gone == 1 && alternationBits == (bits ^ 2),
        "Configured die-time is respected and alternate-on-death fires at the native completion phase rather than the initial hit");
    startStage(0);
    stopSounds();
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
}
