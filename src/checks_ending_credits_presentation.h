#pragma once

// Regression coverage for ending credits presentation. Runs only in the isolated BQ_ENABLE_TESTS executable.
#pragma float_control(precise, on, push)

u32 endingCreditsActualFrameHash() {
    nativeViewportProjection();
    glDisable(GL_SCISSOR_TEST);
    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawOriginalScene(page);
    int w = clientW, h = clientH;
    u8 *bytes = (u8 *)HeapAlloc(GetProcessHeap(), 0, w * h * 4);
    if (!bytes)
        return 0;
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
    u32 hash = hashBytes(bytes, w * h * 4);
    HeapFree(GetProcessHeap(), 0, bytes);
    return hash;
}

int endingCreditsNode(u64 id) {
    for (int i = 0; i < page.level.objectCount; i++)
        if (page.nodes[i].object->id == id)
            return i;
    return -1;
}

bool endingCreditsNodeActuallyDraws(u64 id) {
    int i = endingCreditsNode(id);
    if (i < 0)
        return false;
    bool hidden = page.nodes[i].hidden;
    u32 shown = endingCreditsActualFrameHash();
    page.nodes[i].hidden = true;
    u32 without = endingCreditsActualFrameHash();
    page.nodes[i].hidden = hidden;
    return shown != without;
}

void endingCreditsAutoplayIntegrationChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = true;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    memset(keys, 0, sizeof(keys));
    startStage(stageById(976871008727ULL));
    readyTimer = deathTimer = transitionTime = 0;
    entities[playerIndex].state.x = 6944;
    entities[playerIndex].state.y = 107;
    updateCamera(true);
    bool began = beginEndingSequence();
    for (int i = 0; i < 1500 && screen != ENDING; i++) {
        tickInterface(dt);
        tick(dt, false, false);
    }
    check(
        began && screen == ENDING && page.level.id == 137502285691ULL && !endingScene.active &&
            !endingCredits.prompt && !endingCredits.exiting,
        "Real final-stage rescue reaches the scrolling credits without keyboard or mouse input and without leaking the previous cutscene lock");
    if (screen != ENDING) {
        interfaceEnabled = false;
        startStage(0);
        return;
    }
    char trace[2000] =
        "age_milli,camera_centre_y_milli,title_visible,credits_visible,name_visible\r\n";
    const int targets[] = {2, 15, 25, 35};
    bool camera = true, art = false, names = false, laterNames = false;
    for (int k = 0; k < 4; k++) {
        while (page.age < targets[k]) {
            tickInterface(dt);
            tick(dt, false, false);
        }
        camera &= absf(page.cameraY + viewH * 0.5f - (6144 - 30 * page.age)) < 0.01f &&
                  !endingCredits.prompt && !endingCredits.exiting;
        bool title = endingCreditsNodeActuallyDraws(5988538666855140ULL),
             credits = endingCreditsNodeActuallyDraws(1200001703360377ULL),
             name = endingCreditsNodeActuallyDraws(8379907825954604ULL);
        if (k == 0)
            art = endingCreditsNodeActuallyDraws(168960717282056ULL);
        if (k == 2)
            names = credits && name;
        if (k == 3)
            laterNames = endingCreditsNodeActuallyDraws(7781280673241061ULL) &&
                         endingCreditsNodeActuallyDraws(9206999031573047ULL);
        char row[150], file[100];
        wsprintfA(row, "%d,%d,%d,%d,%d\r\n", (int)(page.age * 1000),
                  (int)((page.cameraY + viewH * 0.5f) * 1000), title ? 1 : 0, credits ? 1 : 0,
                  name ? 1 : 0);
        lstrcatA(trace, row);
        wsprintfA(file, "test-output/ending-credits-no-input-%ds.bmp", targets[k]);
        captureCase(file);
    }
    atomicWrite("test-output/ending-credits-no-input-trace.csv", trace, lstrlenA(trace));
    check(
        camera && art,
        "Actual no-input credits keep their native30pixel-per-second camera throughout2through35seconds and draw the authored initial illustration");
    check(
        names && laterNames,
        "Actual OpenGL credits and original artist/code/music names scroll into view automatically; hiding each real text node changes the composed frame without any input");
    float beforeAge = page.age, beforeCamera = page.cameraY;
    u32 beforeFrame = endingCreditsActualFrameHash();
    acceptEndingCredits(page.age);
    bool prompt = endingCredits.prompt && !endingCredits.exiting;
    u32 afterFrame = endingCreditsActualFrameHash();
    int promptNode = endingCreditsNode(6373888218169984ULL);
    bool hidden = false;
    if (promptNode >= 0) {
        hidden = page.nodes[promptNode].hidden;
        page.nodes[promptNode].hidden = true;
    }
    u32 noPrompt = endingCreditsActualFrameHash();
    if (promptNode >= 0)
        page.nodes[promptNode].hidden = hidden;
    check(
        prompt && page.age == beforeAge && page.cameraY == beforeCamera &&
            beforeFrame == afterFrame && beforeFrame == noPrompt,
        "First accept during the real scrolling credits only arms the initially transparent Continue pulse and neither changes camera nor reveals ordinary names");
    interfaceEnabled = false;
    memset(keys, 0, sizeof(keys));
    startStage(0);
}

