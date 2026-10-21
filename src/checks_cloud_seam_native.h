#pragma once

// Regression coverage for cloud seam native. Runs only in the isolated BQ_ENABLE_TESTS executable.

class CloudSeamContactProbeNative : public OriginalContacts {
  public:
    int target, step;
    char *rows;

    CloudSeamContactProbeNative(int index) : target(index), step(0) {
        rows = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 300000);
        lstrcpyA(
            rows,
            "phase,step,other,kind,crumble,enabled,points,vy1000,otherVy1000,bottom1000,normalY1000,minPointY1000,physicsVyBits\r\n");
    }

    void record(b2Contact *c, int phase) {
        if (step < 160 || step > 180 || c->GetFixtureA()->IsSensor() ||
            c->GetFixtureB()->IsSensor())
            return;
        b2Fixture *f[] = {c->GetFixtureA(), c->GetFixtureB()};
        int side = f[0]->GetBody() == bodies[target]   ? 0
                   : f[1]->GetBody() == bodies[target] ? 1
                                                       : -1;
        if (side < 0)
            return;
        b2Body *body = f[side]->GetBody();
        int other = (int)f[1 - side]->GetBody()->GetUserData() - 1;
        b2WorldManifold m;
        c->GetWorldManifold(&m);
        float y = 10000;
        for (int i = 0; i < c->GetManifold()->pointCount; i++)
            y = minf(y, m.points[i].y / metres);

        union {
            float value;
            unsigned bits;
        } vy;

        vy.value = body->GetLinearVelocity().y;
        char row[256];
        wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%08x\r\n", phase, step, other,
                  other >= 0 && other < entityCount ? entities[other].kind : -1,
                  other >= 0 && other < entityCount ? entities[other].crumble : 0, c->IsEnabled(),
                  c->GetManifold()->pointCount, (int)(body->GetLinearVelocity().y / metres * 1000),
                  (int)(f[1 - side]->GetBody()->GetLinearVelocity().y / metres * 1000),
                  (int)((body->GetPosition().y / metres + entities[target].hit.b) * 1000),
                  (int)((side ? m.normal.y : -m.normal.y) * 1000), (int)(y * 1000), vy.bits);
        if (lstrlenA(rows) < 299700)
            lstrcatA(rows, row);
    }

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        record(c, 0);
        OriginalContacts::PreSolve(c, old);
        record(c, 1);
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        OriginalContacts::PostSolve(c, impulse);
        record(c, 2);
    }
};

// Observe genuine Wizard births on the unmodified continuous cloud row.
// A spring launch and an entry from underneath are not a top-to-bottom fall.
bool cloudRestingClassContactNative(u64 klass, int mode) {
    Entity actors[2];
    Object *enemy = classById(klass);
    Object *cloud = classById(14815823507569313204ULL);
    if (!enemy || !cloud)
        return false;
    initializeEntity(actors[0], *enemy, enemy->rect);
    initializeEntity(actors[1], *cloud, cloud->rect);
    actors[0].state.active = 1;
    actors[0].state.x = actors[1].state.x = 0;
    actors[1].state.y = 80;
    actors[0].state.y = 80 + actors[1].hit.t - actors[0].hit.b + (mode == 2 ? -.01f : .64f);
    actors[0].state.vx = 0;
    actors[0].state.vy = (mode == 1   ? 2 * b2_epsilon
                          : mode == 2 ? -b2_epsilon
                                      : 8.540050872013873e-18f) /
                         metres;
    b2World *world = new b2World(b2Vec2(0, 0), true);
    OneWayNativeTestContacts listener(actors, 2);
    world->SetContactListener(&listener);
    b2Body *platform = createPhysicsEntity(*world, actors[1], 1);
    b2Body *body = createPhysicsEntity(*world, actors[0], 0);
    bool touched = false, enabled = false;
    if (platform && body) {
        world->Step(1 / 120.0f, 10, 10);
        for (b2ContactEdge *edge = body->GetContactList(); edge; edge = edge->next) {
            b2Contact *c = edge->contact;
            if (edge->other == platform && c->IsTouching() && !c->GetFixtureA()->IsSensor() &&
                !c->GetFixtureB()->IsSensor() && c->GetManifold()->pointCount) {
                touched = true;
                enabled |= c->IsEnabled();
            }
        }
    }
    clearOneWayWorld(world);
    delete world;
    return touched && (mode ? !enabled : enabled);
}

