#pragma once

// Regression coverage for group collectable lifecycle native. Runs only in the isolated BQ_ENABLE_TESTS executable.

class GroupCollectableObserverNative : public OriginalContacts {
  public:
    int began[16], beginCount = 0, endCount = 0, endsAfterFirstCoin = 0, salamanderEndCoins = -1;
    bool goalAtEnd = false;

    int source(b2Contact *c) {
        b2Fixture *fs[] = {c->GetFixtureA(), c->GetFixtureB()};
        for (int side = 0; side < 2; side++) {
            int a = (int)fs[side]->GetBody()->GetUserData() - 1,
                b = (int)fs[1 - side]->GetBody()->GetUserData() - 1;
            if (a >= 0 && a < entityCount && b == playerIndex &&
                (entities[a].kind == COIN || entities[a].kind == SALAMANDER))
                return a;
        }
        return -1;
    }

    void BeginContact(b2Contact *c) {
        OriginalContacts::BeginContact(c);
        int i = source(c);
        if (i >= 0 && beginCount < 16)
            began[beginCount++] = i;
    }

    void EndContact(b2Contact *c) {
        OriginalContacts::EndContact(c);
        int i = source(c);
        if (i >= 0) {
            endCount++;
            if (coins == 1)
                endsAfterFirstCoin++;
            if (entities[i].kind == SALAMANDER)
                salamanderEndCoins = coins;
            goalAtEnd |= complete;
        }
    }
};

int prepareGroupCollectableNative(int &second) {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    stopMusic();
    startStage(0);
    readyTimer = 0;
    current.geometryCount = 0;
    int first = -1;
    second = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == COIN) {
            if (first < 0)
                first = i;
            else if (entities[i].source == entities[first].source) {
                second = i;
                break;
            }
        }
    if (first < 0 || second < 0)
        return -1;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && i != first && i != second)
            entities[i].state.gone = 1;
    Entity &p = entities[playerIndex];
    p.state.x = p.state.y = 128;
    p.state.vx = p.state.vy = 0;
    p.state.timer = .2f;
    entities[first].state.x = 150;
    entities[first].state.y = 128;
    entities[second].state.x = 300;
    entities[second].state.y = 128;
    rebuildPhysics();
    stopSounds();
    clearEffects();
    return first;
}

int prepareUiGroupHelperNative(bool corner) {
    interfaceEnabled = debugMode = musicEnabled = false;
    soundEnabled = true;
    stopMusic();
    navigate(worldScenes[0]);
    transitionTime = 0;
    page.level.geometryCount = 0;
    destroyUiPhysics(page);
    int gem = -1;
    for (int i = 0; i < page.playfieldCount; i++) {
        if (gem < 0 && page.playfield[i].kind == COIN)
            gem = i;
        else
            page.playfield[i].state.gone = 1;
    }
    if (gem < 0)
        return -1;
    Entity &item = page.playfield[gem];
    item.state.x = item.state.y = 128;
    Object &bird = *classById(937947234821ULL);
    Vec c = center(bird.rect);
    int index = page.playfieldCount++;
    initializeEntity(
        page.playfield[index], bird,
        shifted(bird.rect, 128 - c.x + (corner ? 40 : 0), 128 - c.y + (corner ? 40 : 0)));
    page.playfield[index].state.active = 0;
    page.bean.state.x = page.bean.state.y = 1000;
    page.bean.state.vx = page.bean.state.vy = 0;
    page.bean.state.timer = .2f;
    createUiPhysics(page);
    stopSounds();
    clearEffects(1);
    return gem;
}

