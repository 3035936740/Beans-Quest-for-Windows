#pragma once

// Goal spring launch calculations before stage completion.

// TomPlayerBehaviour::OnEndOfLevel 0x312d78 saves the takeoff position
// and leaves the disabled body's horizontal velocity intact. Its win branch
// 0x31242c uses that velocity, not a contact-normal or GoTo target direction.
struct GoalLaunchState {
    float age, x, y, vx, jumpVelocity, gravity;
    int active;
};

GoalLaunchState goalLaunch;

GoalLaunchState makeGoalLaunch(float x, float y, float vx, float jumpVelocity, float gravity) {
    return GoalLaunchState{0, x, y, vx, jumpVelocity, gravity, 1};
}

void beginGoalLaunch(Entity &bean) {
    float jump = bean.lastLaunchSpeed > 0 ? bean.lastLaunchSpeed : bean.jumpPower;
    float gravity = bean.physics ? bean.physics->num("gravity", -400) : -400;
    goalLaunch = makeGoalLaunch(bean.state.x, bean.state.y, bean.state.vx, jump, gravity);
    bean.state.timer = 0;
}

// Return true only on the original 5.2s win-music boundary. Run while the
// completion overlay is showing too; it is independent of menu visibility.
bool advanceGoalLaunch(Entity &bean, float dt) {
    if (!goalLaunch.active)
        return false;
    GoalLaunchState &w = goalLaunch;
    float previous = w.age;
    w.age += dt;
    bean.state.timer += dt;
    // Native compares the previous age with2 then evaluates at the new age.
    // Therefore the final moving frame may be one dt beyond2, then freezes.
    if (previous < 2) {
        float t = w.age;
        bean.state.x = w.x + w.vx * t;
        bean.state.y = w.y + (3 * w.jumpVelocity + 0.5f * w.gravity * t) * t;
    }
    return previous < 5.2f && w.age >= 5.2f;
}
