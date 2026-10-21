#pragma once

// Regression coverage for goal launch. Runs only in the isolated BQ_ENABLE_TESTS executable.

void goalLaunchChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    startStage(0);
    readyTimer = 0;
    Entity &bean = entities[playerIndex];
    const float xs[3] = {-160, 0, 160};
    float end[3];
    bool vertical = true, horizontal = true;
    for (int i = 0; i < 3; i++) {
        bean.state.x = 200;
        bean.state.y = 100;
        bean.state.vx = xs[i];
        bean.lastLaunchSpeed = bean.jumpPower;
        beginGoalLaunch(bean);
        advanceGoalLaunch(bean, 0.5f);
        end[i] = bean.state.x;
        horizontal &= absf(bean.state.x - (200 + xs[i] * 0.5f)) < 0.001f;
        vertical &= absf(bean.state.y - (100 + (3 * bean.jumpPower - 100) * 0.5f)) < 0.01f;
    }
    check(
        horizontal && end[0] < end[1] && end[1] < end[2],
        "Original GOAL flight preserves left, zero and right incoming velocity instead of forcing every finish vertically upward");
    check(
        vertical,
        "GOAL takeoff uses three times the Bean jump velocity and authored gravity in the native analytic trajectory");
    bean.state.x = 200;
    bean.state.y = 100;
    bean.state.vx = 100;
    bean.lastLaunchSpeed = bean.jumpPower;
    beginGoalLaunch(bean);
    advanceGoalLaunch(bean, 2.01f);
    float x = bean.state.x, y = bean.state.y;
    bool earlyMusic = advanceGoalLaunch(bean, 3.18f);
    check(
        bean.state.x == x && bean.state.y == y && !earlyMusic,
        "GOAL actor freezes after its original two-second flight while the completion sequence continues");
    bool music = advanceGoalLaunch(bean, 0.02f), again = advanceGoalLaunch(bean, 0.1f);
    check(music && !again,
          "Win music begins exactly once when the original completion clock crosses5.2 seconds");
    // CurrentJumpVelocity (+0x100) is retained from the last actual jump.
    bean.state.x = bean.state.y = 0;
    bean.state.vx = 0;
    bean.lastLaunchSpeed = 400;
    beginGoalLaunch(bean);
    advanceGoalLaunch(bean, 0.25f);
    check(
        absf(bean.state.y - 287.5f) < 0.001f,
        "GOAL retains a boosted preceding jump velocity rather than replacing it with a hardcoded650 launch");
    startStage(0);
    readyTimer = 0;
    int exit = -1;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].kind == EXIT)
            exit = i;
        if (entities[i].kind == ENEMY)
            entities[i].state.gone = 1;
    }
    if (exit >= 0) {
        Entity &goal = entities[exit];
        Entity &player = entities[playerIndex];
        player.state.x = goal.state.x - 12;
        player.state.y = goal.state.y + goal.hit.t - player.hit.b + 8;
        player.state.vx = 100;
        player.state.vy = -100;
        restorePhysics();
        for (int step = 0; step < 120 && !complete; step++)
            tick(1.0f / 120.0f, false, false);
        float takeoff = player.state.x;
        bool preserved = complete && goalLaunch.active && goalLaunch.vx > 0;
        tick(0.1f, false, false);
        check(
            preserved && player.state.x > takeoff + 1,
            "Landing on the actual Grasslands GOAL preserves physical sideways motion through finish and its subsequent win animation");
    } else
        check(false, "Original Grasslands GOAL exists for physical takeoff integration");
    memset(&goalLaunch, 0, sizeof(goalLaunch));
    startStage(0);
    readyTimer = 0;
}
