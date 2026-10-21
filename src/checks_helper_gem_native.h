#pragma once

// Regression coverage for helper gem native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void helperGemNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/helper-gem-saves");
    CreateDirectoryA(saveRoot, 0);
    interfaceEnabled = debugMode = musicEnabled = false;
    soundEnabled = true;
    stopMusic();
    bool flying = false, roost = false, returning = false, corner = false, other = false,
         unique = false, feedback = false, saved = false, single = false;
    for (int scenario = 0; scenario < 7; scenario++) {
        startStage(0);
        readyTimer = deathTimer = 0;
        current.geometryCount = 0;
        for (int i = 0; i < entityCount; i++)
            if (i != playerIndex)
                entities[i].state.gone = 1;
        State &bean = entities[playerIndex].state;
        bean.x = 128;
        bean.y = 128;
        bean.vx = bean.vy = 0;
        int bird = entityCount;
        addEntity(*classById(scenario == 4 ? 14264481342587988014ULL : 937947234821ULL),
                  Rect{104, 176, 152, 224});
        Entity &helper = entities[bird];
        helper.state.active = scenario == 1 ? 0 : scenario == 2 ? 2 : 1;
        helper.state.timer = .2f;
        if (scenario == 0) {
            helper.state.x = 128;
            helper.state.y = 162;
            helper.originX = 128;
            helper.originY = 162;
        }
        if (scenario == 2) {
            helper.originX = 240;
            helper.originY = 240;
        }
        Vec point = {128, scenario == 0 ? 168.0f : 200.0f};
        if (scenario == 3)
            point = Vec{150, 222};
        if (scenario == 5) {
            point = Vec{128, 144};
            helper.state.y = 162;
            helper.originY = 162;
        }
        if (scenario == 6) {
            point = Vec{128, 200};
        }
        int gem = entityCount;
        u64 klass = 314060601782828169ULL;
        if (scenario == 6)
            for (int i = 0; i < classCount; i++)
                if (classes[i].has("CollectableSalamanderBehaviour")) {
                    klass = classes[i].id;
                    break;
                }
        addEntity(*classById(klass), Rect{point.x - 16, point.y - 16, point.x + 16, point.y + 16});
        rebuildPhysics();
        stopSounds();
        clearEffects();
        u32 voices = audioVoicesStarted;
        int before = coins;
        tick(1 / 120.0f, false, false);
        bool taken = entities[gem].state.gone && coins == before + 1;
        if (scenario == 0) {
            flying = taken;
            feedback = audioVoicesStarted == voices + 1 && liveEffects() > 0;
            captureCheckpoint(false);
            int recorded = coins;
            restoreCheckpoint();
            saved = entities[gem].state.gone && coins == recorded;
        }
        if (scenario == 1)
            roost = taken;
        if (scenario == 2)
            returning = taken;
        if (scenario == 3)
            corner = !taken && !entities[gem].state.gone;
        if (scenario == 4)
            other = !taken && !entities[gem].state.gone;
        if (scenario == 5) {
            int after = coins;
            tick(1 / 120.0f, false, false);
            unique = taken && coins == after && audioVoicesStarted == voices + 1;
        }
        if (scenario == 6)
            single = !entities[gem].state.gone && salamander == 0;
    }
    check(
        flying,
        "A real flying helper main fixture collects a gem above Bean when Bean own fixture never touches the circle");
    check(
        roost && returning,
        "Native group collection accepts both roosting and returning HelperBird bodies without inventing a flying-only restriction");
    check(
        corner,
        "Helper gem collection uses the adjusted physical bird fixture instead of the broad bird artwork or an AABB corner");
    check(other, "Ordinary enemy contact with a gem cannot collect it on behalf of Bean");
    check(
        unique,
        "A gem touched by both Bean and its helper counts and sounds once without repeating on following frames");
    check(
        feedback,
        "Helper gem collection uses the existing coin sound and authored collection particle effect");
    check(saved, "A checkpoint retains the helper collected gem and gem counter after restore");
    check(
        single,
        "The group-only helper exception does not incorrectly grant helper collection of single salamanders");
    atomicWrite("test-output/helper-gem-results.txt", testLog, lstrlenA(testLog));
    lstrcpyA(saveRoot, oldRoot);
    stopSounds();
    startStage(0);
    readyTimer = 0;
}
