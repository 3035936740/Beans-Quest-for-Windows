#pragma once

// Regression coverage for creature support native. Runs only in the isolated BQ_ENABLE_TESTS executable.
#include "checks_creature_crush_matrix.h"

class CreatureSeamProbeNative : public OriginalContacts {
  public:
    int step, rate, category;
    char *rows;

    CreatureSeamProbeNative(char *out, int hz, int cat)
        : step(0), rate(hz), category(cat), rows(out) {}

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        OriginalContacts::PreSolve(c, old);
        b2Fixture *f[] = {c->GetFixtureA(), c->GetFixtureB()};
        if (c->IsEnabled() || !c->GetManifold()->pointCount || f[0]->IsSensor() || f[1]->IsSensor())
            return;
        for (int side = 0; side < 2; side++) {
            int a = (int)f[side]->GetBody()->GetUserData() - 1,
                b = (int)f[1 - side]->GetBody()->GetUserData() - 1;
            if (a < 0 || a >= entityCount || b < 0 || b >= entityCount || !entities[a].walk ||
                !entities[b].crumble)
                continue;
            float bottom = f[side]->GetBody()->GetPosition().y / metres + entities[a].hit.b;
            if (absf(bottom - 80) > 4)
                continue;
            b2WorldManifold m;
            c->GetWorldManifold(&m);
            char row[256];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", rate, category, step, a,
                      (int)(bottom * 100000),
                      (int)(f[side]->GetBody()->GetLinearVelocity().y / metres * 100000),
                      (int)((side ? m.normal.y : -m.normal.y) * 100000),
                      (int)(m.points[0].y / metres * 100000),
                      (int)(m.points[1].y / metres * 100000), c->GetManifold()->pointCount);
            if (lstrlenA(rows) < 1999500)
                lstrcatA(rows, row);
        }
    }
};