void endingCreditsPresentationChecks() {
    const EndingCreditsPresentationInput in = {6144, 256, 216, 608, 208};
    EndingCreditsPresentation p = endingCreditsPresentation(in, 0);
    check(
        p.tintR == 1 && p.tintG == 1 && p.tintB == 1 && p.tintA == 1 && p.backdropA == 0 &&
            p.nameAlpha == 1 && p.endAlpha == 0,
        "Native credits start with the coloured fixed-layer illustration, transparent backdrop tint and noTheEnd text independent of any click");
    p = endingCreditsPresentation(in, 9);
    check(
        p.tintR == 0.5f && p.tintG == 0.5f && p.tintB == 0.5f && p.tintA == 1 && p.backdropR == 1 &&
            p.backdropG == 1 && p.backdropB == 1 && p.backdropA == 0.5f,
        "Original credits independently darken illustration RGB and raise backdrop alpha halfway during seconds6through12");
    p = endingCreditsPresentation(in, 24);
    check(
        p.tintR == 0 && p.tintA == 1 && p.backdropA == 1 && p.emilioX == 256 && p.mariaX == 608 &&
            p.emilioY == 216 && p.mariaY == 208 && eq(p.characterAnimation, "") && p.endAlpha == 0,
        "Original no-input credits finish the initial background darkening before ordinary names scroll into view while final characters remain at their authored locations");
    float begin = (in.startY - 192) / 30 + 2, end = begin + 3;
    p = endingCreditsPresentation(in, begin);
    check(
        eq(p.characterAnimation, "") && p.emilioX == 256 && p.mariaX == 608 && p.nameAlpha == 1,
        "Native credits retain default character instances at the exact start threshold and begin walking strictly after it");
    p = endingCreditsPresentation(in, begin + 1.5f);
    check(
        absf(p.emilioX - 339) < 0.001f && absf(p.mariaX - 525) < 0.001f && p.emilioY == 216 &&
            p.mariaY == 208 && p.mariaScaleX == -1 && eq(p.characterAnimation, "walking") &&
            absf(p.walkingAge - 1.5f) < 0.001f && p.nameAlpha == 0,
        "Original final credits characters walk toward midpoint plus-minus10 horizontally while the separate name labels fade out");
    p = endingCreditsPresentation(in, end);
    check(
        p.emilioX == 422 && p.mariaX == 442 && eq(p.characterAnimation, "walking") &&
            absf(p.walkingAge - 3) < 0.001f,
        "Credits native endpoint keeps the walking instance attached through the exact final threshold");
    p = endingCreditsPresentation(in, end + 0.01f);
    check(
        eq(p.characterAnimation, "") && absf(p.defaultAge - (end + 0.01f - 3)) < 0.001f &&
            p.endAlpha == 0,
        "Original credits return to the retained default instances after walking instead of using an absent idle animation or restarting their ages");
    p = endingCreditsPresentation(in, (in.startY - 192) / 30 + 8);
    check(absf(p.endAlpha - 0.5f) < 0.0001f && p.nameAlpha == 0 && p.emilioX == 422 &&
              p.mariaX == 442,
          "OriginalTheEnd text fades halfway atTplus8 after the two credits characters have met");
    endingCreditsAutoplayIntegrationChecks();
}

#pragma float_control(pop)
