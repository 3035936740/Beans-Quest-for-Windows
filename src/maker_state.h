#pragma once

// Custom-course run flags shared with campaign code. The maker implementation is included after
// interface.h; campaign paths only consult these flags and the forward-declared hooks.
struct MakerRunState {
    bool active;    // the running stage was baked from a custom course
    bool testing;   // launched from the editor; leaving the stage returns there
    bool verifying; // launched from the course start, so a clear verifies the course
    int slot;
    int recordJumps; // best clear used for the HUD jump target, 0 while uncleared
};

MakerRunState makerRun;

void makerLoadCurrentLevel();
void makerRecordClear();
void makerExitPlay();
void makerUnlock();

// Custom weather is screen-relative. Side-emitted petals/sand must start beside the
// visible window, not at the edge of the much wider course or an oversized fixed view.
Rect makerWeatherEmitterRect(const Object &source, float width, float height) {
    if (source.klass == 697074819893ULL)
        return Rect{0, 0, width, minf(32, height)}; // Birds rise from a shallow lower strip.
    if (source.klass == 10170490229994225569ULL)
        return Rect{0, 0, width, height}; // Stationary stars span the visible sky.
    return Rect{-32, -32, width + 32, height + 32};
}
