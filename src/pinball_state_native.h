#pragma once

// Transient strong-bounce request data and reset behavior.

// Tom SaveState0x31579c..0x3157e8 persists +c9, direction and speed.
// This is Tom's shared MegaJump request, used by Pinball AND Spring.
// The historic type/field names retain the exact format13/14 byte layout.
// Effects have already played during contact; only the future launch is saved.
struct PinballPendingNative {
    int pending, source;
    Vec normal;
    float speed;
};

PinballPendingNative pinballPendingNative, pinballConsumedNative;
bool pinballLaunchedThisStepNative;

void clearPinballPendingNative() {
    memset(&pinballPendingNative, 0, sizeof(pinballPendingNative));
    pinballPendingNative.source = -1;
    memset(&pinballConsumedNative, 0, sizeof(pinballConsumedNative));
    pinballConsumedNative.source = -1;
    pinballLaunchedThisStepNative = false;
}

bool validPinballPendingNative(const PinballPendingNative &s, int count) {
    if (s.pending != 0 && s.pending != 1)
        return false;
    if (!s.pending)
        return true;
    float length = s.normal.x * s.normal.x + s.normal.y * s.normal.y;
    return s.source >= 0 && s.source < count && s.speed >= 0 && s.speed <= 100000 &&
           s.normal.x >= -1 && s.normal.x <= 1 && s.normal.y >= -1 && s.normal.y <= 1 &&
           length <= 1.0001f;
}
