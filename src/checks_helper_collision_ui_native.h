#pragma once

// Regression coverage for helper collision ui native. Runs only in the isolated BQ_ENABLE_TESTS executable.
class HelperCollisionScopeUiObserverNative : public CreditsContactsNative {
  public:
    int bird, target, begin, pre, disabled, solved;
    b2Body *targetBody;

    HelperCollisionScopeUiObserverNative(UiScene &s, int h, int t)
        : CreditsContactsNative(s), bird(h), target(t), begin(0), pre(0), disabled(0), solved(0),
          targetBody(0) {}

    bool pair(b2Contact *c) {
        b2Body *against = targetBody ? targetBody : scene->bodies[target];
        return c->GetFixtureA()->GetBody() == scene->bodies[bird] &&
                   c->GetFixtureB()->GetBody() == against ||
               c->GetFixtureB()->GetBody() == scene->bodies[bird] &&
                   c->GetFixtureA()->GetBody() == against;
    }

    void BeginContact(b2Contact *c) {
        if (pair(c))
            begin++;
        CreditsContactsNative::BeginContact(c);
    }

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        CreditsContactsNative::PreSolve(c, old);
        if (pair(c)) {
            pre++;
            disabled += !c->IsEnabled();
        }
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        if (pair(c))
            solved++;
        CreditsContactsNative::PostSolve(c, impulse);
    }
};

