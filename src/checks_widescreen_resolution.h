#pragma once

// Regression coverage for widescreen resolution. Runs only in the isolated BQ_ENABLE_TESTS executable.

void widescreenResolutionChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/widescreen-resolution-saves");
    CreateDirectoryA(saveRoot, 0);
    bool oldInterface = interfaceEnabled, oldTurbo = turbo;
    int oldChoice = resolutionChoice;
    bool oldMode = windowMode;
    RECT oldClient;
    GetClientRect(window, &oldClient);
    musicEnabled = soundEnabled = false;
    bool presets = true;
    for (int i = 0; i < WINDOW_RESOLUTION_COUNT; i++) {
        const WindowResolution &r = windowResolutions[i];
        char expected[48];
        wsprintfA(expected, "%d x %d", r.width, r.height);
        presets &= r.width * 9 == r.height * 16 && eq(expected, r.caption);
    }
    check(
        presets && DEFAULT_WINDOW_RESOLUTION == 2 && windowResolutions[2].width == 1024 &&
            windowResolutions[2].height == 576 &&
            initialWindowResolution(1920, 1040, 16, 39) == 2 &&
            initialWindowResolution(900, 600, 16, 39) == 1 &&
            initialWindowResolution(700, 400, 16, 39) == 0,
        "Every Resolution option and its exact caption is16:9; default1024x576falls back to768x432or512x288when work-area borders require it");
    initInterface();
    u64 graphics = 0;
    for (int i = 0; i < levelCount; i++)
        if (eq(levels[i].props.get("name"), "Options - Graphics"))
            graphics = levels[i].id;
    navigate(graphics);
    int cycle = -1, apply = -1;
    for (int i = 0; i < page.level.objectCount; i++) {
        if (page.nodes[i].object->has("GfxResolutionButton"))
            cycle = i;
        if (page.nodes[i].object->has("GfxApplyButton"))
            apply = i;
    }
    bool buttons = graphics && cycle >= 0 && apply >= 0, actual = buttons, legacy = buttons,
         world = true;
    startStage(stageById(739118707375ULL));
    readyTimer = 0;
    captureCheckpoint(false);
    u32 saved = checkpoint.checksum, actorHash = hashBytes(entities, sizeof(Entity) * entityCount);
    navigate(graphics);
    windowMode = true;
    resolutionChoice = 0;
    for (int i = 0; i < WINDOW_RESOLUTION_COUNT && buttons; i++) {
        const WindowResolution &r = windowResolutions[i];
        buttons &= resolutionChoice == i;
        activate(page, apply);
        RECT client;
        GetClientRect(window, &client);
        Vec corner = nativePointerPosition((float)r.width, (float)r.height);
        actual &= client.right == r.width && client.bottom == r.height && clientW == r.width &&
                  clientH == r.height && viewportW == r.width && viewportH == r.height &&
                  absf(viewW / viewH - 16.0f / 9) < .00001f && absf(corner.x - viewW) < .001f &&
                  corner.y == 0;
        world &= checkpoint.checksum == saved &&
                 hashBytes(entities, sizeof(Entity) * entityCount) == actorHash;
        if (i == 0 || i == 2) {
            transitionTime = 0;
            render();
            char path[128];
            wsprintfA(path, "test-output/graphics-widescreen-%d.bmp", r.width);
            capture(path);
        }
        activate(page, cycle);
    }
    check(
        buttons && resolutionChoice == 0,
        "The real Graphics Resolution button cycles all four16:9choices and wraps back to the first option");
    check(
        actual,
        "The real Graphics Apply button produces exact16:9client dimensions and matching full-window viewport/pointer coordinates for every listed resolution");
    for (int i = 0; i < WINDOW_RESOLUTION_COUNT && buttons; i++) {
        UiPreferences old = {0x55535142, 1, i, 0, 1, 0};
        old.checksum = hashBytes(&old, sizeof(old) - 4);
        char path[MAX_PATH];
        savePath(path, "settings.dat");
        atomicWrite(path, &old, sizeof(old));
        initInterface();
        RECT client;
        GetClientRect(window, &client);
        legacy &= resolutionChoice == i && windowMode && turbo &&
                  client.right == windowResolutions[i].width &&
                  client.bottom == windowResolutions[i].height;
        world &= checkpoint.checksum == saved &&
                 hashBytes(entities, sizeof(Entity) * entityCount) == actorHash;
    }
    check(
        legacy,
        "Existing version1settings keep their width-choice index,window mode and Turbo preference while reopening at the corresponding new16:9height");
    check(
        world,
        "Applying every widescreen choice and reloading settings leaves the actual5-3Bean world state and existing checkpoint bytes unchanged");
    resolutionChoice = oldChoice;
    windowMode = oldMode;
    turbo = oldTurbo;
    interfaceEnabled = oldInterface;
    lstrcpyA(saveRoot, oldRoot);
    resizeWindow(oldClient.right, oldClient.bottom);
    screen = PLAY;
    startStage(0);
    readyTimer = 0;
    atomicWrite("test-output/widescreen-resolution-results.txt", testLog, lstrlenA(testLog));
}