void creatureSupportNativeChecks() {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    debugMode = false;
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/creature-support-saves");
    CreateDirectoryA(saveRoot, 0);
    char *rows = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 2000000);
    lstrcpyA(
        rows,
        "rate,otherCategory,step,actor,bottom100000,vy100000,ny100000,p0y100000,p1y100000,points\r\n");
    char summary[50000] = "rate,otherCategory,delay,middleCrossings,firstStep\r\n";
    int crossings = 0, cases = 0;
    bool intact = true;
    const int rates[] = {30, 60, 120, 144, 240};
    for (int r = 0; r < 5; r++)
        for (int category = 1; category <= 9; category++)
            for (int delay = 0; delay < 3; delay++) {
                int selected[3], boss = prepareWizardWalkSpawnNative(1, 2, selected);
                if (boss < 0) {
                    check(false, "Mixed-wave diagnostic resolves Wizard arena");
                    continue;
                }
                int peers[3], peerCount = 0;
                for (int i = 0; i < entityCount; i++)
                    if (entities[i].bossSpawn == category && peerCount < 3)
                        peers[peerCount++] = i;
                CreatureSeamProbeNative *probe =
                    new CreatureSeamProbeNative(rows, rates[r], category);
                physicsWorld->SetContactListener(probe);
                float previous[3];
                bool landed[3] = {}, risk[3] = {}, confirmed[3] = {};
                for (int n = 0; n < 3; n++)
                    previous[n] = entities[selected[n]].state.y + entities[selected[n]].hit.b;
                int local = 0, first = -1;
                for (int step = 0; step < rates[r] * 12; step++) {
                    probe->step = step;
                    if (category != 2 && step == delay * rates[r] && peerCount == 3) {
                        wizardMotion.pendingCount = 3;
                        for (int n = 0; n < 3; n++)
                            wizardMotion.pending[n] = peers[n];
                        wizardMaterializeSummons(entities[boss]);
                    }
                    tick(1.0f / rates[r], false, false);
                    for (int n = 0; n < 3; n++) {
                        Entity &e = entities[selected[n]];
                        float bottom = e.state.y + e.hit.b;
                        bool row = e.state.x + e.hit.l > 4288.1f && e.state.x + e.hit.r < 4575.9f;
                        for (b2ContactEdge *edge = bodies[selected[n]]->GetContactList(); edge;
                             edge = edge->next) {
                            int i = (int)edge->other->GetUserData() - 1;
                            b2Contact *c = edge->contact;
                            if (i >= 0 && i < entityCount && entities[i].crumble &&
                                c->IsEnabled() && c->IsTouching() &&
                                !c->GetFixtureA()->IsSensor() && !c->GetFixtureB()->IsSensor()) {
                                b2WorldManifold m;
                                c->GetWorldManifold(&m);
                                if ((c->GetFixtureA()->GetBody() == bodies[selected[n]]
                                         ? -m.normal.y
                                         : m.normal.y) > .95f)
                                    landed[n] = true;
                            }
                        }
                        if (row && landed[n] && previous[n] >= 79.9f && bottom < 79.9f &&
                            !e.state.gone)
                            risk[n] = true;
                        if (!row || bottom > 80)
                            risk[n] = false;
                        if (risk[n] && !confirmed[n] && bottom < 64 && !e.state.gone) {
                            confirmed[n] = true;
                            local++;
                            if (first < 0)
                                first = step;
                        }
                        previous[n] = bottom;
                    }
                }
                for (int i = 0; i < entityCount; i++)
                    if (entities[i].crumble)
                        intact &= entities[i].state.active != 2;
                char line[160];
                wsprintfA(line, "%d,%d,%d,%d,%d\r\n", rates[r], category, delay, local, first);
                lstrcatA(summary, line);
                crossings += local;
                cases++;
                atomicWrite("test-output/creature-seam-progress.csv", summary, lstrlenA(summary));
                physicsWorld->SetContactListener(originalContacts);
                delete probe;
            }
    atomicWrite("test-output/creature-seam-refused.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/creature-seam-summary.csv", summary, lstrlenA(summary));
    HeapFree(GetProcessHeap(), 0, rows);
    check(
        cases == 135 && intact,
        "All135 mixed Wizard wave contexts preserve continuous cloud fixtures at30/60/120/144/240Hz");
    check(crossings == 0,
          "Mixed-wave blue walkers never cross from above to below the intact interior cloud row");
    char *carrierRows = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 50000);
    lstrcpyA(carrierRows,
             "class,rate,path,step,x100,y100,vy100,age1000,grounded,launches,penetration100\r\n");
    const u64 jumperClasses[] = {15190319732989319544ULL, 6425677159578671338ULL,
                                 3692840978977075ULL};
    bool launchedAll = true, wallSafe = true;
    int carrierCases = 0;
    for (int klass = 0; klass < 3; klass++)
        for (int r = 0; r < 3; r++)
            for (int path = 0; path < 4; path++) {
                int selected[3], boss = prepareWizardWalkSpawnNative(1, 2, selected),
                                 target = selected[0], fishIndex = selected[1];
                if (boss < 0)
                    continue;
                for (int i = 0; i < entityCount; i++)
                    if (i != playerIndex && i != target && i != fishIndex)
                        entities[i].state.gone = 1;
                Object *source = classById(jumperClasses[klass]);
                Object *fishSource = classById(706166303646ULL);
                initializeEntity(entities[target], *source, source->rect);
                initializeEntity(entities[fishIndex], *fishSource, fishSource->rect);
                Entity &e = entities[target];
                Entity &fish = entities[fishIndex];
                e.state.active = fish.state.active = 1;
                fish.state.x = 0;
                fish.state.y = path == 3 ? 180.0f : 80.0f;
                e.state.x = 0;
                e.state.y = fish.state.y + fish.hit.t - e.hit.b + .65f;
                e.state.vx = e.state.vy = e.state.aux = 0;
                current.geometryCount = 0;
                rebuildPhysics();
                resetNativeMotion(fishIndex, false);
                DiagonalRuntimeState &d = diagonalRuntime[fishIndex];
                d.selected = 1;
                d.effectiveSpeed = 100;
                d.directionX = path == 0   ? 1
                               : path == 1 ? .707106781f
                               : path == 2 ? -.707106781f
                                           : 0;
                d.directionY = path == 0 ? 0 : path == 3 ? 1 : .707106781f;
                b2BodyDef bd;
                b2Body *room = physicsWorld->CreateBody(&bd);
                b2Vec2 points[] = {b2Vec2(-120 * metres, 0), b2Vec2(120 * metres, 0),
                                   b2Vec2(120 * metres, 220 * metres),
                                   b2Vec2(-120 * metres, 220 * metres)};
                b2ChainShape chain;
                chain.CreateLoop(points, 4);
                b2FixtureDef fd;
                fd.shape = &chain;
                fd.filter.categoryBits = 1;
                room->CreateFixture(&fd);
                entities[playerIndex].state.x = 0;
                entities[playerIndex].state.y = 1000;
                bodies[playerIndex]->SetType(b2_staticBody);
                bodies[playerIndex]->SetTransform(b2Vec2(0, 1000 * metres), 0);
                int hz = r == 0 ? 30 : r == 1 ? 120 : 240, launches = 0;
                bool touched = false;
                float worst = 0;
                for (int step = 0; step < hz * 4; step++) {
                    bool launch =
                        jumperNativeGrounded(target) &&
                        e.state.aux >
                            e.source->behavior("EnemyJumperBehaviour")->num("jump-delay", 1);
                    bool onFish = false;
                    for (JumperGroundFixture *g = jumperGroundFixtures[target]; g; g = g->next)
                        onFish |= g->fixture->GetBody() == bodies[fishIndex];
                    tick(1.0f / hz, false, false);
                    launches += launch && onFish;
                    touched |= onFish;
                    float penetration =
                        maxf(maxf(e.state.x + e.hit.r - 120, -120 - e.state.x - e.hit.l),
                             e.state.y + e.hit.t - 220);
                    if (!e.state.gone)
                        worst = maxf(worst, penetration);
                    if (step % hz == 0 || (!e.state.gone && penetration > 2)) {
                        char row[200];
                        wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", klass, hz, path,
                                  step, (int)(e.state.x * 100), (int)(e.state.y * 100),
                                  (int)(e.state.vy * 100), (int)(e.state.aux * 1000),
                                  jumperNativeGrounded(target), launches, (int)(penetration * 100));
                        if (lstrlenA(carrierRows) < 49500)
                            lstrcatA(carrierRows, row);
                    }
                }
                launchedAll &= path == 3   ? (touched || e.state.gone != 0)
                               : path == 0 ? touched
                                           : touched && launches > 0;
                wallSafe &= worst < 2;
                carrierCases++;
            }
    atomicWrite("test-output/creature-carrier-observations.csv", carrierRows,
                lstrlenA(carrierRows));
    HeapFree(GetProcessHeap(), 0, carrierRows);
    check(
        carrierCases == 36 && launchedAll,
        "All3authored jumper species acquire fish support and launch during sustained vertical carrying; a faster horizontal carrier may move away before the native delay");
    check(
        wallSafe,
        "A crushed killable jumper dies through the shared death queue before it can remain alive outside the room");
    creatureCrushMatrixChecks();
    creatureCrushUiChecks();
    atomicWrite("test-output/creature-support-results.txt", testLog, lstrlenA(testLog));
    lstrcpyA(saveRoot, oldRoot);
    startStage(0);
    readyTimer = 0;
}
