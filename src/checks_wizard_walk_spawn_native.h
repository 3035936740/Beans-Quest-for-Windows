#pragma once

// Regression coverage for wizard walk spawn native. Runs only in the isolated BQ_ENABLE_TESTS executable.

class WizardWalkSpawnProbeNative : public OriginalContacts {
  public:
    int targets[3];
    bool solved[3];

    WizardWalkSpawnProbeNative(int *selected) {
        for (int i = 0; i < 3; i++)
            targets[i] = selected[i];
        reset();
    }

    void reset() {
        memset(solved, 0, sizeof(solved));
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        OriginalContacts::PostSolve(c, impulse);
        if (!c->IsEnabled() || !c->GetManifold()->pointCount || c->GetFixtureA()->IsSensor() ||
            c->GetFixtureB()->IsSensor())
            return;
        int a = (int)c->GetFixtureA()->GetBody()->GetUserData() - 1,
            b = (int)c->GetFixtureB()->GetBody()->GetUserData() - 1;
        for (int n = 0; n < 3; n++)
            if (a == targets[n] || b == targets[n])
                solved[n] = true;
    }
};

int prepareWizardWalkSpawnNative(int area, int category, int *selected) {
    int unused[3], boss = batSmokePrepareNative(area, unused), count = 0;
    wizardMotion.pendingCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].bossSpawn == category && count < 3) {
            selected[count++] = i;
            wizardMotion.pending[wizardMotion.pendingCount++] = i;
        }
    if (boss < 0 || count != 3)
        return -1;
    wizardMaterializeSummons(entities[boss]);
    entities[boss].state.gone = 1;
    State &bean = entities[playerIndex].state;
    bean.x = wizardArena(entities[boss]);
    bean.y = 1000;
    bean.vx = bean.vy = 0;
    bodies[playerIndex]->SetType(b2_staticBody);
    bodies[playerIndex]->SetTransform(b2Vec2(bean.x * metres, bean.y * metres), 0);
    return boss;
}

void wizardWalkSpawnNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/wizard-walk-spawn-saves");
    CreateDirectoryA(saveRoot, 0);
    bool classes = true, birth = true, fall = true, reuse = true, restore = true;
    int members = 0, samples = 0;
    char rows[16000] = "arena,class,member,step,x1000,vx1000,mainSolved,everSolved\r\n";
    for (int area = 0; area < 5; area++)
        for (int category = 2; category <= 8; category += 6) {
            int selected[3], boss = prepareWizardWalkSpawnNative(area, category, selected);
            if (boss < 0) {
                classes = false;
                continue;
            }
            float x[3];
            bool seen[3] = {};
            for (int n = 0; n < 3; n++) {
                Entity &e = entities[selected[n]];
                x[n] = e.state.x;
                classes &= e.walk && e.speed == (category == 2 ? 75 : 150);
                birth &= e.state.vx == 0 && e.state.vy == 0 &&
                         bodies[selected[n]]->GetLinearVelocity().x == 0 &&
                         bodies[selected[n]]->GetLinearVelocity().y == 0 && !e.walkCanWalk;
                members++;
            }
            if (area == 1 && category == 2) {
                captureCheckpoint(false);
                for (int n = 0; n < 3; n++) {
                    entities[selected[n]].state.vx = 123;
                    entities[selected[n]].walkCanWalk = 1;
                }
                restoreCheckpoint();
                bodies[playerIndex]->SetType(b2_staticBody);
                for (int n = 0; n < 3; n++)
                    restore &= entities[selected[n]].state.vx == 0 &&
                               !entities[selected[n]].walkCanWalk &&
                               bodies[selected[n]]->GetLinearVelocity().x == 0;
            }
            WizardWalkSpawnProbeNative *probe = new WizardWalkSpawnProbeNative(selected);
            physicsWorld->SetContactListener(probe);
            for (int step = 0; step < 12; step++) {
                probe->reset();
                tick(1 / 120.0f, false, false);
                for (int n = 0; n < 3; n++) {
                    Entity &e = entities[selected[n]];
                    seen[n] |= probe->solved[n];
                    if (!seen[n] && !e.state.gone) {
                        fall &= absf(e.state.x - x[n]) < .001f && absf(e.state.vx) < .001f;
                        samples++;
                    }
                    char row[180];
                    wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d\r\n", area, category, n, step,
                              (int)(e.state.x * 1000), (int)(e.state.vx * 1000), probe->solved[n],
                              seen[n]);
                    lstrcatA(rows, row);
                }
            }
            physicsWorld->SetContactListener(originalContacts);
            delete probe;
            wizardMotion.pendingCount = 3;
            for (int n = 0; n < 3; n++) {
                int i = selected[n];
                wizardMotion.pending[n] = i;
                Entity &e = entities[i];
                e.state.gone = 1;
                e.state.vx = 123;
                e.state.vy = -41;
                e.walkCanWalk = 1;
                e.walkRight = 0;
                bodies[i]->SetActive(false);
                bodies[i]->SetLinearVelocity(b2Vec2(123 * metres, -41 * metres));
            }
            wizardMaterializeSummons(entities[boss]);
            for (int n = 0; n < 3; n++) {
                Entity &e = entities[selected[n]];
                reuse &= e.state.vx == 0 && e.state.vy == 0 && !e.walkCanWalk &&
                         e.walkRight == !e.source->behavior("WalkBehaviour")->flag("walk-left") &&
                         bodies[selected[n]]->GetLinearVelocity().x == 0;
            }
        }
    check(
        classes && members == 30,
        "All30actual Wizard beetle/turtle members across5arenas use their authored75/150walking speeds");
    check(
        birth,
        "All30actual Wizard walkers materialize with native zero actor/body velocity and no inherited walking permission");
    check(
        fall && samples >= 30,
        "Real unopposed summon falls preserve horizontal position until an actual solved contact instead of starting walking in midair");
    check(
        reuse,
        "Every reused Wizard walker clears old velocity, permission and direction through the original zero-velocity spawn path");
    check(
        restore,
        "A checkpoint captured at actual walker materialization restores zero horizontal motion without inventing a supporting contact");
    atomicWrite("test-output/wizard-walk-spawn-observations.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/wizard-walk-spawn-results.txt", testLog, lstrlenA(testLog));
    lstrcpyA(saveRoot, oldRoot);
    startStage(0);
    readyTimer = 0;
}
