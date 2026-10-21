#pragma once

// Regression coverage for orbit attachment birth native. Runs only in the isolated BQ_ENABLE_TESTS executable.

Vec orbitAttachmentBirthExpectedNative(const Entity &child, const Entity &parent) {
    const Props &config = *parent.source->behavior("OrbitingObjectBehaviour");
    float radians = config.num("start-angle") * .01745329238474369f,
          angle = child.state.timer * 6.2831854820251465f / config.num("period", 2) + radians,
          radius = config.num("radius", 40);
    return Vec{parent.state.x + bqCos(angle) * radius, parent.state.y + bqSin(angle) * radius};
}

bool orbitAttachmentBirthPoseNative(const Entity &child, const Entity &parent) {
    Vec target = orbitAttachmentBirthExpectedNative(child, parent);
    return absf(child.state.x - target.x) < .005f && absf(child.state.y - target.y) < .005f;
}

void orbitAttachmentBirthNativeChecks() {
    interfaceEnabled = musicEnabled = soundEnabled = debugMode = false;
    const u64 scenes[] = {806701038719ULL,    116499217023ULL,     8458860001749117ULL,
                          789619714186ULL,    1966799885083198ULL, 1071369106910786ULL,
                          5679669680221079ULL};
    int children = 0, paused = 0, quarter = 0;
    bool initial = true, physics = true, snapshots = true, still = true, first = true;
    for (int scene = 0; scene < 7; scene++) {
        startStage(stageById(scenes[scene]));
        readyTimer = 0;
        for (int i = 0; i < entityCount; i++) {
            Entity &child = entities[i];
            if (child.motion != 2 || child.parent < 0)
                continue;
            Entity &parent = entities[child.parent];
            children++;
            quarter += parent.source->behavior("OrbitingObjectBehaviour")->num("start-angle") == 90;
            initial &= child.state.timer == 0 && orbitAttachmentBirthPoseNative(child, parent);
            Vec expected = orbitAttachmentBirthExpectedNative(child, parent);
            b2Vec2 body = bodies[i]->GetPosition();
            physics &= absf(body.x / metres - expected.x) < .005f &&
                       absf(body.y / metres - expected.y) < .005f;
            snapshots &= checkpoint.state[i].x == child.state.x &&
                         checkpoint.state[i].y == child.state.y && checkpoint.state[i].timer == 0;
            if (parent.source->paused) {
                paused++;
                still &= !parent.state.active && !child.state.active &&
                         orbitAttachmentBirthPoseNative(child, parent);
            }
        }
        State &bean = entities[playerIndex].state;
        bean.x = 64;
        bean.y = 10000;
        bean.vx = bean.vy = 0;
        restorePhysics();
        bodies[playerIndex]->SetType(b2_staticBody);
        static Vec before[MAX_ENTITIES];
        for (int i = 0; i < entityCount; i++)
            before[i] = Vec{entities[i].state.x, entities[i].state.y};
        tick(1 / 120.0f, false, false);
        for (int i = 0; i < entityCount; i++)
            if (entities[i].motion == 2) {
                Entity &child = entities[i];
                first &=
                    absf(child.state.x - before[i].x) < 2 && absf(child.state.y - before[i].y) < 2;
                if (!entities[child.parent].state.active)
                    still &= !bodies[i]->IsActive();
            }
        restartStage();
        for (int i = 0; i < entityCount; i++)
            if (entities[i].motion == 2)
                snapshots &=
                    entities[i].state.timer == 0 &&
                    orbitAttachmentBirthPoseNative(entities[i], entities[entities[i].parent]);
    }
    check(
        children == 9 && quarter == 1 && initial,
        "All nine actual campaign orbit attachments start at their authored angle, including the 5-8 90-degree child");
    check(
        physics,
        "Every actual orbit attachment main body begins at the same native initial centre as its actor");
    check(
        paused == 2 && still,
        "The Keep and Sewer paused attachments retain their correct initial pose while their actual bodies are inactive");
    check(
        first,
        "The first actual physics tick of all nine attachments advances continuously instead of correcting a below-parent placeholder");
    check(
        snapshots,
        "Initial checkpoint and stage restart preserve authored attachment phase-zero poses without saving a placeholder");
    startStage(stageById(5679669680221079ULL));
    readyTimer = 0;
    int selected = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].motion == 2 && entities[entities[i].parent]
                                               .source->behavior("OrbitingObjectBehaviour")
                                               ->num("start-angle") == 90)
            selected = i;
    bool loaded = false;
    if (selected >= 0) {
        State &bean = entities[playerIndex].state;
        bean.x = 64;
        bean.y = 10000;
        restorePhysics();
        bodies[playerIndex]->SetType(b2_staticBody);
        for (int n = 0; n < 30; n++)
            tick(1 / 120.0f, false, false);
        captureCheckpoint(false);
        State saved = entities[selected].state;
        for (int n = 0; n < 80; n++)
            tick(1 / 120.0f, false, false);
        restoreCheckpoint();
        loaded = entities[selected].state.x == saved.x && entities[selected].state.y == saved.y &&
                 entities[selected].state.timer == saved.timer;
        bodies[playerIndex]->SetType(b2_staticBody);
        Entity predicted = entities[selected];
        predicted.state.timer += 1 / 120.0f;
        Vec next = orbitAttachmentBirthExpectedNative(predicted, entities[predicted.parent]);
        tick(1 / 120.0f, false, false);
        loaded &= absf(entities[selected].state.x - next.x) < .005f &&
                  absf(entities[selected].state.y - next.y) < .005f;
    }
    check(
        loaded,
        "A real 5-8 quarter-turn attachment checkpoint restores its saved centre and clock and then continues the existing orbit");
    atomicWrite("test-output/orbit-attachment-birth-results.txt", testLog, lstrlenA(testLog));
    startStage(0);
    readyTimer = 0;
}
