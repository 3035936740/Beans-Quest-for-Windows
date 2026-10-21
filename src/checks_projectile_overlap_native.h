#pragma once

// Regression coverage for projectile overlap native. Runs only in the isolated BQ_ENABLE_TESTS executable.
int prepareCreditsProjectileNative(u64 klass);

int prepareProjectileOverlapNative(int count = 2) {
    startStage(stageById(7138745870511843ULL));
    readyTimer = 0;
    int owner = -1;
    for (int i = 0; i < entityCount; i++) {
        const Props *p = entities[i].source->behavior("BulletSource");
        if (p && idnum(p->get("bulletClass")) == 6430600862785566ULL)
            owner = i;
        if (i != playerIndex)
            entities[i].state.gone = 1;
    }
    current.geometryCount = current.tileCount = 0;
    entities[playerIndex].state.x = 1200;
    entities[playerIndex].state.y = 2000;
    entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
    entities[owner].state.x = 256;
    entities[owner].state.y = 128;
    rebuildPhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    for (int i = 0; i < count; i++) {
        shoot(entities[owner], *entities[owner].source->behavior("BulletSource"));
        bullets[i].state.vx = bullets[i].state.vy = 0;
    }
    return owner;
}

void projectileOverlapFrameNative() {
    simulateBodies(1.0f / 120);
    projectilePostStep();
}

void placeProjectileOverlapNative(int slot, float x, float y) {
    bullets[slot].state.x = x;
    bullets[slot].state.y = y;
    bullets[slot].state.vx = bullets[slot].state.vy = 0;
    if (shotBodies[slot]) {
        shotBodies[slot]->SetTransform(b2Vec2(x * metres, y * metres), 0);
        shotBodies[slot]->SetLinearVelocity(b2Vec2(0, 0));
    }
}

bool creditsShotIgnoredIdentityNative(const CreditsShotNative &shot, int id) {
    return id >= 0 && id < CREDITS_PHYSICS_ID_LIMIT_NATIVE &&
           (shot.ignored[id / 32] & (1u << (id % 32))) != 0;
}

