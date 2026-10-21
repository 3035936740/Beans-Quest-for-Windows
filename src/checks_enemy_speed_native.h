#pragma once

// Regression coverage for enemy speed native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void enemySpeedNativeChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    debugMode = false;
    int orbitCount = 0, negativePeriods = 0, negativeRadii = 0;
    bool configured = true, initialPose = true;
    int walkerCount = 0, oscillatorCount = 0, diagonalCount = 0, jumperCount = 0;
    Arena memory;
    memory.init(32 * 1024 * 1024);
    Level scan = {};
    for (int scene = 0; scene < levelCount; scene++) {
        readLevelData(levels[scene].file, scan, memory);
        for (int i = 0; i < scan.objectCount; i++) {
            Object &o = scan.objects[i];
            bool enemy = o.has("KillableEnemyBehaviour") || o.has("FinalBossBehaviour");
            for (int j = 0; j < o.behaviorCount; j++)
                enemy |= o.behaviors[j].type[0] == 'E' && o.behaviors[j].type[1] == 'n' &&
                         o.behaviors[j].type[2] == 'e' && o.behaviors[j].type[3] == 'm' &&
                         o.behaviors[j].type[4] == 'y';
            if (enemy) {
                walkerCount += o.has("WalkBehaviour");
                oscillatorCount += o.has("OscillatingBehaviour");
                diagonalCount += o.has("DiagonalBehaviour");
                jumperCount += o.has("EnemyJumperBehaviour");
            }
            const Props *orbit = o.behavior("OrbitBehaviour");
            if (!orbit)
                continue;
            orbitCount++;
            float period = orbit->num("period", 1), radius = orbit->num("radius", 100),
                  phase = orbit->num("start-angle") * 0.017453293f;
            negativePeriods += period < 0;
            negativeRadii += radius < 0;
            Entity e;
            initializeEntity(e, o, o.rect);
            Vec c = center(o.rect);
            configured &= e.orbit && e.period == period && e.radius == radius && period != 0;
            initialPose &= absf(e.originX - c.x) < 0.001f && absf(e.originY - c.y) < 0.001f &&
                           absf(e.state.x - (c.x + bqCos(phase) * radius)) < 0.003f &&
                           absf(e.state.y - (c.y + bqSin(phase) * radius)) < 0.003f;
        }
    }
    releaseLevel(scan, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        levelCount == 74 && orbitCount == 18 && negativePeriods == 1 && negativeRadii == 3 &&
            configured,
        "All original scenes retain all18 signed orbit configurations including the one reverse-period bat and three negative radii");
    check(
        initialPose,
        "Every original orbiting enemy starts at the native phase-zero radius position without a first-tick radial teleport");
    check(
        walkerCount == 78 && oscillatorCount == 52 && diagonalCount == 9 && jumperCount == 31,
        "The complete original enemy-controller inventory resolves78 walkers52 oscillators9 diagonal fish and31 jumpers without inferring speed from sprites");
    const float fpsCases[] = {30, 75, 144};
    bool speed = true, direction = true, revolution = true, fpsInvariant = true;
    float referenceX = 0, referenceY = 0;
    for (int mode = 0; mode < 3; mode++) {
        nativeMotionRenderFPS = fpsCases[mode];
        startStage(stageById(685488901231ULL));
        readyTimer = deathTimer = 0;
        int backward = -1, forward = -1;
        for (int i = 0; i < entityCount; i++) {
            if (entities[i].source->id == 292776676901ULL)
                backward = i;
            if (entities[i].source->id == 163072480779ULL)
                forward = i;
            if (entities[i].kind == ENEMY && entities[i].source->id != 292776676901ULL &&
                entities[i].source->id != 163072480779ULL)
                entities[i].state.gone = 1;
        }
        check(backward >= 0 && forward >= 0,
              "Displayed4-9 resolves both exact Boulder Tracks bats with period-4 and+4");
        if (backward < 0 || forward < 0)
            continue;
        State &bean = entities[playerIndex].state;
        bean.x = 1500;
        bean.y = 400;
        bean.vx = bean.vy = 0;
        restorePhysics();
        bodies[playerIndex]->SetType(b2_staticBody);
        Entity &reverse = entities[backward];
        Entity &normal = entities[forward];
        float oldX = reverse.state.x, oldY = reverse.state.y, maxSpeed = 0;
        float sampleX = 0, sampleY = 0;
        for (int frame = 0; frame < 480; frame++) {
            tick(1.0f / 120.0f, false, false);
            float dx = reverse.state.x - oldX, dy = reverse.state.y - oldY;
            maxSpeed = maxf(maxSpeed, root(dx * dx + dy * dy) * 120);
            oldX = reverse.state.x;
            oldY = reverse.state.y;
            if (frame == 119) {
                sampleX = reverse.state.x;
                sampleY = reverse.state.y;
                direction &= absf(reverse.state.x - reverse.originX) < 0.04f &&
                             absf(reverse.state.y - (reverse.originY - 40)) < 0.04f &&
                             absf(normal.state.x - normal.originX) < 0.04f &&
                             absf(normal.state.y - (normal.originY + 35)) < 0.04f;
            }
        }
        speed &= maxSpeed < 63.0f && maxSpeed > 62.6f;
        revolution &= absf(reverse.state.x - (reverse.originX + 40)) < 0.05f &&
                      absf(reverse.state.y - reverse.originY) < 0.05f;
        if (mode == 0) {
            referenceX = sampleX;
            referenceY = sampleY;
        } else
            fpsInvariant &=
                absf(sampleX - referenceX) < 0.001f && absf(sampleY - referenceY) < 0.001f;
        char path[128], trace[384];
        wsprintfA(path, "test-output/enemy-speed-boulder-bat-%d.txt", mode);
        wsprintfA(
            trace,
            "scene=685488901231 reverse=292776676901 fps=%d period100=%d radius100=%d sampleX1000=%d sampleY1000=%d maxSpeed1000=%d finalX1000=%d finalY1000=%d death=%d\r\n",
            (int)fpsCases[mode], (int)(reverse.period * 100), (int)(reverse.radius * 100),
            (int)(sampleX * 1000), (int)(sampleY * 1000), (int)(maxSpeed * 1000),
            (int)(reverse.state.x * 1000), (int)(reverse.state.y * 1000), deathTimer > 0);
        atomicWrite(path, trace, lstrlenA(trace));
    }
    check(
        speed && direction && revolution,
        "The real reverse-period4-9 bat moves clockwise at2pi*40/4 pixels per second and returns after four seconds while its positive-period neighbour moves the other way");
    check(
        fpsInvariant,
        "Real bat motion at30,75and144 render-FPS inputs has identical one-second position without any75FPS speed multiplier");
    nativeMotionRenderFPS = 60;
    startStage(0);
    readyTimer = 0;
}
