#pragma once

// Authored animation state and timing for fish.
bool hasFishAnimationNative(const Entity &e) {
    return e.source && e.source->has("DiagonalBehaviour");
}

const char *fishAnimationNameNative(const Entity &e) {
    return e.fishAnimation.selected == FISH_LEFT_NATIVE ? "left"
           : e.fishAnimation.selected == FISH_RIGHT_NATIVE
               ? "right"
               : e.source->props.get("default-animation");
}

float fishAnimationAgeNative(const Entity &e) {
    return e.fishAnimation.selected == FISH_LEFT_NATIVE    ? e.fishAnimation.leftAge
           : e.fishAnimation.selected == FISH_RIGHT_NATIVE ? e.fishAnimation.rightAge
                                                           : e.state.timer;
}

void selectFishAnimationNative(Entity &e, float directionX) {
    if (hasFishAnimationNative(e))
        e.fishAnimation.selected = directionX < 0 ? FISH_LEFT_NATIVE : FISH_RIGHT_NATIVE;
}

void advanceFishAnimationNative(Entity &e, float dt) {
    if (!hasFishAnimationNative(e) || dt <= 0 || e.fishAnimation.selected == FISH_DEFAULT_NATIVE)
        return;
    float length = animationLength(animation(e.art, fishAnimationNameNative(e)));
    if (length <= 0)
        return;
    float &age = e.fishAnimation.selected == FISH_LEFT_NATIVE ? e.fishAnimation.leftAge
                                                              : e.fishAnimation.rightAge;
    age += dt;
    if (age >= length)
        age -= length;
}

void restoreLegacyFishAnimationNative(Entity &e) {
    e.fishAnimation = FishAnimationNative{};
    if (!hasFishAnimationNative(e))
        return;
    // Old files rendered facing directly from velocity, even before activation.
    // Preserve that known visible phase; the other instance was never saved.
    selectFishAnimationNative(e, e.state.vx);
    float age = maxf(0, e.state.timer),
          length = animationLength(animation(e.art, fishAnimationNameNative(e)));
    if (length > 0 && age >= length)
        age -= (int)(age / length) * length;
    if (e.fishAnimation.selected == FISH_LEFT_NATIVE)
        e.fishAnimation.leftAge = age;
    else
        e.fishAnimation.rightAge = age;
}