void projectileOverlapNativeChecks() {
    bool oldInterface = interfaceEnabled, oldSound = soundEnabled, oldMusic = musicEnabled,
         oldDebug = debugMode;
    interfaceEnabled = soundEnabled = musicEnabled = debugMode = false;
    stopMusic();
    stopSounds();
    int owner = prepareProjectileOverlapNative(3);
    u32 births = emittedParticles;
    projectileOverlapFrameNative();
    check(
        campaignLiveShotsNative() == 3 && emittedParticles == births && !shotContacts[0].first &&
            !shotContacts[1].first,
        "Three same-muzzle original fireball members survive their actual first physics step instead of mutually exploding");
    int idA = projectilePhysicsIdentityNative(shotBodies[0]),
        idB = projectilePhysicsIdentityNative(shotBodies[1]);
    check(
        idA == MAX_ENTITIES + projectileAnimationsNative.binding[0] &&
            idB == MAX_ENTITIES + projectileAnimationsNative.binding[1] && idA != idB &&
            shotIgnored(0, idB) && shotIgnored(1, idA),
        "Actual launch contacts remember distinct persistent pool-member PhysicsObject identities");
    placeProjectileOverlapNative(1, 512, 128);
    placeProjectileOverlapNative(2, 768, 128);
    projectileOverlapFrameNative();
    placeProjectileOverlapNative(1, 256, 128);
    projectileOverlapFrameNative();
    check(
        campaignLiveShotsNative() == 3 && shotIgnored(0, idB) && !shotContacts[0].first,
        "Remembered projectile overlap survives separation and later contact after the first-tick flag is cleared");
    owner = prepareProjectileOverlapNative();
    projectileOverlapFrameNative();
    shoot(entities[owner], *entities[owner].source->behavior("BulletSource"));
    placeProjectileOverlapNative(2, 768, 128);
    projectileOverlapFrameNative();
    placeProjectileOverlapNative(2, 256, 128);
    births = emittedParticles;
    projectileOverlapFrameNative();
    check(
        bullets[0].life == 0 && bullets[2].life == 0 && emittedParticles > births,
        "A later contact with an unremembered member still produces real solved impacts instead of globally disabling projectile collisions");
    owner = prepareProjectileOverlapNative();
    projectileOverlapFrameNative();
    idB = projectilePhysicsIdentityNative(shotBodies[1]);
    destroyShot(1);
    int plasma = -1;
    for (int i = 0; i < entityCount; i++)
        if (const Props *p = entities[i].source->behavior("BulletSource"))
            if (idnum(p->get("bulletClass")) == 12978618438667112518ULL)
                plasma = i;
    shoot(entities[plasma], *entities[plasma].source->behavior("BulletSource"));
    bool reset = shotContacts[1].first;
    placeProjectileOverlapNative(1, 768, 128);
    projectileOverlapFrameNative();
    int plasmaId = projectilePhysicsIdentityNative(shotBodies[1]);
    check(
        reset && plasmaId != idB && !shotIgnored(0, plasmaId) && !shotIgnored(1, idA),
        "Zero-flight-age cross-class reuse clears its new launch set and never aliases the old target member by physical slot");
    placeProjectileOverlapNative(1, 256, 128);
    projectileOverlapFrameNative();
    check(
        bullets[0].life == 0 && bullets[1].life == 0,
        "A previously remembered physical slot becomes collidable when occupied by a different emitter's plasma member");
    owner = prepareProjectileOverlapNative();
    projectileOverlapFrameNative();
    idB = projectilePhysicsIdentityNative(shotBodies[1]);
    destroyShot(1);
    for (int i = 0; i < entityCount; i++)
        if (const Props *p = entities[i].source->behavior("BulletSource"))
            if (idnum(p->get("bulletClass")) == 12978618438667112518ULL)
                plasma = i;
    shoot(entities[plasma], *entities[plasma].source->behavior("BulletSource"));
    placeProjectileOverlapNative(1, 768, 128);
    projectileOverlapFrameNative();
    shoot(entities[owner], *entities[owner].source->behavior("BulletSource"));
    placeProjectileOverlapNative(2, 1024, 128);
    projectileOverlapFrameNative();
    int relocated = projectilePhysicsIdentityNative(shotBodies[2]);
    placeProjectileOverlapNative(2, 256, 128);
    projectileOverlapFrameNative();
    check(
        relocated == idB && shotIgnored(0, relocated) &&
            !shotIgnored(2, projectilePhysicsIdentityNative(shotBodies[0])) &&
            campaignLiveShotsNative() == 3,
        "The same native member remains remembered after its next launch uses a different shared physical slot, while its own new launch set starts fresh");
    captureCheckpoint(false);
    char path[MAX_PATH];
    savePath(path, "projectile-overlap-v19.dat");
    bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    u32 saved = hashBytes(&checkpoint.projectileIgnores, sizeof(checkpoint.projectileIgnores));
    memset(shotContacts, 0, sizeof(shotContacts));
    bool read = written && readCheckpoint(path);
    restoreCheckpoint();
    projectileOverlapFrameNative();
    ProjectileExtraIgnoresNative *actual = (ProjectileExtraIgnoresNative *)HeapAlloc(
        GetProcessHeap(), 0, sizeof(ProjectileExtraIgnoresNative));
    captureProjectileExtraIgnoresNative(*actual);
    check(
        read && checkpoint.version == 28 && sizeof(SnapshotV18) == 366720 &&
            sizeof(SnapshotV19) == 401536 && sizeof(SnapshotV21) == 486016 &&
            sizeof(SnapshotV22) == 510592 && sizeof(SnapshotV23) == 510624 &&
            sizeof(Snapshot) == 543392 && validSnapshot(checkpoint) &&
            campaignLiveShotsNative() == 3 && shotIgnored(0, idB) &&
            saved == hashBytes(actual, sizeof(*actual)),
        "Version28 disk restoration preserves actual remembered projectile identities and overlapping live shots after runtime contact caches are erased");
    HeapFree(GetProcessHeap(), 0, actual);
    SnapshotV18 *old = (SnapshotV18 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV18));
    memcpy(old, &checkpoint, sizeof(*old) - 4);
    old->version = 18;
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    savePath(path, "projectile-overlap-legacy-v18.dat");
    written = atomicWrite(path, old, sizeof(*old));
    HeapFree(GetProcessHeap(), 0, old);
    read = written && readCheckpoint(path);
    restoreCheckpoint();
    bool empty = true;
    for (int i = 0; i < MAX_PROJECTILES; i++)
        for (int n = 0; n < PROJECTILE_EXTRA_IGNORE_WORDS_NATIVE; n++)
            empty &= shotContacts[i].extraIgnored[n] == 0;
    check(
        read && checkpoint.version == 18 && validSnapshot(checkpoint) && empty &&
            campaignLiveShotsNative() == 3,
        "Exact version18 files retain their existing bullets and animation state without inventing unsaved inter-projectile ignore history");
    owner = prepareProjectileOverlapNative(1);
    int proxyIndex = 0;
    while (proxyIndex == owner || proxyIndex == playerIndex || bodies[proxyIndex] ||
           oscillatorProxies[proxyIndex])
        proxyIndex++;
    b2BodyDef def;
    def.type = b2_kinematicBody;
    def.position.Set(256 * metres, 128 * metres);
    def.userData = (void *)(proxyIndex + 1);
    b2Body *proxy = physicsWorld->CreateBody(&def);
    b2CircleShape shape;
    shape.m_radius = 8 * metres;
    proxy->CreateFixture(&shape, 0);
    oscillatorProxies[proxyIndex] = proxy;
    projectileOverlapFrameNative();
    int proxyId = projectilePhysicsIdentityNative(proxy);
    bool distinct = bullets[0].life > 0 && shotIgnored(0, proxyId) && !shotIgnored(0, proxyIndex);
    def.position.x = 768 * metres;
    b2Body *canonical = physicsWorld->CreateBody(&def);
    canonical->CreateFixture(&shape, 0);
    bodies[proxyIndex] = canonical;
    canonical->SetTransform(b2Vec2(256 * metres, 128 * metres), 0);
    physicsWorld->Step(1.0f / 120, 10, 10);
    projectilePostStep();
    check(
        distinct && proxyId == MAX_ENTITIES + MAX_PROJECTILES + proxyIndex &&
            projectilePhysicsIdentityNative(canonical) == proxyIndex && bullets[0].life == 0,
        "Separate canonical and oscillator-proxy PhysicsObjects never share a launch-ignore identity even when their port actor index matches");
    oscillatorProxies[proxyIndex] = bodies[proxyIndex] = 0;
    physicsWorld->DestroyBody(proxy);
    physicsWorld->DestroyBody(canonical);
    startStage(0);
    u32 campaign = hashBytes(bullets, sizeof(bullets)),
        clocks = hashBytes(&projectileAnimationsNative, sizeof(projectileAnimationsNative));
    int emitter = prepareCreditsProjectileNative(6430600862785566ULL);
    creditsFire(page, emitter);
    creditsFire(page, emitter);
    for (int i = 0; i < 3; i++)
        creditsRuntime.shots[i].body->SetLinearVelocity(b2Vec2(0, 0));
    page.physics->Step(1.0f / 120, 10, 10);
    creditsProjectilePostStepNative(page, 1.0f / 120);
    int creditsB = creditsProjectilePhysicsIdentityNative(page, creditsRuntime.shots[1].body);
    check(
        creditsRuntime.shots[0].live && creditsRuntime.shots[1].live &&
            creditsRuntime.shots[2].live &&
            creditsShotIgnoredIdentityNative(creditsRuntime.shots[0], creditsB),
        "The original ending runtime also preserves three overlapping muzzle shots using its own scene-local member identities");
    creditsRuntime.shots[1].body->SetTransform(b2Vec2(512 * metres, 128 * metres), 0);
    creditsRuntime.shots[2].body->SetTransform(b2Vec2(768 * metres, 128 * metres), 0);
    page.physics->Step(1.0f / 120, 10, 10);
    creditsProjectilePostStepNative(page, 1.0f / 120);
    creditsRuntime.shots[1].body->SetTransform(b2Vec2(256 * metres, 128 * metres), 0);
    page.physics->Step(1.0f / 120, 10, 10);
    creditsProjectilePostStepNative(page, 1.0f / 120);
    check(
        creditsRuntime.shots[0].live && creditsRuntime.shots[1].live &&
            campaign == hashBytes(bullets, sizeof(bullets)) &&
            clocks == hashBytes(&projectileAnimationsNative, sizeof(projectileAnimationsNative)),
        "Ending remembered projectile contacts survive real separation/recontact and remain isolated from the campaign world and identity state");
    clearEffects(0);
    clearEffects(1);
    startStage(0);
    interfaceEnabled = oldInterface;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
    debugMode = oldDebug;
}
