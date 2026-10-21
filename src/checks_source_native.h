#pragma once

// Regression coverage for source native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void sourceNativeChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    debugMode = false;
    startStage(stageById(789619714186ULL));
    readyTimer = 0;
    int right = -1, left = -1, beetle = -1, orb = -1;
    for (int i = 0; i < entityCount; i++) {
        u64 id = entities[i].source->id;
        if (id == 6717860002119500ULL)
            right = i;
        if (id == 829198267025ULL)
            left = i;
        if (id == 4927520020398172ULL)
            beetle = i;
    }
    for (int i = 0; i < entityCount; i++)
        if (entities[i].parent == beetle && entities[i].motion == 2)
            orb = i;
    check(right >= 0 && left >= 0 && beetle >= 0 && orb >= 0,
          "Blaster Climb resolves both authored cannon phases and the orbiting beetle child");
    if (right < 0 || left < 0 || beetle < 0 || orb < 0)
        return;
    check(entities[beetle].state.active && !entities[beetle].source->paused,
          "The visible Blaster Climb beetle starts active from its original paused flag");
    float firstRight = 0, firstLeft = 0, minX = entities[beetle].state.x, maxX = minX;
    for (int step = 1; step <= 220; step++) {
        State &bean = entities[playerIndex].state;
        bean.x = 320;
        bean.y = 1050;
        bean.vx = bean.vy = 0;
        tick(dt, false, false);
        for (int shot = 0; shot < 64; shot++)
            if (bullets[shot].life > 0) {
                if (bullets[shot].state.active == right + 1 && !firstRight)
                    firstRight = step * dt;
                if (bullets[shot].state.active == left + 1 && !firstLeft)
                    firstLeft = step * dt;
            }
        minX = minf(minX, entities[beetle].state.x);
        maxX = maxf(maxX, entities[beetle].state.x);
    }
    check(
        firstRight > 0.64f && firstRight < 0.67f && firstLeft >= 1.49f && firstLeft < 1.52f,
        "Real cannon timeOffset 0.85 produces the native 0.65-second first volley before the 1.5-second group");
    check(
        maxX - minX > 20 && !entities[beetle].state.gone,
        "The real orbit beetle patrols its platform while Bean remains outside its 100-pixel proximity window");
    captureCheckpoint(false);
    float clock = bulletSourceStates[right].clock, phase = entities[orb].state.timer;
    sourceRandomWord();
    static SourceRandomState liveSource;
    liveSource = sourceRandom;
    u32 rngNext = sourceRandomWord();
    sourceRandom = liveSource;
    bulletSourceStates[right].clock = 100;
    entities[orb].state.timer = 100;
    restoreCheckpoint();
    check(
        bulletSourceStates[right].clock == clock && entities[orb].state.timer == phase &&
            sourceRandomWord() == rngNext,
        "Checkpoint restores cannon clocks and attached-orb phase while the native level random stream continues from its live position");
    killEnemy(entities[beetle]);
    syncAttachedActors();
    simulateBodies(dt);
    check(
        entities[orb].state.gone && (!bodies[orb] || !bodies[orb]->IsActive()),
        "Killing the orbit beetle immediately hides and deactivates its attached hazard through the actual physics activation pass");
    restoreCheckpoint();
    syncAttachedActors();
    check(!entities[beetle].state.gone && !entities[orb].state.gone,
          "Restoring the checkpoint revives the original beetle and its attached orb together");
    restartStage();
    check(absf(bulletSourceStates[right].clock - 0.85f) < 0.00001f &&
              bulletSourceStates[left].clock == 0,
          "Full restart resets each cannon to its authored initial offset");
    memset(bullets, 0, sizeof(bullets));
    Entity &cannon = entities[right];
    Props props = *cannon.source->behavior("BulletSource");
    Pair values[64];
    values[0] = Pair{(char *)"extraShots", (char *)"-30,0,30"};
    values[1] = Pair{(char *)"bulletPoolSize", (char *)"10"};
    for (int i = 0; i < props.count; i++)
        values[i + 2] = props.pairs[i];
    props.pairs = values;
    props.count += 2;
    shoot(cannon, props);
    int count = 0;
    bool origin = true;
    float lowY = 100000, highY = -100000;
    for (int i = 0; i < 64; i++)
        if (bullets[i].life > 0) {
            count++;
            origin &= bullets[i].state.x == cannon.state.x && bullets[i].state.y == cannon.state.y;
            lowY = minf(lowY, bullets[i].state.vy);
            highY = maxf(highY, bullets[i].state.vy);
        }
    check(
        count == 4 && origin && lowY < -30 && highY > 30,
        "BulletSource emits primary then every relative extra angle from the original actor centre, including a zero-degree extra");
    startStage(motionStage(2, 6));
    readyTimer = 0;
    int paused = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->id == 5150090421494260444ULL)
            paused = i;
    check(paused >= 0 && entities[paused].source->paused,
          "Authored paused Tunnels block remains available as the contrasting activation fixture");
    if (paused >= 0) {
        Entity &e = entities[paused];
        float y = e.state.y;
        State &bean = entities[playerIndex].state;
        bean.x = 64;
        bean.y = 350;
        bean.vx = bean.vy = 0;
        tick(dt, false, false);
        check(
            !e.state.active && e.state.y == y && !bodies[paused]->IsActive(),
            "An actually paused object waits without integrating motion outside its authored range");
        bean.x = e.originX;
        bean.y = e.originY + 90;
        tick(dt, false, false);
        bool woke = e.state.active != 0;
        bean.x = 64;
        bean.y = 350;
        tick(dt, false, false);
        check(
            woke && e.state.active,
            "Entering the original rectangle wakes the paused object and sticky activation retains it");
    }
    startStage(0);
    readyTimer = 0;
}
