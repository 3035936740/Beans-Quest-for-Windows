#pragma once

// Regression coverage for magnet solved native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int magnetSolvedActiveCellsNative(Entity *actors, int count, Object *source) {
    int active = 0;
    for (int i = 0; i < count; i++)
        if (actors[i].source == source && actors[i].state.active == 2)
            active++;
    return active;
}

void magnetSolvedResetCellsNative(Entity *actors, int count, Object *source) {
    for (int i = 0; i < count; i++)
        if (actors[i].source == source)
            actors[i].state.active = 1;
}

class MagnetSolvedObserverNative : public OriginalContacts {
  public:
    int magnet, solved;
    bool block;
    b2Contact *last;

    explicit MagnetSolvedObserverNative(int index)
        : magnet(index), solved(0), block(false), last(0) {}

    bool pair(b2Contact *c) {
        b2Body *a = c->GetFixtureA()->GetBody();
        b2Body *b = c->GetFixtureB()->GetBody();
        return (a == bodies[magnet] && b == bodies[playerIndex]) ||
               (b == bodies[magnet] && a == bodies[playerIndex]);
    }

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        OriginalContacts::PreSolve(c, old);
        if (pair(c) && block)
            c->SetEnabled(false);
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        if (pair(c)) {
            solved++;
            last = c;
        }
        OriginalContacts::PostSolve(c, impulse);
    }
};

int magnetSolvedPrepareNative() {
    interfaceEnabled = musicEnabled = soundEnabled = debugMode = false;
    startStage(motionStage(5, 4));
    readyTimer = 0;
    int first = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == MAGNET && entities[i].source->id == 4715698496963111ULL &&
            magnetMatrixOwner(entities[i]))
            first = i;
    if (first < 0)
        return -1;
    Object *source = entities[first].source;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && entities[i].source != source)
            entities[i].state.gone = 1;
    State &bean = entities[playerIndex].state;
    bean.x = 720;
    bean.y = 224 - entities[playerIndex].hit.b + .1f;
    bean.vx = 0;
    bean.vy = -10;
    bean.active = 1;
    rebuildPhysics();
    return first;
}

class UiMagnetSolvedObserverNative : public UiContacts {
  public:
    int magnet, solved;
    bool suppress;

    UiMagnetSolvedObserverNative(UiScene &s, int index, bool omit)
        : UiContacts(s), magnet(index), solved(0), suppress(omit) {}

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        b2Body *a = c->GetFixtureA()->GetBody();
        b2Body *b = c->GetFixtureB()->GetBody();
        bool pair = (a == scene->bodies[magnet] && b == scene->bodies[scene->playfieldCount]) ||
                    (b == scene->bodies[magnet] && a == scene->bodies[scene->playfieldCount]);
        if (pair) {
            solved++;
            if (suppress)
                return;
        }
        UiContacts::PostSolve(c, impulse);
    }
};

