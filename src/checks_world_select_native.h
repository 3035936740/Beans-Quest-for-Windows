#pragma once

// Regression coverage for world select native. Runs only in the isolated BQ_ENABLE_TESTS executable.
void worldSelectNativeChecks() {
    bool oldInterface = interfaceEnabled, oldMusic = musicEnabled, oldSound = soundEnabled,
         oldDebug = debugMode;
    Progress oldProgress = progress;
    float oldW = viewW, oldH = viewH, oldScale = screenScale;
    int oldStage = currentStage;
    musicEnabled = soundEnabled = debugMode = false;
    interfaceEnabled = true;
    currentStage = 0;
    navigate(18006219643615701355ULL);
    transitionTime = 0;
    int cards = 0;
    bool spacing = true;
    for (int i = 0; i < page.level.objectCount; i++)
        if (page.nodes[i].world > 0) {
            cards++;
            spacing &=
                absf(center(page.nodes[i].rect).x -
                     (page.level.size.x * .5f + (displayWorld(page.nodes[i].world) - 1) * 224)) <
                .001f;
        }
    check(
        cards == 6 && spacing,
        "All six authored chapter cards match native224-unit visual slots including the2/3/4world remapping and the final boss card");
    int second = -1;
    for (int i = 0; i < page.level.objectCount; i++)
        if (page.nodes[i].world == 4)
            second = i;
    float origin = page.cameraX;
    page.focus = second;
    tickInterface(1 / 120.0f);
    check(
        second >= 0 && absf(page.cameraX - origin - 2048.0f / 120) < .001f,
        "The production chapter camera moves exactly2048original units per second instead of an exponential distance-dependent speed");
    for (int i = 0; i < 30; i++)
        tickInterface(1 / 120.0f);
    check(
        absf(page.cameraX - origin - 224) < .001f,
        "Chapter camera reaches the next authored card exactly without overshoot or a residual easing tail");
    WorldSelectPointerNative pointer = {};
    worldSelectBeginNative(pointer, 200);
    worldSelectDragNative(pointer, 0, 2);
    pointer.x = 190;
    float d = worldSelectDragNative(pointer, .01f, 2);
    bool below = !pointer.dragged && d == 0;
    pointer.x = 189;
    d = worldSelectDragNative(pointer, .01f, 2);
    check(
        below && pointer.dragged && pointer.anchorX == 195 && d == 3,
        "Native drag begins strictly beyond five camera units and consumes five physical pixels once even at2xscale");
    bool flicks = true;
    for (int direction = -1; direction <= 1; direction += 2) {
        worldSelectBeginNative(pointer, 200);
        worldSelectDragNative(pointer, 0, 1);
        pointer.x = 200 - direction * 30.0f;
        worldSelectDragNative(pointer, .2f, 1);
        flicks &= worldSelectReleaseNative(pointer, 3, pointer.x, 1) == 3 + direction;
        worldSelectBeginNative(pointer, 200);
        worldSelectDragNative(pointer, 0, 1);
        pointer.x = 200 - direction * 30.0f;
        worldSelectDragNative(pointer, .20001f, 1);
        flicks &= worldSelectReleaseNative(pointer, 3, pointer.x, 1) == 3;
    }
    check(
        flicks,
        "Short native swipes advance one chapter through the inclusive200msboundary while slower sub-half-card drags return to the original chapter");
    bool halves = true;
    for (int direction = -1; direction <= 1; direction += 2) {
        pointer = WorldSelectPointerNative{};
        pointer.anchorX = 0;
        pointer.dragged = true;
        pointer.age = 1;
        halves &= worldSelectReleaseNative(pointer, 3, -direction * 112.0f, 1) == 3 + direction;
    }
    check(
        halves,
        "A released drag rounds both positive and negative exact half-card displacements away from zero before clamping focus");
    pointer = WorldSelectPointerNative{};
    pointer.dragged = true;
    pointer.age = 1;
    bool limits = worldSelectReleaseNative(pointer, 1, 4000, 1) == 1;
    pointer.dragged = true;
    limits &= worldSelectReleaseNative(pointer, 6, -4000, 1) == 6;
    check(
        limits,
        "World drag selection remains inside all six chapters even after a large drag past either endpoint");
    check(
        absf(worldSelectTargetNative(1, -50) + 34.657359f) < .0001f &&
            absf(worldSelectTargetNative(6, 50) - 1154.657349f) < .001f &&
            worldSelectTargetNative(3, 17) == 465,
        "Both chapter-strip ends use the original logarithmic50*ln(1+.02*overflow)resistance instead of a hard camera clamp");
    bool scales = true;
    const float widths[] = {512, 864, 1280}, scalesToTest[] = {1, 2, 3};
    for (int pass = 0; pass < 3; pass++) {
        viewW = widths[pass];
        screenScale = scalesToTest[pass];
        currentStage = 0;
        navigate(18006219643615701355ULL);
        float before = page.cameraX;
        pointerDown(200, 300);
        tickInterface(0);
        mouseInterface(170, 300, false);
        tickInterface(.1f);
        bool dragged =
            pointerDragged && absf(page.cameraX - before - (30 - 5 / screenScale)) < .001f;
        pointerUp(170, 300);
        scales &= dragged && screen == WORLD && selectedWorld == 4 &&
                  page.nodes[page.focus].world == 4 && !pointerHeld;
        tickInterface(.2f);
        scales &= absf(page.cameraX - (page.level.size.x * .5f + 224 - viewW * .5f)) < .001f;
    }
    check(
        scales,
        "Real down/move/up handlers perform short-flick chapter selection and centre the correct remapped card at three viewport widths and pixel scales");
    currentStage = 0;
    navigate(18006219643615701355ULL);
    pointerDown(200, 300);
    tickInterface(0);
    mouseInterface(170, 300, false);
    tickInterface(.21f);
    pointerUp(170, 300);
    tickInterface(.1f);
    check(
        selectedWorld == 1 && absf(page.cameraX - (page.level.size.x - viewW) * .5f) < .001f,
        "A slow short drag through the production input path snaps back to the original card instead of inventing a flick");
    pointerDown(200, 300);
    tickInterface(0);
    mouseInterface(200, 100, false);
    tickInterface(.1f);
    bool vertical = !pointerDragged;
    pointerUp(200, 100);
    check(
        vertical && selectedWorld == 1 && screen == WORLD,
        "Vertical-only pointer motion cannot start the native horizontal chapter drag or change its selected slot");
    pointerDown(200, 300);
    tickInterface(0);
    mouseInterface(100, 300, false);
    tickInterface(.1f);
    pointerHeld = false;
    tickInterface(.1f);
    check(
        !worldSelectPointerNative.down && !worldSelectPointerNative.dragged && !pointerDragged &&
            absf(page.cameraX - (page.level.size.x - viewW) * .5f) < .001f,
        "Losing pointer capture cancels chapter displacement and returns to its current slot without leaving a stuck drag");
    pointerDown(200, 300);
    tickInterface(0);
    mouseInterface(100, 300, false);
    tickInterface(.1f);
    navigate(titleAttractTitle);
    check(
        !worldSelectPointerNative.down && !worldSelectPointerNative.dragged && !pointerHeld,
        "Navigation destroys an unfinished chapter gesture so later menus inherit no held pointer or drag");
    currentStage = 0;
    progress.unlocked = stageCount - 1;
    navigate(18006219643615701355ULL);
    activate(page, page.focus);
    check(
        screen == SELECT && transitionTime == .35f && transitionDuration == .35f,
        "The original chapter-card callback opens its selector with the authored350msblack transition rather than the generic220msfade");
    navigate(18006219643615701355ULL);
    int bossCard = -1;
    for (int i = 0; i < page.level.objectCount; i++)
        if (page.nodes[i].world == 6)
            bossCard = i;
    if (bossCard >= 0)
        activate(page, bossCard);
    check(
        bossCard >= 0 && screen == PLAY && current.props.num("bean-world") == 6 &&
            transitionTime == .35f && transitionDuration == .35f,
        "The final boss card retains the same native350msentry even though its target opens gameplay and Ready directly");
    navigate(titleAttractTitle);
    viewW = oldW;
    viewH = oldH;
    screenScale = oldScale;
    currentStage = oldStage;
    pointerHeld = pointerDragged = false;
    worldSelectPointerNative = WorldSelectPointerNative{};
    interfaceEnabled = oldInterface;
    musicEnabled = oldMusic;
    soundEnabled = oldSound;
    debugMode = oldDebug;
    progress = oldProgress;
}
