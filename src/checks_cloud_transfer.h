#pragma once

// Long repeated seam traversal with real authored fixtures. The body reverses
// only well inside the row, so spring/outer-edge falls cannot mask a failure.
class CloudTransferContacts : public OneWayNativeTestContacts {
  public:
    CloudTransferContacts(Entity *actors, int n) : OneWayNativeTestContacts(actors, n) {}

    void PostSolve(b2Contact *c, const b2ContactImpulse *) {
        oneWaySupportPostSolveNative(c);
    }
};

void cloudTransferChecks() {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    startStage(0);
    const u64 classes[] = {14264481342587988014ULL, 13516539842961264564ULL, 970132910526ULL,
                           3216410667731258ULL};
    const int rates[] = {30, 60, 120, 144, 240, 0};
    char *rows = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 50000);
    lstrcpyA(rows, "class,rate,phase,mode,step,x1000,bottom1000,vy1000,failed\r\n");
    int cases = 0, falls = 0;
    for (int klass = 0; klass < 4; klass++)
        for (int rate = 0; rate < 6; rate++)
            for (int phase = 0; phase < 4; phase++)
                for (int mode = 0; mode < 3; mode++) {
                    Entity *actors = (Entity *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                         13 * sizeof(Entity));
                    Object &source = *classById(classes[klass]);
                    Object &cloud = *classById(14815823507569313204ULL);
                    initializeEntity(actors[0], source, source.rect);
                    actors[0].state.active = 1;
                    actors[0].state.x = 4400 + phase * .137f;
                    actors[0].state.y = 80 - actors[0].hit.b + .64f;
                    actors[0].state.vx = actors[0].state.vy = 0;
                    b2World *world = new b2World(b2Vec2(0, nativeWorldGravity * metres), true);
                    CloudTransferContacts listener(actors, 13);
                    world->SetContactListener(&listener);
                    for (int i = 1; i <= 12; i++) {
                        initializeEntity(actors[i], cloud, cloud.rect);
                        actors[i].state.x = 4272 + (i - 1) * 32.0f;
                        actors[i].state.y = 71;
                        createPhysicsEntity(*world, actors[i], i);
                    }
                    b2Body *body = createPhysicsEntity(*world, actors[0], 0);
                    if (!body) {
                        clearOneWayWorld(world);
                        delete world;
                        check(
                            false,
                            "Cloud seam fixture factory returns the requested authored walker body");
                        continue;
                    }
                    bool right = phase < 2, failed = false;
                    int step = 0;
                    float age = 0;
                    while (age < 30 && !failed) {
                        float dt = rate == 5 ? (step % 3 == 0   ? 1 / 30.0f
                                                : step % 3 == 1 ? 1 / 144.0f
                                                                : 1 / 60.0f)
                                             : 1.0f / rates[rate];
                        float x = body->GetPosition().x / metres;
                        if (x < 4300)
                            right = true;
                        if (x > 4590)
                            right = false;
                        b2Vec2 velocity = body->GetLinearVelocity();
                        velocity.x = (right ? actors[0].speed : -actors[0].speed) * metres;
                        // A small upward contact impulse and a downward contact
                        // impulse exercise support transfer at the next fixture.
                        if (mode && step > 10 && step % 37 == 0)
                            velocity.y = (mode == 1 ? 60 : -200) * metres;
                        body->SetLinearVelocity(velocity);
                        world->Step(dt, 10, 10);
                        failed = body->GetPosition().y / metres + actors[0].hit.b < 64;
                        age += dt;
                        step++;
                    }
                    char id[24], row[256];
                    idstr(classes[klass], id);
                    wsprintfA(row, "%s,%d,%d,%d,%d,%d,%d,%d,%d\r\n", id, rates[rate], phase, mode,
                              step, (int)(body->GetPosition().x / metres * 1000),
                              (int)((body->GetPosition().y / metres + actors[0].hit.b) * 1000),
                              (int)(body->GetLinearVelocity().y / metres * 1000), failed);
                    lstrcatA(rows, row);
                    falls += failed;
                    cases++;
                    clearOneWayWorld(world);
                    delete world;
                    HeapFree(GetProcessHeap(), 0, actors);
                }
    atomicWrite("test-output/cloud-transfer-cases.csv", rows, lstrlenA(rows));
    HeapFree(GetProcessHeap(), 0, rows);
    check(
        cases == 288 && falls == 0,
        "Repeated authored walker traversal stays above continuous cloud seams across288 frame/phase/impulse cases");
    atomicWrite("test-output/cloud-transfer-results.txt", testLog, lstrlenA(testLog));
}

