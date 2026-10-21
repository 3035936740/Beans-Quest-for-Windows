#pragma once

// Wizard encounter arena and its authored state transitions.

// Compatibility only: reconstruct the event history lost by the former
// absolute-bit implementation. Runtime alternation must still emit XOR
// events, rather than repeatedly assigning this derived table.
int legacyWizardArenaParity(int hp, int mode, bool gone) {
    const int starts[] = {4, 20, 20, 4, 12}, ends[] = {0, 0, 0, 8, 8};
    if (hp <= 0)
        return gone || mode >= 5 ? 8 : 12;
    if (hp > 5)
        return 0;
    int area = 5 - hp;
    if (mode == 3)
        return starts[area];
    return area ? ends[area - 1] : 0;
}

int migrateLegacyWizardArenaBits(int bits, int hp, int mode, bool gone) {
    return (bits & ~(4 | 8 | 16)) | legacyWizardArenaParity(hp, mode, gone);
}
