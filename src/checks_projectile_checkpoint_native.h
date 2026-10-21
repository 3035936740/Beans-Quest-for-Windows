#pragma once

// Regression coverage for projectile checkpoint native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void projectileCheckpointNativeChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    startStage(stageById(7138745870511843ULL));
    readyTimer = 0;
    int source = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->has("BulletSource")) {
            source = i;
            break;
        }
    int klass = (int)(classById(6430600862785566ULL) - classes);
    check(source >= 0 && klass >= 0, "Native projectile checkpoint fixture contains a fire source");
    if (source < 0 || klass < 0)
        return;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex)
            entities[i].state.gone = 1;
    entities[playerIndex].state.x = 800;
    entities[playerIndex].state.y = 1000;
    rebuildPhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    memset(bullets, 0, sizeof(bullets));
    Bullet &b = bullets[0];
    b.life = 1;
    b.klass = klass;
    b.state.x = 100;
    b.state.y = 1000;
    b.state.timer = 0.25f;
    b.state.active = source + 1;
    memset(shotContacts, 0, sizeof(shotContacts));
    shotBodies[0] = createShotBody(0);
    shotContacts[0].ignored[source / 32] |= 1u << (source % 32);
    shotContacts[0].first = false;
    shotContacts[0].lastTime = b.state.timer;
    BulletSourceAnimationNativeState &visual = bulletSourceAnimations[source];
    visual = BulletSourceAnimationNativeState{0.13f, 0.27f, 0.19f, SOURCE_VISUAL_PRE, 1, 0};
    captureCheckpoint(false);
    check(
        checkpoint.version == 28 && validSnapshot(checkpoint),
        "Version28 snapshot validates native projectile contacts and independent launcher animation clocks");
    char path[MAX_PATH];
    savePath(path, "native-projectile-v12.dat");
    bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    memset(shotContacts, 0, sizeof(shotContacts));
    shotContacts[0].first = true;
    shotContacts[0].ignored[playerIndex / 32] |= 1u << (playerIndex % 32);
    resetBulletSourceAnimations();
    haveCheckpointSourceVisuals = false;
    bool loaded = written && readCheckpoint(path);
    restoreCheckpoint();
    check(
        loaded && shotIgnored(0, source) && !shotIgnored(0, playerIndex) && !shotContacts[0].first,
        "Disk reload retains the exact projectile launch ignore set without promoting an old projectile to its first frame");
    prepareShot(0);
    check(
        shotIgnored(0, source) && !shotIgnored(0, playerIndex) && !shotContacts[0].first &&
            absf(shotContacts[0].lastTime - b.state.timer) < 0.000001f,
        "First simulation after reload preserves restored projectile ignore IDs and initialized body identity");
    check(
        !bulletSourceAnimations[source].reconstructed &&
            bulletSourceAnimations[source].phase == SOURCE_VISUAL_PRE &&
            absf(bulletSourceAnimations[source].idleAge - 0.13f) < 0.000001f &&
            absf(bulletSourceAnimations[source].preAge - 0.27f) < 0.000001f &&
            absf(bulletSourceAnimations[source].postAge - 0.19f) < 0.000001f,
        "Disk reload preserves launcher pre-shoot phase and all retained animation ages without same-process caches");
    SnapshotV11 *legacy = (SnapshotV11 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV11));
    if (legacy) {
        memcpy(legacy, &checkpoint, sizeof(*legacy) - 4);
        legacy->version = 11;
        legacy->checksum = hashBytes(legacy, sizeof(*legacy) - 4);
        savePath(path, "native-projectile-v11.dat");
        written = atomicWrite(path, legacy, sizeof(*legacy));
        bool migrated = written && readCheckpoint(path);
        check(
            migrated && checkpoint.version == 11 && checkpoint.bullets[0].klass == klass &&
                validSnapshot(checkpoint),
            "Actual version11 checkpoint bytes migrate to current memory while retaining legacy projectile and scene state");
        HeapFree(GetProcessHeap(), 0, legacy);
    }
    captureCheckpoint(false);
    checkpoint.shots[0].first = 2;
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    check(!validSnapshot(checkpoint),
          "Malformed new projectile contact state is rejected even with a recomputed checksum");
    startStage(0);
    readyTimer = 0;
}
