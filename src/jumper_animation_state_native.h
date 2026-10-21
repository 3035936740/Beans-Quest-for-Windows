#pragma once

// State records for jumper animation; live physics ownership stays with the calling scene.
struct JumperAnimationNative {
    float standingAge, jumpingAge;
    int selected;
};

bool validJumperAnimationNative(const JumperAnimationNative &s) {
    return s.selected >= 0 && s.selected <= 1 && s.standingAge >= 0 && s.standingAge < 100000000 &&
           s.jumpingAge >= 0 && s.jumpingAge < 100000000;
}
