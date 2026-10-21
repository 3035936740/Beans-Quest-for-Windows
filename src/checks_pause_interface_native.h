#pragma once

// Regression coverage for pause interface native. Runs only in the isolated BQ_ENABLE_TESTS executable.

// Exercise actual interface entry points with the original HUD and stage.
// The optional callback lets standalone probes keep the shared test log intact.
void pauseInterfaceNativeChecks(void (*verify)(bool, const char *) = check, bool images = false) {
    char previousRoot[MAX_PATH];
    lstrcpyA(previousRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/pause-detail-interface-saves");
    CreateDirectoryA(saveRoot, 0);
    Progress oldProgress = progress;
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldMusic = musicEnabled,
         oldSound = soundEnabled, oldTurbo = turbo, oldWindowMode = windowMode;
    int oldResolution = resolutionChoice, oldRebind = rebindAction;
    Bindings oldBindings = bindings;
    bool oldKeys[256];
    memcpy(oldKeys, keys, sizeof(keys));
    debugMode = true;
    musicEnabled = soundEnabled = false;
    interfaceEnabled = true;
    rebindAction = -1;
    startStage(0);
    readyTimer = 0;
    readyOpening.showing = false;
    transitionTime = 0;
    bindPauseInterfaceNative();
    int resume = -1;
    for (int i = 0; i < hudScene.level.objectCount; i++)
        if (hudScene.nodes[i].object->id == pauseInitialFocusNative)
            resume = i;
    bool authored = resume >= 0 && hudScene.level.id == 14897698531695279339ULL &&
                    pauseLayerNative == 1 && pauseBackgroundNative == 8401461749820010943ULL;
    Rect resting = {};
    u32 nodeHash = 0;
    if (resume >= 0) {
        nodeHash = hashBytes(&hudScene.nodes[resume].rect, sizeof(Rect));
        resting = visualRect(hudScene, hudScene.nodes[resume]);
    }
    State beanBefore = entities[playerIndex].state;
    float timeBefore = elapsed;
    pointerHeld = true;
    pointerNode = resume;
    openPause();
    Rect entry = resume >= 0 ? visualRect(hudScene, hudScene.nodes[resume]) : Rect{};
    verify(
        authored && screen == PAUSE && hudScene.focus == -1 && !pointerHeld && pointerNode == -1 &&
            !pauseMenuInputReady() && absf(entry.l - resting.l + viewW) < .001f &&
            entry.b == resting.b,
        "Actual original HUD Pause starts at the native initial X displacement, clears held pointer and defers focus");
    tick(.075f, true, false);
    tickInterface(.075f);
    Rect middle = resume >= 0 ? visualRect(hudScene, hudScene.nodes[resume]) : Rect{};
    Vec oldCenter = center(resting);
    confirm();
    keyPressed(VK_TAB);
    mouseInterface(oldCenter.x, oldCenter.y, true);
    pointerDown(oldCenter.x, oldCenter.y);
    pointerUp(oldCenter.x, oldCenter.y);
    bool unchanged = hashBytes(&beanBefore, sizeof(beanBefore)) ==
                         hashBytes(&entities[playerIndex].state, sizeof(State)) &&
                     elapsed == timeBefore;
    verify(
        screen == PAUSE && !pauseInterfaceNative.resuming && hudScene.focus == -1 && unchanged &&
            absf(middle.l - resting.l) < .001f &&
            absf(middle.b - resting.b + viewH * .5857864376f) < .002f,
        "Real pause entry uses the sine Y offset while tick, confirm, Tab and pointer input leave Bean and gameplay time paused");
    if (images) {
        render();
        capture("test-output/pause-detail-interface-entry.bmp");
    }
    tickInterface(pauseTransitionNativeSeconds - pauseInterfaceNative.showAge);
    Rect settled = resume >= 0 ? visualRect(hudScene, hudScene.nodes[resume]) : Rect{};
    float dx = 0, dy = 0;
    if (resume >= 0)
        layerOffset(hudScene, hudScene.nodes[resume].object->layer, &dx, &dy);
    Vec click = center(shifted(settled, dx, dy));
    verify(
        screen == PAUSE && pauseMenuInputReady() && hudScene.focus == resume && resume >= 0 &&
            pointerHit(click.x, click.y) == resume && absf(settled.l - resting.l) < .001f &&
            absf(settled.b - resting.b) < .001f &&
            hashBytes(&hudScene.nodes[resume].rect, sizeof(Rect)) == nodeHash,
        "Entry completion focuses configured Resume and the production pointer hit test uses its settled rendered rectangle without mutating authored bounds");
    if (images) {
        render();
        capture("test-output/pause-detail-interface-settled.bmp");
    }
    pointerDown(click.x, click.y);
    pointerUp(click.x, click.y);
    tick(.05f, false, true);
    tickInterface(.05f);
    verify(
        screen == PAUSE && pauseInterfaceNative.resuming && hudScene.focus == -1 && !pointerHeld &&
            elapsed == timeBefore &&
            hashBytes(&beanBefore, sizeof(beanBefore)) ==
                hashBytes(&entities[playerIndex].state, sizeof(State)),
        "A genuine Resume pointer press/release begins deferred exit and preserves paused Bean and elapsed gameplay time");
    if (images) {
        render();
        capture("test-output/pause-detail-interface-exit.bmp");
    }
    float exitAge = pauseInterfaceNative.resumeAge;
    keyPressed(VK_ESCAPE);
    keyPressed(VK_ESCAPE);
    bool preserved = pauseInterfaceNative.resumeAge == exitAge;
    tick(.1f, false, false);
    tickInterface(pauseTransitionNativeSeconds - pauseInterfaceNative.resumeAge);
    verify(
        preserved && screen == PAUSE && pauseInterfaceNative.active &&
            pauseInterfaceNative.resuming && hudScene.focus == -1 && elapsed == timeBefore,
        "Repeated desktop Escape requests preserve the running exit clock and reaching its terminal age keeps gameplay paused");
    tickInterface(0);
    bool terminal = screen == PLAY && !pauseInterfaceNative.active && hudScene.focus == -1 &&
                    elapsed == timeBefore;
    tick(1.0f / 120, false, false);
    verify(
        terminal && elapsed > timeBefore,
        "The following production UI update completes resume once and only the subsequent gameplay tick advances world time");
    keys[VK_LEFT] = keys[VK_RIGHT] = true;
    procedure(window, WM_KILLFOCUS, 0, 0);
    Rect unfocused = resume >= 0 ? visualRect(hudScene, hudScene.nodes[resume]) : Rect{};
    verify(
        screen == PAUSE && pauseMenuInputReady() && !pauseInterfaceNative.resuming &&
            hudScene.focus == resume && !keys[VK_LEFT] && !keys[VK_RIGHT] &&
            absf(unfocused.l - resting.l) < .001f && absf(unfocused.b - resting.b) < .001f,
        "The actual WM_KILLFOCUS handler clears movement keys and opens an immediate settled pause with Resume focused");
    keyPressed('S');
    bool checkpointStopped = screen == PLAY && !pauseInterfaceNative.active &&
                             respawnTransition.request == RESPAWN_CHECKPOINT;
    respawnCancel(respawnTransition);
    openPause(false);
    keyPressed('R');
    bool restartStopped = screen == PLAY && !pauseInterfaceNative.active &&
                          respawnTransition.request == RESPAWN_RESTART;
    respawnCancel(respawnTransition);
    tickInterface(.3f);
    verify(
        checkpointStopped && restartStopped && screen == PLAY && !pauseInterfaceNative.active,
        "Both desktop S and R interrupt pause presentation immediately and leave no delayed resume after their restore requests");
    openPause();
    tickInterface(.06f);
    resumePauseInterfaceNative();
    tickInterface(.05f);
    openSelect();
    u64 selector = page.level.id;
    bool left = screen == SELECT && !pauseInterfaceNative.active;
    tickInterface(.3f);
    verify(
        left && screen == SELECT && page.level.id == selector && !pauseInterfaceNative.active,
        "Leaving an unfinished pause exit for the original selector cancels presentation so later UI updates cannot return to PLAY");
    respawnCancel(respawnTransition);
    resetPauseInterfaceNative();
    startStage(0);
    readyTimer = 0;
    transitionTime = 0;
    lstrcpyA(saveRoot, previousRoot);
    progress = oldProgress;
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    musicEnabled = oldMusic;
    soundEnabled = oldSound;
    turbo = oldTurbo;
    windowMode = oldWindowMode;
    resolutionChoice = oldResolution;
    rebindAction = oldRebind;
    bindings = oldBindings;
    memcpy(keys, oldKeys, sizeof(keys));
}
