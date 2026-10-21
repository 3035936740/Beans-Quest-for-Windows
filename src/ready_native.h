#pragma once

// Ready-screen timing and the initial input lock before campaign simulation.

// LevelStartController::OnUpdate 0x264308. World/HUD coordinates are y-up.
struct ReadyNativeState {
    float age;
    bool showing;
};

ReadyNativeState readyNativeInitial(bool resume) {
    ReadyNativeState s = {0, !resume};
    return s;
}

void readyNativeAdvance(ReadyNativeState &s, float dt, bool pointerDown, bool leftDown,
                        bool rightDown) {
    if (!s.showing)
        return;
    s.age += dt;
    if (s.age >= 0.45f && s.age <= 2.0f && (pointerDown || leftDown || rightDown))
        s.age = 2.0f;
    if (s.age > 2.5f)
        s.showing = false;
}

float readyNativeFraction(float age, float a, float b) {
    return clamp((age - a) / (b - a), 0, 1);
}

float readyNativeEnter(float age) {
    float u = readyNativeFraction(age, 0.2f, 0.45f);
    if (u <= 0 || u >= 1)
        return u;
    float tail = 1 - u, phase = 3.1415927410125732f * u * (0.2f + 2.5f * u * u * u);
    return (u + bqSin(phase) * bqPow(tail, 2.2f)) * (1 + 1.2f * tail);
}

float readyNativeExit(float age) {
    float u = readyNativeFraction(age, 2.25f, 2.5f);
    if (u <= 0 || u >= 1)
        return u;
    return 1 - bqCos(3.1415927410125732f * u * 0.5f);
}

Vec readyNativePosition(float age, Vec authored, Rect view, bool ready) {
    float height = view.t - view.b;
    if (ready)
        return Vec{(view.l + view.r) * 0.5f - (view.r - view.l) * (1 - readyNativeEnter(age)),
                   authored.y - 2 * height * readyNativeExit(age)};
    return Vec{authored.x - (age == 0 ? view.r - view.l : 0),
               authored.y + (authored.y < 224 ? -1.0f : 1.0f) * 2 * height * readyNativeExit(age)};
}

// Tom +0x124 latches on displacement, rather than the first held input or
// the Ready controller disappearing. Save/Load serialize this and initial X.
void readyNativeCountMovement(int &latch, float x, float initialX) {
    if (latch == 0 && absf(x - initialX) > 10)
        latch = 1;
}

int readyNativeRestoreCountLatch(int saved, float x, float initialX, int savedJumps) {
    return saved == 0 || saved == 1 ? saved : (savedJumps > 0 || absf(x - initialX) > 10 ? 1 : 0);
}
