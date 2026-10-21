#pragma once

// Regression coverage for wizard presentation. Runs only in the isolated BQ_ENABLE_TESTS executable.
void wizardPresentationChecks() {
    interfaceEnabled = false;
    musicEnabled = false;
    soundEnabled = true;
    debugMode = false;
    startStage(49);
    readyTimer = 0;
    int boss = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            boss = i;
    check(boss >= 0, "Original Wizard encounter is available for head and sound lifecycle checks");
    if (boss < 0)
        return;
    resetWizardMotion(entities[boss], 5489);
    resetWizardPresentation();
    stopSounds();
    Actor *head = actor(106556998914888ULL);
    bool allAnimations = true;
    const char *names[] = {"trolling1", "trolling2", "trolling3", "hurt"};
    for (int i = 0; i < 4; i++) {
        Animation *a = animation(head, names[i]);
        allAnimations &= a && eq(a->name, names[i]) && a->count > 0 && (i == 3 || a->count > 1);
    }
    check(
        allAnimations,
        "Wizard head resolves all three original trolling animations and the original hurt animation");
    wizardTroll(1);
    check(
        wizardPresentation.animation == 1 && activeAudioVoices() == 0,
        "At the native exact1.5-second cooldown trolling changes the idle head without playing an early laugh");
    wizardPresentationTick(0.01f);
    wizardTroll(2);
    int playing = wizardPresentation.animation;
    check(
        playing == 1 && activeAudioVoices() == 1,
        "A busy head keeps its animation while the independent greater-than1.5-second cooldown plays one original laugh");
    wizardTroll(0);
    check(activeAudioVoices() == 1,
          "Repeated same-frame trolling requests cannot restart or duplicate the laugh");
    float length = animationLength(animation(head, "trolling2"));
    wizardPresentationTick(length);
    check(wizardPresentation.animation == -1,
          "A completed trolling animation returns to the original looping base animation");
    wizardHeadHurt();
    wizardPresentationTick(0.21f);
    check(wizardPresentation.animation == 3,
          "Wizard hurt remains visible beyond the port's former0.2-second cutoff");
    wizardTroll(2);
    check(wizardPresentation.animation == 3,
          "Trolling cannot interrupt the original hurt animation");
    wizardPresentationTick(0.78f);
    check(wizardPresentation.animation == 3,
          "Wizard hurt remains selected before the native one-second boundary");
    wizardPresentationTick(0.02f);
    check(wizardPresentation.animation == -1,
          "Wizard hurt returns to idle at the native one-second boundary");
    resetWizardPresentation();
    wizardPresentation.cooldown = 2;
    stopSounds();
    wizardArmShots(entities[boss], 0.19f, 0.21f, true);
    check(
        wizardPresentation.animation >= 0 && wizardPresentation.animation <= 2 &&
            activeAudioVoices() == 1,
        "Native left-hand warning triggers trolling and laughter half a second before its first combat shot");
    cloudRenderBegin();
    drawActor(head, Rect{80, 80, 176, 208}, "", 0);
    u32 idle = cloudRenderPixels(Rect{80, 80, 176, 208}, false);
    bool visual = false;
    for (int i = 0; i < 3; i++) {
        Animation *a = animation(head, names[i]);
        for (int frame = 0; frame < a->count; frame++) {
            cloudRenderBegin();
            drawActor(head, Rect{80, 80, 176, 208}, names[i], (float)frame / a->duration, 0, false,
                      -1, 1, ANIMATION_STOP);
            visual |= cloudRenderPixels(Rect{80, 80, 176, 208}, false) != idle;
        }
    }
    check(
        visual,
        "Actual OpenGL rendering selects visible original trolling frames instead of always drawing idle");
    wizardPresentation.animation = 2;
    wizardPresentation.animationAge = 0.17f;
    wizardPresentation.cooldown = 0.43f;
    captureCheckpoint(false);
    resetWizardPresentation();
    restoreCheckpoint();
    check(
        wizardPresentation.animation == 2 &&
            absf(wizardPresentation.animationAge - 0.17f) < 0.0001f &&
            absf(wizardPresentation.cooldown - 0.43f) < 0.0001f,
        "Version-nine checkpoint preserves the current Wizard expression, animation age and laugh cooldown");
    SnapshotV8 *old =
        (SnapshotV8 *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SnapshotV8));
    memcpy(old, &checkpoint, sizeof(*old) - 4);
    old->version = 8;
    old->checksum = hashBytes(old, sizeof(*old) - 4);
    atomicWrite("test-output/checkpoint-v8.dat", old, sizeof(*old));
    HeapFree(GetProcessHeap(), 0, old);
    bool migrated = readCheckpoint("test-output/checkpoint-v8.dat");
    if (migrated)
        restoreCheckpoint();
    check(
        migrated && checkpoint.version == 8 && wizardPresentation.animation == -1,
        "Existing version-eight checkpoint migrates with original motion intact and safe new presentation defaults");
    soundEnabled = false;
    stopSounds();
    startStage(0);
}
