#pragma once

// Regression coverage for stomp native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void stompNativeChecks() {
    check(absf(nativeStompLaunch(74, 0, 0, -400) - 243.3105f) < 0.001f,
          "Zero-boost enemy stomp uses the original 74-pixel jump height");
    float beetle = nativeStompLaunch(74, 0, 4, -400);
    check(
        absf(beetle - 249.7999f) < 0.001f && absf(beetle * beetle / 800 - 78) < 0.001f,
        "Beetle and ruin turtle boost adds four pixels of height instead of eighty velocity units");
    check(absf(nativeStompLaunch(74, 10, 4, -400) * nativeStompLaunch(74, 10, 4, -400) / 800 - 88) <
              0.001f,
          "Native jump bonus and enemy boost are both applied before the square root");
    Vec velocity = nativeStompVelocity(Vec{67, -500}, Vec{0, 1}, 74, 0, 4, -400, 0.2f);
    check(
        absf(velocity.x - 67) < 0.0001f && absf(velocity.y - beetle) < 0.0001f,
        "Ordinary stomp preserves horizontal motion and replaces falling momentum with its native launch speed");
    check(
        absf(nativeStompLaunch(74, 0, 4, -200) - 176.6352f) < 0.001f,
        "Stomp launch scales with the player's configured gravity rather than a hardcoded multiplier");
    // Exercise the actual collision branch and ensuing flight, so a correct
    // standalone formula cannot conceal an unchanged boost*20 call site.
    startStage(0);
    readyTimer = 0;
    deathTimer = 0;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex)
            entities[i].state.gone = 1;
    int beetleIndex = entityCount;
    addEntity(*classById(14264481342587988014ULL), Rect{240, 112, 272, 144});
    Entity &enemy = entities[beetleIndex];
    enemy.walk = enemy.dynamic = 0;
    enemy.state.active = 1;
    enemy.state.vx = enemy.state.vy = 0;
    Entity &bean = entities[playerIndex];
    bean.state.x = 256;
    bean.state.y = hitRect(enemy).t - bean.hit.b + 1;
    bean.state.vx = 0;
    bean.state.vy = -60;
    bean.state.timer = .2f;
    rebuildPhysics();
    const float dt = 1 / 120.0f;
    for (int step = 0; step < 12 && !enemy.state.gone; step++)
        tick(dt, false, false);
    bool killed = enemy.state.gone && deathTimer <= 0;
    float launchY = bean.state.y, apex = launchY;
    tick(dt, false, false);
    float actualLaunch = bean.state.vy;
    check(killed && absf(actualLaunch - (beetle - 400 * dt)) < 0.01f &&
              absf(bean.lastLaunchSpeed - beetle) < 0.01f,
          "Real beetle collision kills the enemy and launches Bean with the native boosted speed");
    for (int step = 0; step < 100 && bean.state.vy > 0; step++) {
        tick(dt, false, false);
        apex = maxf(apex, bean.state.y);
    }
    check(killed && apex - launchY > 76 && apex - launchY < 79,
          "Actual beetle stomp rises about 78 pixels instead of the previous 131-pixel arc");
    startStage(0);
}
