#pragma once

// Regression coverage for attachment layer native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void attachmentLayerNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/attachment-layer-saves");
    CreateDirectoryA(saveRoot, 0);
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    startStage(stageById(806701038719ULL));
    readyTimer = 0;
    int ball = -1, jelly = -1, parents = 0;
    bool retained = true;
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        if (e.parent >= 0 && e.motion == 2) {
            parents++;
            retained &= e.z == classById(e.source->id)->z && e.layer == entities[e.parent].layer;
            ball = i;
        }
        if (e.kind == TRAMPOLINE && absf(e.state.x - 1024) < 1)
            jelly = i;
    }
    check(
        parents == 2 && retained,
        "Both authored5-6orbiting enemy balls retain their own class Z6 while inheriting only the owner layer");
    if (ball >= 0 && jelly >= 0) {
        Entity &e = entities[ball];
        Entity &j = entities[jelly];
        e.state.x = j.state.x;
        e.state.y = j.state.y;
        e.state.gone = 0;
        camX = j.state.x - 256;
        camY = 0;
        clearEffects();
        cloudRenderBegin();
        drawWorld();
        u32 front = cloudRenderPixels(Rect{240, 32, 272, 64}, false);
        captureCase("test-output/attachment-layer-ball-front.bmp");
        e.state.gone = 1;
        cloudRenderBegin();
        drawWorld();
        u32 hidden = cloudRenderPixels(Rect{240, 32, 272, 64}, false);
        e.state.gone = 0;
        check(
            e.z == 6 && j.z == 1 && e.layer == j.layer && front != hidden,
            "Production world rendering shows the5-6enemy ball through the middle of the opaque jelly instead of drawing it underneath");
    } else
        check(false, "Production5-6jelly and orbiting ball exist for overlap verification");
    bool all = true;
    int attachments = 0;
    for (int stage = 0; stage < stageCount; stage++) {
        startStage(stage);
        readyTimer = 0;
        for (int i = 0; i < entityCount; i++) {
            Entity &e = entities[i];
            if (e.parent < 0)
                continue;
            Object *base = classById(e.source->id);
            all &= base && e.z == base->z && e.layer == entities[e.parent].layer;
            attachments++;
        }
    }
    check(
        all && attachments > 100,
        "All actual campaign attachments including Wizard pools and wrecking-ball links retain their source class Z and intended owner layer");
    atomicWrite("test-output/attachment-layer-results.txt", testLog, lstrlenA(testLog));
    lstrcpyA(saveRoot, oldRoot);
    startStage(0);
    readyTimer = 0;
}