void cloudTransferCampaignChecks() {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    const int rates[] = {30, 60, 120, 144, 240, 0};
    char *rows = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 100000);
    lstrcpyA(rows, "rate,category,delay,falls,firstStep,firstX1000,firstBottom1000\r\n");
    int cases = 0, falls = 0;
    for (int rate = 0; rate < 6; rate++)
        for (int category = 1; category <= 9; category++)
            for (int delay = 0; delay < 3; delay++) {
                int selected[3], boss = prepareWizardWalkSpawnNative(1, 2, selected);
                if (boss < 0) {
                    check(false, "Actual cloud-transfer wave resolves blue walkers");
                    continue;
                }
                alternationBits = legacyWizardArenaParity(4, 3, false);
                int peers[3], peerCount = 0;
                for (int i = 0; i < entityCount; i++)
                    if (entities[i].bossSpawn == category && peerCount < 3)
                        peers[peerCount++] = i;
                float age = 0, nextWave = delay * .137f, previous[3];
                bool landed[3] = {}, confirmed[3] = {}, pending = true;
                for (int n = 0; n < 3; n++)
                    previous[n] = entities[selected[n]].state.y + entities[selected[n]].hit.b;
                int local = 0, firstStep = -1;
                float firstX = 0, firstBottom = 0;
                for (int step = 0; age < 30; step++) {
                    float dt = rate == 5 ? (step % 3 == 0   ? 1 / 30.0f
                                            : step % 3 == 1 ? 1 / 144.0f
                                                            : 1 / 60.0f)
                                         : 1.0f / rates[rate];
                    if (pending && age >= nextWave && category != 2 && peerCount == 3) {
                        wizardMotion.pendingCount = 3;
                        for (int n = 0; n < 3; n++)
                            wizardMotion.pending[n] = peers[n];
                        wizardMaterializeSummons(entities[boss]);
                        pending = false;
                    }
                    tick(dt, false, false);
                    age += dt;
                    for (int n = 0; n < 3; n++) {
                        Entity &e = entities[selected[n]];
                        float bottom = e.state.y + e.hit.b;
                        bool inRow = e.state.x + e.hit.l > 4288.1f && e.state.x + e.hit.r < 4575.9f;
                        for (b2ContactEdge *edge = bodies[selected[n]]->GetContactList(); edge;
                             edge = edge->next) {
                            int i = (int)edge->other->GetUserData() - 1;
                            b2Contact *c = edge->contact;
                            if (i >= 0 && i < entityCount && entities[i].crumble &&
                                c->IsEnabled() && c->IsTouching() &&
                                !c->GetFixtureA()->IsSensor() && !c->GetFixtureB()->IsSensor())
                                landed[n] = true;
                        }
                        if (inRow && landed[n] && bottom < 64 && !e.state.gone && !confirmed[n]) {
                            confirmed[n] = true;
                            local++;
                            if (firstStep < 0) {
                                firstStep = step;
                                firstX = e.state.x;
                                firstBottom = bottom;
                            }
                        }
                        if (!inRow)
                            landed[n] = false;
                        previous[n] = bottom;
                    }
                }
                char line[220];
                wsprintfA(line, "%d,%d,%d,%d,%d,%d,%d\r\n", rates[rate], category, delay, local,
                          firstStep, (int)(firstX * 1000), (int)(firstBottom * 1000));
                lstrcatA(rows, line);
                cases++;
                falls += local;
                atomicWrite("test-output/cloud-transfer-campaign.csv", rows, lstrlenA(rows));
            }
    check(
        cases == 162 && falls == 0,
        "Extended actual Wizard waves do not send grounded blue walkers below continuous clouds across162 frame/phase contexts");
    atomicWrite("test-output/cloud-transfer-campaign-results.txt", testLog, lstrlenA(testLog));
    HeapFree(GetProcessHeap(), 0, rows);
}

