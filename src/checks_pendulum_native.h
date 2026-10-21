#pragma once

// Regression coverage for pendulum native. Runs only in the isolated BQ_ENABLE_TESTS executable.
float pendulumExpectedRadius(const Entity &child) {
    if (child.parent < 0)
        return -1;
    const Entity &root = entities[child.parent];
    const Props *cfg = root.source->behavior("WreckingBallControllerBehaviour");
    if (!cfg)
        return -1;
    int number = 0;
    for (int i = child.parent + 1; i < entityCount; i++)
        if (entities[i].parent == child.parent && entities[i].motion == 1) {
            if (&entities[i] == &child)
                break;
            number++;
        }
    Object *link = classById(idnum(cfg->get("LinkClass")));
    Object *end = classById(idnum(cfg->get("EndClass")));
    if (!link || !end)
        return -1;
    int n = (int)cfg->num("NumLinks", 4);
    float half = (root.source->rect.t - root.source->rect.b) * 0.5f,
          linkHeight = link->rect.t - link->rect.b, spacing = cfg->num("LinkSpacing", 0);
    if (number < n)
        return half + number * (linkHeight + spacing) + linkHeight * 0.5f + spacing;
    return half + n * (linkHeight + spacing) + (end->rect.t - end->rect.b) * 0.5f +
           cfg->num("LastLinkSpacing", 0);
}

bool pendulumEntityMatchesNative(const Entity &e, float tolerance) {
    if (e.motion != 1 || e.parent < 0)
        return true;
    const Entity &root = entities[e.parent];
    const Props *cfg = root.source->behavior("WreckingBallControllerBehaviour");
    if (!cfg)
        return false;
    float radius = pendulumExpectedRadius(e), period = cfg->num("Period", 1),
          a = bqCos((root.state.timer + cfg->num("TimeOffset")) * 3.1415927410125732f / period) *
              cfg->num("MaxAngle", 60) * 0.01745329238474369f;
    return absf(e.radius - radius) < 0.001f &&
           absf(e.state.x - root.state.x - bqSin(a) * radius) < tolerance &&
           absf(e.state.y - root.state.y + bqCos(a) * radius) < tolerance &&
           absf(e.state.angle - e.source->angle) < 0.001f;
}

void pendulumNativeChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    const u64 ids[] = {10756198405680061017ULL, 5687107557475985288ULL, 161899780601ULL,
                       2986143812017068ULL};
    const int counts[] = {1, 5, 3, 5};
    int roots = 0;
    bool initial = true, driven = true, radii = true, clocks = true, restored = true;
    for (int scene = 0; scene < 4; scene++) {
        startStage(stageById(ids[scene]));
        readyTimer = deathTimer = 0;
        int count = 0;
        for (int i = 0; i < entityCount; i++) {
            Entity &e = entities[i];
            if (e.source->has("WreckingBallControllerBehaviour")) {
                count++;
                roots++;
            }
            if (e.motion == 1) {
                initial &= pendulumEntityMatchesNative(e, 0.2f);
                radii &= absf(e.radius - pendulumExpectedRadius(e)) < 0.001f;
            } else if (i != playerIndex && !e.source->has("WreckingBallControllerBehaviour"))
                e.state.gone = 1;
        }
        check(
            count == counts[scene],
            "Actual original stage retains every authored wrecking-ball controller including overlapping ball and safe-platform variants");
        Entity &bean = entities[playerIndex];
        bean.state.x = 64;
        bean.state.y = 10000;
        bean.state.vx = bean.state.vy = 0;
        restorePhysics();
        if (bodies[playerIndex])
            bodies[playerIndex]->SetType(b2_staticBody);
        for (int step = 0; step < 720; step++) {
            tick(1 / 120.0f, false, false);
            if (step % 30 == 29)
                for (int i = 0; i < entityCount; i++)
                    if (entities[i].motion == 1)
                        driven &= pendulumEntityMatchesNative(entities[i], 0.3f);
        }
        elapsed += 17;
        tick(1 / 120.0f, false, false);
        for (int i = 0; i < entityCount; i++)
            if (entities[i].motion == 1)
                clocks &= pendulumEntityMatchesNative(entities[i], 0.3f);
        captureCheckpoint(false);
        float savedClock = 0;
        int controller = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].source->has("WreckingBallControllerBehaviour")) {
                controller = i;
                savedClock = entities[i].state.timer;
                break;
            }
        for (int step = 0; step < 100; step++)
            tick(1 / 120.0f, false, false);
        restoreCheckpoint();
        if (controller >= 0)
            restored &= absf(entities[controller].state.timer - savedClock) < 0.0001f;
        tick(1 / 120.0f, false, false);
        for (int i = 0; i < entityCount; i++)
            if (entities[i].motion == 1)
                restored &= pendulumEntityMatchesNative(entities[i], 0.3f);
    }
    check(
        roots == 14 && initial,
        "All14real stage pendulums initially use original cosine endpoints rather than spawning vertically at the wrong phase");
    check(
        radii,
        "Actual chain-link and end centres accumulate parent half-height, each child half-height and original LinkSpacing or LastLinkSpacing");
    check(
        driven,
        "Actual physics ticks across all original2second3second1point8second1point5second pendulums follow the native pi-time-overPeriod cosine and preserve authored sprite rotation");
    check(
        clocks,
        "Pendulum phases depend on each saved controller clock and do not jump when unrelated global elapsed time changes");
    check(
        restored,
        "Actual checkpoint restore resumes every pendulum controller clock and native chain pose without restarting its swing");
    startStage(0);
}
