#pragma once

// Regression coverage for respawn controls. Runs only in the isolated BQ_ENABLE_TESTS executable.

void respawnControlChecks() {
    const float dt = 1.0f / 120.0f;
    interfaceEnabled = true;
    musicEnabled = soundEnabled = false;
    debugMode = false;
    RespawnTransition state = {};
    respawnBegin(state, RESPAWN_DEATH);
    respawnAdvance(state, 0.1f);
    check(respawnBegin(state, RESPAWN_CHECKPOINT) && state.phase == RESPAWN_FADE_OUT &&
              state.request == RESPAWN_CHECKPOINT && state.age == 0,
          "Manual checkpoint interrupts the death wait immediately");
    respawnCancel(state);
    respawnBegin(state, RESPAWN_DEATH);
    respawnAdvance(state, 2.01f);
    respawnAdvance(state, 0.14f);
    float alpha = respawnAlpha(state);
    check(respawnBegin(state, RESPAWN_RESTART) && state.request == RESPAWN_RESTART &&
              absf(respawnAlpha(state) - alpha) < 0.00001f,
          "Manual restart takes over an existing death fade without changing opacity");
    respawnAdvance(state, 1);
    respawnRestored(state);
    respawnAdvance(state, 0.1f);
    state.request = RESPAWN_DEATH;
    alpha = respawnAlpha(state);
    check(respawnBegin(state, RESPAWN_RESTART) && state.phase == RESPAWN_FADE_OUT &&
              absf(respawnAlpha(state) - alpha) < 0.00001f,
          "Manual restart during death reveal reverses the fade continuously");

    bool fastCheckpoint = false, fastRestart = false, boundCheckpoint = false, boundRestart = false,
         noDuplicate = true;
    int oldPrimary[2] = {primaryKey[6], primaryKey[7]},
        oldSecondary[2] = {secondaryKey[6], secondaryKey[7]};
    for (int scenario = 0; scenario < 4; scenario++) {
        startStage(0);
        readyTimer = 0;
        coins = 7;
        jumps = 9;
        entities[playerIndex].state.vy = 0;
        captureCheckpoint(false);
        float savedX = entities[playerIndex].state.x;
        entities[playerIndex].state.x += 70;
        coins = 11;
        die();
        int deathsBefore = deaths;
        tick(dt, false, false);
        int key = scenario == 0 ? 'S' : scenario == 1 ? 'R' : scenario == 2 ? VK_F5 : VK_F6;
        if (scenario >= 2) {
            primaryKey[6] = VK_F5;
            primaryKey[7] = VK_F6;
            secondaryKey[6] = secondaryKey[7] = 0;
        }
        keyPressed(key);
        bool accepted =
            screen == PLAY && respawnTransition.phase == RESPAWN_FADE_OUT &&
            respawnTransition.request == (scenario % 2 ? RESPAWN_RESTART : RESPAWN_CHECKPOINT);
        int frames = 0;
        while (respawnBusy(respawnTransition) && frames++ < 120)
            tick(dt, false, false);
        bool correct = accepted && frames < 100 && deathTimer == 0 &&
                       respawnAlpha(respawnTransition) == 0 &&
                       absf(entities[playerIndex].state.x - savedX) < 0.01f &&
                       coins == (scenario % 2 ? 0 : 7) && jumps == (scenario % 2 ? 0 : 9);
        if (scenario == 0)
            fastCheckpoint = correct;
        if (scenario == 1)
            fastRestart = correct;
        if (scenario == 2)
            boundCheckpoint = correct;
        if (scenario == 3)
            boundRestart = correct;
        noDuplicate &= deaths == deathsBefore;
    }
    primaryKey[6] = oldPrimary[0];
    primaryKey[7] = oldPrimary[1];
    secondaryKey[6] = oldSecondary[0];
    secondaryKey[7] = oldSecondary[1];
    check(
        fastCheckpoint,
        "Actual S during death restores saved position and counters within the manual fade, without waiting two seconds");
    check(
        fastRestart,
        "Actual R during death resets the stage within the manual fade, without waiting two seconds");
    check(boundCheckpoint && boundRestart,
          "Rebound checkpoint and retry keys can also interrupt death immediately");
    check(noDuplicate,
          "Manual death interruption cannot issue a second death or automatic delayed restore");

    startStage(0);
    readyTimer = 0;
    coins = 6;
    captureCheckpoint(false);
    float x = entities[playerIndex].state.x;
    entities[playerIndex].state.x += 80;
    die();
    for (int i = 0; i < 260; i++)
        tick(dt, false, false);
    bool dark = respawnAlpha(respawnTransition) > 0;
    keyPressed(VK_ESCAPE);
    int frames = 0;
    while (respawnBusy(respawnTransition) && frames++ < 120)
        tick(dt, false, false);
    check(
        dark && frames < 120 && screen == PAUSE && deathTimer == 0 &&
            respawnAlpha(respawnTransition) == 0 && coins == 6 &&
            absf(entities[playerIndex].state.x - x) < 0.01f,
        "Death fade completes under the pause menu and restores the scene while keeping the menu paused");
    float pausedX = entities[playerIndex].state.x;
    tick(1, false, true);
    check(screen == PAUSE && entities[playerIndex].state.x == pausedX,
          "Finishing a death fade under pause does not resume gameplay physics");

    bool leftClean = true, noStaleLoad = true, menuVisible = true;
    for (int phase = 0; phase < 4; phase++) {
        startStage(0);
        readyTimer = 0;
        die();
        if (phase >= 1) {
            respawnAdvance(respawnTransition, 2.01f);
            respawnAdvance(respawnTransition, phase == 1 ? 0.175f : 0.35f);
        }
        if (phase == 3) {
            restoreCheckpoint();
            respawnRestored(respawnTransition);
            respawnAdvance(respawnTransition, 0.175f);
        }
        openPause();
        tickInterface(pauseTransitionNativeSeconds);
        int select = -1;
        for (int i = 0; i < hudScene.level.objectCount; i++)
            if (hudScene.nodes[i].object->has("LevelSelectButton")) {
                select = i;
                break;
            }
        if (select < 0) {
            leftClean = false;
            continue;
        }
        activate(hudScene, select);
        leftClean &= screen == SELECT && !respawnBusy(respawnTransition) &&
                     respawnAlpha(respawnTransition) == 0 && deathTimer == 0;
        for (int i = 0; i < 360; i++) {
            tickInterface(dt);
            tick(dt, false, false);
        }
        noStaleLoad &= screen == SELECT && !respawnBusy(respawnTransition) &&
                       respawnAlpha(respawnTransition) == 0;
        transitionTime = 0;
        page.minimized = false;
        page.minimizePhase = 0;
        render();
        u8 clean[4] = {};
        glReadBuffer(GL_FRONT);
        glReadPixels(viewportX + viewportW / 2, viewportY + viewportH / 2, 1, 1, GL_RGBA,
                     GL_UNSIGNED_BYTE, clean);
        menuVisible &= clean[0] + clean[1] + clean[2] > 20;
        if (phase == 2)
            capture("test-output/death-exit-selector-clear.bmp");
    }
    check(
        leftClean,
        "Actual pause Level Select button clears death wait, fade out, opaque black and fade in ownership");
    check(
        noStaleLoad,
        "Leaving death for the selector cannot later restore the old stage or revive its black overlay");
    check(
        menuVisible,
        "Actual OpenGL selector remains visibly rendered after leaving every death transition phase");
    startStage(0);
    readyTimer = 0;
    die();
    respawnAdvance(respawnTransition, 2.01f);
    respawnAdvance(respawnTransition, 0.175f);
    navigate(12192614360109026946ULL);
    check(screen == TITLE && !respawnBusy(respawnTransition) &&
              respawnAlpha(respawnTransition) == 0,
          "Direct navigation to title also releases the old stage's death overlay");
    startStage(0);
    readyTimer = 0;
    die();
    tick(0.1f, false, false);
    openPause();
    keyPressed('R');
    frames = 0;
    while (respawnBusy(respawnTransition) && frames++ < 120)
        tick(dt, false, false);
    check(screen == PLAY && frames < 100 && deathTimer == 0 && !respawnBusy(respawnTransition),
          "R pressed from the death pause menu resumes into an immediate manual restart");
    startStage(0);
    interfaceEnabled = false;
}
