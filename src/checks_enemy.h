#pragma once

// Regression coverage for enemy. Runs only in the isolated BQ_ENABLE_TESTS executable.

// Exercise the authored stage, including its real edge-chain geometry and
// enemy class settings. The observer stays above the beetle and inside its
// activation range so that player death cannot conceal an enemy stall.
void enemyChecks() {
    const float dt = 1.0f / 120.0f;
    musicEnabled = soundEnabled = false;
    interfaceEnabled = false;
    debugMode = false;
    startStage(6);
    readyTimer = 0;
    int wallWalker = -1, edgeWalker = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].walk) {
            if (absf(entities[i].originX - 1584) < 1)
                wallWalker = i;
            if (absf(entities[i].originX - 752) < 1)
                edgeWalker = i;
        }
    check(wallWalker >= 0 && edgeWalker >= 0,
          "Stage 1-07 contains the original wall and ledge beetles");
    if (wallWalker >= 0) {
        Entity &e = entities[wallWalker];
        float start = e.state.x, minX = start, maxX = start;
        bool wentLeft = false, wentRight = false;
        for (int step = 0; step < 300; step++) {
            State &p = entities[playerIndex].state;
            p.x = 1540;
            p.y = 190;
            p.vx = p.vy = 0;
            tick(dt, false, false);
            minX = minf(minX, e.state.x);
            maxX = maxf(maxX, e.state.x);
            wentLeft |= e.state.vx < -10;
            wentRight |= e.state.vx > 10;
        }
        check(!e.state.gone && e.state.active && wentLeft && wentRight && maxX - minX > 30 &&
                  e.state.x < start - 20,
              "Stage 1-07 beetle turns away from the checkpoint wall and keeps walking");
        camX = 1328;
        camY = 0;
        captureCase("test-output/native-walker-wall.bmp");
    }
    startStage(6);
    readyTimer = 0;
    if (edgeWalker >= 0) {
        Entity &e = entities[edgeWalker];
        float minX = e.state.x, maxX = e.state.x, minY = e.state.y;
        bool wentLeft = false, wentRight = false;
        for (int step = 0; step < 480; step++) {
            State &p = entities[playerIndex].state;
            p.x = 712;
            p.y = 312;
            p.vx = p.vy = 0;
            tick(dt, false, false);
            minX = minf(minX, e.state.x);
            maxX = maxf(maxX, e.state.x);
            minY = minf(minY, e.state.y);
            wentLeft |= e.state.vx < -10;
            wentRight |= e.state.vx > 10;
        }
        check(!e.state.gone && e.state.active && wentLeft && wentRight && maxX - minX > 12 &&
                  minY > 230 && minX > 735 && maxX < 801,
              "Stage 1-07 ledge beetle patrols both ways without walking off its platform");
        camX = 496;
        camY = 0;
        captureCase("test-output/native-walker-ledge.bmp");
    }
    int jumperStage = -1;
    for (int i = 0; i < stageCount; i++)
        if (levels[stageList[i]].props.num("bean-world") == 2 &&
            levels[stageList[i]].props.num("bean-level") == 1)
            jumperStage = i;
    if (jumperStage >= 0) {
        startStage(jumperStage);
        readyTimer = 0;
        int hopper = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].source->has("EnemyJumperBehaviour")) {
                hopper = i;
                break;
            }
        check(hopper >= 0, "The original first Forest stage contains its authored red hopper");
        if (hopper >= 0) {
            Entity &e = entities[hopper];
            int firstGround = -1, launch = -1;
            bool rose = false, apexJump = false;
            for (int step = 0; step < 600; step++) {
                State &p = entities[playerIndex].state;
                p.x = e.state.x - 64;
                p.y = e.state.y + 80;
                p.vx = p.vy = 0;
                tick(dt, false, false);
                if (firstGround < 0 && enemyGrounded(hopper) && !e.state.hp)
                    firstGround = step;
                if (launch < 0 && e.state.hp == 1)
                    launch = step;
                if (launch >= 0 && e.state.vy > 10)
                    rose = true;
                if (rose && e.state.vy <= 0) {
                    apexJump = e.state.hp == 1;
                    break;
                }
            }
            float delay = (launch - firstGround) * dt;
            check(firstGround >= 0 && launch > firstGround && delay >= 0.95f && delay <= 1.1f,
                  "Red hopper waits its authored delay on the ground before taking off");
            check(rose && apexJump,
                  "Hopper retains its jumping animation state through the top of its arc");
            updateCamera(true);
            captureCase("test-output/native-hopper-apex.bmp");
            startStage(jumperStage);
            readyTimer = 0;
            Entity &air = entities[hopper];
            air.state.y += 80;
            air.state.aux = 0.8f;
            air.state.hp = 0;
            air.state.active = 1;
            restorePhysics();
            bool airborneDelay = true;
            for (int step = 0; step < 12; step++) {
                State &p = entities[playerIndex].state;
                p.x = air.state.x - 64;
                p.y = air.state.y + 48;
                p.vx = p.vy = 0;
                tick(dt, false, false);
                airborneDelay &= air.state.aux == 0 && air.state.hp == 0;
            }
            check(airborneDelay,
                  "Airborne red hopper clears its ground wait and cannot jump before landing");
        }
    } else
        check(false, "Original first Forest stage exists for hopper behavior checks");
}
