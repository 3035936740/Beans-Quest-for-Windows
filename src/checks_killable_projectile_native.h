#pragma once

// Regression coverage for killable projectile native. Runs only in the isolated BQ_ENABLE_TESTS executable.

bool nativeStompSoundPair() {
    AudioClip *jump = 0;
    AudioClip *death = 0;
    for (int i = 0; i < soundClipCount; i++) {
        if (eq(soundClips[i].name, "jump"))
            jump = &soundClips[i].clip;
        if (eq(soundClips[i].name, "enemyexplosion"))
            death = &soundClips[i].clip;
    }
    int jumpsPlaying = 0, deathsPlaying = 0;
    for (int i = 0; i < 32; i++)
        if (audioVoices[i].clip) {
            if (audioVoices[i].clip == jump)
                jumpsPlaying++;
            if (audioVoices[i].clip == death)
                deathsPlaying++;
        }
    return jump && death && activeAudioVoices() == 2 && jumpsPlaying == 1 && deathsPlaying == 1;
}

void killableProjectileFixture(u64 klass, float x, float y, float vx, float vy) {
    startStage(0);
    readyTimer = deathTimer = 0;
    current.geometryCount = 0;
    clearEffects();
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex)
            entities[i].state.gone = 1;
    State &p = entities[playerIndex].state;
    p.x = 48;
    p.y = 300;
    p.vx = p.vy = 0;
    rebuildPhysics();
    Bullet &b = bullets[0];
    memset(&b, 0, sizeof(b));
    b.klass = (int)(classById(klass) - classes);
    b.life = 1;
    b.state.x = x;
    b.state.y = y;
    b.state.vx = vx;
    b.state.vy = vy;
}

