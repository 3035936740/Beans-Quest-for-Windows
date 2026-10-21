#pragma once

// Authored animation state and timing for helper.
int helperAnimationSlotNative(const Entity &e) {
    return e.state.active == 1 ? 1 : e.state.active == 2 ? 2 : 0;
}

const char *helperAnimationNameNative(const Entity &e) {
    return e.state.active == 1   ? "flying"
           : e.state.active == 2 ? "manic"
                                 : e.source->props.get("default-animation");
}

float helperAnimationAgeNative(const Entity &e) {
    return e.helperAnimation.age[helperAnimationSlotNative(e)];
}

void advanceHelperAnimationNative(Entity &e, float dt) {
    if (e.kind != HELPER || dt <= 0)
        return;
    float length = animationLength(animation(e.art, helperAnimationNameNative(e)));
    if (length <= 0)
        return;
    float &age = e.helperAnimation.age[helperAnimationSlotNative(e)];
    age += dt;
    if (age >= length)
        age -= length;
}

void restoreLegacyHelperAnimationNative(Entity &e) {
    e.helperAnimation = HelperAnimationNative{};
    if (e.kind != HELPER)
        return;
    // Legacy files only knew the visible state timer; inactive instances
    // had no saved phase. Preserve that known phase and default the others.
    float age = maxf(0, e.state.timer),
          length = animationLength(animation(e.art, helperAnimationNameNative(e)));
    if (length > 0 && age >= length)
        age -= (int)(age / length) * length;
    e.helperAnimation.age[helperAnimationSlotNative(e)] = age;
}
