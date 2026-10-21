#pragma once

// Regression coverage for frog. Runs only in the isolated BQ_ENABLE_TESTS executable.

void frogChecks() {
    const float dt = 1.0f / 120.0f;
    musicEnabled = soundEnabled = false;
    interfaceEnabled = false;
    debugMode = false;
    int stage = -1;
    for (int i = 0; i < stageCount; i++)
        if (levels[stageList[i]].props.num("bean-world") == 4 &&
            levels[stageList[i]].props.num("bean-level") == 9)
            stage = i;
    check(stage >= 0, "Original Moonlight Oasis exists for the cooperative frog ball checks");
    if (stage < 0)
        return;
    startStage(stage);
    readyTimer = 0;
    check(frogLinkCount == 1,
          "Moonlight Oasis resolves its original left frog, right frog and special ball links");
    if (frogLinkCount != 1)
        return;
    FrogLink &link = frogLinks[0];
    Entity &ball = entities[link.ball];
    Entity &left = entities[link.left];
    Entity &right = entities[link.right];
    check(link.period == 2 && link.leftTarget.x == 624 && link.rightTarget.x == 784 &&
              link.leftTarget.y == 300 && link.rightTarget.y == 300,
          "Frog ball targets and default two second period match the original controller");
    check(left.state.aux == 1 && right.state.aux == 0 && ball.state.hp == 1 &&
              ball.state.timer < 0 && ball.state.x == 784 && ball.state.y == 300 &&
              ball.state.vx < 0,
          "Left and right frogs begin half a cycle apart and the ball starts at the right target");
    float time = link.period * 0.5f - ball.state.timer;
    check(absf(ball.state.x + ball.state.vx * time - link.leftTarget.x) < 0.001f &&
              absf(ball.state.y + ball.state.vy * time - 250 * time * time - link.leftTarget.y) <
                  0.001f,
          "Initial frog ball velocity solves the original remaining-time ballistic equations");

    // Exercise the real Box2D callback rather than invoking the state update
    // directly: the linked frog collision is disabled, then the ball's next
    // arc is calculated after the world unlocks.
    left.state.active = right.state.active = 1;
    ball.state.x = left.state.x + 30;
    ball.state.y = left.state.y + 30;
    ball.state.vx = ball.state.vy = 0;
    ball.state.timer = 0.8f;
    entities[playerIndex].state.x = 100;
    entities[playerIndex].state.y = 350;
    restorePhysics();
    simulateBodies(dt);
    frogPostStep();
    bool disabled = false;
    for (b2ContactEdge *edge = bodies[link.ball]->GetContactList(); edge; edge = edge->next)
        if (edge->other == bodies[link.left] && edge->contact->IsTouching() &&
            !edge->contact->IsEnabled())
            disabled = true;
    check(
        disabled && ball.state.hp == 1 && ball.state.active == link.left + 1 && ball.state.vx > 0 &&
            ball.state.vy > 0,
        "Contact with the linked left frog disables collision and launches a new arc toward the right frog");
    float savedPhase = left.state.aux, savedTime = ball.state.timer;
    int savedLast = ball.state.active;
    captureCheckpoint(true);
    ball.state.hp = 0;
    left.state.aux = 0;
    restoreCheckpoint();
    check(ball.state.hp == 1 && ball.state.timer == savedTime && ball.state.active == savedLast &&
              left.state.aux == savedPhase,
          "Checkpoint restores the cooperative ball mode, last contact and both timing clocks");

    // The normal jumper clears its wait in mid-air, whereas a cooperative
    // frog retains a periodic phase even while ascending or descending.
    left.state.y += 100;
    left.state.vx = left.state.vy = 0;
    left.state.aux = 1.5f;
    restorePhysics();
    bool special = frogJumperStep(link.left, dt);
    check(special && left.state.aux > 1.5f,
          "A special frog's clock continues in mid-air instead of resetting its jump delay");
    ball.state.x = entities[playerIndex].state.x = 704;
    ball.state.y = entities[playerIndex].state.y = 400;
    ball.state.vx = ball.state.vy = 0;
    entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
    restorePhysics();
    simulateBodies(dt);
    frogPostStep();
    check(
        ball.state.hp == 0 && !frogJumperStep(link.left, dt) && !frogJumperStep(link.right, dt),
        "A player collision interrupts the ball exchange and releases both frogs to normal behavior");
    updateCamera(true);
    captureCase("test-output/native-frog-ball.bmp");

    // Start from original positions and original geometry, isolating this
    // exchange from unrelated enemies (the authored diagonal fish can otherwise
    // physically interrupt it). Briefly enter both activation rectangles, then
    // observe remotely. The ball and frogs receive no teleports or forced
    // velocities during this full eight second exchange.
    startStage(stage);
    readyTimer = 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == ENEMY && i != link.left && i != link.right)
            entities[i].state.gone = 1;
    restorePhysics();
    entities[playerIndex].state.x = 704;
    entities[playerIndex].state.y = 230;
    entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
    tick(dt, false, false);
    bool naturalFinite = true, naturalActive = true;
    int sawLeft = 0, sawRight = 0, last = 0;
    char trace[16384] =
        "tick,ball_x,ball_y,ball_vx,ball_vy,mode,last_frog,left_y,left_phase_ms,right_y,right_phase_ms\r\n";
    for (int step = 0; step < 960; step++) {
        State &observer = entities[playerIndex].state;
        observer.x = 128;
        observer.y = 350;
        observer.vx = observer.vy = 0;
        tick(dt, false, false);
        naturalFinite &= ball.state.x == ball.state.x && ball.state.y == ball.state.y &&
                         ball.state.vx == ball.state.vx && ball.state.vy == ball.state.vy &&
                         absf(ball.state.x) < 10000 && absf(ball.state.y) < 10000;
        naturalActive &=
            ball.state.hp == 1 && !ball.state.gone && !left.state.gone && !right.state.gone;
        if (ball.state.active != last) {
            last = ball.state.active;
            if (last == link.left + 1)
                sawLeft++;
            if (last == link.right + 1)
                sawRight++;
        }
        if (step % 30 == 0) {
            char row[240];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", step, (int)ball.state.x,
                      (int)ball.state.y, (int)ball.state.vx, (int)ball.state.vy, ball.state.hp,
                      ball.state.active, (int)left.state.y, (int)(left.state.aux * 1000),
                      (int)right.state.y, (int)(right.state.aux * 1000));
            lstrcatA(trace, row);
        }
        if (step == 239 || step == 479 || step == 959) {
            camX = 448;
            camY = 100;
            char file[100];
            wsprintfA(file, "test-output/native-frog-exchange-%d.bmp", (step + 1) / 120);
            captureCase(file);
        }
    }
    atomicWrite("test-output/native-frog-exchange.csv", trace, lstrlenA(trace));
    check(
        naturalFinite,
        "Moonlight Oasis frog exchange remains finite during eight seconds of uninterrupted gameplay");
    check(
        naturalActive && sawLeft >= 2 && sawRight >= 2,
        "Moonlight Oasis ball exchanges between both frogs for eight seconds when isolated from unrelated enemies");
    startStage(0);
    readyTimer = 0;
}
