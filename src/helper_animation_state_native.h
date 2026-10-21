#pragma once

// State records for helper animation; live physics ownership stays with the calling scene.
// HelperBird caches its default, flying and manic AnimationInstances.
struct HelperAnimationNative {
    float age[3];
};

bool validHelperAnimationNative(const HelperAnimationNative &value) {
    for (int i = 0; i < 3; i++)
        if (!(value.age[i] >= 0 && value.age[i] <= 100000))
            return false;
    return true;
}