void helperCollisionScopeUiNativeChecks(int mode = 1) {
    interfaceEnabled = true;
    musicEnabled = soundEnabled = false;
    startStage(0);
    u32 campaign = hashBytes(entities, sizeof(Entity) * entityCount),
        saved = hashBytes(&checkpoint, sizeof(checkpoint));
    Object targetCopy = {};
    char *rows = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 150000);
    lstrcpyA(
        rows,
        "class_id,direction,kind,sensor,authored_filter,oneway,expected_stop,actual_stop,bird_voices,begin,pre,disabled,solved\r\n");
    int represented = 0, cases = 0;
    for (int c = 0; c < classCount; c++) {
        Entity prototype;
        initializeEntity(prototype, classes[c], classes[c].rect);
        if ((!prototype.physics && prototype.kind != COIN && prototype.kind != SECRET &&
             prototype.kind != FORCE && prototype.kind != WATER) ||
            prototype.kind == PLAYER || prototype.kind == HELPER)
            continue;
        bool real = false, contact = true, audio = true;
        int classCases = 0;
        for (int direction = 0; direction < 4; direction++) {
            navigate(137502285691ULL);
            page.level.geometryCount = 0;
            int bird = -1;
            for (int i = 0; i < page.playfieldCount; i++) {
                if (page.playfield[i].kind == HELPER)
                    bird = i;
                else
                    page.playfield[i].state.gone = 1;
            }
            if (bird < 0) {
                contact = audio = false;
                continue;
            }
            destroyUiPhysics(page);
            targetCopy = classes[c];
            Vec origin = center(targetCopy.rect);
            targetCopy.rect = shifted(targetCopy.rect, 500 - origin.x, 400 - origin.y);
            Rect first = targetCopy.rect;
            if (boxGroupMatrix(prototype)) {
                first.r = minf(first.r, first.l + targetCopy.props.num("actlet-width", 32));
                first.t = minf(first.t, first.b + targetCopy.props.num("actlet-height", 32));
            }
            int target = page.playfieldCount++;
            initializeEntity(page.playfield[target], targetCopy, first);
            Entity &actor = page.playfield[target];
            actor.state.gone = 0;
            actor.state.active = actor.crumble ? 0 : 1;
            actor.state.vx = actor.state.vy = 0;
            if (actor.kind == SPIKE || actor.kind == ALTERNATOR)
                actor.state.hp = 1;
            Entity &helper = page.playfield[bird];
            helper.state.gone = 0;
            helper.state.active = mode;
            helper.state.timer = .2f;
            helper.state.vx = helper.state.vy = 0;
            page.bean.state.x = 1500;
            page.bean.state.y = 1500;
            page.bean.state.vx = page.bean.state.vy = 0;
            createUiPhysics(page);
            if (!page.bodies[target])
                continue;
            real = true;
            b2Fixture *main = page.bodies[target]->GetFixtureList();
            for (b2Fixture *f = page.bodies[target]->GetFixtureList(); f; f = f->GetNext())
                if (!f->IsSensor() && f->GetUserData() != (void *)15) {
                    main = f;
                    break;
                }
            b2AABB aabb;
            main->GetShape()->ComputeAABB(&aabb, page.bodies[target]->GetTransform(), 0);
            float x = (aabb.lowerBound.x + aabb.upperBound.x) * .5f / metres,
                  y = (aabb.lowerBound.y + aabb.upperBound.y) * .5f / metres;
            if (direction == 0)
                x = aabb.upperBound.x / metres - helper.hit.l - .25f;
            if (direction == 1)
                x = aabb.lowerBound.x / metres - helper.hit.r + .25f;
            if (direction == 2)
                y = aabb.lowerBound.y / metres - helper.hit.t + .25f;
            if (direction == 3)
                y = aabb.upperBound.y / metres - helper.hit.b - .25f;
            helper.state.x = x;
            helper.state.y = y;
            page.bodies[bird]->SetTransform(b2Vec2(x * metres, y * metres), 0);
            page.bodies[bird]->SetLinearVelocity(b2Vec2(0, 0));
            page.bodies[page.playfieldCount]->SetType(b2_staticBody);
            for (b2Fixture *f = page.bodies[page.playfieldCount]->GetFixtureList(); f;
                 f = f->GetNext()) {
                b2Filter filter = f->GetFilterData();
                filter.maskBits = 0;
                f->SetFilterData(filter);
            }
            delete page.contacts;
            HelperCollisionScopeUiObserverNative *probe =
                new HelperCollisionScopeUiObserverNative(page, bird, target);
            page.contacts = probe;
            page.physics->SetContactListener(probe);
            bool authoredFilter = actor.kind == BOUNCER || !actor.physics ||
                                  actor.physics->flag("collide-other", true);
            bool oneWay = main->GetUserData() == (void *)1 || main->GetUserData() == (void *)2;
            bool hazard =
                actor.kind == SPIKE && actor.source->has("SpikeGroupBehaviour") && actor.state.hp;
            bool expected = mode == 1 && authoredFilter && (!main->IsSensor() || hazard) &&
                            (!oneWay || direction == 3);
            stopSounds();
            soundEnabled = true;
            stepUiPhysics(page, 1.0f / 120);
            creditsHelperPostStep(page, 1.0f / 120);
            int voices = helperCollisionScopeBirdVoicesNative();
            bool stopped = mode == 1 && helper.state.active == 2;
            contact &= (mode == 1 ? stopped == expected
                                  : helper.state.active == mode && probe->solved == 0);
            audio &= voices == (expected ? 1 : 0);
            creditsHelperPostStep(page, 0);
            audio &= voices == helperCollisionScopeBirdVoicesNative();
            soundEnabled = false;
            char row[280];
            wsprintfA(row, "%I64u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", classes[c].id,
                      direction, actor.kind, main->IsSensor(), authoredFilter, oneWay, expected,
                      stopped, voices, probe->begin, probe->pre, probe->disabled, probe->solved);
            if (lstrlenA(rows) < 149000)
                lstrcatA(rows, row);
            cases++;
            classCases++;
            stopSounds();
        }
        if (real) {
            represented++;
            char label[250];
            wsprintfA(
                label,
                "Independent UI helper mode%d collider class%I64u preserves the original filter, sensor and one-way decisions in all four directions",
                mode, classes[c].id);
            check(contact && classCases == 4, label);
            wsprintfA(
                label,
                "Independent UI helper mode%d collider class%I64u plays one accepted impact and preserves silent pass-through without repeat voices",
                mode, classes[c].id);
            check(audio && classCases == 4, label);
        }
    }
    char matrixPath[120];
    if (mode == 1)
        lstrcpyA(matrixPath, "test-output/helper-collision-ui-matrix.csv");
    else
        wsprintfA(matrixPath, "test-output/helper-ui-nonflight-mode%d-matrix.csv", mode);
    atomicWrite(matrixPath, rows, lstrlenA(rows));
    char summary[200];
    wsprintfA(summary, "runtime_classes=%d cases=%d\r\n", represented, cases);
    char summaryPath[120];
    if (mode == 1)
        lstrcpyA(summaryPath, "test-output/helper-collision-ui-summary.txt");
    else
        wsprintfA(summaryPath, "test-output/helper-ui-nonflight-mode%d-summary.txt", mode);
    atomicWrite(summaryPath, summary, lstrlenA(summary));
    check(
        represented > 50 && cases == represented * 4 &&
            campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
            saved == hashBytes(&checkpoint, sizeof(checkpoint)),
        "The complete independent UI collider matrix leaves campaign actors and checkpoint bytes unchanged");
    HeapFree(GetProcessHeap(), 0, rows);
    interfaceEnabled = musicEnabled = soundEnabled = false;
    startStage(0);
}
