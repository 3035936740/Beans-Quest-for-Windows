#pragma once

// Regression coverage for tom contact. Runs only in the isolated BQ_ENABLE_TESTS executable.

void tomContactChecks() {
    check(
        !nativeTomCanAutoJump(-100, 0) && !nativeTomCanAutoJump(-100, 0.49f) &&
            nativeTomCanAutoJump(-100, 0.5f) && !nativeTomCanAutoJump(0.01f, 1),
        "Tom only autojumps while descending onto a support normal of at least0.5; walls, shallow corner normals and rising contacts cannot trigger jumps");
    check(
        nativeTomCollapsedManifold(Vec{0, 0}, Vec{0.01f, 0}) &&
            !nativeTomCollapsedManifold(Vec{0, 0}, Vec{0.02f, 0}),
        "Original PreSolve discards a subpixel collapsed two-point contact while retaining an ordinary separated manifold");
    Rect core = {-8, -10, 8, 10};
    check(
        nativeTomLoopCorner(core, Vec{8.25f, 0}, Vec{-1, 0}) &&
            !nativeTomLoopCorner(core, Vec{0, -10}, Vec{0, 1}) &&
            !nativeTomLoopCorner(core, Vec{8, 0}, Vec{-1, 0}),
        "Loop corner suppression rejects outside-core axial ghost contacts without removing the real floor or an inside-core wall contact");
    Vec flat = nativeTomJumpFactors(0, 0.2f), slope = nativeTomJumpFactors(0.6f, 0.2f);
    check(
        flat.x == 0 && flat.y == 1 && absf(slope.x - 0.12f) < 0.0001f &&
            absf(slope.x * slope.x + slope.y * slope.y - 1) < 0.0001f,
        "Native slope jumping uses authored0.2 horizontal normal influence while preserving normalized launch magnitude");
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    startStage(0);
    readyTimer = 0;
    b2Fixture *f = bodies[playerIndex]->GetFixtureList();
    bool shape = f && f->GetShape()->GetType() == b2Shape::e_polygon;
    float left = 1000, right = -1000, bottom = 1000, top = -1000;
    if (shape) {
        const b2PolygonShape *p = (const b2PolygonShape *)f->GetShape();
        shape = p->m_vertexCount == 4;
        for (int i = 0; i < p->m_vertexCount; i++) {
            left = minf(left, p->m_vertices[i].x / metres);
            right = maxf(right, p->m_vertices[i].x / metres);
            bottom = minf(bottom, p->m_vertices[i].y / metres);
            top = maxf(top, p->m_vertices[i].y / metres);
        }
    }
    check(
        shape && absf(left + 8) < 0.001f && absf(right - 8) < 0.001f &&
            absf(bottom + 10) < 0.001f && absf(top - 10) < 0.001f && f->GetFriction() == 0 &&
            f->GetRestitution() == -1,
        "Original Bean uses its adjusted16x20 rectangular fixture, zero friction and negative restitution without a guessed capsule or rounded collision hull");
    // Actual vertical alternator column from Grasslands Fortress. Keeping the
    // entire authored object also verifies that visual actlet seams do not
    // masquerade as floors against Bean's side.
    int stage = -1;
    for (int i = 0; i < stageCount; i++)
        if (levels[stageList[i]].props.num("bean-world") == 1 &&
            levels[stageList[i]].props.num("bean-level") == 8)
            stage = i;
    if (stage >= 0) {
        startStage(stage);
        readyTimer = 0;
        Rect column = {};
        int found = -1;
        current.geometryCount = 0;
        for (int i = 0; i < entityCount; i++) {
            Entity &e = entities[i];
            if (e.source->id == 6208068556487846068ULL) {
                column = e.source->rect;
                found = i;
            } else if (i != playerIndex)
                e.state.gone = 1;
        }
        if (found >= 0) {
            if (!activeBlock(entities[found]))
                alternationBits ^= entities[found].mask;
            Entity &player = entities[playerIndex];
            player.state.x = column.l - player.hit.r - 0.2f;
            player.state.y = (column.b + column.t) * 0.5f;
            player.state.vx = 100;
            player.state.vy = -10;
            jumps = 0;
            rebuildPhysics();
            bool falling = true;
            for (int step = 0; step < 60; step++) {
                tick(1.0f / 120.0f, false, true);
                falling &= player.state.vy <= 0.1f;
            }
            check(
                falling && jumps == 0 && player.state.x <= column.l - player.hit.r + 1.5f,
                "Pressing against the actual Fortress alternator column falls along its side without seam jumps or entering the wall");
        } else
            check(false, "Original Fortress vertical alternator column resolves");
    } else
        check(false, "Original Fortress stage resolves for the reported wall contact regression");
    startStage(0);
    readyTimer = 0;
}
