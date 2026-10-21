#pragma once

// Regression coverage for skeletonkey native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void skeletonKeyNativeChecks() {
    Progress beforeTest = progress;
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/skeletonkey-saves");
    CreateDirectoryA(saveRoot, 0);
    musicEnabled = soundEnabled = false;
    interfaceEnabled = false;
    debugMode = false;
    debugSequence = 0;
    currentStage = 0;
    memset(&progress, 0, sizeof(progress));
    progress.bestCoins[0] = 3;
    progress.bestJumps[0] = 7;
    progress.cleared[0] = 1;
    Progress original = progress;
    screen = TITLE;
    titleAttractReset();
    const char *old = "DEBUG";
    for (int i = 0; old[i]; i++)
        keyPressed(old[i]);
    check(!debugMode, "The old DEBUG title sequence no longer unlocks the campaign");
    const char *code = "SKELETONKEY";
    debugSequence = 0;
    jumps = 7;
    for (int i = 0; code[i]; i++)
        keyPressed(code[i]);
    bool all = true;
    for (int i = 0; i < stageCount; i++)
        all &= selectorLevelUnlocked(levels[stageList[i]].id);
    check(
        !debugMode && debugSequence == 0 && jumps == 7 && worldOpen(6) &&
            progress.unlocked == stageCount - 1 && all,
        "SKELETONKEY uses normal saved unlocks for every stage without debug mode or a jump-count reset");
    check(
        memcmp(progress.bestCoins, original.bestCoins, sizeof(progress.bestCoins)) == 0 &&
            memcmp(progress.bestJumps, original.bestJumps, sizeof(progress.bestJumps)) == 0 &&
            memcmp(progress.axolotls, original.axolotls, sizeof(progress.axolotls)) == 0 &&
            memcmp(progress.cleared, original.cleared, sizeof(progress.cleared)) == 0,
        "Persistent title unlock preserves every existing diamond, shoe, axolotl and completion record");
    Progress saved = progress;
    memset(&progress, 0, sizeof(progress));
    readProgress();
    check(
        !debugMode && progress.unlocked == stageCount - 1 &&
            memcmp(&progress, &saved, sizeof(progress)) == 0,
        "The actual startup progress loader reads the immediately saved full unlock with intact records and preferences");
    prepareOrdinaryPhaseAuditNative();
    debugMode = false;
    tick(1 / 120.0f, false, false);
    tick(1 / 120.0f, false, false);
    check(ordinaryJumpLaunchedNative && jumps == 1,
          "Persistent unlock counts an actual ordinary floor bounce once");
    int pad = prepareSpringPhaseNative();
    debugMode = false;
    for (int n = 0; n < 30 && jumps == 0; n++)
        tick(1 / 120.0f, false, false);
    check(pad >= 0 && pinballLaunchedThisStepNative && jumps == 1,
          "Persistent unlock counts a real spring MegaJump once");
    PinballDetailImpactNative pin = runPinballDetailNative(0, true);
    check(
        pin.contact && pin.jumps == 1 && pin.pinVoices == 2 && pin.jumpVoices == 0,
        "Persistent unlock counts a real pinball bounce while keeping its existing dedicated sound");
    pad = prepareCampaignTrampolineNative(false);
    debugMode = false;
    for (int n = 0; n < 40 && jumps == 0; n++)
        tick(1 / 120.0f, false, false);
    check(pad >= 0 && trampolineLaunchedThisStepNative && jumps == 1,
          "Persistent unlock counts a real jelly bounce once");
    jumps = 9;
    captureCheckpoint(false);
    jumps = 23;
    restoreCheckpoint();
    check(!debugMode && jumps == 9,
          "Checkpoint restore preserves the saved jump count with normal saving enabled");
    debugMode = false;
    debugSequence = 0;
    interfaceEnabled = false;
    screen = OPTIONS;
    for (int i = 0; code[i]; i++)
        keyPressed(code[i]);
    check(!debugMode,
          "SKELETONKEY remains restricted to the title rather than activating in other menus");
    atomicWrite("test-output/skeletonkey-results.txt", testLog, lstrlenA(testLog));
    lstrcpyA(saveRoot, oldRoot);
    progress = beforeTest;
    startStage(0);
    readyTimer = 0;
}

// Preview the real composed scenes in the owned hidden test process.
void previewWindowsPortCredits() {
    interfaceEnabled = true;
    debugMode = musicEnabled = soundEnabled = false;
    char root[MAX_PATH];
    lstrcpyA(root, saveRoot);
    lstrcpyA(saveRoot, "test-output/port-credit-preview-saves");
    CreateDirectoryA(saveRoot, 0);
    initInterface();
    navigate(8466014814437108852ULL);
    page.age = 2;
    transitionTime = endingCreditsFade = 0;
    render();
    capture("test-output/port-credits-settings.bmp");
    navigate(137502285691ULL);
    const float centre[] = {4520, 4260};
    for (int i = 0; i < 2; i++) {
        page.age = (6144 - centre[i]) / 30;
        page.cameraY = centre[i] - viewH * .5f;
        transitionTime = endingCreditsFade = 0;
        render();
        capture(i ? "test-output/port-credits-ending-lower.bmp"
                  : "test-output/port-credits-ending-upper.bmp");
    }
    interfaceEnabled = false;
    lstrcpyA(saveRoot, root);
    startStage(0);
    readyTimer = 0;
}
