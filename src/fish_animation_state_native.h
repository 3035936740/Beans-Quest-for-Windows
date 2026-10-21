#pragma once

// State records for fish animation; live physics ownership stays with the calling scene.
enum { FISH_DEFAULT_NATIVE, FISH_LEFT_NATIVE, FISH_RIGHT_NATIVE };

struct FishAnimationNative {
    float leftAge, rightAge;
    int selected;
};

bool validFishAnimationNative(const FishAnimationNative &s) {
    return s.selected >= FISH_DEFAULT_NATIVE && s.selected <= FISH_RIGHT_NATIVE && s.leftAge >= 0 &&
           s.leftAge < 100000000 && s.rightAge >= 0 && s.rightAge < 100000000;
}
