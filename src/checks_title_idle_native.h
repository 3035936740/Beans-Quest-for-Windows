#pragma once

// Regression coverage for title idle native. Runs only in the isolated BQ_ENABLE_TESTS executable.
void titleIdleRuntimeChecks() {
    bool oldInterface = interfaceEnabled, oldMusic = musicEnabled, oldSound = soundEnabled;
    interfaceEnabled = true;
    musicEnabled = soundEnabled = false;
    memset(keys, 0, sizeof(keys));
    pointerHeld = false;
    navigate(titleAttractTitle);
    titleAttractRuntimeFrame(14.75, true);
    float before = titleAttract.idle;
    for (int i = 0; i < 120; i++)
        tickInterface(1 / 120.0f);
    check(
        titleAttract.idle == before && !titleAttractBusy(),
        "Fixed UI/physics catch-up ticks neither double-count nor advance the independent real-time title idle clock");
    titleAttractRuntimeFrame(.25, true);
    check(
        titleAttract.phase == ATTRACT_TITLE_OUT,
        "Exactly fifteen real seconds trigger the title fade independently of fixed simulation time");
    navigate(titleAttractTitle);
    for (int frame = 0; frame < 59; frame++) {
        mouseInterface(32, 32, false);
        titleAttractRuntimeFrame(.25, true);
    }
    bool waiting = titleAttract.idle == 14.75f && !titleAttractBusy();
    mouseInterface(32, 32, false);
    titleAttractRuntimeFrame(.25, true);
    check(
        waiting && titleAttract.phase == ATTRACT_TITLE_OUT,
        "A four-FPS title enters its fade after fifteen real seconds even with a same-coordinate mouse message every frame");
    navigate(titleAttractTitle);
    titleAttractRuntimeFrame(10, true);
    mouseInterface(32, 32, false);
    check(
        titleAttract.idle == 10,
        "The first stationary pointer sample establishes position without inventing user activity");
    mouseInterface(33, 32, false);
    check(titleAttract.idle == 0,
          "Actual pointer coordinate changes reset the full title idle interval");
    titleAttractRuntimeFrame(14, true);
    keyPressed('Q');
    check(titleAttract.idle == 0 && !titleAttractBusy(),
          "An unbound physical key restarts the title idle interval through the real input path");
    keys['Q'] = true;
    titleAttractRuntimeFrame(25, true);
    keys['Q'] = false;
    check(titleAttract.idle == 0 && !titleAttractBusy(),
          "Held keys block attract entry even when a real-time frame exceeds fifteen seconds");
    titleAttractRuntimeFrame(14, true);
    titleAttractRuntimeFrame(.75, false);
    check(titleAttract.idle == 14.75f && !titleAttractBusy(),
          "Focus loss preserves and continues the real-time title idle count");
    titleAttractRuntimeFrame(.25, false);
    check(titleAttract.phase == ATTRACT_TITLE_OUT,
          "A title left in the background still starts its intro at fifteen real seconds");
    navigate(titleAttractTitle);
    keys['Q'] = true;
    pointerHeld = true;
    titleAttractRuntimeFrame(14.75, false);
    bool staleIgnored = titleAttract.idle == 14.75f;
    keys['Q'] = false;
    pointerHeld = false;
    titleAttractRuntimeFrame(.25, true);
    check(
        staleIgnored && titleAttract.phase == ATTRACT_TITLE_OUT,
        "Stale held input in an unfocused game cannot freeze idle and focus return does not restart the elapsed interval");
    const u64 scenes[] = {18006219643615701355ULL, 610822198339107ULL};
    bool returns = true;
    for (int i = 0; i < 2; i++) {
        navigate(scenes[i]);
        titleAttract.idle = 14.75f;
        titleAttract.unavailable = true;
        cancel();
        returns &= screen == TITLE && titleAttract.idle == 0 && !titleAttract.unavailable;
        titleAttractRuntimeFrame(14.75, true);
        returns &= !titleAttractBusy();
        titleAttractRuntimeFrame(.25, true);
        returns &= titleAttract.phase == ATTRACT_TITLE_OUT;
    }
    check(
        returns,
        "Actual chapter-selection and settings Back actions each restart a fresh full fifteen-second title interval including after a previous decoder failure");
    navigate(137502285691ULL);
    page.age = 2;
    acceptEndingCredits(page.age);
    endingCredits.promptAge = .6f;
    acceptEndingCredits(page.age);
    titleAttract.idle = 14.75f;
    tickInterface(2.01f);
    bool ended = screen == TITLE && titleAttract.idle == 0;
    titleAttractRuntimeFrame(14.75, true);
    ended &= !titleAttractBusy();
    titleAttractRuntimeFrame(.25, true);
    check(
        ended && titleAttract.phase == ATTRACT_TITLE_OUT,
        "The production credits-completion return starts its own full fifteen-second title interval");
    titleAttractReset();
    navigate(titleAttractTitle);
    interfaceEnabled = oldInterface;
    musicEnabled = oldMusic;
    soundEnabled = oldSound;
}
