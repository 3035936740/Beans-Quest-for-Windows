#pragma once

// Regression coverage for campaign projectile animation native. Runs only in the isolated BQ_ENABLE_TESTS executable.
int prepareCampaignProjectileAnimationNative() {
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
    if (owner >= 0) {
        entities[owner].state.gone = 0;
        entities[owner].state.x = 256;
        entities[owner].state.y = 128;
        bulletSourceStates[owner] = BulletSourceState{0, 1000, 0, 1};
    }
    rebuildPhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    return owner;
}

u32 campaignProjectileAnimationPixelsNative(Bullet &shot) {
    Vec position = {shot.state.x, shot.state.y};
    float angle = shot.state.angle;
    shot.state.x = 256;
    shot.state.y = 128;
    shot.state.angle = 0;
    cloudRenderBegin();
    drawProjectile(shot);
    u32 pixels = cloudRenderPixels(Rect{232, 104, 280, 152}, false);
    shot.state.x = position.x;
    shot.state.y = position.y;
    shot.state.angle = angle;
    return pixels;
}

void campaignProjectileAnimationNativeChecks() {
    bool oldInterface = interfaceEnabled, oldSound = soundEnabled, oldMusic = musicEnabled,
         oldDebug = debugMode;
    interfaceEnabled = soundEnabled = musicEnabled = debugMode = false;
    stopMusic();
    stopSounds();
    const float dt = 1.0f / 120;
    int scenes = 0, total = 0;
    bool authored = true;
    Level scan = {};
    Arena memory;
    memory.init(32 * 1024 * 1024);
    for (int stage = 0; stage < stageCount; stage++) {
        int expected = 0;
        bool hasSource = false;
        readLevelData(levels[stageList[stage]].file, scan, memory);
        for (int i = 0; i < scan.objectCount; i++)
            if (const Props *p = scan.objects[i].behavior("BulletSource")) {
                expected += (int)p->num("bulletPoolSize", 10);
                hasSource = true;
            }
        if (!hasSource)
            continue;
        startStage(stage);
        scenes++;
        total += expected;
        authored &= projectileAnimationsNative.count == expected &&
                    compatibleProjectileAnimationsNative(projectileAnimationsNative);
        for (int i = 0; i < projectileAnimationsNative.count; i++) {
            const ProjectileAnimationMemberNative &m = projectileAnimationsNative.members[i];
            authored &= m.age == 0 && !classes[m.klass].paused &&
                        !classes[m.klass].props.flag("animate-while-paused") &&
                        !classes[m.klass].props.flag("no-tick");
        }
    }
    releaseLevel(scan, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        scenes == 7 && total == 274 && authored,
        "All seven original campaign turret stages create274source-owned ordinary projectile animation instances before firing");
    int owner = prepareCampaignProjectileAnimationNative();
    int klass = projectileAnimationClassNative(*entities[owner].source->behavior("BulletSource")),
        member = freeProjectileAnimationNative(owner, klass);
    Animation *clip =
        animation(actor(classes[klass].id), classes[klass].props.get("default-animation"));
    float length = animationLength(clip);
    for (int i = 0; i < 19; i++)
        tick(dt, false, false);
    float phase = projectileAnimationsNative.members[member].age;
    check(
        owner >= 0 && phase > 0 && campaignLiveShotsNative() == 0,
        "Actual campaign ticks advance a never-fired ordinary projectile animation while its source has no active bullets");
    shoot(entities[owner], *entities[owner].source->behavior("BulletSource"));
    Bullet &shot = bullets[0];
    check(
        shot.life > 0 && projectileAnimationsNative.binding[0] == member && shot.state.timer == 0 &&
            projectileAnimationAgeNative(shot) == phase,
        "The first real campaign shot binds its original first-free pool member without restarting its pre-existing animation");
    u32 retained = campaignProjectileAnimationPixelsNative(shot);
    projectileAnimationsNative.members[member].age = 0;
    u32 restarted = campaignProjectileAnimationPixelsNative(shot);
    projectileAnimationsNative.members[member].age = phase;
    check(
        clip->count > 1 && retained != restarted,
        "Production OpenGL campaign projectile pixels use retained member phase rather than zero flight age");
    destroyShot(0);
    float before = projectileAnimationsNative.members[member].age;
    for (int i = 0; i < 7; i++)
        tick(dt, false, false);
    float expected = before;
    for (int i = 0; i < 7; i++) {
        expected += dt;
        if (expected >= length)
            expected -= length;
    }
    phase = projectileAnimationsNative.members[member].age;
    check(shot.life == 0 && absf(phase - expected) < .000001f,
          "A deactivated campaign projectile continues animating while hidden between shots");
    shoot(entities[owner], *entities[owner].source->behavior("BulletSource"));
    check(
        shot.life > 0 && projectileAnimationsNative.binding[0] == member && shot.state.timer == 0 &&
            projectileAnimationAgeNative(shot) == phase,
        "A repeated campaign launch reuses the same first-free native member with retained phase and fresh flight state");
    shoot(entities[owner], *entities[owner].source->behavior("BulletSource"));
    int second = projectileAnimationsNative.binding[1];
    bullets[1].state.x = 512;
    bullets[1].state.y = 160;
    projectileAnimationsNative.members[member].age = length * .2f;
    projectileAnimationsNative.members[second].age = length * .6f;
    before = projectileAnimationsNative.members[member].age;
    float beforeSecond = projectileAnimationsNative.members[second].age;
    tick(dt, false, false);
    check(
        second != member &&
            absf(projectileAnimationsNative.members[member].age - before - dt) < .000001f &&
            absf(projectileAnimationsNative.members[second].age - beforeSecond - dt) < .000001f,
        "Distinct simultaneous campaign pool members retain and advance independent animation ages");
    u32 frozen = hashBytes(&projectileAnimationsNative, sizeof(projectileAnimationsNative));
    screen = PAUSE;
    tick(dt, false, false);
    screen = PLAY;
    check(
        frozen == hashBytes(&projectileAnimationsNative, sizeof(projectileAnimationsNative)),
        "Actual campaign pause freezes every original ordinary projectile clip, whose authored classes do not opt into paused animation");
    projectileAnimationsNative.members[member].age = length - dt;
    advanceProjectileAnimationsNative(dt);
    check(absf(projectileAnimationsNative.members[member].age) < .000001f,
          "Campaign pooled animation wraps at the native exact-duration boundary");
    projectileAnimationsNative.members[member].age = length * .2f;
    projectileAnimationsNative.members[second].age = length * .6f;
    captureCheckpoint(false);
    u32 saved = hashBytes(&projectileAnimationsNative, sizeof(projectileAnimationsNative));
    char path[MAX_PATH];
    savePath(path, "campaign-animation-v18.dat");
    bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    memset(&projectileAnimationsNative, 0, sizeof(projectileAnimationsNative));
    memset(bullets, 0, sizeof(bullets));
    bool read = written && readCheckpoint(path);
    restoreCheckpoint();
    check(
        read && checkpoint.version == 28 && sizeof(SnapshotV17) == 364156 &&
            sizeof(SnapshotV18) == 366720 && sizeof(SnapshotV19) == 401536 &&
            sizeof(SnapshotV21) == 486016 && sizeof(SnapshotV22) == 510592 &&
            sizeof(SnapshotV23) == 510624 && sizeof(Snapshot) == 543392 &&
            validSnapshot(checkpoint) && campaignLiveShotsNative() == 2 &&
            saved == hashBytes(&projectileAnimationsNative, sizeof(projectileAnimationsNative)),
        "Actual version28 disk restoration preserves live and hidden member ages and bindings after every runtime pool cache is erased");
    checkpoint.projectileAnimations.members[member].age = -1;
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    bool badAge = !validSnapshot(checkpoint);
    readCheckpoint(path);
    checkpoint.projectileAnimations.binding[1] = checkpoint.projectileAnimations.binding[0];
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    check(
        badAge && !validSnapshot(checkpoint),
        "Negative saved animation ages and duplicate live member identities are rejected even with recomputed checksums");
    readCheckpoint(path);
    int last = checkpoint.projectileAnimations.count - 1;
    checkpoint.projectileAnimations.members[last].ordinal++;
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    frozen = hashBytes(&projectileAnimationsNative, sizeof(projectileAnimationsNative));
    bool rejected = !checkpointEntitiesCompatible();
    restoreCheckpoint();
    check(
        rejected &&
            frozen == hashBytes(&projectileAnimationsNative, sizeof(projectileAnimationsNative)),
        "A saved pool whose source/member metadata differs from the original scene is rejected before checkpoint restoration");
    readCheckpoint(path);
    restoreCheckpoint();
    SnapshotV17 *old = (SnapshotV17 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV17));
    memcpy(old, &checkpoint, sizeof(*old) - 4);
    old->version = 17;
    old->bullets[0].state.timer = .03f;
    old->bullets[1].state.timer = .07f;
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    savePath(path, "campaign-animation-legacy-v17.dat");
    written = atomicWrite(path, old, sizeof(*old));
    HeapFree(GetProcessHeap(), 0, old);
    read = written && readCheckpoint(path);
    restoreCheckpoint();
    member = projectileAnimationsNative.binding[0];
    second = projectileAnimationsNative.binding[1];
    bool hiddenZero = true;
    for (int i = 0; i < projectileAnimationsNative.count; i++)
        if (i != member && i != second)
            hiddenZero &= projectileAnimationsNative.members[i].age == 0;
    check(
        read && checkpoint.version == 17 && validSnapshot(checkpoint) &&
            projectileAnimationAgeNative(bullets[0]) == .03f &&
            projectileAnimationAgeNative(bullets[1]) == .07f && hiddenZero,
        "Exact version17 disk bytes retain each known live visual phase while safely initializing previously unsaved hidden member phases");
    int plasma = -1;
    for (int i = 0; i < entityCount; i++)
        if (const Props *p = entities[i].source->behavior("BulletSource"))
            if (idnum(p->get("bulletClass")) == 12978618438667112518ULL)
                plasma = i;
    destroyShot(0);
    before = projectileAnimationsNative.members[member].age;
    int plasmaClass =
            projectileAnimationClassNative(*entities[plasma].source->behavior("BulletSource")),
        plasmaMember = freeProjectileAnimationNative(plasma, plasmaClass);
    projectileAnimationsNative.members[plasmaMember].age = .09f;
    shoot(entities[plasma], *entities[plasma].source->behavior("BulletSource"));
    check(
        plasma >= 0 && bullets[0].klass == plasmaClass &&
            projectileAnimationsNative.binding[0] == plasmaMember &&
            projectileAnimationAgeNative(bullets[0]) == .09f &&
            projectileAnimationsNative.members[member].age == before,
        "A physical slot reused by another campaign emitter selects that source's retained plasma member without erasing its old fireball instance");
    restartStage();
    bool fresh = true;
    for (int i = 0; i < projectileAnimationsNative.count; i++)
        fresh &= projectileAnimationsNative.members[i].age == 0;
    for (int i = 0; i < MAX_PROJECTILES; i++)
        fresh &= projectileAnimationsNative.binding[i] == -1;
    check(
        fresh && campaignLiveShotsNative() == 0,
        "A full campaign retry creates fresh original animation instances and releases all physical slot bindings");
    startStage(stageById(789619714186ULL));
    readyTimer = 0;
    for (int i = 0; i < entityCount; i++)
        if (const Props *p = entities[i].source->behavior("BulletSource"))
            for (int n = 0; n < p->num("bulletPoolSize", 10); n++)
                shoot(entities[i], *p);
    bool unique = true;
    bool occupied[MAX_PROJECTILES] = {};
    for (int slot = 0; slot < MAX_PROJECTILES; slot++)
        if (bullets[slot].life > 0) {
            int m = projectileAnimationsNative.binding[slot];
            unique &= m >= 0 && m < projectileAnimationsNative.count && !occupied[m] &&
                      projectileAnimationMatchesNative(slot, m);
            if (m >= 0 && m < MAX_PROJECTILES)
                occupied[m] = true;
        }
    check(
        projectileAnimationsNative.count == 120 && campaignLiveShotsNative() == 120 && unique &&
            projectileAnimationsNative.binding[119] >= 0,
        "All120original campaign pool members retain unique source identity across the full expanded physical slot range");
    for (int i = 0; i < projectileAnimationsNative.count; i++)
        projectileAnimationsNative.members[i].age = (i % 5) * .02f;
    captureCheckpoint(false);
    saved = hashBytes(&projectileAnimationsNative, sizeof(projectileAnimationsNative));
    savePath(path, "campaign-animation-max-v18.dat");
    written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    resetProjectileAnimationsNative();
    read = written && readCheckpoint(path);
    restoreCheckpoint();
    check(
        read && validSnapshot(checkpoint) && campaignLiveShotsNative() == 120 &&
            saved == hashBytes(&projectileAnimationsNative, sizeof(projectileAnimationsNative)),
        "Actual maximum-capacity version28 disk restoration retains every source member age and all120live bindings including slot119");
    frozen = hashBytes(&projectileAnimationsNative, sizeof(projectileAnimationsNative));
    u32 campaign = hashBytes(bullets, sizeof(bullets));
    navigate(137502285691ULL);
    tickCreditsRuntime(page, dt);
    check(
        frozen == hashBytes(&projectileAnimationsNative, sizeof(projectileAnimationsNative)) &&
            campaign == hashBytes(bullets, sizeof(bullets)),
        "The autonomous ending keeps campaign animation records and bullets isolated while its own separate pool updates");
    clearEffects(0);
    clearEffects(1);
    startStage(0);
    interfaceEnabled = oldInterface;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
    debugMode = oldDebug;
}
