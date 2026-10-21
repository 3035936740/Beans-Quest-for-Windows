#pragma once

// State records for trampoline; live physics ownership stays with the calling scene.

// Tom saves +e4 (request), +e8 (absolute apex), and +ec (launch speed).
// Source is a port-side index used to validate the deferred request.
struct TrampolineStateNative {
    int pending, source;
    float apex, speed;
};

TrampolineStateNative trampolineStateNative;
bool trampolineLaunchedThisStepNative, trampolineContactThisStepNative;

void resetTrampolineStateNative(float y) {
    trampolineStateNative = TrampolineStateNative{0, -1, y, 0};
    trampolineLaunchedThisStepNative = trampolineContactThisStepNative = false;
}

bool validTrampolineStateNative(const TrampolineStateNative &s, int count) {
    return (s.pending == 0 || s.pending == 1) &&
           (!s.pending || (s.source >= 0 && s.source < count)) && s.apex >= -100000 &&
           s.apex <= 100000 && s.speed >= 0 && s.speed <= 100000;
}
