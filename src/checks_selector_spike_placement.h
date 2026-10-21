#pragma once

// Regression coverage for selector spike placement. Runs only in the isolated BQ_ENABLE_TESTS executable.

void selectorSpikePlaceFresh(UiScene &s, float x, float y) {
    // End the preceding real contact before testing another BeginContact.
    s.miniDeath = 0;
    s.bean.state.x = x + 100;
    s.bean.state.y = y + 140;
    s.bean.state.vx = s.bean.state.vy = 0;
    resetUiPhysics(s);
    stepUiPhysics(s, 1 / 120.0f);
    s.miniDeath = 0;
    s.bean.state.x = x;
    s.bean.state.y = y;
    s.bean.state.vx = s.bean.state.vy = 0;
    resetUiPhysics(s);
}

void selectorSpikePlacementChecks() {
    interfaceEnabled = true;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    progress.unlocked = stageCount - 1;
    memset(keys, 0, sizeof(keys));
    u32 suspended = hashBytes(entities, sizeof(Entity) * entityCount);
    int suspendedCount = entityCount;
    navigate(8808768056724772154ULL);
    page.minimized = true;
    const float dt = 1 / 120.0f;
    int owners[2] = {-1, -1};
    const u64 ids[2] = {8050772260726685103ULL, 17898598417641120077ULL};
    int cells = 0;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].kind == SPIKE) {
            cells++;
            for (int j = 0; j < 2; j++)
                if (page.playfield[i].source->id == ids[j] && page.bodies[i])
                    owners[j] = i;
        }
    check(
        page.physics && owners[0] >= 0 && owners[1] >= 0 && cells > 2,
        "Actual Grasslands selector owns both authored spike matrices and their visual actlets in its independent physics world");
    bool exact = true, physical = true, side = true, above = true, damage = true, beginOnly = true,
         angles = true;
    for (int j = 0; j < 2; j++) {
        if (owners[j] < 0) {
            exact = physical = side = above = damage = beginOnly = angles = false;
            continue;
        }
        int index = owners[j];
        Entity &e = page.playfield[index];
        Rect local = boxGroupMatrixFixture(e);
        Rect r = shifted(local, e.state.x, e.state.y);
        float expectedLeft = j ? 338.0f : 258.0f, expectedRight = j ? 366.0f : 286.0f,
              expectedY = j ? 68.0f : 36.0f;
        exact &= r.l == expectedLeft && r.r == expectedRight && r.b == expectedY &&
                 r.t == expectedY && e.source->rect.t - e.source->rect.b == 8 &&
                 e.source->angle == 0;
        b2Body *body = page.bodies[index];
        b2Fixture *fixture = body->GetFixtureList();
        physical &= body->GetType() == b2_kinematicBody && body->GetAngle() == 0 && fixture &&
                    fixture->IsSensor() && fixture->GetShape()->GetType() == b2Shape::e_polygon;
        if (fixture && fixture->GetShape()->GetType() == b2Shape::e_polygon) {
            const b2PolygonShape *p = (const b2PolygonShape *)fixture->GetShape();
            for (int k = 0; k < p->m_vertexCount; k++) {
                b2Vec2 v = body->GetWorldPoint(p->m_vertices[k]);
                physical &= absf(v.y / metres - expectedY) < 0.001f &&
                            v.x / metres >= expectedLeft - 0.001f &&
                            v.x / metres <= expectedRight + 0.001f;
            }
        }
        float cx = (r.l + r.r) * 0.5f, cy = r.b;
        selectorSpikePlaceFresh(page, r.r - page.bean.hit.l + 2, cy);
        tickInterface(dt);
        side &= page.miniDeath == 0 && !page.contacts->spikeBeanPending;
        selectorSpikePlaceFresh(page, r.l - page.bean.hit.r - 2, cy);
        tickInterface(dt);
        side &= page.miniDeath == 0 && !page.contacts->spikeBeanPending;
        selectorSpikePlaceFresh(page, cx, cy - page.bean.hit.b + 2);
        tickInterface(dt);
        above &= page.miniDeath == 0 && !page.contacts->spikeBeanPending;
        selectorSpikePlaceFresh(page, cx, cy);
        stepUiPhysics(page, dt);
        bool pending = page.contacts->spikeBeanPending;
        stepUiPhysics(page, dt);
        beginOnly &= pending && !page.contacts->spikeBeanPending;
        selectorSpikePlaceFresh(page, cx, cy);
        tickInterface(dt);
        damage &= page.miniDeath > 0;
        selectorSpikePlaceFresh(page, cx, cy + 150);
        e.state.angle = 57;
        resetUiPhysics(page);
        angles &= e.state.angle == e.source->angle && body->GetAngle() == 0;
        for (int k = 0; k < 24; k++)
            tickInterface(dt);
        for (int k = 0; k < page.playfieldCount; k++)
            if (page.playfield[k].source == e.source)
                angles &= page.playfield[k].state.angle == e.source->angle;
    }
    check(
        exact && physical,
        "Both original selector spike sources retain their native zero-height sensor boundaries258to286at36and338to366at68 without invented thickness or rotated group bodies");
    check(
        side && above,
        "Selector Bean beside or just above the original damage line survives even where its visible sprite overlaps spike artwork");
    check(
        beginOnly,
        "Actual selector spike damage is raised only on a fresh non-sensor Bean BeginContact and is not continuously reissued by an overlapping visual rectangle");
    check(
        damage,
        "Entering each original selector spike sensor through tickInterface starts the miniature Bean death sequence");
    check(
        angles,
        "Repeated selector simulation and reset preserve every spike actlet's authored zero visual angle while group collision angles remain zero");
    check(entityCount == suspendedCount &&
              hashBytes(entities, sizeof(Entity) * entityCount) == suspended,
          "Selector spike contacts and reset tests leave suspended gameplay entities unchanged");
    navigate(8808768056724772154ULL);
    interfaceEnabled = false;
}
