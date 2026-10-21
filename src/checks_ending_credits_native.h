#pragma once

// Regression coverage for ending credits native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void endingCreditsNativeChecks() {
    resetEndingCredits(6144, 12192614360109026946ULL);
    check(absf(endingCredits.scrollSeconds - 198.4f) < 0.001f && endingCreditsCameraY(0) == 6144 &&
              endingCreditsCameraY(1) == 6114 && endingCreditsCameraY(300) == 192,
          "Original credits camera scrolls6144to192at30units per second and holds its endpoint");
    acceptEndingCredits(1);
    bool guarded = !endingCredits.prompt;
    acceptEndingCredits(1.001f);
    bool first = endingCredits.prompt && !endingCredits.exiting;
    advanceEndingCredits(1.501f, 0.5f);
    bool exact = !acceptEndingCredits(1.501f) && !endingCredits.exiting &&
                 absf(endingCreditsPromptAlpha() - 1) < 0.00001f;
    advanceEndingCredits(1.502f, 0.001f);
    bool next = acceptEndingCredits(1.502f);
    check(
        guarded && first && exact && next && endingCredits.exiting,
        "Native credits acceptance ignores the first second, pulses Continue on first accept and requires strictly more than half a second before the exit accept");
    check(!advanceEndingCredits(2, 1.99f) && advanceEndingCredits(2.01f, 0.011f),
          "Credits exit preserves the original two-second white fade before changing levels");
    resetEndingCredits(6144, 12192614360109026946ULL);
    acceptEndingCredits(2);
    advanceEndingCredits(3, 1);
    float full = endingCreditsPromptAlpha();
    advanceEndingCredits(4, 1);
    float dark = endingCreditsPromptAlpha();
    advanceEndingCredits(6, 2);
    check(
        full == 1 && absf(dark) < 0.00001f && !endingCredits.prompt && endingCredits.promptAge == 0,
        "Original Continue opacity usesclamp1minuscospiTime and a temporary request clears after four seconds without moving the camera");
    float natural = endingCredits.scrollSeconds + 10;
    advanceEndingCredits(natural - 0.001f, 0.01f);
    bool before = !endingCredits.prompt;
    advanceEndingCredits(natural, 0.001f);
    bool after = endingCredits.prompt;
    advanceEndingCredits(endingCredits.scrollSeconds + 20, 10);
    check(
        before && after && endingCredits.autoSleepAllowed && !endingCredits.exiting,
        "Natural credits ending enables Continue ten seconds after the camera finishes and only allows autosleep ten seconds later without automatically changing levels");
    interfaceEnabled = true;
    musicEnabled = soundEnabled = false;
    navigate(137502285691ULL);
    transitionTime = endingCreditsFade = 0;
    tickInterface(0.5f);
    keyPressed(VK_SPACE);
    bool firstGuard = screen == ENDING && !endingCredits.prompt;
    tickInterface(0.6f);
    float age = page.age, y = page.cameraY;
    SendMessageA(window, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(5, 5));
    ReleaseCapture();
    bool mouse = screen == ENDING && endingCredits.prompt && !endingCredits.exiting &&
                 page.age == age && page.cameraY == y && !endingScene.active;
    tickInterface(0.2f);
    keyPressed(VK_SPACE);
    bool debounce = !endingCredits.exiting;
    tickInterface(0.31f);
    keyPressed(VK_SPACE);
    bool fade = endingCredits.exiting;
    tickInterface(1.9f);
    bool notEarly = screen == ENDING;
    tickInterface(0.11f);
    check(
        firstGuard && mouse && debounce && fade && notEarly &&
            page.level.id == 12192614360109026946ULL && endingCreditsFade > 0,
        "Actual credits accept mouse-down anywhere and keyboard confirmation after the guard, keep scrolling on first input and return to the title after the complete white fade");
    navigate(137502285691ULL);
    page.age = 225;
    tickInterface(0.01f);
    check(
        screen == ENDING && endingCredits.prompt && !endingCredits.exiting &&
            absf(page.cameraY + viewH * 0.5f - 192) < 0.001f,
        "Actual225second credits screen remains interactive at its authored camera endpoint instead of forcing a timed title return");
    resetEndingScene();
    interfaceEnabled = false;
    startStage(0);
}
