#pragma once

// Regression coverage for breakable solved native. Runs only in the isolated BQ_ENABLE_TESTS executable.
class BreakableSolvedProbeNative : public OriginalContacts {
  public:
    int block, affector, solved, bottom;
    float normalY;

    explicit BreakableSolvedProbeNative(int b, int who = -1)
        : block(b), affector(who < 0 ? playerIndex : who), solved(0), bottom(0), normalY(0) {}

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        b2Body *a = c->GetFixtureA()->GetBody();
        b2Body *b = c->GetFixtureB()->GetBody();
        if ((a == bodies[block] && b == bodies[affector]) ||
            (b == bodies[block] && a == bodies[affector])) {
            b2WorldManifold m;
            c->GetWorldManifold(&m);
            normalY = a == bodies[block] ? m.normal.y : -m.normal.y;
            solved++;
            if (normalY < -.9f)
                bottom++;
        }
        OriginalContacts::PostSolve(c, impulse);
    }
};

int breakableImpactPrepareNative(int &helper, bool follow) {
    startStage(stageById(641851699177ULL));
    readyTimer = 0;
    int brick = -1;
    helper = -1;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].source->id == 86078069570ULL)
            brick = i;
        if (entities[i].kind == HELPER)
            helper = i;
    }
    if (brick < 0 || helper < 0)
        return -1;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != brick && i != helper && i != playerIndex)
            entities[i].state.gone = 1;
    Entity &bird = entities[helper];
    Entity &bean = entities[playerIndex];
    Rect r = hitRect(entities[brick]);
    float x = (r.l + r.r) * .5f;
    bird.state.active = follow ? 1 : 0;
    bird.state.timer = .2f;
    bird.state.x = x;
    bird.state.y = r.b - bird.hit.t + .5f;
    bird.state.vx = bird.state.vy = 0;
    bean.state.x = follow ? x + 100 : x;
    bean.state.y = follow ? r.b - 100 : r.b - bean.hit.t - 1;
    bean.state.vx = 0;
    bean.state.vy = follow ? 0 : 180;
    if (!follow)
        bird.state.gone = 1;
    rebuildPhysics();
    return brick;
}