void cloudEndpointChecks() {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    char *rows = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1000000);
    lstrcpyA(
        rows,
        "rate,mode,member,step,x1000,bottom1000,vx1000,vy1000,firstSupported,allowed,refused\r\n");
    const int rates[] = {30, 60, 120, 144, 240, 0};
    int cases = 0, leaks = 0;
    for (int rate = 0; rate < 6; rate++)
        for (int mode = 0; mode < 2; mode++) {
            int selected[3], boss = prepareWizardWalkSpawnNative(1, 2, selected);
            if (boss < 0)
                continue;
            alternationBits = legacyWizardArenaParity(4, 3, false);
            for (int i = 0; i < 3; i++) {
                Entity &e = entities[selected[i]];
                if (!mode && i) {
                    e.state.gone = 1;
                    bodies[selected[i]]->SetActive(false);
                    continue;
                }
                e.state.x = 4320 + i * 48.0f;
                e.state.y = 110;
                e.state.vx = e.state.vy = 0;
                nativeWalkDirection(e, true, true);
            }
            restorePhysics();
            bodies[playerIndex]->SetType(b2_staticBody);
            bool supported[3] = {};
            float priorBottom[3] = {110, 110, 110}, priorRight[3] = {};
            float age = 0;
            for (int step = 0; age < 15; step++) {
                float dt = rate == 5 ? (step % 3 == 0   ? 1 / 30.0f
                                        : step % 3 == 1 ? 1 / 144.0f
                                                        : 1 / 60.0f)
                                     : 1.0f / rates[rate];
                tick(dt, false, false);
                age += dt;
                for (int n = 0; n < (mode ? 3 : 1); n++) {
                    Entity &e = entities[selected[n]];
                    b2Body *body = bodies[selected[n]];
                    float bottom = e.state.y + e.hit.b, right = e.state.x + e.hit.r;
                    bool first = false;
                    int allowed = 0, refused = 0;
                    for (int j = 0; j < oneWayEntryLimit; j++) {
                        OneWayEntry &entry = oneWayEntries[j];
                        if (entry.contact && entry.other == body) {
                            allowed += entry.above;
                            refused += !entry.above;
                        }
                    }
                    for (b2ContactEdge *edge = body->GetContactList(); edge; edge = edge->next) {
                        int other = (int)edge->other->GetUserData() - 1;
                        b2Contact *c = edge->contact;
                        if (other >= 0 && other < entityCount && entities[other].crumble &&
                            entities[other].state.x == 4304 && c->IsTouching() && c->IsEnabled() &&
                            !c->GetFixtureA()->IsSensor() && !c->GetFixtureB()->IsSensor())
                            first = true;
                    }
                    supported[n] |= first;
                    if (supported[n] && priorBottom[n] >= 79.5f && bottom < 79.5f && right > 4290 &&
                        priorRight[n] > 4290 && !e.state.gone)
                        leaks++;
                    if (right < 4287)
                        supported[n] = false;
                    if (e.state.x > 4260 && e.state.x < 4360 && bottom < 120 &&
                        lstrlenA(rows) < 999700) {
                        char row[220];
                        wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", rates[rate], mode, n,
                                  step, (int)(e.state.x * 1000), (int)(bottom * 1000),
                                  (int)(e.state.vx * 1000), (int)(e.state.vy * 1000), first,
                                  allowed, refused);
                        lstrcatA(rows, row);
                    }
                    priorBottom[n] = bottom;
                    priorRight[n] = right;
                }
            }
            cases++;
        }
    atomicWrite("test-output/cloud-endpoint-observations.csv", rows, lstrlenA(rows));
    check(
        cases == 12 && leaks == 0,
        "Actual solo and three-member blue walkers retain first-cloud support until leaving its physical endpoint before spring descent");
    atomicWrite("test-output/cloud-endpoint-results.txt", testLog, lstrlenA(testLog));
    HeapFree(GetProcessHeap(), 0, rows);
}

void cloudSupportFaceChecks() {
    Entity actors[2];
    Object &enemy = *classById(14264481342587988014ULL),
           &source = *classById(14815823507569313204ULL);
    initializeEntity(actors[0], enemy, enemy.rect);
    initializeEntity(actors[1], source, source.rect);
    actors[0].state.active = 1;
    actors[0].state.x = actors[1].state.x = 0;
    actors[1].state.y = 71;
    actors[0].state.y = 80 - actors[0].hit.b + .64f;
    actors[0].state.vy = -10;
    b2World *world = new b2World(b2Vec2(0, 0), true);
    CloudTransferContacts listener(actors, 2);
    world->SetContactListener(&listener);
    b2Body *cloud = createPhysicsEntity(*world, actors[1], 1),
           *body = createPhysicsEntity(*world, actors[0], 0);
    world->Step(1 / 120.0f, 10, 10);
    bool top = false;
    for (int i = 0; i < oneWayEntryLimit; i++)
        top |= oneWayEntries[i].contact && oneWayEntries[i].other == body &&
               oneWayEntries[i].supported;
    // Re-evaluation of an existing pair on its underside must not inherit a
    // top-face solver exemption. Box2D keeps touching pairs across face changes.
    body->SetTransform(b2Vec2(0, (62 - actors[0].hit.t - .64f) * metres), 0);
    body->SetLinearVelocity(b2Vec2(0, 60 * metres));
    world->Step(1 / 120.0f, 10, 10);
    bool underside = false, blocked = false;
    for (b2ContactEdge *edge = body->GetContactList(); edge; edge = edge->next) {
        b2Contact *c = edge->contact;
        if (edge->other != cloud || !c->IsTouching() || c->GetFixtureA()->IsSensor() ||
            c->GetFixtureB()->IsSensor() || !c->GetManifold()->pointCount)
            continue;
        b2WorldManifold m;
        c->GetWorldManifold(&m);
        float normal = c->GetFixtureA()->GetBody() == cloud ? m.normal.y : -m.normal.y;
        underside |= normal < -.95f;
        blocked |= normal < -.95f && c->IsEnabled();
    }
    char row[200];
    wsprintfA(row, "top=%d underside=%d blocked=%d vy1000=%d\r\n", top, underside, blocked,
              (int)(body->GetLinearVelocity().y / metres * 1000));
    atomicWrite("test-output/cloud-support-face-case.txt", row, lstrlenA(row));
    check(
        top && underside && !blocked && body->GetLinearVelocity().y / metres > 59,
        "A retained top-support pair never turns the cloud underside into a ceiling on later contact evaluation");
    atomicWrite("test-output/cloud-support-face-results.txt", testLog, lstrlenA(testLog));
    clearOneWayWorld(world);
    delete world;
}

