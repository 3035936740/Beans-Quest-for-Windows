#pragma once

// Regression coverage for magnet. Runs only in the isolated BQ_ENABLE_TESTS executable.

void magnetSeamChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = false;
    debugMode = true;
    musicEnabled = soundEnabled = false;
    int stage = motionStage(5, 4);
    check(stage >= 0, "Magnet seam regression uses authored fifth-world stage four");
    if (stage < 0)
        return;
    startStage(stage);
    readyTimer = 0;
    int first = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == MAGNET && entities[i].source->id == 4715698496963111ULL &&
            magnetMatrixOwner(entities[i])) {
            first = i;
            break;
        }
    check(first >= 0, "Authored 192-pixel magnetic matrix is found");
    if (first < 0)
        return;
    Entity &owner = entities[first];
    Object *source = owner.source;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && entities[i].source != source)
            entities[i].state.gone = 1;
    rebuildPhysics();
    int cells = 0, bodyCount = 0, fixtureCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source == source) {
            cells++;
            if (bodies[i]) {
                bodyCount++;
                for (b2Fixture *f = bodies[i]->GetFixtureList(); f; f = f->GetNext())
                    fixtureCount++;
            }
        }
    check(cells == 6 && bodyCount == 1 && fixtureCount == 1,
          "Six magnetic visual actlets share the native single full-width physical fixture");
    const b2PolygonShape *shape =
        (const b2PolygonShape *)bodies[first]->GetFixtureList()->GetShape();
    float low = 100000, high = -100000;
    for (int i = 0; i < shape->GetVertexCount(); i++) {
        float x = bodies[first]->GetWorldPoint(shape->GetVertex(i)).x / metres;
        low = minf(low, x);
        high = maxf(high, x);
    }
    check(absf(low - 640) < 0.001f && absf(high - 832) < 0.001f,
          "Magnetic fixture covers authored source bounds rather than a 32-pixel cell");
    b2Fixture *beanFixture = bodies[playerIndex]->GetFixtureList();
    check(beanFixture->GetFriction() == 0 && beanFixture->GetRestitution() == -1,
          "Bean main fixture retains native friction zero and restitution minus one");
    for (int direction = 0; direction < 2; direction++) {
        State &bean = entities[playerIndex].state;
        bean.x = direction ? 816.0f : 656.0f;
        bean.y = 224 - entities[playerIndex].hit.b + 0.1f;
        bean.vx = 0;
        bean.vy = -20;
        restorePhysics();
        int oldJumps = jumps;
        float minSpeed = 10000;
        int seamSamples = 0;
        for (int frame = 0; frame < 100; frame++) {
            tick(dt, direction != 0, direction == 0);
            if (frame > 55) {
                minSpeed = minf(minSpeed, absf(bean.vx));
                seamSamples++;
            }
        }
        check(
            (direction ? bean.x < 720 : bean.x > 752) && minSpeed > 175 && seamSamples > 0,
            "Bean crosses several internal magnet actlet seams in either direction without invisible side blockage");
        check(bean.active == 2 && jumps == oldJumps &&
                  absf(hitRect(entities[playerIndex]).b - 224) < 2,
              "Crossing magnetic seams preserves top contact and suppresses automatic bounce");
        int active = 0, ownActive = 0;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].source == source && entities[i].state.active == 2) {
                active++;
                if (i == first)
                    ownActive++;
            }
        check(
            active >= 1 && active <= 2 && ownActive == 0,
            "Contact points activate only the one or two touched magnetic actlets, not the matrix body owner's first cell");
    }
    State &bean = entities[playerIndex].state;
    bean.x = 820;
    bean.y = 224 - entities[playerIndex].hit.b + 0.1f;
    bean.vx = 200;
    bean.vy = 0;
    restorePhysics();
    for (int i = 0; i < 60; i++)
        tick(dt, false, true);
    check(bean.x > 860 && bean.active != 2 && bean.y < 224,
          "Bean freely leaves the end of a magnetic matrix and falls without an attachment joint");
}

void selectorMagnetChecks() {
    interfaceEnabled = true;
    musicEnabled = soundEnabled = false;
    navigate(worldScenes[4]);
    page.minimized = true;
    page.minimizePhase = 1;
    int first = -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].kind == MAGNET &&
            page.playfield[i].source->id == 8759011704404663ULL &&
            magnetMatrixOwner(page.playfield[i])) {
            first = i;
            break;
        }
    check(first >= 0, "World-five selector retains its original 16-pixel magnetic actlets");
    if (first < 0)
        return;
    Object *source = page.playfield[first].source;
    page.level.geometryCount = 0;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].source != source)
            page.playfield[i].state.gone = 1;
    destroyUiPhysics(page);
    createUiPhysics(page);
    u32 stageBefore = hashBytes(entities, sizeof(Entity) * entityCount);
    int stageJumps = jumps, stageBits = alternationBits;
    page.bean.state.x = 424;
    page.bean.state.y = 80 - page.bean.hit.b + 0.1f;
    page.bean.state.vx = 0;
    page.bean.state.vy = -10;
    resetUiPhysics(page);
    for (int i = 0; i < 12; i++) {
        page.bean.state.vy -= 200.0f / 120;
        stepUiPhysics(page, 1 / 120.0f);
    }
    int active = 0;
    bool firstActive = false;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].source == source && page.playfield[i].state.active == 2) {
            active++;
            firstActive |= i == first;
        }
    check(page.bean.state.active == 2 && active >= 1 && active <= 2 && !firstActive,
          "Selector magnetic contact activates only the touched miniature actlets and Bean");
    page.bean.state.x = 490;
    page.bean.state.y = 130;
    page.bean.state.vy = 0;
    resetUiPhysics(page);
    stepUiPhysics(page, 1 / 120.0f);
    active = 0;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].kind == MAGNET && page.playfield[i].state.active == 2)
            active++;
    check(
        page.bean.state.active != 2 && active == 0,
        "Leaving selector magnetic contact resets every transient miniature attachment animation");
    check(stageBefore == hashBytes(entities, sizeof(Entity) * entityCount) && jumps == stageJumps &&
              alternationBits == stageBits,
          "Selector magnetic actlet updates cannot mutate suspended stage entities or counters");
}
