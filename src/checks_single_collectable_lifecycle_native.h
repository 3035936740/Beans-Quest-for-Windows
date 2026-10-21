#pragma once

// Regression coverage for single collectable lifecycle native. Runs only in the isolated BQ_ENABLE_TESTS executable.

class SingleCollectableObserverNative : public OriginalContacts {
  public:
    int beginCount = 0, endCount = 0;
    int began[16], ended[16];
    bool endingOnBegin = false, completeOnEnd = false, uncollectedOnEnd = false;

    int source(b2Contact *c) {
        b2Fixture *f[] = {c->GetFixtureA(), c->GetFixtureB()};
        for (int side = 0; side < 2; side++) {
            int a = (int)f[side]->GetBody()->GetUserData() - 1,
                b = (int)f[1 - side]->GetBody()->GetUserData() - 1;
            if (a >= 0 && a < entityCount && b == playerIndex &&
                (entities[a].kind == SALAMANDER || entities[a].kind == POTION))
                return a;
        }
        return -1;
    }

    void BeginContact(b2Contact *c) {
        OriginalContacts::BeginContact(c);
        int i = source(c);
        if (i >= 0 && beginCount < 16) {
            began[beginCount++] = i;
            if (endingOnBegin)
                endingScene.active = 1;
        }
    }

    void EndContact(b2Contact *c) {
        OriginalContacts::EndContact(c);
        int i = source(c);
        if (i >= 0 && endCount < 16) {
            ended[endCount++] = i;
            completeOnEnd = complete;
            uncollectedOnEnd = !entities[i].state.gone && !salamander && !playerWalkMode;
        }
    }
};

