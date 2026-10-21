#pragma once

// Regression coverage for shot death native. Runs only in the isolated BQ_ENABLE_TESTS executable.
u32 shotDeathPixelsNative(const Bullet &b) {
    cloudRenderBegin();
    glPushMatrix();
    glTranslatef(128 - b.state.x, 128 - b.state.y, 0);
    if (b.life > 0)
        drawProjectile(b);
    glPopMatrix();
    return cloudRenderPixels(Rect{80, 80, 176, 176}, false);
}

int shotDeathEffectsNative(const Bullet &b) {
    Entity e = {};
    e.source = classes + b.klass;
    return killableDeathEffectCountNative(e);
}

void shotDeathNativeChecks() {
    bool oldInterface = interfaceEnabled, oldMusic = musicEnabled, oldSound = soundEnabled,
         oldDebug = debugMode;
    interfaceEnabled = false;
    musicEnabled = false;
    soundEnabled = true;
    debugMode = false;
    stopMusic();
    stopSounds();
    killableProjectileFixture(537747635566ULL, 300, 200, 0, 0);
    State &bean = entities[playerIndex].state;
    bean.x = 300;
    bean.y = 240;
    bean.vy = -160;
    restorePhysics();
    u32 voices = audioVoicesStarted;
    for (int i = 0; i < 30 && shotFlyingNative(bullets[0]) && deathTimer <= 0; i++)
        tick(1 / 120.0f, false, false);
    Bullet &b = bullets[0];
    Vec position = {b.state.x, b.state.y};
    check(
        shotDeathVisibleNative(b) && b.state.aux == 0 && !shotBodies[0]->IsActive() &&
            shotDeathEffectsNative(b) == 0 && audioVoicesStarted == voices + 1 && deathTimer <= 0,
        "Real wizard-ball stomp retains the visible occupied projectile, immediately disables physics and plays its death sound before the deferred Tom launch without an early explosion");
    u32 full = shotDeathPixelsNative(b);
    advanceShotDeathNative(0, .05f);
    Rect r = shotDeathRectNative(b, Rect{284, 184, 316, 216});
    simulateBodies(.01f);
    check(
        absf(r.r - r.l - 24) < .001f && absf(r.t - r.b - 16) < .001f && b.state.x == position.x &&
            b.state.y == position.y && !shotBodies[0]->IsActive(),
        "A dying wizard ball retains its exact hit position and original centered75percent width50percent height midpoint without physics reactivation");
    u32 half = shotDeathPixelsNative(b);
    Bullet hidden = b;
    hidden.life = 0;
    u32 empty = shotDeathPixelsNative(hidden);
    check(
        full != half && half != empty,
        "Actual OpenGL projectile drawing distinguishes full-size, squashed and hidden wizard-ball death frames");
    b.state.aux = 0;
    advanceShotDeathNative(0, .1f);
    r = shotDeathRectNative(b, Rect{284, 184, 316, 216});
    check(
        shotDeathVisibleNative(b) && r.t == r.b && shotDeathEffectsNative(b) == 0,
        "Wizard-ball death retains the native terminal pose at exactly100ms and does not recycle its slot at equality");
    u32 before = emittedParticles;
    voices = audioVoicesStarted;
    advanceShotDeathNative(0, .00001f);
    u32 exploded = emittedParticles;
    advanceShotDeathNative(0, 1);
    beginShotDeathNative(0);
    check(
        b.life == 0 && exploded > before && emittedParticles == exploded &&
            audioVoicesStarted == voices,
        "Crossing wizard-ball die-time releases its slot and emits one explosion without replaying its sound");
    killableProjectileFixture(537747635566ULL, 300, 200, 80, 20);
    restorePhysics();
    beginShotDeathNative(0);
    advanceShotDeathNative(0, .04f);
    float age = b.state.aux;
    screen = PAUSE;
    tick(.5f, false, false);
    screen = PLAY;
    check(shotDeathVisibleNative(b) && b.state.aux == age,
          "Gameplay pause freezes an independent projectile death transition");
    shotContacts[0].first = false;
    shotContacts[0].ignored[playerIndex / 32] |= 1u << (playerIndex % 32);
    captureCheckpoint(false);
    char path[MAX_PATH];
    savePath(path, "shot-death-v14.dat");
    bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    advanceShotDeathNative(0, .2f);
    bool loaded = written && readCheckpoint(path);
    voices = audioVoicesStarted;
    restoreCheckpoint();
    check(
        loaded && validSnapshot(checkpoint) && sizeof(SnapshotV14) == 295292 &&
            sizeof(SnapshotV15) == 319868 && sizeof(SnapshotV16) == 344444 &&
            sizeof(SnapshotV17) == 364156 && sizeof(SnapshotV18) == 366720 &&
            sizeof(SnapshotV19) == 401536 && sizeof(SnapshotV21) == 486016 &&
            sizeof(SnapshotV22) == 510592 && sizeof(SnapshotV23) == 510624 &&
            sizeof(Snapshot) == 543392 && checkpoint.version == 28 && shotDeathVisibleNative(b) &&
            b.state.aux == age && shotBodies[0] && !shotBodies[0]->IsActive() &&
            !shotContacts[0].first && shotIgnored(0, playerIndex) && audioVoicesStarted == voices,
        "Version28 disk restore retains projectile death age, occupied slot and launch identity with inactive physics and no repeated sound");
    clearEffects();
    before = emittedParticles;
    for (int i = 0; i < 12; i++)
        tick(1 / 120.0f, false, false);
    exploded = emittedParticles;
    for (int i = 0; i < 12; i++)
        tick(1 / 120.0f, false, false);
    check(
        b.life == 0 && exploded > before && emittedParticles == exploded,
        "A restored projectile death completes once through the real stage update without a second explosion");
    checkpoint.bullets[0].state.aux = -1;
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    check(!validSnapshot(checkpoint),
          "A malformed saved projectile death age is rejected even with a recomputed checksum");
    startStage(49);
    readyTimer = 0;
    int boss = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            boss = i;
    bool occupied = false, reused = false;
    if (boss >= 0) {
        Entity &wizard = entities[boss];
        wizard.state.x = 4000;
        wizard.state.y = 250;
        entities[playerIndex].state.x = 3800;
        entities[playerIndex].state.y = 100;
        for (int i = 0; i < 10; i++)
            wizardShot(wizard, 1);
        for (int i = 0; i < 10; i++) {
            beginShotDeathNative(i);
            advanceShotDeathNative(i, .05f);
        }
        wizardShot(wizard, 1);
        int count = 0;
        for (int i = 0; i < 64; i++)
            count += bullets[i].life > 0;
        occupied = count == 10 && shotDeathVisibleNative(bullets[0]) && bullets[10].life == 0;
        advanceShotDeathNative(0, .051f);
        wizardShot(wizard, 1);
        reused = shotFlyingNative(bullets[0]) && bullets[0].state.aux == 0 &&
                 bullets[0].state.timer == 0 && bullets[0].state.active == boss + 1 &&
                 bullets[0].state.hp == 2;
    }
    check(
        occupied,
        "The original ten-shot wizard pool keeps dying balls occupied and cannot overwrite their unfinished presentation");
    check(
        reused,
        "The actual wizard firing path reuses a completed death slot with fresh velocity, hand, owner and animation state");
    killableProjectileFixture(537747635566ULL, 300, 200, 0, 0);
    restorePhysics();
    beginShotDeathNative(0);
    b.state.timer = 1;
    camX = 10000;
    camY = 10000;
    before = emittedParticles;
    projectilePostStep();
    advanceShotDeathNative(0, 1);
    check(
        b.life == 0 && emittedParticles == before,
        "Native offscreen projectile removal cancels even a pending death without leaving a delayed explosion");
    startStage(0);
    stopSounds();
    interfaceEnabled = oldInterface;
    musicEnabled = oldMusic;
    soundEnabled = oldSound;
    debugMode = oldDebug;
}
