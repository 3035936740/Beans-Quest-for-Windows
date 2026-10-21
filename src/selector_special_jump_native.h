#pragma once

// Separates ordinary, jelly and strong-bounce requests in the selector physics world.

// Every miniature/ending Tom has its own +c9/+e4 requests, never the
// campaign globals. Contacts write these; the next PrePhysics consumes them.
struct SelectorSpecialJumpNative {
    PinballPendingNative mega;
    TrampolineStateNative jelly;
    OrdinaryJumpStateNative ordinary;
    int launchedSource, launchedKind;
    bool contact;
};

void resetSelectorSpecialJumpNative(SelectorSpecialJumpNative &s) {
    memset(&s, 0, sizeof(s));
    s.mega.source = s.jelly.source = s.launchedSource = -1;
}
