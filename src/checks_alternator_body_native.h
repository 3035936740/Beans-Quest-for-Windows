#pragma once

// Regression coverage for alternator body native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void alternatorBodyNativeChecks() {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    stopMusic();
    stopSounds();
    bool immediate = true, unrelated = true, repeated = true;
    int matched = 0, normal = 0, touch = 0;
    for (int stage = 0; stage < stageCount; stage++) {
        startStage(stage);
        readyTimer = 0;
        int mask = 0;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == ALTERNATOR)
                mask |= entities[i].mask;
        for (int i = 0; i < current.objectCount; i++)
            if (current.objects[i].has("EndOfGameCutsceneController"))
                mask &= ~1;
        if (!mask)
            continue;
        bool before[MAX_ENTITIES];
        for (int i = 0; i < entityCount; i++)
            before[i] = bodies[i] && bodies[i]->IsActive();
        alternateScene(mask);
        for (int i = 0; i < entityCount; i++)
            if (bodies[i]) {
                if (entities[i].kind == ALTERNATOR && (entities[i].mask & mask)) {
                    matched++;
                    if (entities[i].touch)
                        touch++;
                    else
                        normal++;
                    immediate &= bodies[i]->IsActive() != before[i];
                } else
                    unrelated &= bodies[i]->IsActive() == before[i];
            }
        alternateScene(mask);
        for (int i = 0; i < entityCount; i++)
            if (bodies[i])
                repeated &= bodies[i]->IsActive() == before[i];
    }
    check(
        matched > 0 && normal > 0 && touch > 0,
        "The original campaign event matrix exercises both normal and touch alternator collision bodies");
    check(
        immediate,
        "Every actual campaign alternator body reverses collision membership in the same native DoAlternate event");
    check(
        unrelated && repeated,
        "Immediate alternation leaves unrelated bodies alone and two events restore the original collision membership");

    startStage(stageById(4412182657474862604ULL));
    readyTimer = 0;
    current.geometryCount = 0;
    int sw = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == SWITCH && sw < 0)
            sw = i;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && i != sw && entities[i].kind != ALTERNATOR)
            entities[i].state.gone = 1;
    if (sw >= 0) {
        if (switchOn(entities[sw]))
            alternationBits ^= entities[sw].mask;
        Entity &bean = entities[playerIndex];
        bean.state.x = entities[sw].state.x;
        bean.state.y = entities[sw].state.y + 20;
        bean.state.vx = bean.state.vy = 0;
        rebuildPhysics();
        bool before[MAX_ENTITIES];
        for (int i = 0; i < entityCount; i++)
            before[i] = bodies[i] && bodies[i]->IsActive();
        u32 voices = audioVoicesStarted;
        soundEnabled = true;
        simulateBodies(1.0f / 120);
        bool began = switchPending[sw];
        mechanismsPostStep(1.0f / 120);
        soundEnabled = false;
        bool changed = true;
        int affected = 0;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == ALTERNATOR && (entities[i].mask & entities[sw].mask) &&
                bodies[i]) {
                affected++;
                changed &= bodies[i]->IsActive() != before[i];
            }
        check(
            began && switchOn(entities[sw]) && affected > 0,
            "The actual1-8 upper switch face produces a real BeginContact and toggles its original linked group");
        check(
            changed,
            "The real1-8 switch updates all linked collision bodies inside PostPhysicsStep without waiting for the next frame");
        mechanismsPostStep(1.0f / 120);
        check(
            audioVoicesStarted == voices + 1,
            "The real switch plays one switch sound without repeating it when body synchronization removes contacts");
        stopSounds();
        int block = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == ALTERNATOR && bodies[i]) {
                block = i;
                break;
            }
        if (block >= 0) {
            captureCheckpoint(false);
            int savedBits = alternationBits;
            bool savedActive = bodies[block]->IsActive();
            alternateScene(entities[block].mask);
            restoreCheckpoint();
            check(
                alternationBits == savedBits && bodies[block]->IsActive() == savedActive,
                "Checkpoint restore rebuilds the saved alternator collision membership after an immediate event");
            entities[block].state.gone = 1;
            bodies[block]->SetActive(false);
            alternateScene(entities[block].mask);
            check(!bodies[block]->IsActive(),
                  "Alternating a removed block never resurrects its collision body");
        } else {
            check(
                false,
                "Checkpoint restore rebuilds the saved alternator collision membership after an immediate event");
            check(false, "Alternating a removed block never resurrects its collision body");
        }
    } else {
        check(
            false,
            "The actual1-8 upper switch face produces a real BeginContact and toggles its original linked group");
        check(
            false,
            "The real1-8 switch updates all linked collision bodies inside PostPhysicsStep without waiting for the next frame");
        check(
            false,
            "The real switch plays one switch sound without repeating it when body synchronization removes contacts");
        check(
            false,
            "Checkpoint restore rebuilds the saved alternator collision membership after an immediate event");
        check(false, "Alternating a removed block never resurrects its collision body");
    }
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    int wizard = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            wizard = i;
    bool arena = wizard >= 0, other = true;
    int events = 0;
    if (wizard >= 0)
        for (int hp = 5; hp >= 1; hp--) {
            entities[wizard].state.hp = hp;
            for (int open = 0; open < 2; open++) {
                bool before[MAX_ENTITIES];
                for (int i = 0; i < entityCount; i++)
                    before[i] = bodies[i] && bodies[i]->IsActive();
                bossAlternate(entities[wizard], open != 0);
                events++;
                for (int i = 0; i < entityCount; i++)
                    if (bodies[i]) {
                        if (entities[i].kind == ALTERNATOR)
                            arena &= bodies[i]->IsActive() == activeBlock(entities[i]);
                        else
                            other &= bodies[i]->IsActive() == before[i];
                    }
            }
        }
    check(
        arena && events == 10,
        "All ten original Wizard arena start/end events synchronize their actual multi-mask block bodies immediately");
    check(
        other,
        "Wizard arena collision alternation leaves Bean, the Wizard and unrelated actor bodies enabled as before");
    startStage(0);
    readyTimer = 0;
}
