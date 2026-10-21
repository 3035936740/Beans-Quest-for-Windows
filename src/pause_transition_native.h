#pragma once

// Tracks pause-menu transition phases and the boundary at which gameplay may resume.
#pragma float_control(precise, on, push)

// PauseMenuController0x2b384c/0x2b3244/0x2b8b70. This state belongs to
// presentation only. The caller keeps gameplay paused until RESUMED and
// assigns the configured initial focus only on FOCUS (or immediate entry).
const float pauseTransitionNativeSeconds = .150000006f;

enum PauseTransitionNativeEvent {
    PAUSE_NATIVE_NONE = 0,
    PAUSE_NATIVE_FOCUS = 1,
    PAUSE_NATIVE_RESUMED = 2
};

struct PauseTransitionNativeState {
    float showAge, resumeAge;
    bool active, resuming, updated;
};

bool pauseTransitionNativeBegin(PauseTransitionNativeState &state, bool animate = true) {
    if (state.active)
        return false;
    state = PauseTransitionNativeState{};
    state.active = true;
    state.showAge = animate ? 0 : pauseTransitionNativeSeconds;
    state.updated = !animate;
    return true;
}

bool pauseTransitionNativeResume(PauseTransitionNativeState &state, bool animate = true) {
    if (!state.active)
        return false;
    if (!animate) {
        state = PauseTransitionNativeState{};
        return true;
    }
    if (state.resuming)
        return false;
    state.resuming = true;
    state.resumeAge = 0;
    return true;
}

__declspec(noinline) int pauseTransitionNativeAdvance(PauseTransitionNativeState &state, float dt) {
    if (!state.active)
        return PAUSE_NATIVE_NONE;
    state.updated = true;
    dt = maxf(0, dt);
    if (state.resuming) {
        float previous = state.resumeAge;
        state.resumeAge += dt;
        if (previous >= pauseTransitionNativeSeconds) {
            state = PauseTransitionNativeState{};
            return PAUSE_NATIVE_RESUMED;
        }
        return PAUSE_NATIVE_NONE;
    }
    float previous = state.showAge;
    state.showAge += dt;
    return previous < pauseTransitionNativeSeconds && state.showAge >= pauseTransitionNativeSeconds
               ? PAUSE_NATIVE_FOCUS
               : PAUSE_NATIVE_NONE;
}

bool pauseTransitionNativeInputReady(const PauseTransitionNativeState &state) {
    return state.active && !state.resuming && state.showAge >= pauseTransitionNativeSeconds;
}

Vec pauseTransitionNativeOffset(const PauseTransitionNativeState &state, float width,
                                float height) {
    if (!state.active)
        return Vec{0, 0};
    // Pause(true) first writes cachedX-screenWidth, cachedY. Its first
    // OnUpdate replaces that initial pose with the vertical easing below.
    if (!state.updated)
        return Vec{-width, 0};
    float fraction = clamp(
        (state.resuming ? state.resumeAge : state.showAge) / pauseTransitionNativeSeconds, 0, 1);
    float curveValue = fraction <= 0    ? 0
                       : fraction >= 1  ? 1
                       : state.resuming ? 1 - bqCos(3.1415927410125732f * fraction * .5f)
                                        : bqSin(3.1415927410125732f * fraction * .5f);
    return Vec{0, -2 * height * (state.resuming ? curveValue : 1 - curveValue)};
}

#pragma float_control(pop)