void breakableSolvedNativeChecks() {
    interfaceEnabled = musicEnabled = soundEnabled = debugMode = false;
    stopMusic();
    stopSounds();
    const float speeds[] = {1, 60, 180, 600, 1500}, gaps[] = {.1f, 1, 30};
    bool valid = true, breaks = true;
    int contacts = 0, eligible = 0, misses = 0;
    char csv[24000] = {};
    lstrcpyA(csv,
             "offset,gap1000,speed,dt1000000,solved,bottom,normalY1000,hp,gone,cooldown1000\r\n");
    for (int timing = 0; timing < 2; timing++)
        for (int offset = -24; offset <= 24; offset += 8)
            for (int gap = 0; gap < 3; gap++)
                for (int speed = 0; speed < 5; speed++) {
                    startStage(stageById(8315980062923458623ULL));
                    readyTimer = 0;
                    int block = -1;
                    for (int i = 0; i < entityCount; i++)
                        if (entities[i].source->id == 3069783675016361695ULL)
                            block = i;
                    if (block < 0) {
                        valid = false;
                        continue;
                    }
                    current.geometryCount = 0;
                    for (int i = 0; i < entityCount; i++)
                        if (i != playerIndex && i != block)
                            entities[i].state.gone = 1;
                    Entity &bean = entities[playerIndex];
                    Rect r = hitRect(entities[block]);
                    bean.state.x = (r.l + r.r) * .5f + offset;
                    bean.state.y = r.b - bean.hit.t - gaps[gap];
                    bean.state.vx = 0;
                    bean.state.vy = speeds[speed];
                    rebuildPhysics();
                    BreakableSolvedProbeNative *probe = new BreakableSolvedProbeNative(block);
                    physicsWorld->SetContactListener(probe);
                    float dt = timing ? 1.0f / 30 : 1.0f / 120;
                    for (int f = 0; f < 120 && !probe->solved && deathTimer <= 0; f++)
                        tick(dt, false, false);
                    if (probe->solved)
                        contacts++;
                    if (probe->bottom) {
                        eligible++;
                        bool good = entities[block].state.gone != 0;
                        breaks &= good;
                        if (!good)
                            misses++;
                    }
                    char row[220];
                    wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", offset,
                              (int)(gaps[gap] * 1000), (int)speeds[speed], (int)(dt * 1000000),
                              probe->solved, probe->bottom, (int)(probe->normalY * 1000),
                              entities[block].state.hp, entities[block].state.gone,
                              (int)(entities[block].turnCooldown * 1000));
                    lstrcatA(csv, row);
                    physicsWorld->SetContactListener(originalContacts);
                    delete probe;
                }
    check(
        valid && contacts > 0 && eligible > 0,
        "The actual2-8 breakable block receives real underside solver impacts across gap, edge, speed and TOI conditions");
    check(
        breaks,
        "Every actual original brick underside PostSolve applies its native three damage even if the final contact list changes");
    atomicWrite("test-output/breakable-contact-matrix.csv", csv, lstrlenA(csv));
    char summary[140];
    wsprintfA(summary, "contacts=%d eligible=%d missed=%d\r\n", contacts, eligible, misses);
    atomicWrite("test-output/breakable-contact-summary.txt", summary, lstrlenA(summary));
    startStage(stageById(641851699177ULL));
    readyTimer = 0;
    int brick = -1, helper = -1;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].source->id == 86078069570ULL)
            brick = i;
        if (entities[i].kind == HELPER)
            helper = i;
    }
    if (brick >= 0 && helper >= 0) {
        current.geometryCount = 0;
        for (int i = 0; i < entityCount; i++)
            if (i != brick && i != helper && i != playerIndex)
                entities[i].state.gone = 1;
        Rect r = hitRect(entities[brick]);
        State &bird = entities[helper].state;
        bird.active = 1;
        bird.timer = .2f;
        bird.x = (r.l + r.r) * .5f;
        bird.y = r.b - entities[helper].hit.t + .5f;
        bird.vx = bird.vy = 0;
        entities[playerIndex].state.x = bird.x + 100;
        entities[playerIndex].state.y = r.b - 100;
        entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
        rebuildPhysics();
        BreakableSolvedProbeNative *probe = new BreakableSolvedProbeNative(brick, helper);
        physicsWorld->SetContactListener(probe);
        tick(1.0f / 120, false, false);
        check(
            probe->bottom > 0,
            "The actual3-6 helper produces a real underside PostSolve against that stage's authored breakable block");
        check(
            probe->bottom > 0 && !entities[brick].state.gone && entities[brick].state.hp == 3 &&
                bird.active == 2,
            "Native helper BoxPhysics body ownership leaves the3-6 brick intact while its separate HelperBird listener ends flight");
        char trace[180];
        wsprintfA(trace,
                  "helperSolved=%d bottom=%d normalY1000=%d hp=%d gone=%d helperState=%d\r\n",
                  probe->solved, probe->bottom, (int)(probe->normalY * 1000),
                  entities[brick].state.hp, entities[brick].state.gone, bird.active);
        atomicWrite("test-output/breakable-helper-impact.txt", trace, lstrlenA(trace));
        physicsWorld->SetContactListener(originalContacts);
        delete probe;
    } else {
        check(
            false,
            "The actual3-6 helper produces a real underside PostSolve against that stage's authored breakable block");
        check(
            false,
            "Native helper BoxPhysics body ownership leaves the3-6 brick intact while its separate HelperBird listener ends flight");
    }
    int h = -1, b = breakableImpactPrepareNative(h, true);
    soundEnabled = true;
    u32 voices = audioVoicesStarted;
    tick(1.0f / 120, false, false);
    soundEnabled = false;
    u32 after = audioVoicesStarted;
    updateBreakableContacts();
    check(
        b >= 0 && !entities[b].state.gone && after == voices + 1 && audioVoicesStarted == after,
        "A real helper brick strike chirps once without inventing a brick destruction voice or a repeated impact");
    stopSounds();
    b = breakableImpactPrepareNative(h, true);
    Entity &helperBird = entities[h];
    Rect br = hitRect(entities[b]);
    helperBird.state.x = br.l - helperBird.hit.r + .5f;
    helperBird.state.y = (br.b + br.t) * .5f;
    restorePhysics();
    BreakableSolvedProbeNative *probe = new BreakableSolvedProbeNative(b, h);
    physicsWorld->SetContactListener(probe);
    tick(1.0f / 120, false, false);
    check(
        probe->solved > 0 && probe->bottom == 0 && entities[b].state.hp == 3 &&
            !entities[b].state.gone && entities[b].turnCooldown == 0,
        "The original helper's real side collision retains authored zero other-hit damage instead of destroying the brick");
    physicsWorld->SetContactListener(originalContacts);
    delete probe;
    b = breakableImpactPrepareNative(h, false);
    entities[b].turnCooldown = .2f;
    probe = new BreakableSolvedProbeNative(b);
    physicsWorld->SetContactListener(probe);
    tick(1.0f / 120, false, false);
    check(
        probe->bottom > 0 && entities[b].state.hp == 3 && !entities[b].state.gone &&
            absf(entities[b].turnCooldown - (.2f - 1.0f / 120)) < .00001f,
        "Native breakable cooldown suppresses a real Bean underside impact while preserving the original quarter-second damage gate");
    physicsWorld->SetContactListener(originalContacts);
    delete probe;
    b = breakableImpactPrepareNative(h, false);
    for (b2Fixture *f = bodies[playerIndex]->GetFixtureList(); f; f = f->GetNext())
        f->SetSensor(true);
    simulateBodies(1.0f / 120);
    updateBreakableContacts();
    check(entities[b].state.hp == 3 && !entities[b].state.gone && entities[b].turnCooldown == 0,
          "A sensor-only Bean overlap produces no PostSolve damage or cooldown");
    b = breakableImpactPrepareNative(h, true);
    entities[h].state.gone = 1;
    int crate = entityCount;
    addEntity(*classById(2913132245228912724ULL), Rect{208, 1168, 240, 1200});
    Entity &box = entities[crate];
    br = hitRect(entities[b]);
    box.state.x = (br.l + br.r) * .5f;
    box.state.y = br.b - box.hit.t + .5f;
    box.state.vx = box.state.vy = 0;
    rebuildPhysics();
    probe = new BreakableSolvedProbeNative(b, crate);
    physicsWorld->SetContactListener(probe);
    tick(1.0f / 120, false, false);
    check(
        probe->bottom > 0 && entities[b].state.hp == 3 && !entities[b].state.gone,
        "An actual original loose crate underside collision does not gain Tom's brick-affector interface");
    physicsWorld->SetContactListener(originalContacts);
    delete probe;
    b = breakableImpactPrepareNative(h, false);
    b2Fixture *main = bodies[playerIndex]->GetFixtureList();
    while (main && main->IsSensor())
        main = main->GetNext();
    b2FixtureDef extra;
    extra.shape = main->GetShape();
    extra.density = 0;
    extra.filter = main->GetFilterData();
    bodies[playerIndex]->CreateFixture(&extra);
    probe = new BreakableSolvedProbeNative(b);
    physicsWorld->SetContactListener(probe);
    soundEnabled = true;
    voices = audioVoicesStarted;
    tick(1.0f / 120, false, false);
    soundEnabled = false;
    check(
        probe->bottom >= 2 && entities[b].state.gone &&
            entities[b].state.hp == 3 - 3 * probe->bottom && audioVoicesStarted == voices + 1,
        "Multiple real Bean fixture callbacks accumulate native damage but consume it once with one destruction sound");
    physicsWorld->SetContactListener(originalContacts);
    delete probe;
    stopSounds();
    b = breakableImpactPrepareNative(h, false);
    probe = new BreakableSolvedProbeNative(b);
    physicsWorld->SetContactListener(probe);
    simulateBodies(1.0f / 120);
    bodies[playerIndex]->SetActive(false);
    updateBreakableContacts();
    check(
        probe->bottom > 0 && entities[b].state.gone,
        "A real solved brick underside event still applies after the affector body is removed before damage consumption");
    physicsWorld->SetContactListener(originalContacts);
    delete probe;
    b = breakableImpactPrepareNative(h, false);
    captureCheckpoint(false);
    simulateBodies(1.0f / 120);
    restoreCheckpoint();
    updateBreakableContacts();
    check(
        entities[b].state.hp == 3 && !entities[b].state.gone,
        "Checkpoint restoration discards unconsumed damage from the old physics step rather than breaking a restored brick");
    Arena memory;
    memory.init(32 * 1024 * 1024);
    Level scan = {};
    int placements = 0, scenes = 0;
    bool authored = true;
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scan, memory);
        bool any = false;
        for (int i = 0; i < scan.objectCount; i++) {
            const Props *props = scan.objects[i].behavior("BreakableBehaviour");
            if (!props)
                continue;
            placements++;
            any = true;
            authored &= props->num("hp", 3) == 3 && props->num("hp-on-bottom-hit", 3) == 3 &&
                        props->num("hp-on-other-hit", 0) == 0;
        }
        scenes += any;
    }
    check(
        placements == 49 && scenes == 8 && authored,
        "All74 original scenes retain49 authored three-HP bricks in8stages with three underside damage and zero other-hit damage");
    startStage(0);
    readyTimer = 0;
}