void killableProjectileNativeChecks() {
    const float dt = 1 / 120.0f;
    bool maskCases = true;
    for (int bits = 0; bits < 8; bits++) {
        Pair values[] = {{(char *)"collide-player", (char *)(bits & 1 ? "1" : "0")},
                         {(char *)"collide-oscillating", (char *)(bits & 2 ? "1" : "0")},
                         {(char *)"collide-other", (char *)(bits & 4 ? "1" : "0")}};
        Props props = {3, values};
        unsigned short mask = nativePhysicsMask(&props);
        maskCases &= ((mask & 2) != 0) == ((bits & 1) != 0) &&
                     ((mask & (8 | 32)) == (8 | 32)) == ((bits & 2) != 0) &&
                     ((mask & (1 | 4 | 16)) == (1 | 4 | 16)) == ((bits & 4) != 0);
    }
    check(
        maskCases,
        "All eight original collision flag combinations map player, oscillators, loose actors and terrain together into their native groups");
    killableProjectileFixture(537747635566ULL, 280, 200, 180, 0);
    Vec vertices[] = {{320, 64}, {320, 320}, {336, 320}, {336, 64}};
    Geometry wall = {};
    wall.kind = 1;
    wall.flags = 1;
    wall.count = 4;
    wall.points = vertices;
    current.geometry = &wall;
    current.geometryCount = 1;
    rebuildPhysics();
    for (int i = 0; i < 40 && bullets[0].life > 0; i++)
        tick(dt, false, false);
    check(
        bullets[0].life > 0 && shotBodies[0] && shotBodies[0]->IsActive() &&
            bullets[0].state.x > 338 && liveEffects() == 0 && deathTimer <= 0,
        "Original wizard projectile passes through actual solid terrain without a forbidden contact or wall explosion");
    // A deliberate fixture override checks PostSolve independently without
    // pretending the authored Wizard ball should collide with map walls.
    killableProjectileFixture(537747635566ULL, 280, 200, 180, 0);
    current.geometry = &wall;
    current.geometryCount = 1;
    rebuildPhysics();
    tick(dt, false, false);
    b2Filter wallEnabled = shotBodies[0]->GetFixtureList()->GetFilterData();
    wallEnabled.maskBits |= 1;
    shotBodies[0]->GetFixtureList()->SetFilterData(wallEnabled);
    for (int i = 0; i < 40 && bullets[0].life > 0; i++)
        tick(dt, false, false);
    check(
        bullets[0].life <= 0 && (!shotBodies[0] || !shotBodies[0]->IsActive()) &&
            liveEffects() > 0 && deathTimer <= 0,
        "Explicit terrain-enabled projectile test fixture still runs genuine PostSolve and its original explosion callback");
    killableProjectileFixture(537747635566ULL, 256, 200, 100, 0);
    for (int i = 0; i < 12; i++)
        tick(dt, false, false);
    b2Fixture *fixture = shotBodies[0] ? shotBodies[0]->GetFixtureList() : 0;
    bool filters = false;
    if (fixture)
        filters = fixture->GetFilterData().maskBits == 2;
    check(
        bullets[0].life > 0 && absf(bullets[0].state.y - 200) < 0.001f &&
            absf(bullets[0].state.vy) < 0.001f && absf(bullets[0].state.vx - 100) < 0.001f,
        "Wizard projectile activation clears native gravity even though the original CirclePhysics configuration enables it");
    check(
        filters && fixture && absf(fixture->GetFriction() - 0.2f) < 0.0001f &&
            absf(fixture->GetRestitution() - 0.5f) < 0.0001f && !shotBodies[0]->IsFixedRotation(),
        "Wizard projectile's original flags allow only player contacts and preserve source friction, restitution and free rotation");
    killableProjectileFixture(6430600862785566ULL, 128, 180, 100, 0);
    for (int i = 0; i < 12; i++)
        tick(dt, false, false);
    check(
        bullets[0].life > 0 && absf(bullets[0].state.y - 180) < 0.001f &&
            absf(bullets[0].state.vy) < 0.001f,
        "Original non-gravitated gargoyle fireball remains horizontal under the same projectile simulation");
    killableProjectileFixture(12978618438667112518ULL, 128, 180, 100, 0);
    for (int i = 0; i < 12; i++)
        tick(dt, false, false);
    check(
        bullets[0].life > 0 && absf(bullets[0].state.y - 180) < 0.001f &&
            absf(bullets[0].state.vy) < 0.001f,
        "Original gun-turret plasma projectile retains its authored nongravitated horizontal trajectory");
    check(
        !projectileGravitated(*classById(537747635566ULL)) &&
            !projectileGravitated(*classById(6430600862785566ULL)) &&
            projectileGravitated(*classById(14441582509727370349ULL)),
        "Original bullet-controller gravity overrides remain distinct from an ordinary gravitated CirclePhysics actor");
    killableProjectileFixture(537747635566ULL, 200, 180, 120, 80);
    for (int i = 0; i < 12; i++)
        tick(dt, false, false);
    check(
        bullets[0].life > 0 && absf(bullets[0].state.vx - 120) < 0.001f &&
            absf(bullets[0].state.vy - 80) < 0.001f && absf(bullets[0].state.x - 212) < 0.001f &&
            absf(bullets[0].state.y - 188) < 0.001f,
        "Actual diagonally fired wizard ball retains its original straight trajectory instead of acquiring downward acceleration");
    const u64 emitters[] = {12795324931407506847ULL, 303150949257ULL, 117211337098ULL};
    bool emittersStable = true;
    for (int target = 0; target < 3; target++) {
        startStage(0);
        readyTimer = deathTimer = 0;
        current.geometryCount = 0;
        for (int i = 0; i < entityCount; i++)
            if (i != playerIndex)
                entities[i].state.gone = 1;
        int index = entityCount;
        addEntity(*classById(emitters[target]), Rect{240, 112, 272, 144});
        Entity &emitter = entities[index];
        emitter.state.active = 1;
        State &player = entities[playerIndex].state;
        player.x = 48;
        player.y = 300;
        player.vx = player.vy = 0;
        rebuildPhysics();
        float initialY = emitter.state.y;
        for (int i = 0; i < 60; i++)
            tick(dt, false, false);
        emittersStable &= bodies[index] && bodies[index]->GetType() != b2_dynamicBody &&
                          absf(emitter.state.y - initialY) < 0.0001f &&
                          absf(emitter.state.vy) < 0.0001f;
    }
    check(
        emittersStable,
        "All three original BulletSource emitter classes retain static-position and cannot fall under world gravity");
    killableProjectileFixture(537747635566ULL, 270, 200, 180, 0);
    State &side = entities[playerIndex].state;
    side.x = 300;
    side.y = 180;
    side.vx = side.vy = 0;
    restorePhysics();
    for (int i = 0; i < 20 && deathTimer <= 0; i++)
        tick(dt, false, false);
    check(
        deathTimer > 0 && bullets[0].life > 0,
        "Low side contact with a killable wizard projectile kills Bean and disables contact before PostSolve, preserving native callback order");
    killableProjectileFixture(537747635566ULL, 300, 200, 0, 0);
    State &above = entities[playerIndex].state;
    above.x = 300;
    above.y = 240;
    above.vx = 0;
    above.vy = -160;
    restorePhysics();
    int oldJumps = jumps;
    bool savedSound = soundEnabled;
    soundEnabled = true;
    stopSounds();
    for (int i = 0; i < 30 && shotFlyingNative(bullets[0]) && deathTimer <= 0; i++)
        tick(dt, false, false);
    tick(dt, false, false);
    check(
        shotDeathVisibleNative(bullets[0]) && !shotBodies[0]->IsActive() && deathTimer <= 0 &&
            above.vy > 200 && jumps == oldJumps && nativeStompSoundPair(),
        "Actual wizard-ball stomp uses Killable PreSolve, gives a free native bounce and plays enemyexplosion plus jump exactly through the original paths");
    soundEnabled = savedSound;
    stopSounds();
    startStage(0);
}
