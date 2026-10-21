#pragma once

// Regression coverage for boss projectile instances native. Runs only in the isolated BQ_ENABLE_TESTS executable.
void bossProjectileInstancesNativeChecks() {
    bool oldInterface = interfaceEnabled, oldSound = soundEnabled, oldMusic = musicEnabled,
         oldDebug = debugMode;
    interfaceEnabled = soundEnabled = musicEnabled = debugMode = false;
    stopMusic();
    stopSounds();
    startStage(49);
    readyTimer = 0;
    int boss = -1;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].kind == BOSS)
            boss = i;
        if (i != playerIndex)
            entities[i].state.gone = 1;
    }
    current.geometryCount = current.tileCount = 0;
    Entity &wizard = entities[boss];
    wizard.state.x = 4000;
    wizard.state.y = 250;
    entities[playerIndex].state.x = 3800;
    entities[playerIndex].state.y = 100;
    entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
    rebuildPhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    int klass = bossProjectileAnimationClassNative(*wizard.source->behavior("FinalBossBehaviour"));
    Animation *clip = animation(actor(classes[klass].id), "idle");
    bool all = true;
    for (int i = 0; i < projectileAnimationsNative.count; i++)
        all &= projectileAnimationsNative.members[i].owner == boss &&
               projectileAnimationsNative.members[i].klass == klass &&
               projectileAnimationsNative.members[i].age == 0;
    check(
        boss >= 0 && projectileAnimationsNative.count == 10 && all && clip->count == 6 &&
            absf(animationLength(clip) - .4f) < .00001f,
        "The original Wizard creates ten independent physics/animation members using its six-frame15FPS idle clip before firing");
    const float dt = 1.0f / 120;
    for (int i = 0; i < 17; i++)
        tick(dt, false, false);
    float phase = projectileAnimationsNative.members[0].age;
    all = true;
    for (int i = 0; i < 10; i++)
        all &= projectileAnimationsNative.members[i].age == phase;
    check(
        campaignLiveShotsNative() == 0 && phase > .13f && all,
        "Real campaign ticks advance all hidden and never-fired Wizard bullet animation instances");
    wizardShot(wizard, 1);
    Bullet &shot = bullets[0];
    int member = projectileAnimationsNative.binding[0];
    check(
        shot.life > 0 && shot.klass == klass && member == 0 && shot.state.timer == 0 &&
            projectileAnimationAgeNative(shot) == phase,
        "The real Wizard firing function selects its first free member without resetting that member's running animation");
    u32 visible = campaignProjectileAnimationPixelsNative(shot);
    projectileAnimationsNative.members[member].age = 0;
    u32 restarted = campaignProjectileAnimationPixelsNative(shot);
    projectileAnimationsNative.members[member].age = phase;
    check(
        visible != restarted,
        "Production OpenGL Wizard bullet pixels retain their native running phase instead of restarting every shot");
    beginShotDeathNative(0);
    advanceProjectileAnimationsNative(.05f);
    advanceShotDeathNative(0, .05f);
    wizardShot(wizard, -1);
    check(
        shotDeathVisibleNative(shot) && projectileAnimationsNative.binding[0] == 0 &&
            projectileAnimationsNative.binding[1] == 1 &&
            projectileAnimationAgeNative(shot) > phase,
        "The Wizard's native death squash retains its occupied member while its clip advances and the other hand takes the next free member");
    advanceProjectileAnimationsNative(.051f);
    advanceShotDeathNative(0, .051f);
    phase = projectileAnimationsNative.members[0].age;
    wizardShot(wizard, 1);
    check(
        shotFlyingNative(shot) && projectileAnimationsNative.binding[0] == 0 &&
            shot.state.timer == 0 && projectileAnimationAgeNative(shot) == phase,
        "A completed Wizard death releases the same native pool member for reuse without resetting its hidden animation");
    captureCheckpoint(false);
    char path[MAX_PATH];
    savePath(path, "boss-instances-v19.dat");
    u32 state = hashBytes(&projectileAnimationsNative, sizeof(projectileAnimationsNative));
    bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    memset(&projectileAnimationsNative, 0, sizeof(projectileAnimationsNative));
    bool read = written && readCheckpoint(path);
    restoreCheckpoint();
    int identity0 = projectilePhysicsIdentityNative(shotBodies[0]),
        identity1 = projectilePhysicsIdentityNative(shotBodies[1]);
    check(
        read && checkpoint.version == 28 && validSnapshot(checkpoint) &&
            state == hashBytes(&projectileAnimationsNative, sizeof(projectileAnimationsNative)) &&
            identity0 == MAX_ENTITIES && identity1 == MAX_ENTITIES + 1 && identity0 != identity1,
        "A real version28 disk reload preserves every Wizard member phase and binding with distinct stable rebuilt PhysicsObject identities");
    SnapshotV18 *old = (SnapshotV18 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV18));
    memcpy(old, &checkpoint, sizeof(*old) - 4);
    old->version = 18;
    memset(&old->projectileAnimations, 0, sizeof(old->projectileAnimations));
    for (int i = 0; i < MAX_PROJECTILES; i++)
        old->projectileAnimations.binding[i] = -1;
    old->bullets[0].state.timer = .13f;
    old->bullets[1].state.timer = .07f;
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    savePath(path, "boss-instances-legacy-v18.dat");
    written = atomicWrite(path, old, sizeof(*old));
    HeapFree(GetProcessHeap(), 0, old);
    read = written && readCheckpoint(path);
    restoreCheckpoint();
    bool hidden = true;
    for (int i = 2; i < 10; i++)
        hidden &= projectileAnimationsNative.members[i].age == 0;
    check(
        read && checkpoint.version == 18 && validSnapshot(checkpoint) &&
            projectileAnimationsNative.count == 10 &&
            projectileAnimationAgeNative(bullets[0]) == .13f &&
            projectileAnimationAgeNative(bullets[1]) == .07f && hidden,
        "Exact version18 Wizard saves migrate their known visible phases into the newly represented boss pool without inventing unsaved hidden phases");
    restartStage();
    all = true;
    for (int i = 0; i < 10; i++)
        all &= projectileAnimationsNative.members[i].age == 0;
    for (int i = 0; i < MAX_PROJECTILES; i++)
        all &= projectileAnimationsNative.binding[i] == -1;
    check(
        all && projectileAnimationsNative.count == 10 && campaignLiveShotsNative() == 0,
        "A fresh Wizard retry rebuilds its original ten members with zero phases and no stale physical bindings");
    clearEffects(0);
    startStage(0);
    interfaceEnabled = oldInterface;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
    debugMode = oldDebug;
}
