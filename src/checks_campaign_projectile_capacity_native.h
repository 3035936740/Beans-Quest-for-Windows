#pragma once

// Regression coverage for campaign projectile capacity native. Runs only in the isolated BQ_ENABLE_TESTS executable.
int campaignLiveShotsNative() {
    int n = 0;
    for (int i = 0; i < MAX_PROJECTILES; i++)
        n += bullets[i].life > 0;
    return n;
}

void campaignProjectileCapacityNativeChecks() {
    bool oldInterface = interfaceEnabled, oldSound = soundEnabled, oldMusic = musicEnabled,
         oldDebug = debugMode;
    interfaceEnabled = soundEnabled = musicEnabled = debugMode = false;
    stopMusic();
    stopSounds();
    const float dt = 1.0f / 120;
    Arena memory;
    memory.init(32 * 1024 * 1024);
    Level scan = {};
    int scenes = 0, maxPool = 0;
    for (int scene = 0; scene < levelCount; scene++) {
        readLevelData(levels[scene].file, scan, memory);
        int count = 0;
        for (int i = 0; i < scan.objectCount; i++) {
            const Props *p = scan.objects[i].behavior("BulletSource");
            if (p)
                count += (int)p->num("bulletPoolSize", 10);
            const Props *boss = scan.objects[i].behavior("FinalBossBehaviour");
            if (boss)
                count += (int)boss->num("bullet-pool-size", 10);
        }
        if (count) {
            scenes++;
            maxPool = (int)maxf((float)maxPool, (float)count);
        }
    }
    check(
        maxPool == 120 && maxPool <= MAX_PROJECTILES && scenes >= 8,
        "All original scene projectile pool configurations fit the expanded runtime storage while the authored maximum remains120");
    startStage(stageById(789619714186ULL));
    readyTimer = 0;
    int sources = 0, total = 0;
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        const Props *p = e.source->behavior("BulletSource");
        if (p) {
            sources++;
            int size = (int)p->num("bulletPoolSize", 10);
            total += size;
            for (int n = 0; n < size; n++)
                shoot(e, *p);
        }
        if (i != playerIndex)
            e.state.gone = 1;
    }
    check(
        sources == 12 && total == 120 && campaignLiveShotsNative() == 120,
        "The original twelve-emitter stage admits every one of its120authored pool requests instead of starving six emitters at64");
    bool owned = true;
    for (int i = 0; i < entityCount; i++)
        if (const Props *p = entities[i].source->behavior("BulletSource")) {
            int n = 0;
            for (int k = 0; k < MAX_PROJECTILES; k++)
                n += bullets[k].life > 0 && bullets[k].state.active == i + 1;
            owned &= n == p->num("bulletPoolSize", 10);
        }
    check(owned, "Each original source receives only its own configured capacity");
    u32 full = hashBytes(bullets, sizeof(bullets));
    for (int i = 0; i < entityCount; i++)
        if (const Props *p = entities[i].source->behavior("BulletSource"))
            shoot(entities[i], *p);
    check(
        full == hashBytes(bullets, sizeof(bullets)) && campaignLiveShotsNative() == 120,
        "Additional firing requests at full source capacity never overwrite an active shot or borrow another source's allowance");
    current.geometryCount = current.tileCount = 0;
    for (int i = 0; i < MAX_PROJECTILES; i++)
        if (bullets[i].life > 0) {
            bullets[i].state.x = 10000 + i * 64.0f;
            bullets[i].state.y = 4000;
            bullets[i].state.vx = bullets[i].state.vy = 0;
            bullets[i].state.timer = .3f;
        }
    entities[playerIndex].state.x = 1200;
    entities[playerIndex].state.y = 2000;
    entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
    rebuildPhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    int slot = 119;
    Bullet &high = bullets[slot];
    high.state.x = 256;
    high.state.y = 128;
    high.state.vx = 120;
    restorePhysics();
    shotContacts[slot].first = false;
    int owner = high.state.active - 1;
    shotContacts[slot].ignored[owner / 32] |= 1u << (owner % 32);
    int physical = 0;
    for (int i = 0; i < MAX_PROJECTILES; i++)
        physical += shotBodies[i] && shotBodies[i]->IsActive();
    check(
        physical == 120 && shotBodies[slot] && shotBodyClasses[slot] == high.klass,
        "All120 real Box2D bodies are constructed with class-correct fixtures including slots above63");
    int entity = -1, resolved = -1;
    Object *contactClass = nativeKillableContactSource(shotBodies[slot], entity, resolved);
    b2BodyDef proxyDef;
    proxyDef.type = b2_kinematicBody;
    proxyDef.userData = (void *)(owner + 1);
    b2Body *proxy = physicsWorld->CreateBody(&proxyDef);
    oscillatorProxies[owner] = proxy;
    int proxyKey = nativeForceAreaBodyKey(proxy),
        shotKey = nativeForceAreaBodyKey(shotBodies[slot]);
    oscillatorProxies[owner] = 0;
    physicsWorld->DestroyBody(proxy);
    check(
        contactClass == classes + high.klass && entity == -1 && resolved == slot &&
            shotKey == MAX_ENTITIES + slot && proxyKey == MAX_ENTITIES + MAX_PROJECTILES + owner &&
            proxyKey != shotKey,
        "Extended projectile contact and force-area identities remain distinct from oscillator proxy keys");
    Object *force = 0;
    for (int scene = 0; scene < levelCount && !force; scene++) {
        readLevelData(levels[scene].file, scan, memory);
        for (int i = 0; i < scan.objectCount; i++)
            if (const Props *p = scan.objects[i].behavior("ForceAreaBehaviour"))
                if (p->flag("use-velocity-cap-y") && p->num("velocity-cap-y") > 0 &&
                    p->num("sw-force-y") + p->num("se-force-y") + p->num("nw-force-y") +
                            p->num("ne-force-y") >
                        0) {
                    force = scan.objects + i;
                    break;
                }
    }
    bool pushed = false;
    if (force) {
        Entity field;
        initializeEntity(field, *force, Rect{128, 0, 384, 256});
        nativeForceAreaApply(field, *shotBodies[slot], dt);
        pushed = high.state.vy > 0 &&
                 absf(high.state.vy - shotBodies[slot]->GetLinearVelocity().y / metres) < .00001f;
    }
    check(
        pushed,
        "An original capped upward force area updates both the high-slot body and its retained projectile velocity");
    high.state.vx = 120;
    high.state.vy = 0;
    shotBodies[slot]->SetLinearVelocity(b2Vec2(120 * metres, 0));
    physicsWorld->ClearForces();
    camX = camY = 0;
    cloudRenderBegin();
    drawWorld();
    u32 visible = cloudRenderPixels(Rect{232, 104, 280, 152}, false);
    float life = high.life;
    high.life = 0;
    cloudRenderBegin();
    drawWorld();
    u32 empty = cloudRenderPixels(Rect{232, 104, 280, 152}, false);
    high.life = life;
    check(visible != empty,
          "Actual production world rendering includes a visible projectile in slot119");
    captureCheckpoint(false);
    char path[MAX_PATH];
    savePath(path, "campaign-capacity-v17.dat");
    u32 saved = hashBytes(bullets, sizeof(bullets));
    bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    memset(bullets, 0, sizeof(bullets));
    memset(shotContacts, 0, sizeof(shotContacts));
    bool read = written && readCheckpoint(path);
    restoreCheckpoint();
    check(
        read && checkpoint.version == 28 && sizeof(SnapshotV16) == 344444 &&
            sizeof(SnapshotV17) == 364156 && sizeof(SnapshotV18) == 366720 &&
            sizeof(SnapshotV19) == 401536 && sizeof(SnapshotV21) == 486016 &&
            sizeof(SnapshotV22) == 510592 && sizeof(SnapshotV23) == 510624 &&
            sizeof(Snapshot) == 543392 && validSnapshot(checkpoint) &&
            campaignLiveShotsNative() == 120 && hashBytes(bullets, sizeof(bullets)) == saved,
        "Actual version28 disk restoration retains all120projectiles while preserving the exact344444byte version16 prefix");
    check(
        shotBodies[slot] && shotBodies[slot]->IsActive() && shotIgnored(slot, owner) &&
            !shotContacts[slot].first,
        "An extended slot retains its saved launch-ignore IDs and class-correct active physics after disk reload");
    checkpoint.extraShots[slot - 64].first = 2;
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    bool badContact = !validSnapshot(checkpoint);
    checkpoint.extraShots[slot - 64].first = 0;
    checkpoint.extraBullets[slot - 64].klass = classCount;
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    check(
        badContact && !validSnapshot(checkpoint),
        "Malformed extended contacts and projectile class IDs fail checkpoint validation even with recomputed checksums");
    read = readCheckpoint(path);
    restoreCheckpoint();
    b2BodyDef def;
    def.position.Set(300 * metres, 128 * metres);
    b2Body *wall = physicsWorld->CreateBody(&def);
    b2PolygonShape box;
    box.SetAsBox(8 * metres, 60 * metres);
    wall->CreateFixture(&box, 0);
    u32 births = emittedParticles;
    for (int frame = 0; frame < 100 && high.life > 0; frame++) {
        simulateBodies(dt);
        projectilePostStep();
    }
    check(
        high.life == 0 && shotBodies[slot] && !shotBodies[slot]->IsActive() &&
            emittedParticles > births && campaignLiveShotsNative() == 119,
        "A genuine solved wall contact destroys slot119 once and emits its original impact effect without removing other emitters' shots");
    int refill = 0;
    for (int i = 0; i < MAX_PROJECTILES; i++)
        refill += bullets[i].life > 0 && bullets[i].state.active == owner + 1;
    shoot(entities[owner], *entities[owner].source->behavior("BulletSource"));
    check(
        refill == 9 && campaignLiveShotsNative() == 120 && high.life > 0 &&
            high.state.active == owner + 1,
        "The original owner can refill its released high slot without exceeding its ten-shot limit");
    SnapshotV16 *old = (SnapshotV16 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV16));
    captureCheckpoint(false);
    memcpy(old, &checkpoint, sizeof(*old) - 4);
    old->version = 16;
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    savePath(path, "campaign-capacity-legacy-v16.dat");
    written = atomicWrite(path, old, sizeof(*old));
    HeapFree(GetProcessHeap(), 0, old);
    read = written && readCheckpoint(path);
    restoreCheckpoint();
    int extras = 0;
    for (int i = 64; i < MAX_PROJECTILES; i++)
        extras += bullets[i].life > 0;
    check(
        read && checkpoint.version == 16 && validSnapshot(checkpoint) &&
            campaignLiveShotsNative() == 64 && extras == 0,
        "Exact original version16 disk bytes migrate their64saved shots and safely initialize unsaved extended slots as empty");
    restartStage();
    check(
        campaignLiveShotsNative() == 0 && checkpoint.version == 28 && validSnapshot(checkpoint),
        "Fresh retry clears every extended projectile and captures a valid new-format checkpoint");
    startStage(0);
    interfaceEnabled = oldInterface;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
    debugMode = oldDebug;
}