void cloudSupportHandoffChecks() {
    Entity actors[3];
    Object &enemy = *classById(14264481342587988014ULL),
           &source = *classById(14815823507569313204ULL);
    initializeEntity(actors[0], enemy, enemy.rect);
    for (int i = 1; i < 3; i++) {
        initializeEntity(actors[i], source, source.rect);
        actors[i].state.x = (i - 1) * 32.0f;
        actors[i].state.y = 71;
    }
    actors[0].state.active = 1;
    actors[0].state.x = 32;
    actors[0].state.y = 80 - actors[0].hit.b + .64f;
    actors[0].state.vy = -10;
    b2World *world = new b2World(b2Vec2(0, nativeWorldGravity * metres), true);
    CloudTransferContacts listener(actors, 3);
    world->SetContactListener(&listener);
    createPhysicsEntity(*world, actors[1], 1);
    createPhysicsEntity(*world, actors[2], 2);
    b2Body *body = createPhysicsEntity(*world, actors[0], 0);
    for (int i = 0; i < 5; i++)
        world->Step(1 / 120.0f, 10, 10);
    bool supported = false;
    for (int i = 0; i < oneWayEntryLimit; i++)
        supported |= oneWayEntries[i].contact && oneWayEntries[i].other == body &&
                     oneWayEntries[i].supported;
    // Reproduce the subpixel solved penetration from mixed creature contact
    // at the arrival of a new neighboring cloud fixture, not a missing tile.
    body->SetTransform(b2Vec2(28 * metres, (79.8f - actors[0].hit.b) * metres), 0);
    bool fell = false;
    float lowest = 1000, interiorLowest = 1000;
    for (int i = 0; i < 75; i++) {
        b2Vec2 v = body->GetLinearVelocity();
        v.x = -75 * metres;
        body->SetLinearVelocity(v);
        world->Step(1 / 120.0f, 10, 10);
        float bottom = body->GetPosition().y / metres + actors[0].hit.b;
        lowest = minf(lowest, bottom);
        fell |= bottom < 62;
        float x = body->GetPosition().x / metres;
        if (x + actors[0].hit.l > -16 && x + actors[0].hit.r < 16)
            interiorLowest = minf(interiorLowest, bottom);
    }
    char row[200];
    wsprintfA(row, "supported=%d fell=%d lowest1000=%d interiorLowest1000=%d\r\n", supported, fell,
              (int)(lowest * 1000), (int)(interiorLowest * 1000));
    atomicWrite("test-output/cloud-support-handoff-case.txt", row, lstrlenA(row));
    check(
        supported && !fell && lowest >= 79.5f,
        "A subpixel creature-contact penetration at a neighboring cloud handoff cannot permanently refuse the next top surface");
    atomicWrite("test-output/cloud-support-handoff-results.txt", testLog, lstrlenA(testLog));
    clearOneWayWorld(world);
    delete world;
}