void cloudSeamNativeChecks() {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    debugMode = false;
    check(
        nativeOneWayFace(1, oneWaySolverVelocity(8.540050872013873e-18f)) &&
            nativeOneWayFace(1, oneWaySolverVelocity(b2_epsilon)) &&
            !nativeOneWayFace(1, oneWaySolverVelocity(2 * b2_epsilon)) &&
            !nativeOneWayFace(0, oneWaySolverVelocity(0)) &&
            !nativeOneWayFace(1, oneWaySolverVelocity(5 * metres)),
        "Windows resting-velocity roundoff is zero only within float epsilon; actual upward motion and lateral faces remain nonblocking");
    bool tiny = true, rising = true, below = true;
    const u64 classes[] = {14264481342587988014ULL, 13516539842961264564ULL, 970132910526ULL,
                           3216410667731258ULL};
    for (int i = 0; i < 4; i++) {
        tiny &= cloudRestingClassContactNative(classes[i], 0);
        rising &= cloudRestingClassContactNative(classes[i], 1);
        below &= cloudRestingClassContactNative(classes[i], 2);
    }
    check(
        tiny,
        "All four original Walk classes keep actual cloud main-fixture contacts enabled with the reproduced resting residue");
    check(
        rising,
        "All four original Walk classes still pass through a cloud with a real positive velocity outside machine epsilon");
    check(
        below,
        "All four original Walk classes retain the refused-below contact latch instead of becoming solid from underneath");
    char *rows = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 4000000);
    lstrcpyA(
        rows,
        "category,rate,mode,member,step,x1000,bottom1000,vy1000,previousBottom1000,gone,clouds,allowed,refused,mainTouch,mainEnabled\r\n");
    bool intact = true;
    int crossings = 0, cases = 0, supported = 0;
    const int rates[] = {30, 60, 120, 240, 0};
    for (int category = 2; category <= 8; category += 6)
        for (int rate = 0; rate < 5; rate++)
            for (int mode = 0; mode < 4; mode++) {
                int selected[3], boss = prepareWizardWalkSpawnNative(1, category, selected);
                if (boss < 0) {
                    check(
                        false,
                        "Actual Wizard cloud-seam observation resolves three original summon members");
                    continue;
                }
                bool observed[3] = {};
                float previousBottom[3], previousX[3];
                bool landed[3] = {};
                for (int n = 0; n < 3; n++) {
                    Entity &e = entities[selected[n]];
                    observed[n] = !mode || mode == n + 1;
                    previousBottom[n] = e.state.y + e.hit.b;
                    previousX[n] = e.state.x;
                    if (!observed[n]) {
                        e.state.gone = 1;
                        bodies[selected[n]]->SetActive(false);
                    }
                }
                CloudSeamContactProbeNative *probe = 0;
                if (category == 2 && rate == 1 && mode == 0) {
                    probe = new CloudSeamContactProbeNative(selected[1]);
                    physicsWorld->SetContactListener(probe);
                }
                float age = 0;
                int steps = 0;
                while (age < 20) {
                    float dt = rate == 4 ? (steps % 3 == 0   ? 1 / 30.0f
                                            : steps % 3 == 1 ? 1 / 144.0f
                                                             : 1 / 60.0f)
                                         : 1.0f / rates[rate];
                    if (probe)
                        probe->step = steps;
                    tick(dt, false, false);
                    age += dt;
                    for (int n = 0; n < 3; n++)
                        if (observed[n]) {
                            int target = selected[n];
                            Entity &e = entities[target];
                            float bottom = e.state.y + e.hit.b;
                            bool inRow = true;
                            int clouds = 0, allowed = 0, refused = 0, touch = 0, enabled = 0;
                            // The actor's full main-fixture width must remain inside the
                            // uninterrupted row to exclude the intentional spring edge.
                            float left = e.state.x + e.hit.l, right = e.state.x + e.hit.r;
                            inRow = left > 4288.1f && right < 4575.9f &&
                                    previousX[n] + e.hit.l > 4288.1f &&
                                    previousX[n] + e.hit.r < 4575.9f;
                            for (int i = 0; i < entityCount; i++)
                                if (entities[i].crumble) {
                                    Rect r = hitRect(entities[i]);
                                    if (r.l >= 4288 && r.r <= 4576 && r.t > 79.9f && r.t < 80.1f) {
                                        clouds++;
                                        intact &= entities[i].state.active != 2 && bodies[i] &&
                                                  bodies[i]->IsActive();
                                    }
                                }
                            for (int i = 0; i < oneWayEntryLimit; i++)
                                if (oneWayEntries[i].contact &&
                                    oneWayEntries[i].other == bodies[target]) {
                                    allowed += oneWayEntries[i].above;
                                    refused += !oneWayEntries[i].above;
                                }
                            for (b2ContactEdge *edge = bodies[target]->GetContactList(); edge;
                                 edge = edge->next) {
                                b2Contact *c = edge->contact;
                                int other = (int)edge->other->GetUserData() - 1;
                                if (other < 0 || other >= entityCount || !entities[other].crumble ||
                                    c->GetFixtureA()->IsSensor() || c->GetFixtureB()->IsSensor() ||
                                    !c->IsTouching())
                                    continue;
                                touch++;
                                enabled += c->IsEnabled();
                                if (c->IsEnabled() && c->GetManifold()->pointCount)
                                    landed[n] = true;
                            }
                            bool crossing = inRow && landed[n] && previousBottom[n] >= 79.9f &&
                                            bottom < 79.9f && !e.state.gone;
                            if (crossing)
                                crossings++;
                            if (steps % 8 == 0 || crossing) {
                                char row[256];
                                wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
                                          category, rates[rate], mode, n, steps,
                                          (int)(e.state.x * 1000), (int)(bottom * 1000),
                                          (int)(e.state.vy * 1000), (int)(previousBottom[n] * 1000),
                                          e.state.gone, clouds, allowed, refused, touch, enabled);
                                if (lstrlenA(rows) < 3999600)
                                    lstrcatA(rows, row);
                            }
                            previousBottom[n] = bottom;
                            previousX[n] = e.state.x;
                        }
                    steps++;
                }
                if (probe) {
                    physicsWorld->SetContactListener(originalContacts);
                    atomicWrite("test-output/cloud-seam-contact-events.csv", probe->rows,
                                lstrlenA(probe->rows));
                    HeapFree(GetProcessHeap(), 0, probe->rows);
                    delete probe;
                }
                for (int n = 0; n < 3; n++)
                    if (observed[n])
                        supported += landed[n];
                cases++;
            }
    char summary[256];
    wsprintfA(summary, "contexts=%d supported_members=%d middle_crossings=%d clouds_intact=%d\r\n",
              cases, supported, crossings, intact);
    atomicWrite("test-output/cloud-seam-summary.txt", summary, lstrlenA(summary));
    atomicWrite("test-output/cloud-seam-observations.csv", rows, lstrlenA(rows));
    HeapFree(GetProcessHeap(), 0, rows);
    check(
        intact && cases == 40,
        "Forty actual solo/wave cloud-seam contexts retain every authored continuous cloud at30/60/120/240Hz and variable frame durations");
    check(
        crossings == 0,
        "Actual blue and turtle Wizard walkers never cross downward through the middle of an intact continuous cloud row after landing");
    atomicWrite("test-output/cloud-seam-results.txt", testLog, lstrlenA(testLog));
    startStage(0);
    readyTimer = 0;
}
