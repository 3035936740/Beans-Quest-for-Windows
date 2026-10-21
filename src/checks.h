#pragma once

// Core fidelity regression checks compiled only into the isolated test executable.

// These checks use the self-test saveRoot, never the player's saves directory.
void fidelityChecks() {
    interfaceEnabled = false;
    const int starts[] = {0, 8, 18, 27, 37, 49}, worlds[] = {1, 4, 2, 3, 5, 6};
    bool ordered = true;
    for (int i = 0; i < 6; i++)
        if ((int)levels[stageList[starts[i]]].props.num("bean-world") != worlds[i])
            ordered = false;
    check(ordered, "Authored world progression: Grasslands, Desert, Peaks, Ruins, Lair, Wizard");

    startStage(0);
    int seesaw = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].rotate == 2) {
            seesaw = i;
            break;
        }
    bool hinge = seesaw >= 0 && hinges[seesaw] && bodies[seesaw]->GetType() == b2_dynamicBody;
    if (hinge) {
        b2Body *b = bodies[seesaw];
        b->ApplyLinearImpulse(b2Vec2(0, -1), b->GetWorldPoint(b2Vec2(2, 0)));
        for (int i = 0; i < 30; i++)
            simulateBodies(1 / 120.0f);
        hinge = absf(entities[seesaw].state.angle) > 0.5f;
        captureCheckpoint(false);
        float angle = entities[seesaw].state.angle, angular = entities[seesaw].state.aux;
        entities[seesaw].state.angle = 0;
        entities[seesaw].state.aux = 0;
        restoreCheckpoint();
        check(absf(entities[seesaw].state.angle - angle) < 0.01f &&
                  absf(bodies[seesaw]->GetAngularVelocity() - angular) < 0.001f,
              "Checkpoint restores seesaw rotation and angular velocity");
    }
    check(hinge, "Seesaw reacts to torque through a physical revolute joint");

    startStage(4);
    int sphere = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].circle) {
            sphere = i;
            break;
        }
    bool round = sphere >= 0 && bodies[sphere] &&
                 bodies[sphere]->GetFixtureList()->GetShape()->GetType() == b2Shape::e_circle;
    if (round) {
        bodies[sphere]->ApplyAngularImpulse(0.25f);
        for (int i = 0; i < 20; i++)
            simulateBodies(1 / 120.0f);
        round = absf(entities[sphere].state.angle) > 0.1f;
    }
    check(round, "Boulders use rotating circle fixtures");

    startStage(6);
    int lower = -1, upper = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].dynamic && entities[i].kind == BLOCK && !entities[i].circle) {
            if (lower < 0)
                lower = i;
            else {
                upper = i;
                break;
            }
        }
    bool stacked = lower >= 0 && upper >= 0;
    if (stacked) {
        entities[lower].state.x = entities[upper].state.x = 500;
        entities[lower].state.y = 300;
        entities[upper].state.y = 336;
        entities[lower].state.vx = entities[lower].state.vy = entities[upper].state.vx =
            entities[upper].state.vy = 0;
        rebuildPhysics();
        for (int i = 0; i < 120; i++)
            tick(1 / 120.0f, false, false);
        stacked = entities[lower].state.y < 280 &&
                  absf(entities[lower].state.y - entities[upper].state.y) > 24;
    }
    check(stacked, "Dynamic crates fall and collide with each other");

    startStage(3);
    int secret = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == SECRET) {
            secret = i;
            break;
        }
    if (secret >= 0) {
        Vec c = center(entityRect(entities[secret]));
        entities[playerIndex].state.x = c.x;
        entities[playerIndex].state.y = c.y;
        for (int i = 0; i < 15; i++)
            tick(1 / 120.0f, false, false);
        check(entities[secret].state.aux > 0,
              "Entering an original secret area reveals its foreground");
    }

    Progress retained = progress;
    Snapshot *saved = (Snapshot *)HeapAlloc(GetProcessHeap(), 0, sizeof(Snapshot));
    *saved = checkpoint;
    int legacyIndex = 8, migrated = stageById(levels[legacyStageList[legacyIndex]].id);
    Progress legacy = {};
    legacy.magic = 0x50535142;
    legacy.version = 1;
    legacy.unlocked = legacy.lastStage = legacyIndex;
    legacy.bestCoins[legacyIndex] = 17;
    legacy.bestJumps[legacyIndex] = 22;
    legacy.cleared[legacyIndex] = 1;
    legacy.axolotls[legacyIndex] = 1;
    legacy.checksum = hashBytes(&legacy, sizeof(legacy) - 4);
    char path[MAX_PATH];
    savePath(path, "progress.dat");
    atomicWrite(path, &legacy, sizeof(legacy));
    checkpoint.stage = legacyIndex;
    checkpoint.version = 2;
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    savePath(path, "checkpoint.dat");
    atomicWrite(path, &checkpoint, sizeof(checkpoint));
    readProgress();
    check(progress.version == 2 && currentStage == migrated && progress.bestCoins[migrated] == 17 &&
              progress.bestJumps[migrated] == 22 && progress.cleared[migrated] &&
              progress.axolotls[migrated],
          "Legacy progress follows stage identity after the world order correction");
    check(haveCheckpoint && checkpoint.version == 5 && checkpoint.stage == migrated,
          "Legacy checkpoint migrates to the same original stage");
    progress = retained;
    checkpoint = *saved;
    HeapFree(GetProcessHeap(), 0, saved);
    musicEnabled = soundEnabled = false;

    interfaceEnabled = true;
    progress.unlocked = stageCount - 1;
    startStage(7);
    readyTimer = 0;
    finish();
    for (int step = 0; step < 250; step++)
        tick(1 / 120.0f, false, false);
    confirm();
    check(screen == PLAY && currentStage == 8 && (int)current.props.num("bean-world") == 4,
          "Last Grasslands result continues into the first Desert stage");

    navigate(9740832380187429308ULL);
    tickInterface(1.1f);
    check(screen == SPLASH && page.level.id == 1346587316875469ULL,
          "Original Kumobius splash advances to the music splash");
    tickInterface(1.7f);
    check(screen == TITLE, "Original splash sequence reaches the title");
    page.age = 2;
    transitionTime = 0;
    UiNode &start = page.nodes[page.focus];
    float dx, dy;
    layerOffset(page, start.object->layer, &dx, &dy);
    Vec point = center(shifted(visualRect(page, start), dx, dy));
    pointerDown(point.x, point.y);
    pointerUp(point.x, point.y);
    check(screen == WORLD, "Mouse press and release activate the original Start button");
    int desert = -1;
    for (int i = 0; i < page.level.objectCount; i++)
        if (page.nodes[i].world == 4)
            desert = i;
    if (desert >= 0) {
        page.cameraX = 176;
        for (int i = 0; i < page.level.objectCount; i++)
            if (page.nodes[i].world == 1)
                page.focus = i;
        layerOffset(page, page.nodes[desert].object->layer, &dx, &dy);
        point = center(shifted(page.nodes[desert].rect, dx, dy));
        pointerDown(point.x, point.y);
        pointerUp(point.x, point.y);
        check(screen == WORLD && page.nodes[page.focus].world == 4,
              "First world-card click focuses the adjacent world");
        for (int i = 0; i < 120; i++)
            tickInterface(1 / 120.0f);
        layerOffset(page, page.nodes[desert].object->layer, &dx, &dy);
        point = center(shifted(page.nodes[desert].rect, dx, dy));
        pointerDown(point.x, point.y);
        pointerUp(point.x, point.y);
        check(screen == SELECT && page.level.id == worldScenes[3],
              "Second world-card click opens the authored Desert selector");
    }

    interfaceEnabled = false;
    startStage(49);
    int wizard = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS) {
            wizard = i;
            break;
        }
    bool arenas = wizard >= 0;
    if (arenas) {
        Entity &boss = entities[wizard];
        const Props &bp = *boss.source->behavior("FinalBossBehaviour");
        for (int area = 0; area < 5; area++) {
            float x = listNumber(bp.get("area-offsets"), area);
            boss.state.active = 1;
            entities[playerIndex].state.x = x;
            camX = x - 256;
            updateWizard(boss, 0.01f);
            arenas = arenas && boss.state.active == 3 && (alternationBits & 4);
            boss.state.timer = 4;
            wizardPrepareSummons(boss);
            wizardMaterializeSummons(boss);
            wizardShot(boss, 1);
            boss.state.timer = 2;
            wizardHurt(boss);
            arenas = arenas && boss.state.hp == 4 - area &&
                     ((area == 4) ? (alternationBits & 4) : !(alternationBits & 4));
            if (area == 4)
                updateWizard(boss, 3.01f);
        }
        check(boss.state.gone && screen == PLAY && !complete,
              "Defeating the wizard opens the route to the original potion");
        boss.state.gone = 0;
        boss.state.active = 3;
        boss.state.hp = 3;
        boss.state.timer = 3;
        boss.state.x = 4880;
        boss.state.y = 165;
        updateCamera(true);
        camX = 4624;
        camY = 0;
        transitionTime = 0;
        render();
        capture("test-output/wizard.bmp");
    }
    check(arenas, "Wizard progresses through five authored arenas and gate masks");

    interfaceEnabled = true;
    endGameInterface();
    float before = page.cameraY;
    tickInterface(10);
    check(screen == ENDING && page.cameraY < before && page.level.id == 137502285691ULL,
          "Original ending credits scroll through the authored scene");
    transitionTime = 0;
    render();
    capture("test-output/ending-credits.bmp");
    confirm();
    transitionTime = 0;
    render();
    capture("test-output/ending.bmp");
    check(screen == ENDING && endingCredits.prompt && !endingCredits.exiting,
          "First confirm reveals Continue without skipping the credits camera");
    tickInterface(0.51f);
    confirm();
    tickInterface(2.01f);
    check(screen == TITLE, "Ending returns to the original title after confirmed white fade");
}