// The same handoff must work for every authored Walk collider in either
// direction, at world-sized coordinates and different step durations.
void cloudSupportMatrixChecks() {
    const u64 classes[] = {14264481342587988014ULL, 13516539842961264564ULL, 970132910526ULL,
                           3216410667731258ULL};
    const int rates[] = {30, 60, 120, 144, 240, 0};
    char rows[6000] = "class,rate,direction,supported,lowest1000,controls\r\n";
    bool safe = true, controls = true;
    int cases = 0;
    for (int klass = 0; klass < 4; klass++)
        for (int rate = 0; rate < 6; rate++)
            for (int direction = 0; direction < 2; direction++) {
                Entity actors[3];
                Object &enemy = *classById(classes[klass]),
                       &source = *classById(14815823507569313204ULL);
                initializeEntity(actors[0], enemy, enemy.rect);
                for (int i = 1; i < 3; i++) {
                    initializeEntity(actors[i], source, source.rect);
                    actors[i].state.x = 4288 + (i - 1) * 32.0f;
                    actors[i].state.y = 71;
                }
                actors[0].state.active = 1;
                actors[0].state.x = direction ? 4288.0f : 4320.0f;
                actors[0].state.y = 80 - actors[0].hit.b + .64f;
                actors[0].state.vy = -10;
                b2World *world = new b2World(b2Vec2(0, nativeWorldGravity * metres), true);
                CloudTransferContacts listener(actors, 3);
                world->SetContactListener(&listener);
                b2Body *clouds[2] = {createPhysicsEntity(*world, actors[1], 1),
                                     createPhysicsEntity(*world, actors[2], 2)};
                b2Body *body = createPhysicsEntity(*world, actors[0], 0);
                for (int i = 0; i < 5; i++)
                    world->Step(1 / 120.0f, 10, 10);
                bool supported = false;
                for (int i = 0; i < oneWayEntryLimit; i++)
                    supported |= oneWayEntries[i].contact && oneWayEntries[i].other == body &&
                                 oneWayEntries[i].supported;
                b2Body *target = clouds[direction ? 1 : 0];
                b2Fixture *fixture = target->GetFixtureList();
                b2Vec2 position = target->GetPosition(), a, b;
                bool negative = true;
                // Genuine gaps, a different top height, moving surfaces and
                // feet below solver slop must retain the native entry gate.
                target->SetTransform(position + b2Vec2((direction ? 4.0f : -4.0f) * metres, 0), 0);
                oneWayTopSegment(fixture, actors, 3, a, b);
                negative &= !oneWayAdjacentSupport(fixture, body, a, b, actors, 3);
                target->SetTransform(position + b2Vec2(0, 2 * metres), 0);
                oneWayTopSegment(fixture, actors, 3, a, b);
                negative &= !oneWayAdjacentSupport(fixture, body, a, b, actors, 3);
                target->SetTransform(position, 0);
                target->SetLinearVelocity(b2Vec2(0, metres));
                oneWayTopSegment(fixture, actors, 3, a, b);
                negative &= !oneWayAdjacentSupport(fixture, body, a, b, actors, 3);
                target->SetLinearVelocity(b2Vec2(0, 0));
                b2Vec2 old = body->GetPosition();
                body->SetTransform(b2Vec2(old.x, (78 - actors[0].hit.b) * metres), 0);
                negative &= !oneWayAdjacentSupport(fixture, body, a, b, actors, 3);
                body->SetTransform(old, 0);
                body->SetTransform(b2Vec2((direction ? 4292.0f : 4316.0f) * metres,
                                          (79.8f - actors[0].hit.b) * metres),
                                   0);
                float age = 0, lowest = 1000;
                int step = 0;
                while (age < .6f) {
                    float dt = rate == 5 ? (step % 3 == 0   ? 1 / 30.0f
                                            : step % 3 == 1 ? 1 / 144.0f
                                                            : 1 / 60.0f)
                                         : 1.0f / rates[rate];
                    b2Vec2 v = body->GetLinearVelocity();
                    v.x = (direction ? 75.0f : -75.0f) * metres;
                    body->SetLinearVelocity(v);
                    world->Step(dt, 10, 10);
                    age += dt;
                    step++;
                    lowest = minf(lowest, body->GetPosition().y / metres + actors[0].hit.b);
                }
                safe &= supported && lowest >= 79.5f;
                controls &= negative;
                cases++;
                char id[24], row[150];
                idstr(classes[klass], id);
                wsprintfA(row, "%s,%d,%d,%d,%d,%d\r\n", id, rates[rate], direction, supported,
                          (int)(lowest * 1000), negative);
                lstrcatA(rows, row);
                clearOneWayWorld(world);
                delete world;
            }
    check(
        cases == 48 && safe,
        "All four authored walker colliders cross connected clouds safely in both directions across48 step/phase cases");
    check(
        controls,
        "Support handoff rejects actual gaps, unequal top heights, moving platforms and actors entering from below across192 negative controls");
    atomicWrite("test-output/cloud-support-matrix.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/cloud-support-matrix-results.txt", testLog, lstrlenA(testLog));
}