void groupCollectableLifecycleNativeChecks() {
    bool firstLive = false, lastOff = false;
    int second = -1, first = prepareGroupCollectableNative(second);
    if (first >= 0) {
        tick(1 / 120.0f, false, false);
        tick(1 / 120.0f, false, false);
        firstLive = entities[first].state.gone && !entities[second].state.gone &&
                    bodies[first]->IsActive() && bodies[second]->IsActive() && coins == 1;
        entities[playerIndex].state.x = 300;
        entities[playerIndex].state.y = 128;
        entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
        restorePhysics();
        tick(1 / 120.0f, false, false);
        lastOff = entities[first].state.gone && entities[second].state.gone &&
                  !bodies[first]->IsActive() && !bodies[second]->IsActive() && coins == 2;
    }
    check(
        firstLive,
        "Collecting one actual authored gem actlet hides it while retaining its group sensor until the group's remaining gem is collected");
    check(
        lastOff,
        "The final actual gem actlet deactivates all port fixtures representing its original group in the same collection step");
    int gem = prepareUiGroupHelperNative(false);
    u32 voices = audioVoicesStarted;
    tickInterface(1 / 120.0f);
    check(
        gem >= 0 && page.playfield[gem].state.gone && page.miniCoins == 1 &&
            audioVoicesStarted == voices + 1,
        "A real helper-only sensor contact collects the background world's gem with its own counter and one original coin sound");
    gem = prepareUiGroupHelperNative(true);
    voices = audioVoicesStarted;
    tickInterface(1 / 120.0f);
    check(
        gem >= 0 && !page.playfield[gem].state.gone && page.miniCoins == 0 &&
            audioVoicesStarted == voices,
        "A background helper outside the real circular gem sensor cannot collect through a sprite-only proximity check");
    first = prepareGroupCollectableNative(second);
    bool dedup = false;
    if (first >= 0) {
        Object &bird = *classById(937947234821ULL);
        Vec c = center(bird.rect);
        int helper = entityCount;
        addEntity(bird, shifted(bird.rect, 150 - c.x, 128 - c.y));
        entities[helper].state.active = 0;
        rebuildPhysics();
        soundEnabled = true;
        stopSounds();
        voices = audioVoicesStarted;
        simulateBodies(1 / 120.0f);
        bool before = singleCollectableEventCountNative == 1 &&
                      coinCollectablePendingNative[first] && !entities[first].state.gone &&
                      coins == 0 && audioVoicesStarted == voices;
        singleCollectablePostStepNative();
        u32 after = audioVoicesStarted;
        singleCollectablePostStepNative();
        dedup = before && coins == 1 && entities[first].state.gone &&
                audioVoicesStarted == voices + 1 && after == audioVoicesStarted;
    }
    check(
        dedup,
        "Simultaneous actual Tom/helper contacts lock one gem during BeginContact and defer one count/voice/effect without replay on a second drain");
    bool retained = true;
    for (int actor = 0; actor < 2; actor++)
        for (int rate = 0; rate < 3; rate++) {
            first = prepareGroupCollectableNative(second);
            if (first < 0) {
                retained = false;
                continue;
            }
            int target = playerIndex;
            if (actor) {
                Object &bird = *classById(937947234821ULL);
                Vec c = center(bird.rect);
                target = entityCount;
                addEntity(bird, shifted(bird.rect, 150 - c.x, 128 - c.y));
                entities[target].state.active = 0;
                entities[playerIndex].state.x = entities[playerIndex].state.y = 1000;
                rebuildPhysics();
            }
            simulateBodies(1.0f / (rate == 0 ? 30 : rate == 1 ? 120 : 240));
            bool pending = singleCollectableEventCountNative == 1;
            bodies[target]->SetTransform(b2Vec2(1000 * metres, 1200 * metres), 0);
            physicsWorld->Step(0, 10, 10);
            bool ended = !sensorBodyContact(bodies[first], bodies[target]);
            singleCollectablePostStepNative();
            retained &= pending && ended && entities[first].state.gone && coins == 1;
        }
    check(
        retained,
        "Actual Tom and helper EndContact before dispatch cannot cancel an accepted gem callback at30/120/240Hz");
    bool order = true;
    for (int variant = 0; variant < 2; variant++) {
        first = prepareGroupCollectableNative(second);
        if (first < 0) {
            order = false;
            continue;
        }
        entities[first].state.x = variant ? 142 : 114;
        entities[second].state.x = variant ? 114 : 142;
        rebuildPhysics();
        GroupCollectableObserverNative listener;
        physicsWorld->SetContactListener(&listener);
        simulateBodies(1 / 120.0f);
        bool captured = listener.beginCount == 2 && singleCollectableEventCountNative == 2;
        for (int n = 0; n < listener.beginCount; n++)
            captured &= singleCollectableEventsNative[n].source == listener.began[n];
        singleCollectablePostStepNative();
        order &= captured && coins == 2 && listener.endCount == 2 &&
                 listener.endsAfterFirstCoin == 2 && !bodies[first]->IsActive() &&
                 !bodies[second]->IsActive();
        physicsWorld->SetContactListener(originalContacts);
    }
    check(
        order,
        "Two accepted actlets preserve callback order and empty the logical group before its first callback deactivates all fixtures, matching the native available set");
    first = prepareGroupCollectableNative(second);
    bool mixed = false;
    if (first >= 0) {
        Object &axolotl = *classById(13485834099348442826ULL);
        Vec c = center(axolotl.rect);
        int item = entityCount;
        addEntity(axolotl, shifted(axolotl.rect, 140 - c.x, 128 - c.y));
        rebuildPhysics();
        GroupCollectableObserverNative listener;
        physicsWorld->SetContactListener(&listener);
        soundEnabled = true;
        stopSounds();
        voices = audioVoicesStarted;
        simulateBodies(1 / 120.0f);
        bool captured = singleCollectableEventCountNative == 2;
        int expected = singleCollectableEventsNative[0].source == first ? 1 : 0;
        singleCollectablePostStepNative();
        mixed = captured && coins == 1 && salamander == 1 &&
                listener.salamanderEndCoins == expected && audioVoicesStarted == voices + 2;
        physicsWorld->SetContactListener(originalContacts);
    }
    check(
        mixed,
        "A real simultaneous gem/axolotl pair shares BeginContact insertion order instead of dispatching separate type queues in a fixed order");
    int goal = prepareLevelEndNative();
    bool goalOrder = false;
    if (goal >= 0) {
        placeLevelEndBeanNative(goal, 0, -100);
        Object &object = *classById(314060601782828169ULL);
        Vec c = center(object.rect);
        first = entityCount;
        addEntity(object, shifted(object.rect, entities[playerIndex].state.x - c.x,
                                  entities[playerIndex].state.y - c.y));
        rebuildPhysics();
        int previous = progress.bestCoins[currentStage];
        progress.bestCoins[currentStage] = 0;
        GroupCollectableObserverNative listener;
        physicsWorld->SetContactListener(&listener);
        tick(1 / 120.0f, false, false);
        goalOrder = listener.goalAtEnd && complete && coins == 1 &&
                    progress.bestCoins[currentStage] == 0 && entities[first].state.gone;
        physicsWorld->SetContactListener(originalContacts);
        progress.bestCoins[currentStage] = previous;
    }
    check(
        goalOrder,
        "An actual simultaneous GOAL/gem contact completes pre-callback stats then still collects the accepted gem after Tom physics is disabled");
    first = prepareGroupCollectableNative(second);
    bool disk = false;
    if (first >= 0) {
        tick(1 / 120.0f, false, false);
        captureCheckpoint(false);
        char path[MAX_PATH];
        savePath(path, "gem-group-checkpoint.dat");
        disk = atomicWrite(path, &checkpoint, sizeof(checkpoint));
        entities[second].state.gone = 1;
        rebuildPhysics();
        if (disk)
            disk = readCheckpoint(path);
        if (disk)
            restoreCheckpoint();
        disk &= coins == 1 && entities[first].state.gone && !entities[second].state.gone &&
                bodies[first]->IsActive() && bodies[second]->IsActive() &&
                checkpoint.version == 28 && sizeof(Snapshot) == 543392;
    }
    check(
        disk,
        "Private disk restore reconstructs partially collected group activity while preserving count, hidden actlet state and the existing checkpoint layout");
    gem = prepareUiGroupHelperNative(false);
    bool uiRetained = false;
    if (gem >= 0) {
        int helper = page.playfieldCount - 1;
        page.physics->Step(1 / 120.0f, 10, 10);
        bool pending = page.contacts->collectableCount == 1 && !page.playfield[gem].state.gone;
        page.bodies[helper]->SetTransform(b2Vec2(1200 * metres, 1200 * metres), 0);
        page.physics->Step(0, 10, 10);
        bool ended = !sensorBodyContact(page.bodies[gem], page.bodies[helper]);
        uiGroupCollectablePostStepNative(page);
        uiRetained = pending && ended && page.miniCoins == 1 && page.playfield[gem].state.gone &&
                     !page.bodies[gem]->IsActive();
    }
    check(
        uiRetained,
        "The independent UI world retains its actual helper gem event after genuine EndContact and deactivates its exhausted group");
    u32 campaign = hashBytes(entities, sizeof(Entity) * entityCount),
        saved = hashBytes(&checkpoint, sizeof(checkpoint));
    int stageCoins = coins;
    gem = prepareUiGroupHelperNative(false);
    bool uiReset = false;
    if (gem >= 0) {
        page.physics->Step(1 / 120.0f, 10, 10);
        bool pending = page.contacts->collectableCount == 1;
        resetUiPhysics(page);
        uiGroupCollectablePostStepNative(page);
        bool reset = pending && !page.playfield[gem].state.gone && page.miniCoins == 0;
        destroyUiPhysics(page);
        createUiPhysics(page);
        uiGroupCollectablePostStepNative(page);
        uiReset = reset && !page.playfield[gem].state.gone && page.miniCoins == 0 &&
                  page.contacts->collectableCount == 0 &&
                  campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
                  saved == hashBytes(&checkpoint, sizeof(checkpoint)) && coins == stageCoins;
    }
    check(
        uiReset,
        "UI reset and full world reconstruction discard pending group events while keeping campaign entities, gem count and checkpoint bytes unchanged");
    atomicWrite("test-output/group-collectable-lifecycle-results.txt", testLog, lstrlenA(testLog));
    soundEnabled = false;
    stopSounds();
    startStage(0);
    readyTimer = 0;
}