void singleCollectableLifecycleNativeChecks() {
    interfaceEnabled = debugMode = musicEnabled = false;
    soundEnabled = true;
    stopMusic();
    bool stopped = true, once = true;
    for (int rate = 0; rate < 3; rate++) {
        int item = prepareSingleCollectableNative(13485834099348442826ULL, 0);
        u32 voices = audioVoicesStarted;
        float dt = 1.0f / (rate == 0 ? 30 : rate == 1 ? 120 : 240);
        tick(dt, false, false);
        stopped &= entities[item].state.gone && salamander == 1 && !bodies[item]->IsActive();
        u32 collectedVoices = audioVoicesStarted;
        for (int frame = 0; frame < 3; frame++)
            tick(dt, false, false);
        once &= collectedVoices == voices + 1 && audioVoicesStarted == collectedVoices &&
                salamander == 1;
    }
    check(
        stopped,
        "Actual axolotl collection deactivates its sensor in the same post-physics step at30/120/240Hz instead of retaining an invisible live fixture");
    check(
        once,
        "The original axolotl voice and collection flag occur once and do not repeat on the following three production ticks");
    int item = prepareSingleCollectableNative(8048939671444015ULL, 0);
    u32 voices = audioVoicesStarted;
    tick(1 / 120.0f, false, false);
    check(
        entities[item].state.gone && !bodies[item]->IsActive() && playerWalkMode &&
            audioVoicesStarted == voices + 1,
        "The real potion disables its body and transforms Bean with one axolotl voice in the collection step");
    item = prepareSingleCollectableNative(13485834099348442826ULL, 1);
    voices = audioVoicesStarted;
    tick(1 / 120.0f, false, false);
    check(
        !entities[item].state.gone && bodies[item]->IsActive() && !salamander &&
            audioVoicesStarted == voices,
        "A sprite-box corner outside the circular sensor neither collects nor disables the original axolotl");
    bool deferred = true, retained = true;
    for (int kind = 0; kind < 2; kind++)
        for (int rate = 0; rate < 3; rate++) {
            item = prepareSingleCollectableNative(
                kind ? 8048939671444015ULL : 13485834099348442826ULL, 0);
            voices = audioVoicesStarted;
            SingleCollectableObserverNative observer;
            physicsWorld->SetContactListener(&observer);
            simulateBodies(1.0f / (rate == 0 ? 30 : rate == 1 ? 120 : 240));
            deferred &= observer.beginCount == 1 && singleCollectableEventCountNative == 1 &&
                        !entities[item].state.gone && bodies[item]->IsActive() &&
                        audioVoicesStarted == voices && !playerWalkMode && !salamander;
            // A real second zero-duration collision pass ends the accepted
            // contact before dispatch; no synthetic contact object is used.
            bodies[playerIndex]->SetTransform(b2Vec2(1000 * metres, 1000 * metres), 0);
            physicsWorld->Step(0, 10, 10);
            bool ended =
                observer.endCount == 1 && !sensorBodyContact(bodies[item], bodies[playerIndex]);
            singleCollectablePostStepNative();
            retained &= ended && entities[item].state.gone && !bodies[item]->IsActive() &&
                        audioVoicesStarted == voices + 1 &&
                        (kind ? playerWalkMode : salamander == 1) &&
                        singleCollectableEventCountNative == 0;
            physicsWorld->SetContactListener(originalContacts);
        }
    check(
        deferred,
        "Real BeginContact at30/120/240Hz schedules both single-collectable types without changing visibility, physics, mode or sound inside the solver");
    check(
        retained,
        "A genuine EndContact before dispatch cannot cancel the already scheduled axolotl or potion collection at30/120/240Hz");
    bool ordered = true;
    for (int variant = 0; variant < 2; variant++) {
        item = prepareSingleCollectableNative(13485834099348442826ULL, 0);
        Object &potion = *classById(8048939671444015ULL);
        int second = entityCount;
        Vec c = center(potion.rect);
        addEntity(potion, shifted(potion.rect, 106 - c.x, 128 - c.y));
        if (variant) {
            entities[item].state.x = 106;
            entities[second].state.x = 150;
        }
        rebuildPhysics();
        SingleCollectableObserverNative observer;
        physicsWorld->SetContactListener(&observer);
        voices = audioVoicesStarted;
        simulateBodies(1 / 120.0f);
        bool queued = observer.beginCount == 2 && singleCollectableEventCountNative == 2;
        for (int n = 0; n < observer.beginCount; n++)
            queued &= singleCollectableEventsNative[n].source == observer.began[n];
        singleCollectablePostStepNative();
        ordered &= queued && observer.endCount == 2 && observer.ended[0] == observer.began[0] &&
                   observer.ended[1] == observer.began[1] && entities[item].state.gone &&
                   entities[second].state.gone && salamander == 1 && playerWalkMode &&
                   audioVoicesStarted == voices + 2;
        physicsWorld->SetContactListener(originalContacts);
    }
    check(
        ordered,
        "Two real simultaneous axolotl/potion contacts deactivate and collect in captured BeginContact order under both source-position arrangements");
    item = prepareSingleCollectableNative(13485834099348442826ULL, 0);
    b2Fixture *original = bodies[playerIndex]->GetFixtureList();
    b2FixtureDef duplicate;
    duplicate.shape = original->GetShape();
    duplicate.filter = original->GetFilterData();
    bodies[playerIndex]->CreateFixture(&duplicate);
    SingleCollectableObserverNative observer;
    physicsWorld->SetContactListener(&observer);
    voices = audioVoicesStarted;
    simulateBodies(1 / 120.0f);
    int events = singleCollectableEventCountNative;
    singleCollectablePostStepNative();
    u32 after = audioVoicesStarted;
    singleCollectablePostStepNative();
    check(
        observer.beginCount == 2 && events == 2 && entities[item].state.gone &&
            !bodies[item]->IsActive() && after == voices + 2 && audioVoicesStarted == after,
        "Two genuine Tom fixture contacts retain native repeated callbacks while a second drain cannot replay the collection voice");
    physicsWorld->SetContactListener(originalContacts);
    bool reset = true;
    for (int mode = 0; mode < 2; mode++) {
        item = prepareSingleCollectableNative(13485834099348442826ULL, 0);
        simulateBodies(1 / 120.0f);
        bool pending = singleCollectableEventCountNative == 1;
        voices = audioVoicesStarted;
        if (mode)
            rebuildPhysics();
        else
            restorePhysics();
        singleCollectablePostStepNative();
        reset &= pending && !entities[item].state.gone && bodies[item]->IsActive() && !salamander &&
                 audioVoicesStarted == voices && singleCollectableEventCountNative == 0;
    }
    check(
        reset,
        "In-place physics restore and full world reconstruction discard unconsumed contact callbacks without collecting stale state");
    item = prepareSingleCollectableNative(13485834099348442826ULL, 0);
    simulateBodies(1 / 120.0f);
    voices = audioVoicesStarted;
    bodies[item]->SetActive(false);
    entities[item].state.gone = 1;
    singleCollectablePostStepNative();
    check(
        salamander == 1 && audioVoicesStarted == voices + 1 &&
            singleCollectableEventCountNative == 0,
        "Later physics/visibility deactivation does not cancel the accepted native post-physics collection callback");
    bool goalOrder[2] = {};
    char goalRows[1800] =
        "kind,begin,end,completeAtEnd,uncollectedAtEnd,complete,goalLaunch,gone,bodyActive,newVoices,pending,progressAxolotl,stageAxolotl,walkMode\r\n";
    for (int kind = 0; kind < 2; kind++) {
        int goal = prepareLevelEndNative();
        if (goal < 0)
            continue;
        placeLevelEndBeanNative(goal, 0, -100);
        Object &object = *classById(kind ? 8048939671444015ULL : 13485834099348442826ULL);
        Vec c = center(object.rect);
        item = entityCount;
        addEntity(object, shifted(object.rect, entities[playerIndex].state.x - c.x,
                                  entities[playerIndex].state.y - c.y));
        rebuildPhysics();
        int old = progress.axolotls[currentStage];
        progress.axolotls[currentStage] = 0;
        soundEnabled = true;
        stopSounds();
        voices = audioVoicesStarted;
        SingleCollectableObserverNative listener;
        physicsWorld->SetContactListener(&listener);
        tick(1 / 120.0f, false, false);
        goalOrder[kind] =
            listener.beginCount == 1 && listener.endCount == 1 && listener.completeOnEnd &&
            listener.uncollectedOnEnd && complete && goalLaunch.active &&
            entities[item].state.gone && !bodies[item]->IsActive() &&
            audioVoicesStarted == voices + 2 && pinballDetailVoiceCount("axolotl") == 1 &&
            pinballDetailVoiceCount("bean-win") == 1 && singleCollectableEventCountNative == 0 &&
            (kind ? playerWalkMode : progress.axolotls[currentStage] == 0 && salamander == 1);
        char row[250];
        wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", kind, listener.beginCount,
                  listener.endCount, listener.completeOnEnd, listener.uncollectedOnEnd, complete,
                  goalLaunch.active, entities[item].state.gone, bodies[item]->IsActive(),
                  audioVoicesStarted - voices, singleCollectableEventCountNative,
                  progress.axolotls[currentStage], salamander, playerWalkMode);
        lstrcatA(goalRows, row);
        physicsWorld->SetContactListener(originalContacts);
        progress.axolotls[currentStage] = old;
    }
    check(
        goalOrder[0],
        "An actual simultaneous GOAL/axolotl contact completes original pre-callback stats before collecting the accepted item, even though completion disables Tom's body");
    check(
        goalOrder[1],
        "An actual simultaneous GOAL/potion contact completes before the scheduled transformation and retains its one voice after Tom is disabled");
    atomicWrite("test-output/single-collectable-lifecycle-goal.csv", goalRows, lstrlenA(goalRows));
    item = prepareSingleCollectableNative(13485834099348442826ULL, 0);
    SingleCollectableObserverNative ending;
    ending.endingOnBegin = true;
    physicsWorld->SetContactListener(&ending);
    voices = audioVoicesStarted;
    tick(1 / 120.0f, false, false);
    check(
        ending.beginCount == 1 && endingScene.active && entities[item].state.gone &&
            !bodies[item]->IsActive() && salamander == 1 && audioVoicesStarted == voices + 1 &&
            singleCollectableEventCountNative == 0,
        "A controlled cutscene request during the real physics step still drains accepted collection callbacks before the campaign's early return");
    physicsWorld->SetContactListener(originalContacts);
    atomicWrite("test-output/single-collectable-lifecycle-results.txt", testLog, lstrlenA(testLog));
    soundEnabled = false;
    stopSounds();
    startStage(0);
    readyTimer = 0;
}
