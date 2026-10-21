#pragma once

// Regression coverage for checkpoint native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void checkpointNativeChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    startStage(0);
    readyTimer = 0;
    int point = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == CHECKPOINT) {
            point = i;
            break;
        }
    check(point >= 0, "Original first-stage checkpoint exists for same-point reuse");
    if (point < 0)
        return;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && i != point)
            entities[i].state.gone = 1;
    rebuildPhysics();
    checkpointResetContacts();
    Entity &e = entities[point];
    State &bean = entities[playerIndex].state;
    check(
        bodies[point] && bodies[point]->GetFixtureList()->IsSensor() &&
            bodies[point]->GetFixtureList()->GetType() == b2Shape::e_polygon,
        "Checkpoint uses its adjusted original box fixture as a sensor, not a solid pole or an artwork-only overlap");
    bean.x = e.state.x;
    bean.y = e.state.y;
    bean.vx = bean.vy = 0;
    restorePhysics();
    coins = 2;
    jumps = 7;
    tick(dt, false, false);
    tick(dt, false, false);
    check(
        e.state.hp == 2 && checkpoint.coins == 2 && checkpoint.jumps == 7 &&
            checkpoint.state[point].hp == 2,
        "First checkpoint contact records recent state and current counters before capturing the snapshot");
    float savedElapsed = checkpoint.elapsed;
    int savedCoins = checkpoint.coins;
    coins = 3;
    for (int frame = 0; frame < 30; frame++) {
        bean.x = e.state.x;
        bean.y = e.state.y;
        bean.vy = 0;
        tick(dt, false, false);
    }
    check(
        checkpoint.elapsed == savedElapsed && checkpoint.coins == savedCoins,
        "Continuous checkpoint contact does not repeatedly save or save merely because a gem counter changed");
    const Props &cfg = *e.source->behavior("CheckpointBehaviour");
    float dx = cfg.num("reactivate-dist-x", 240), dy = cfg.num("reactivate-dist-y", 160);
    bean.x = e.state.x + dx;
    bean.y = e.state.y + dy;
    checkpointPrePhysics();
    check(e.state.hp == 2,
          "Checkpoint remains recent exactly at both authored reactivation boundaries");
    bean.x += 0.01f;
    checkpointPrePhysics();
    check(
        e.state.hp == 3 && checkpointSaveVisual(e),
        "Crossing either authored distance rearms the same checkpoint without resetting its saved visual");
    tick(dt, false, false);
    bean.x = e.state.x;
    bean.y = e.state.y;
    bean.vx = bean.vy = 0;
    restorePhysics();
    coins = 5;
    jumps = 11;
    tick(dt, false, false);
    tick(dt, false, false);
    check(
        e.state.hp == 2 && checkpoint.coins == 5 && checkpoint.jumps == 11 &&
            checkpoint.elapsed > savedElapsed,
        "After the native distance rearm and real reentry, the same checkpoint records updated gems and jump count");
    coins = 0;
    jumps = 0;
    restoreCheckpoint();
    check(
        coins == 5 && jumps == 11 && e.state.hp == 2,
        "Restoring the reused checkpoint recovers the updated counters instead of the first visit");
    check(absf(e.state.timer - animationLength(animation(e.art, "save"))) < 0.000001f,
          "Restored checkpoint save animation remains at its native completed frame");
    e.state.hp = 2;
    bean.x = e.state.x;
    bean.y = e.state.y + dy + 0.01f;
    check(checkpointRearmNative(e, bean) && e.state.hp == 3,
          "Vertical-only distance also rearms a recent checkpoint");
    e.state.hp = 2;
    bean.x = e.state.x + 10;
    bean.y = e.state.y + 10;
    check(
        !checkpointRearmNative(e, bean) && e.state.hp == 2,
        "Simply leaving and revisiting the immediate sensor area does not bypass original distance hysteresis");
}
