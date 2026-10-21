#pragma once

// Regression coverage for spike placement native. Runs only in the isolated BQ_ENABLE_TESTS executable.

Rect spikePlacementSourceBounds(const Object &o) {
    Rect r = o.rect;
    const Props &p = *o.behavior("SpikeGroupBehaviour");
    r.l += p.num("adjust-left");
    r.r += p.num("adjust-right");
    r.b += p.num("adjust-bottom");
    r.t += p.num("adjust-top");
    return r;
}

Rect spikePlacementBodyBounds(b2Body &body) {
    Rect r = {100000, 100000, -100000, -100000};
    b2Fixture *fixture = body.GetFixtureList();
    if (!fixture || fixture->GetShape()->GetType() != b2Shape::e_polygon)
        return r;
    const b2PolygonShape &shape = *(const b2PolygonShape *)fixture->GetShape();
    for (int i = 0; i < shape.m_vertexCount; i++) {
        b2Vec2 p = body.GetWorldPoint(shape.m_vertices[i]);
        r.l = minf(r.l, p.x / metres);
        r.r = maxf(r.r, p.x / metres);
        r.b = minf(r.b, p.y / metres);
        r.t = maxf(r.t, p.y / metres);
    }
    return r;
}

bool spikePlacementEqual(Rect a, Rect b) {
    return absf(a.l - b.l) < 0.01f && absf(a.r - b.r) < 0.01f && absf(a.b - b.b) < 0.01f &&
           absf(a.t - b.t) < 0.01f;
}

int spikePlacementOwner(u64 id) {
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->id == id && boxGroupMatrixOwner(entities[i]))
            return i;
    return -1;
}

void spikePlacementNativeChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    int groups = 0, rotated = 0, left = 0, right = 0, down = 0, degenerate = 0;
    bool bounds = true, angle = true, zero = true;
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scene, memory);
        for (int i = 0; i < scene.objectCount; i++) {
            Object &o = scene.objects[i];
            if (!o.has("SpikeGroupBehaviour"))
                continue;
            groups++;
            float w = o.props.num("actlet-width", 32), h = o.props.num("actlet-height", 32);
            Entity e;
            initializeEntity(e, o,
                             Rect{o.rect.l, o.rect.b, minf(o.rect.l + w, o.rect.r),
                                  minf(o.rect.b + h, o.rect.t)});
            Rect expected = spikePlacementSourceBounds(o);
            b2World *world = new b2World(b2Vec2(0, 0), true);
            b2Body *body = createPhysicsEntity(*world, e, 0);
            if (!body) {
                bounds = false;
                delete world;
                continue;
            }
            world->Step(1 / 120.0f, 10, 10);
            bounds &= spikePlacementEqual(spikePlacementBodyBounds(*body), expected) &&
                      body->GetAngle() == 0;
            if (o.angle != 0) {
                rotated++;
                left += o.angle == 90;
                right += o.angle == 270;
                down += o.angle == 180;
                angle &= e.state.angle == o.angle && body->GetAngle() == 0;
            }
            if (expected.b == expected.t) {
                degenerate++;
                zero &= body->GetFixtureList()->IsSensor() &&
                        spikePlacementBodyBounds(*body).b == spikePlacementBodyBounds(*body).t;
            }
            delete world;
        }
    }
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        groups == 373 && rotated == 107 && left == 9 && right == 9 && down == 89 && degenerate == 2,
        "All74 scenes retain373 original spike matrices including107 visual rotations and two explicitly degenerate8px selector sensors");
    check(
        bounds && angle && zero,
        "Every actual original spike fixture matches unrotated source bounds plus its authored adjustments while all107 rotated owner sprites retain their independent source angle");
    const u64 stages[] = {976871008727ULL, 899263731133ULL, 899263731133ULL, 976871008727ULL};
    const u64 ids[] = {3198131710380485ULL, 891949405933ULL, 884170000475ULL, 7927408340391362ULL};
    bool art = true, world = true, leftSafe = true, actualKills = true, legacy = true;
    int glCases = 0;
    for (int sample = 0; sample < 4; sample++) {
        startStage(stageById(stages[sample]));
        readyTimer = 0;
        int owner = spikePlacementOwner(ids[sample]);
        if (owner < 0) {
            art = world = leftSafe = actualKills = legacy = false;
            continue;
        }
        Entity &e = entities[owner];
        Rect expected = spikePlacementSourceBounds(*e.source);
        State &bean = entities[playerIndex].state;
        bean.x = 64;
        bean.y = 400;
        bean.vx = bean.vy = 0;
        restorePhysics();
        for (int step = 0; step < 3; step++)
            tick(1 / 120.0f, false, false);
        world &= bodies[owner] &&
                 spikePlacementEqual(spikePlacementBodyBounds(*bodies[owner]), expected) &&
                 bodies[owner]->GetAngle() == 0;
        float shiftX = 64 - e.source->rect.l, shiftY = 64 - e.source->rect.b;
        Rect crop = {64, 64, 64 + e.source->rect.r - e.source->rect.l,
                     64 + e.source->rect.t - e.source->rect.b};
        cloudRenderBegin();
        glPushMatrix();
        glTranslatef(shiftX, shiftY, 0);
        for (int i = 0; i < entityCount; i++)
            if (entities[i].source == e.source)
                drawEntity(entities[i]);
        glPopMatrix();
        u32 actual = cloudRenderPixels(crop, false), inside = cloudRenderPixels(crop, true),
            allPixels = cloudRenderPixels(Rect{0, 0, 512, 384}, true);
        cloudRenderBegin();
        glPushMatrix();
        glTranslatef(shiftX, shiftY, 0);
        for (int i = 0; i < entityCount; i++)
            if (entities[i].source == e.source) {
                Entity &cell = entities[i];
                drawActor(cell.art, entityRect(cell), spikeAnimationName(cell), cell.state.timer,
                          e.source->angle);
            }
        glPopMatrix();
        u32 oracle = cloudRenderPixels(crop, false);
        art &= actual == oracle && inside > 0 && inside == allPixels;
        glCases++;
        if (sample == 0) {
            cloudRenderBegin();
            glPushMatrix();
            glTranslatef(128, 128, 0);
            glScalef(8, 8, 1);
            glTranslatef(-e.source->rect.l, -e.source->rect.b, 0);
            for (int i = 0; i < entityCount; i++)
                if (entities[i].source == e.source)
                    drawEntity(entities[i]);
            glPopMatrix();
            SwapBuffers(gameDC);
            capture("test-output/wizard-downward-spike-owner-art.bmp");
        }
        captureCheckpoint(false);
        checkpoint.version = 11;
        checkpoint.state[owner].angle = 0;
        restoreCheckpoint();
        legacy &= entities[owner].state.angle == e.source->angle &&
                  bodies[owner]->GetAngle() == 0 &&
                  spikePlacementEqual(spikePlacementBodyBounds(*bodies[owner]), expected);
        // Rotated cases test the obsolete fixture region. The ordinary
        // upward row instead tests clearance above its actual top surface.
        // All probes stay three pixels clear including Box2D's skin.
        bean.x = sample == 3   ? (expected.l + expected.r) * 0.5f
                 : sample == 2 ? expected.r - entities[playerIndex].hit.l + 3
                               : expected.l - entities[playerIndex].hit.r - 3;
        bean.y = sample == 3   ? expected.t - entities[playerIndex].hit.b + 3
                 : sample == 0 ? 248
                               : 120;
        bean.vx = bean.vy = 0;
        restorePhysics();
        for (int step = 0; step < 3; step++)
            tick(1 / 120.0f, false, false);
        leftSafe &= deathTimer <= 0 && !respawnBusy(respawnTransition);
        startStage(stageById(stages[sample]));
        readyTimer = 0;
        owner = spikePlacementOwner(ids[sample]);
        if (owner < 0) {
            actualKills = false;
            continue;
        }
        expected = spikePlacementSourceBounds(*entities[owner].source);
        State &hit = entities[playerIndex].state;
        hit.x = (expected.l + expected.r) * 0.5f;
        hit.y = (expected.b + expected.t) * 0.5f;
        hit.vx = hit.vy = 0;
        restorePhysics();
        for (int step = 0; step < 3; step++)
            tick(1 / 120.0f, false, false);
        actualKills &= deathTimer > 0 || respawnBusy(respawnTransition);
    }
    check(
        glCases == 4 && art && world,
        "Real0/180degree Wizard rows and actual90/270degree columns preserve every cell's exact source-angle GL artwork after physics ticks, with matching world bounds");
    check(
        leftSafe && actualKills,
        "Bean survives above the upward spike row and inside each old wrongly rotated fixture region, but dies on each actual original sensor in all four orientations");
    check(
        legacy,
        "Restoring an old version11 checkpoint with first-cell angle0 restores source0/180/90/270 artwork while the whole-matrix physical body remains unrotated");
    char trace[240];
    wsprintfA(
        trace,
        "groups=%d rotated=%d down=%d left=%d right=%d degenerate=%d bounds=%d angle=%d GLcases=%d art=%d physical=%d old-area-safe=%d actual-hit=%d legacy=%d\r\n",
        groups, rotated, down, left, right, degenerate, bounds, angle, glCases, art, world,
        leftSafe, actualKills, legacy);
    atomicWrite("test-output/spike-placement-native.txt", trace, lstrlenA(trace));
    startStage(0);
}
