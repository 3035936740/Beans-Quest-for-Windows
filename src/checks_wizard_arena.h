#pragma once

// Regression coverage for wizard arena. Runs only in the isolated BQ_ENABLE_TESTS executable.

bool wizardGateBodies(bool on) {
    bool found = false, match = true;
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        if (e.kind != ALTERNATOR || e.mask != 4)
            continue;
        found = true;
        match &= activeBlock(e) == on;
        if (bodies[i])
            match &= bodies[i]->IsActive() == on;
    }
    return found && match;
}

bool wizardFloorBodies(bool on) {
    bool found = false, match = true;
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        if (e.kind != ALTERNATOR || e.mask != 8)
            continue;
        found = true;
        match &= activeBlock(e) == on;
        if (bodies[i])
            match &= bodies[i]->IsActive() == on;
    }
    return found && match;
}

void wizardArenaChecks() {
    const float dt = 1.0f / 120.0f;
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    int stage = stageById(976871008727ULL);
    startStage(stage);
    readyTimer = 0;
    int bossIndex = -1, firstGate = -1, gates = 0;
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        if (e.kind == BOSS)
            bossIndex = i;
        if (e.kind == ALTERNATOR && e.mask == 4) {
            if (e.source->id == 7152901639789759ULL && bodies[i])
                firstGate = i;
            bool first = true;
            for (int j = 0; j < i; j++)
                if (entities[j].kind == ALTERNATOR && entities[j].source->id == e.source->id)
                    first = false;
            if (first)
                gates++;
        }
    }
    check(
        bossIndex >= 0 && firstGate >= 0 && gates == 6,
        "Wizard arena regression resolves all six authored skull columns and the first actual3584px gate");
    if (bossIndex < 0 || firstGate < 0)
        return;
    int gateBodies = 0;
    bool matrixShape = true, parameters = true;
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        if (e.kind != ALTERNATOR || e.mask != 4)
            continue;
        parameters &= e.physics == e.source->behavior("AlternatorBlockBehaviour");
        if (!bodies[i])
            continue;
        gateBodies++;
        b2Body &body = *bodies[i];
        b2Fixture *fixture = body.GetFixtureList();
        parameters &= body.GetType() == b2_kinematicBody && fixture && fixture->GetDensity() == 0 &&
                      fixture->GetFriction() == 0.2f && fixture->GetRestitution() == 0 &&
                      !fixture->IsSensor() && fixture->GetFilterData().maskBits == 0xffff;
        if (fixture && fixture->GetShape()->GetType() == b2Shape::e_polygon) {
            const b2PolygonShape &shape = *(const b2PolygonShape *)fixture->GetShape();
            Rect actual = {100000, 100000, -100000, -100000};
            for (int v = 0; v < shape.m_vertexCount; v++) {
                b2Vec2 p = body.GetWorldPoint(shape.m_vertices[v]);
                actual.l = minf(actual.l, p.x / metres);
                actual.r = maxf(actual.r, p.x / metres);
                actual.b = minf(actual.b, p.y / metres);
                actual.t = maxf(actual.t, p.y / metres);
            }
            Rect original = e.source->rect;
            matrixShape &=
                absf(actual.l - original.l) < 0.001f && absf(actual.r - original.r) < 0.001f &&
                absf(actual.b - original.b) < 0.001f && absf(actual.t - original.t) < 0.001f;
        } else
            matrixShape = false;
    }
    check(
        gateBodies == 6 && matrixShape,
        "Each of the six real Wizard skull columns owns one full-height BoxGroup fixture with the original matrix bounds");
    check(
        parameters,
        "Inherited Wizard BoxGroup uses its own properties and native kinematic, zero-density, friction0.2, solid unfiltered defaults");
    check(
        alternationBits == 0 && wizardGateBodies(false) && wizardFloorBodies(false),
        "Original Alternation0 Wizard columns and mask8 floor start off with no invented initialmask4");
    Entity &gate = entities[firstGate];
    Animation *off = exactEnemyAnimation(gate.art, "off");
    Animation *on = exactEnemyAnimation(gate.art, "on");
    check(off && on && off->count == 1 && on->count == 1 &&
              off->frames[0].uv[0] != on->frames[0].uv[0],
          "Wizard column uses distinct original off-outline and solid-on artwork");
    // Keep the original level geometry. Only the distant boss is suspended
    // so this physical gate crossing cannot itself start a combat transition.
    entities[bossIndex].state.gone = 1;
    State &bean = entities[playerIndex].state;
    bean.x = 3568;
    bean.y = 80;
    bean.vx = bean.vy = 0;
    restorePhysics();
    for (int step = 0; step < 180; step++)
        tick(dt, false, true);
    check(
        deathTimer == 0 && bean.x > 3630,
        "Bean physically crosses the actual first skull column before combat while original terrain stays intact");
    alternationBits = 4;
    bean.x = 3568;
    bean.y = 80;
    bean.vx = bean.vy = 0;
    restorePhysics();
    for (int step = 0; step < 180; step++)
        tick(dt, false, true);
    check(deathTimer == 0 && bean.x < gate.state.x + gate.hit.l - entities[playerIndex].hit.r + 1,
          "The same real Wizard column blocks Bean when the native combat event turns it on");

    startStage(stage);
    readyTimer = 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            bossIndex = i;
    Entity &boss = entities[bossIndex];
    const int starts[] = {4, 20, 20, 4, 12}, ends[] = {0, 0, 0, 8, 8};
    bool sequence = true, geometry = true, keptOther = true, finalHeld = false;
    // Bit1 belongs to the later heart-switch puzzle and must survive every
    // Wizard event. Testing cumulative8/16 history catches absolute rewrites.
    alternationBits = 1;
    for (int area = 0; area < 5; area++) {
        boss.state.hp = 5 - area;
        boss.state.active = 2;
        boss.state.timer = 0;
        wizardBeginCombat(boss);
        restorePhysics();
        sequence &= alternationBits == (starts[area] | 1);
        geometry &= wizardGateBodies(true) && wizardFloorBodies((starts[area] & 8) != 0);
        keptOther &= (alternationBits & 1) != 0;
        boss.state.timer = 2;
        wizardHurt(boss);
        if (area < 4) {
            restorePhysics();
            sequence &= alternationBits == (ends[area] | 1);
            geometry &= wizardGateBodies(false) && wizardFloorBodies((ends[area] & 8) != 0);
        } else {
            finalHeld = alternationBits == (starts[area] | 1) && boss.state.active == 4;
            updateWizard(boss, 2.99f);
            finalHeld &= alternationBits == (starts[area] | 1) && boss.state.active == 4;
            updateWizard(boss, 0.02f);
            restorePhysics();
            sequence &= alternationBits == (ends[area] | 1) && boss.state.active == 5;
            geometry &= wizardGateBodies(false) && wizardFloorBodies(true);
            int finished = alternationBits;
            updateWizard(boss, 1);
            updateWizard(boss, 1);
            sequence &= alternationBits == finished;
        }
    }
    check(
        sequence && keptOther,
        "All five native Wizard start/end event masks retain cumulative8/16 state and preserve unrelated switchbit1");
    check(
        geometry,
        "Authored Wizard skull-column collision and mask8 floor bodies match every native arena phase");
    check(
        finalHeld,
        "Final-hit cages stay closed throughout the three-second death sequence and open exactly once on completion");
    startStage(stage);
    readyTimer = 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            bossIndex = i;
    const int health[] = {5, 5, 4, 2, 1, 0, 0}, mode[] = {0, 3, 2, 3, 3, 4, 5},
              expected[] = {0, 4, 0, 4, 12, 12, 8};
    bool migrated = true;
    for (int sample = 0; sample < 7; sample++) {
        State &b = entities[bossIndex].state;
        b.hp = health[sample];
        b.active = mode[sample];
        b.gone = mode[sample] == 5;
        captureCheckpoint(false);
        checkpoint.version = 9;
        checkpoint.bits = 1 | 4 | 16;
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        migrated &= validSnapshot(checkpoint);
        restoreCheckpoint();
        migrated &= alternationBits == (expected[sample] | 1);
    }
    check(
        migrated,
        "Legacy version9 Wizard checkpoints reconstruct every arena and death-phase gate parity while preserving unrelated bits");
    entities[bossIndex].state.hp = 1;
    entities[bossIndex].state.active = 3;
    entities[bossIndex].state.gone = 0;
    captureCheckpoint(false);
    checkpoint.version = 9;
    checkpoint.bits = 1 | 4;
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    bool written = atomicWrite("test-output/wizard-v9-arena.dat", &checkpoint, sizeof(checkpoint));
    bool loaded = written && readCheckpoint("test-output/wizard-v9-arena.dat");
    if (loaded)
        restoreCheckpoint();
    check(
        loaded && alternationBits == 13 && wizardGateBodies(true) && wizardFloorBodies(true),
        "An actual saved version9 final-arena checkpoint loads with its skull columns and cumulative mask8 floor repaired");
    alternationBits = 1 | 8;
    captureCheckpoint(false);
    bool version10 = checkpoint.version == 28 && validSnapshot(checkpoint);
    alternationBits = 0;
    restoreCheckpoint();
    check(
        version10 && alternationBits == 9,
        "Current version28 checkpoints retain their exact current event parity without reapplying the legacy repair");
    startStage(0);
    readyTimer = 0;
}
