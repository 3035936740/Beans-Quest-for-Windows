#pragma once

// Regression coverage for pause transition native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void pauseTransitionNativeChecks(void (*verify)(bool, const char *) = check) {
    PauseTransitionNativeState state = {};
    bool begin = pauseTransitionNativeBegin(state);
    Vec offset = pauseTransitionNativeOffset(state, 512, 384);
    verify(
        begin && state.active && !state.resuming && !pauseTransitionNativeInputReady(state) &&
            offset.x == -512 && offset.y == 0,
        "Animated Pause preserves the original pre-update cached-X minus one screen width pose and defers focus");
    int event = pauseTransitionNativeAdvance(state, .0375f);
    offset = pauseTransitionNativeOffset(state, 512, 384);
    verify(
        event == PAUSE_NATIVE_NONE && offset.x == 0 && absf(offset.y + 474.0994f) < .002f &&
            !pauseTransitionNativeInputReady(state),
        "The first pause update restores cached X and enters from below with the native sine quarter-phase offset");
    float previous = state.showAge;
    bool duplicate = pauseTransitionNativeBegin(state);
    verify(!duplicate && state.showAge == previous,
           "A duplicate Pause request does not restart an already active entry transition");
    event = pauseTransitionNativeAdvance(state, pauseTransitionNativeSeconds - state.showAge);
    offset = pauseTransitionNativeOffset(state, 512, 384);
    bool focused = event == PAUSE_NATIVE_FOCUS && pauseTransitionNativeInputReady(state) &&
                   offset.x == 0 && offset.y == 0;
    event = pauseTransitionNativeAdvance(state, .01f);
    verify(
        focused && event == PAUSE_NATIVE_NONE,
        "Entry assigns focus once on crossing the exact native 0.15-second boundary and ends at cached actor positions");
    bool resumed = pauseTransitionNativeResume(state);
    offset = pauseTransitionNativeOffset(state, 512, 384);
    verify(resumed && state.active && state.resuming && state.resumeAge == 0 &&
               !pauseTransitionNativeInputReady(state) && offset.y == 0,
           "Resume(true) begins the exit and disables menu focus while gameplay remains paused");
    pauseTransitionNativeAdvance(state, .0375f);
    offset = pauseTransitionNativeOffset(state, 512, 384);
    verify(absf(offset.y + 58.4605f) < .002f && offset.x == 0,
           "Resume moves the menu downward with the native one-minus-cosine quarter-phase offset");
    previous = state.resumeAge;
    duplicate = pauseTransitionNativeResume(state);
    verify(
        !duplicate && state.resumeAge == previous,
        "Repeated Resume(true) requests cannot reset the exit clock or indefinitely extend pause");
    event = pauseTransitionNativeAdvance(state, .1125f);
    offset = pauseTransitionNativeOffset(state, 512, 384);
    bool held = state.active && state.resuming && event == PAUSE_NATIVE_NONE && offset.y == -768;
    event = pauseTransitionNativeAdvance(state, 0);
    verify(
        held && event == PAUSE_NATIVE_RESUMED && !state.active,
        "Exit keeps the offscreen terminal pose paused until the following update observes old resume age at least 0.15 seconds");
    pauseTransitionNativeBegin(state);
    pauseTransitionNativeAdvance(state, .02f);
    pauseTransitionNativeResume(state);
    pauseTransitionNativeAdvance(state, .075f);
    Vec normal = pauseTransitionNativeOffset(state, 512, 384),
        wide = pauseTransitionNativeOffset(state, 1024, 768);
    verify(
        state.active && state.resuming && absf(normal.y + 224.94199f) < .002f &&
            absf(wide.y - 2 * normal.y) < .002f,
        "Resuming during entry uses its independent exit clock and scales displacement with current camera height");
    pauseTransitionNativeResume(state, false);
    bool immediate = pauseTransitionNativeBegin(state, false);
    offset = pauseTransitionNativeOffset(state, 512, 384);
    bool settled =
        immediate && pauseTransitionNativeInputReady(state) && offset.x == 0 && offset.y == 0;
    pauseTransitionNativeResume(state, false);
    bool empty = !pauseTransitionNativeResume(state) &&
                 pauseTransitionNativeAdvance(state, .5f) == PAUSE_NATIVE_NONE;
    verify(
        settled && empty && !state.active,
        "Immediate pause/resume and scene-reset cancellation settle presentation without inventing an extra transition event");
}