void magnetSolvedNativeChecks() {
    const float dt = 1 / 120.0f;
    bool actual = true, repeat = true, sleep = true, restored = true, transient = true,
         filters = true;
    char trace[2048] = "case,solved,magnet_flag,active_cells,bean_active\r\n";
    for (int mode = 0; mode < 7; mode++) {
        int first = magnetSolvedPrepareNative();
        if (first < 0) {
            actual = false;
            continue;
        }
        Object *source = entities[first].source;
        MagnetSolvedObserverNative observer(first);
        physicsWorld->SetContactListener(&observer);
        if (mode == 5)
            observer.block = true;
        if (mode == 6)
            bodies[first]->GetFixtureList()->SetSensor(true);
        simulateBodies(dt);
        bool accepted = observer.solved > 0 && playerMagnetContact &&
                        entities[playerIndex].state.active == 2 &&
                        magnetSolvedActiveCellsNative(entities, entityCount, source) > 0;
        if (mode < 5)
            actual &= accepted;
        if (mode == 1) {
            mechanismsPreStep();
            magnetSolvedResetCellsNative(entities, entityCount, source);
            entities[playerIndex].state.active = 1;
            mechanismsPostStep(dt);
            repeat &= !playerMagnetContact && entities[playerIndex].state.active == 1 &&
                      magnetSolvedActiveCellsNative(entities, entityCount, source) == 0;
        }
        if (mode == 2) {
            mechanismsPreStep();
            magnetSolvedResetCellsNative(entities, entityCount, source);
            entities[playerIndex].state.active = 1;
            entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
            bodies[playerIndex]->SetLinearVelocity(b2Vec2(0, 0));
            bodies[playerIndex]->SetSleepingAllowed(true);
            bodies[playerIndex]->SetAwake(false);
            bodies[first]->SetSleepingAllowed(true);
            bodies[first]->SetAwake(false);
            observer.solved = 0;
            simulateBodies(dt);
            mechanismsPostStep(dt);
            sleep &= observer.solved == 0 && !playerMagnetContact &&
                     entities[playerIndex].state.active == 1 &&
                     magnetSolvedActiveCellsNative(entities, entityCount, source) == 0;
        }
        if (mode == 3) {
            captureCheckpoint(false);
            restoreCheckpoint();
            mechanismsPreStep();
            magnetSolvedResetCellsNative(entities, entityCount, source);
            entities[playerIndex].state.active = 1;
            mechanismsPostStep(dt);
            restored &= !playerMagnetContact && entities[playerIndex].state.active == 1 &&
                        magnetSolvedActiveCellsNative(entities, entityCount, source) == 0;
        }
        if (mode == 4) {
            if (observer.last)
                observer.last->SetEnabled(false);
            mechanismsPostStep(dt);
            transient &= accepted && playerMagnetContact &&
                         entities[playerIndex].state.active == 2 &&
                         magnetSolvedActiveCellsNative(entities, entityCount, source) > 0;
        }
        if (mode >= 5) {
            mechanismsPostStep(dt);
            filters &= observer.solved == 0 && !playerMagnetContact &&
                       entities[playerIndex].state.active == 1 &&
                       magnetSolvedActiveCellsNative(entities, entityCount, source) == 0;
        }
        char row[140];
        wsprintfA(row, "%d,%d,%d,%d,%d\r\n", mode, observer.solved, playerMagnetContact,
                  magnetSolvedActiveCellsNative(entities, entityCount, source),
                  entities[playerIndex].state.active);
        lstrcatA(trace, row);
        physicsWorld->SetContactListener(originalContacts);
    }
    check(
        actual,
        "The actual original192px magnet and Bean generate solved upper contacts and select only genuinely touched actlets");
    check(
        repeat,
        "A second mechanism dispatch without a new solved event cannot invent magnetic attachment from the old contact list");
    check(
        sleep,
        "A controlled sleeping Bean with a retained real manifold and zero new PostSolve callbacks cannot replay the prior magnetic event");
    check(
        restored,
        "Checkpoint reconstruction plus mechanism dispatch cannot turn stale pre-solver contacts into a new magnetic event");
    check(
        transient,
        "A valid solved magnet event remains accepted if the contact is disabled before deferred mechanism processing");
    check(filters,
          "Disabled and sensor magnet contacts cannot attach Bean or select magnetic actlets");
    u32 campaign = hashBytes(entities, sizeof(Entity) * entityCount),
        checkpointHash = hashBytes(&checkpoint, sizeof(checkpoint));
    bool stageFlag = playerMagnetContact, uiAccepted = true, uiOmitted = true;
    for (int omit = 0; omit < 2; omit++) {
        interfaceEnabled = true;
        navigate(worldScenes[4]);
        int first = -1;
        for (int i = 0; i < page.playfieldCount; i++)
            if (page.playfield[i].kind == MAGNET &&
                page.playfield[i].source->id == 8759011704404663ULL &&
                magnetMatrixOwner(page.playfield[i]))
                first = i;
        if (first < 0) {
            uiAccepted = uiOmitted = false;
            continue;
        }
        Object *source = page.playfield[first].source;
        page.level.geometryCount = 0;
        for (int i = 0; i < page.playfieldCount; i++)
            if (page.playfield[i].source != source)
                page.playfield[i].state.gone = 1;
        destroyUiPhysics(page);
        createUiPhysics(page);
        page.bean.state.x = 424;
        page.bean.state.y = 80 - page.bean.hit.b + .1f;
        page.bean.state.vx = 0;
        page.bean.state.vy = -10;
        resetUiPhysics(page);
        UiMagnetSolvedObserverNative *observer =
            new UiMagnetSolvedObserverNative(page, first, omit != 0);
        delete page.contacts;
        page.contacts = observer;
        page.physics->SetContactListener(observer);
        stepUiPhysics(page, dt);
        int cells = magnetSolvedActiveCellsNative(page.playfield, page.playfieldCount, source);
        if (!omit)
            uiAccepted &= observer->solved > 0 && page.contacts->magnetSolved &&
                          page.bean.state.active == 2 && cells > 0;
        else
            uiOmitted &= observer->solved > 0 && !page.contacts->magnetSolved &&
                         page.bean.state.active == 1 && cells == 0;
    }
    check(
        uiAccepted && uiOmitted,
        "Actual selector magnet contact attaches only when its callback is delivered; a controlled omitted callback cannot be replaced by endpoint scanning");
    check(
        campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
            checkpointHash == hashBytes(&checkpoint, sizeof(checkpoint)) &&
            stageFlag == playerMagnetContact,
        "Selector magnet event ownership does not change campaign actors, checkpoint bytes or per-step magnetic flag");
    atomicWrite("test-output/magnet-solved-observed.csv", trace, lstrlenA(trace));
    interfaceEnabled = false;
    startStage(0);
    readyTimer = 0;
    atomicWrite("test-output/magnet-solved-results.txt", testLog, lstrlenA(testLog));
}
