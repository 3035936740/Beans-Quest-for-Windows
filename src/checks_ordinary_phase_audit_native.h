#pragma once

// Regression coverage for ordinary phase audit native. Runs only in the isolated BQ_ENABLE_TESTS executable.

// Diagnostic expectations come from Tom Jump/PostSolve/PrePhysics and its
// Save/Load fields in the shipped ARM library, not the port's current phase.
// Diagnostic expectations include the on-disk request and its lifetime.
struct OrdinaryPhaseAuditContactNative : OriginalContacts {
    bool eligible;
    Vec point, normal;
    int points;
    float solvedY;

    OrdinaryPhaseAuditContactNative()
        : eligible(false), point(Vec{0, 0}), normal(Vec{0, 1}), points(0), solvedY(0) {}

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        b2Fixture *fs[] = {c->GetFixtureA(), c->GetFixtureB()};
        for (int side = 0; side < 2; side++)
            if (fs[side]->GetBody() == bodies[playerIndex] && !fs[0]->IsSensor() &&
                !fs[1]->IsSensor()) {
                b2WorldManifold m;
                c->GetWorldManifold(&m);
                float sign = side ? -1.0f : 1.0f;
                Vec n = {-sign * m.normal.x, -sign * m.normal.y};
                if (c->IsEnabled() && nativeTomCanAutoJump(tomOrdinaryIncomingVyNative, n.y)) {
                    eligible = true;
                    normal = n;
                    points = c->GetManifold()->pointCount;
                    point = Vec{0, 0};
                    for (int k = 0; k < points; k++) {
                        point.x += m.points[k].x / metres;
                        point.y += m.points[k].y / metres;
                    }
                    if (points) {
                        point.x /= points;
                        point.y /= points;
                    }
                    solvedY = bodies[playerIndex]->GetPosition().y / metres;
                }
            }
        OriginalContacts::PostSolve(c, impulse);
    }
};

void ordinaryPhaseAuditFloorNative(b2World &world) {
    b2BodyDef def;
    b2Body *floor = world.CreateBody(&def);
    b2PolygonShape shape;
    shape.SetAsBox(128 * metres, 8 * metres, b2Vec2(500 * metres, 300 * metres), 0);
    floor->CreateFixture(&shape, 0);
}

void prepareOrdinaryPhaseAuditNative(float age = .2f) {
    interfaceEnabled = false;
    debugMode = musicEnabled = false;
    soundEnabled = true;
    stopMusic();
    startStage(0);
    readyTimer = deathTimer = 0;
    readyOpening.showing = false;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex)
            entities[i].state.gone = 1;
    Entity &bean = entities[playerIndex];
    bean.state.x = 500;
    bean.state.y = 308 - bean.hit.b + .2f;
    bean.state.vx = 0;
    bean.state.vy = -30;
    bean.state.timer = age;
    bean.state.hp = 1;
    jumps = 0;
    rebuildPhysics();
    ordinaryPhaseAuditFloorNative(*physicsWorld);
    clearEffects();
    stopSounds();
}

void prepareUiOrdinaryPhaseAuditNative(u64 id) {
    if (!hudScene.level.id)
        initInterface();
    interfaceEnabled = true;
    navigate(id);
    destroyUiPhysics(page);
    page.level.geometryCount = 0;
    for (int i = 0; i < page.playfieldCount; i++)
        page.playfield[i].state.gone = 1;
    page.bean.state.x = 500;
    page.bean.state.y = 308 - page.bean.hit.b + .2f;
    page.bean.state.vx = 0;
    page.bean.state.vy = -30;
    page.bean.state.timer = .2f;
    page.miniDeath = 0;
    createUiPhysics(page);
    ordinaryPhaseAuditFloorNative(*page.physics);
    clearEffects(1);
    stopSounds();
}

void ordinaryPhaseAuditNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/ordinary-phase-audit-saves");
    CreateDirectoryA(saveRoot, 0);
    const float dt = 1.0f / 120;
    char rows[12000] = {};
    prepareOrdinaryPhaseAuditNative();
    OrdinaryPhaseAuditContactNative observer;
    physicsWorld->SetContactListener(&observer);
    tick(dt, false, false);
    physicsWorld->SetContactListener(originalContacts);
    Entity &bean = entities[playerIndex];
    float contactY = bean.state.y;
    int contactCount = jumps, contactVoices = pinballDetailVoiceCount("jump"),
        contactDust = liveEffects();
    float contactVy = bean.state.vy;
    check(
        observer.eligible && observer.points > 0,
        "Ordinary phase audit exercises the real original Tom fixture and eligible solved floor contacts");
    check(
        contactVy < 30 && contactCount == 0 && contactVoices == 0 && contactDust == 0,
        "Native ordinary floor contact retains a silent uncounted jump request until the next PrePhysics step");
    float g = bean.physics->num("gravity", -400),
          speed = root(-2 * g * bean.physics->num("jump-height", 74));
    tick(dt, false, false);
    check(
        absf(bean.state.vy - (speed + g * dt)) < .02f &&
            absf(bean.state.y - (contactY + (speed + g * dt) * dt)) < .02f,
        "A flat ordinary launch reaches the next-step native semi-implicit position and gravity-adjusted velocity");
    char row[300];
    wsprintfA(
        row,
        "flat contactVy100=%d contactCount=%d contactJumpVoices=%d contactEffects=%d points=%d pointX100=%d pointY100=%d followingVy100=%d followingCount=%d\r\n",
        (int)(contactVy * 100), contactCount, contactVoices, contactDust, observer.points,
        (int)(observer.point.x * 100), (int)(observer.point.y * 100), (int)(bean.state.vy * 100),
        jumps);
    lstrcatA(rows, row);

    prepareOrdinaryPhaseAuditNative(.03f);
    tick(dt, false, false);
    tick(dt, false, false);
    check(
        entities[playerIndex].state.vy < 30 && jumps == 0 && pinballDetailVoiceCount("jump") == 0,
        "The native0.05second old-age guard also rejects an ordinary floor request without sound or jump-count credit");

    prepareOrdinaryPhaseAuditNative();
    tick(dt, false, false);
    captureCheckpoint(false);
    float savedVy = checkpoint.state[playerIndex].vy;
    int savedJumps = checkpoint.jumps;
    char path[MAX_PATH];
    savePath(path, "ordinary-contact-before-launch.dat");
    bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    entities[playerIndex].state.x += 20;
    bool loaded = written && readCheckpoint(path);
    if (loaded)
        restoreCheckpoint();
    check(
        loaded && savedVy < 30 && savedJumps == 0 && entities[playerIndex].state.vy < 30,
        "An ordinary-contact checkpoint stores the still-unlaunched body and uncounted queued request rather than an already applied jump");
    int before = jumps;
    tick(dt, false, false);
    float after = entities[playerIndex].state.vy;
    tick(dt, false, false);
    check(
        loaded && after > 200 && jumps == before + 1,
        "Loading an ordinary contact-boundary disk checkpoint consumes its pending request exactly once on the next update");
    wsprintfA(
        row,
        "disk loaded=%d version=%d bytes=%d savedVy100=%d savedJumps=%d firstVy100=%d followingCount=%d\r\n",
        loaded, checkpoint.version, (int)sizeof(checkpoint), (int)(savedVy * 100), savedJumps,
        (int)(after * 100), jumps);
    lstrcatA(rows, row);

    prepareOrdinaryPhaseAuditNative();
    tick(dt, false, false);
    float pausedY = entities[playerIndex].state.y;
    screen = PAUSE;
    for (int frame = 0; frame < 60; frame++)
        tick(dt, false, false);
    bool held = entities[playerIndex].state.y == pausedY && entities[playerIndex].state.vy < 30 &&
                jumps == 0;
    screen = PLAY;
    tick(dt, false, false);
    check(
        held && entities[playerIndex].state.vy > 200 && jumps == 1,
        "Pause freezes an unconsumed ordinary request and resume performs its single delayed launch");

    int campaignCount = jumps;
    u32 campaignHash = hashBytes(entities, sizeof(Entity) * entityCount);
    prepareUiOrdinaryPhaseAuditNative(12038123762473955738ULL);
    stepUiPhysics(page, dt);
    float uiVy = page.bean.state.vy;
    int uiVoices = pinballDetailVoiceCount("jump");
    check(
        uiVy < 30 && uiVoices == 0 && !page.ordinaryJumpNative,
        "The original selector Tom queues ordinary contact without a same-contact launch, ordinary feedback or jump voice");
    float uiY = page.bean.state.y;
    g = page.bean.physics->num("gravity", -200);
    speed = root(-2 * g * page.bean.physics->num("jump-height", 37));
    stepUiPhysics(page, dt);
    check(
        absf(page.bean.state.vy - (speed + g * dt)) < .02f &&
            absf(page.bean.state.y - (uiY + (speed + g * dt) * dt)) < .02f &&
            jumps == campaignCount &&
            campaignHash == hashBytes(entities, sizeof(Entity) * entityCount),
        "Selector deferred ordinary integration and scene ownership preserve the suspended campaign state");
    wsprintfA(
        row, "selector contactVy100=%d contactJumpVoices=%d followingVy100=%d campaignCount=%d\r\n",
        (int)(uiVy * 100), uiVoices, (int)(page.bean.state.vy * 100), jumps);
    lstrcatA(rows, row);

    prepareUiOrdinaryPhaseAuditNative(137502285691ULL);
    stepUiPhysics(page, dt);
    check(
        page.bean.state.vy < 30,
        "The ending Tom uses the same deferred ordinary floor-request phase as the original shared controller");
    prepareOrdinaryPhaseAuditNative();
    tick(dt, false, false);
    entities[playerIndex].state.x += 200;
    entities[playerIndex].state.y += 100;
    entities[playerIndex].state.vy = 0;
    bodies[playerIndex]->SetTransform(
        b2Vec2(entities[playerIndex].state.x * metres, entities[playerIndex].state.y * metres), 0);
    tick(dt, false, false);
    check(
        ordinaryJumpLaunchedNative && jumps == 1 && entities[playerIndex].state.vy > 200,
        "A retained ordinary request survives physical separation from the old support before its consumption");
    prepareOrdinaryPhaseAuditNative();
    tick(dt, false, false);
    entities[playerIndex].state.x += 200;
    entities[playerIndex].state.y += 100;
    entities[playerIndex].state.vy = 0;
    restorePhysics();
    tick(dt, false, false);
    check(
        !ordinaryJumpLaunchedNative && !ordinaryJumpStateNative.pending && jumps == 0 &&
            entities[playerIndex].state.vy < 0,
        "Physics restoration clears unconsumed ordinary requests rather than launching a fresh airborne body");
    bool age = true;
    for (int c = 0; c < 2; c++) {
        prepareOrdinaryPhaseAuditNative();
        tick(dt, false, false);
        entities[playerIndex].state.timer = c ? .05f : .049f;
        tick(dt, false, false);
        age &= ordinaryJumpLaunchedNative == (c != 0) && jumps == c;
    }
    check(
        age,
        "Ordinary requests reject old age0.049 and accept exactly0.05 without a same-step interval exemption");
    prepareOrdinaryPhaseAuditNative();
    tick(dt, false, false);
    nativeMegaJumpRequest(playerIndex, Vec{0, 1}, -400, 74, 0, 200);
    tick(dt, false, false);
    check(
        pinballLaunchedThisStepNative && !ordinaryJumpLaunchedNative &&
            !ordinaryJumpStateNative.pending && jumps == 1 && pinballDetailVoiceCount("jump") == 0,
        "A competing MegaJump takes precedence and clears the old ordinary request without duplicate sound or count");
    bool freeSpecial = true;
    for (int c = 0; c < 2; c++) {
        prepareOrdinaryPhaseAuditNative();
        tick(dt, false, false);
        ordinaryJumpStateNative.stomp = 1;
        if (!c)
            nativeMegaJumpRequest(playerIndex, Vec{0, 1}, -400, 74, 0, 200);
        else {
            trampolineStateNative.pending = 1;
            trampolineStateNative.source = playerIndex;
            trampolineStateNative.speed = 300;
        }
        tick(dt, false, false);
        freeSpecial &= jumps == 0 && !ordinaryJumpLaunchedNative &&
                       (c ? trampolineLaunchedThisStepNative : pinballLaunchedThisStepNative);
    }
    check(
        freeSpecial,
        "The sticky native stomp exemption also suppresses AddJump when a competing MegaJump or jelly wins the same request batch");

    prepareOrdinaryPhaseAuditNative();
    current.geometryCount = 0;
    int enemyIndex = entityCount;
    Object *beetle = classById(14264481342587988014ULL);
    addEntity(*beetle, Rect{240, 112, 272, 144});
    Entity &enemy = entities[enemyIndex];
    enemy.walk = enemy.dynamic = 0;
    enemy.state.active = 1;
    enemy.state.vx = enemy.state.vy = 0;
    Entity &stompBean = entities[playerIndex];
    stompBean.state.x = 256;
    stompBean.state.y = hitRect(enemy).t - stompBean.hit.b + 1;
    stompBean.state.vy = -60;
    rebuildPhysics();
    stopSounds();
    for (int frame = 0; frame < 8 && !enemy.state.gone; frame++)
        tick(dt, false, false);
    bool stompQueued = enemy.state.gone && deathTimer <= 0 && ordinaryJumpStateNative.pending &&
                       ordinaryJumpStateNative.stomp && stompBean.state.vy < 30 &&
                       pinballDetailVoiceCount("jump") == 0;
    float stompSpeed =
        nativeStompLaunch(stompBean.physics->num("jump-height", 74), 0,
                          beetle->behavior("KillableEnemyBehaviour")->num("jump-boost"),
                          stompBean.physics->num("gravity", -400));
    tick(dt, false, false);
    check(
        stompQueued && ordinaryJumpLaunchedNative && jumps == 0 &&
            absf(stompBean.state.vy - (stompSpeed - 400 * dt)) < .02f &&
            pinballDetailVoiceCount("jump") == 1,
        "The actual original beetle stomp queues its boosted ordinary request and launches next step with gravity while excluding jump-count credit");

    prepareOrdinaryPhaseAuditNative();
    tick(dt, false, false);
    captureCheckpoint(false);
    Snapshot *retained = (Snapshot *)HeapAlloc(GetProcessHeap(), 0, sizeof(Snapshot));
    *retained = checkpoint;
    SnapshotV22 *legacy = (SnapshotV22 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV22));
    memcpy(legacy, retained, sizeof(*legacy) - 4);
    legacy->version = 22;
    legacy->state[playerIndex].vy = entities[playerIndex].jumpPower;
    legacy->jumps = 1;
    legacy->checksum = hashBytes(legacy, sizeof(*legacy) - 4);
    savePath(path, "legacy-v22.dat");
    written = atomicWrite(path, legacy, sizeof(*legacy));
    loaded = written && readCheckpoint(path);
    check(
        loaded && sizeof(SnapshotV22) == 510592 && sizeof(SnapshotV23) == 510624 &&
            sizeof(Snapshot) == 543392 && checkpoint.version == 22 && validSnapshot(checkpoint) &&
            !checkpoint.ordinaryJump.pending && checkpoint.jumps == 1 &&
            checkpoint.state[playerIndex].vy == entities[playerIndex].jumpPower,
        "The exact510592byte version22 prefix migrates without inventing a historical request or altering its already launched body/count");
    legacy->checksum ^= 1;
    savePath(path, "corrupt-v22.dat");
    written = atomicWrite(path, legacy, sizeof(*legacy));
    check(
        written && !readCheckpoint(path),
        "A corrupt actual-layout version22 checksum is rejected by the new ordinary-request migration");
    checkpoint = *retained;
    checkpoint.ordinaryJump.direction.x = 2;
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    bool invalid = !validSnapshot(checkpoint);
    checkpoint = *retained;
    checkpoint.ordinaryJump.stomp = 2;
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    check(
        invalid && !validSnapshot(checkpoint),
        "Current checkpoints validate retained ordinary directions and stomp flags rather than trusting a matching checksum alone");
    checkpoint = *retained;
    HeapFree(GetProcessHeap(), 0, legacy);
    HeapFree(GetProcessHeap(), 0, retained);
    atomicWrite("test-output/ordinary-phase-audit-observations.txt", rows, lstrlenA(rows));
    atomicWrite("test-output/ordinary-phase-audit-results.txt", testLog, lstrlenA(testLog));
    stopSounds();
    lstrcpyA(saveRoot, oldRoot);
    interfaceEnabled = soundEnabled = musicEnabled = debugMode = false;
    startStage(0);
    readyTimer = 0;
}

