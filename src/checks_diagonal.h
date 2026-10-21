#pragma once

// Regression coverage for diagonal. Runs only in the isolated BQ_ENABLE_TESTS executable.

void diagonalChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    debugMode = false;
    startStage(stageById(666279532945ULL));
    int fish = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->has("DiagonalBehaviour")) {
            fish = i;
            break;
        }
    check(fish >= 0, "Original Desert fish is present in its authored stage");
    if (fish < 0)
        return;
    for (int i = 0; i < entityCount; i++)
        if (i != fish && i != playerIndex)
            entities[i].state.gone = 1;
    Vec roomPoints[] = {{80, 80}, {432, 80}, {432, 304}, {80, 304}, {80, 80}};
    Geometry room = {0, 0, 5, roomPoints};
    current.geometry = &room;
    current.geometryCount = 1;
    Entity &f = entities[fish];
    f.state.x = 256;
    f.state.y = 192;
    f.state.active = 1;
    f.state.aux = 3;
    resetNativeMotion(fish, true);
    entities[playerIndex].state.x = 48;
    entities[playerIndex].state.y = 48;
    rebuildPhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    check(bodies[fish]->GetType() == b2_kinematicBody && oscillatorProxies[fish],
          "Fish uses the native kinematic carrier and dynamic collision detector");
    float x = f.state.x, y = f.state.y;
    for (int i = 0; i < 60; i++)
        tick(dt, false, false);
    check(absf(f.state.x - x + 35.35534f) < 0.05f && absf(f.state.y - y + 35.35534f) < 0.05f,
          "Fish follows the native normalized 100 pixel-per-second diagonal, without a sine path");
    bool bouncedY = false, bouncedX = false, contained = true;
    for (int i = 0; i < 420; i++) {
        tick(dt, false, false);
        bouncedY |= diagonalRuntime[fish].directionY > 0;
        bouncedX |= diagonalRuntime[fish].directionX > 0;
        Rect r = hitRect(f);
        contained &= r.l >= 79 && r.r <= 433 && r.b >= 79 && r.t <= 305;
    }
    check(bouncedY && bouncedX && contained,
          "Fish reflects at both floor and wall and stays inside static terrain");
    captureCheckpoint(false);
    DiagonalRuntimeState direction = diagonalRuntime[fish];
    diagonalRuntime[fish].directionX = -direction.directionX;
    diagonalRuntime[fish].directionY = -direction.directionY;
    restoreCheckpoint();
    check(diagonalRuntime[fish].directionX == direction.directionX &&
              diagonalRuntime[fish].directionY == direction.directionY &&
              absf(f.state.vx * f.state.vx + f.state.vy * f.state.vy - 10000) < 0.5f,
          "Fish checkpoint retains reflected direction and constant speed");
    // Leave no test-only pointers in the active level after this function.
    startStage(stageById(666279532945ULL));
}
