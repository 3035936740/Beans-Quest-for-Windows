#pragma once

// Authored animation state and timing for jumper.
bool hasJumperAnimationNative(const Entity &e) {
    return e.source && e.source->has("EnemyJumperBehaviour");
}

void selectJumperAnimationNative(Entity &e, bool jumping) {
    if (hasJumperAnimationNative(e))
        e.jumperAnimation.selected = jumping ? 1 : 0;
}

float jumperAnimationAgeNative(const Entity &e, bool jumping) {
    return jumping ? e.jumperAnimation.jumpingAge : e.jumperAnimation.standingAge;
}

void advanceJumperAnimationNative(Entity &e, float dt) {
    if (!hasJumperAnimationNative(e) || dt <= 0)
        return;
    bool jumping = e.jumperAnimation.selected != 0;
    float length = animationLength(animation(e.art, jumping ? "jumping" : "standing"));
    if (length <= 0)
        return;
    float &age = jumping ? e.jumperAnimation.jumpingAge : e.jumperAnimation.standingAge;
    age += dt;
    // AnimationInstance::Update0x1091a4 compares length with age and
    // subtracts one length when age>=length (the normal LOOP action).
    if (age >= length)
        age -= length;
}

void restoreLegacyJumperAnimationNative(Entity &e) {
    e.jumperAnimation = JumperAnimationNative{};
    if (!hasJumperAnimationNative(e))
        return;
    bool jumping = e.state.hp != 0;
    e.jumperAnimation.selected = jumping ? 1 : 0;
    float length = animationLength(animation(e.art, jumping ? "jumping" : "standing")),
          age = maxf(0, e.state.timer);
    if (length > 0 && age > length)
        age -= (int)(age / length) * length;
    if (jumping)
        e.jumperAnimation.jumpingAge = age;
    else
        e.jumperAnimation.standingAge = age;
}