bool ordinaryPhaseDiskReloadNative() {
    interfaceEnabled = debugMode = soundEnabled = musicEnabled = false;
    lstrcpyA(saveRoot, "test-output/ordinary-phase-audit-saves");
    startStage(0);
    readyTimer = 0;
    char path[MAX_PATH];
    savePath(path, "ordinary-contact-before-launch.dat");
    bool loaded = readCheckpoint(path);
    if (loaded)
        restoreCheckpoint();
    bool retained = loaded && checkpoint.version == 28 && ordinaryJumpStateNative.pending &&
                    entities[playerIndex].state.vy < 30 && jumps == 0;
    float speed = ordinaryJumpStateNative.speed,
          g = entities[playerIndex].physics->num("gravity", -400);
    tick(1.0f / 120, false, false);
    bool once = retained && ordinaryJumpLaunchedNative && jumps == 1 &&
                absf(entities[playerIndex].state.vy - (speed + g / 120)) < .02f;
    tick(1.0f / 120, false, false);
    bool ok = once && !ordinaryJumpLaunchedNative && jumps == 1;
    const char *result = ok ? "PASS fresh-process ordinary request disk restoration\r\n"
                            : "FAIL fresh-process ordinary request disk restoration\r\n";
    atomicWrite("test-output/ordinary-phase-fresh-process.txt", result, lstrlenA(result));
    return ok;
}
