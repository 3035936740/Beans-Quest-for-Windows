#pragma once

// Regression coverage for oneway geometry. Runs only in the isolated BQ_ENABLE_TESTS executable.

void oneWayGeometryChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    startStage(0);
    readyTimer = 0;
    int selected = -1;
    for (int i = 0; i < current.geometryCount; i++)
        if (current.geometry[i].kind == 2 && current.geometry[i].count == 2 &&
            absf((float)(current.geometry[i].points[1].x - current.geometry[i].points[0].x)) >=
                64) {
            selected = i;
            break;
        }
    check(
        selected >= 0,
        "Grasslands retains an authored two-point one-way terrain platform for physical regression");
    if (selected >= 0) {
        Geometry original = current.geometry[selected];
        current.geometry[0] = original;
        current.geometryCount = 1;
        for (int i = 0; i < entityCount; i++)
            if (i != playerIndex)
                entities[i].state.gone = 1;
        rebuildPhysics();
        b2Fixture *platform = 0;
        for (b2Body *b = physicsWorld->GetBodyList(); b; b = b->GetNext())
            for (b2Fixture *f = b->GetFixtureList(); f; f = f->GetNext())
                if (f->GetUserData() == (void *)1)
                    platform = f;
        bool polygon = platform && platform->GetShape()->GetType() == b2Shape::e_polygon;
        float thickness = 0;
        b2Vec2 a, b;
        if (polygon) {
            const b2PolygonShape *s = (const b2PolygonShape *)platform->GetShape();
            polygon = s->m_vertexCount == 4;
            if (polygon)
                thickness = (s->m_vertices[0].y - s->m_vertices[1].y) / metres;
        }
        check(
            polygon && absf(thickness - 8) < 0.0001f && platform->GetFriction() == 0.2f &&
                terrainOneWaySegment(platform, &a, &b),
            "Authored one-way terrain uses the native eight-pixel polygon below its top edge and0.2 friction");
        if (polygon) {
            Entity &player = entities[playerIndex];
            State &bean = player.state;
            float top = a.y / metres, mid = (a.x + b.x) * 0.5f / metres;
            bean.x = mid;
            bean.y = top - player.hit.t - 10;
            bean.vx = 0;
            bean.vy = 230;
            jumps = 0;
            restorePhysics();
            bool passed = false;
            for (int step = 0; step < 120 && !passed; step++) {
                tick(1.0f / 120.0f, false, false);
                passed = hitRect(player).b > top + 1;
            }
            check(
                passed && jumps == 0,
                "Bean passes upward through the underside of an actual original terrain platform without a false bounce");
            bool landed = false;
            for (int step = 0; step < 240 && !landed; step++) {
                tick(1.0f / 120.0f, false, false);
                landed = jumps > 0 && bean.vy > 100;
            }
            check(landed && absf(hitRect(player).b - top - bean.vy / 120) < 2,
                  "Bean later lands on and autojumps from the same original one-way polygon's top");
            bean.x = a.x / metres - 20;
            bean.y = top - 4;
            bean.vx = 200;
            bean.vy = 0;
            jumps = 0;
            restorePhysics();
            for (int step = 0; step < 36; step++)
                tick(1.0f / 120.0f, false, true);
            check(
                jumps == 0 && bean.x > a.x / metres + 20,
                "Entering below the one-way polygon's side does not catch an internal face or trigger a ghost jump");
        }
    }
    // All original endpoints happen to be horizontal, but the dedicated top
    // indices must remain correct for a sloped segment as well.
    {
        b2World *world = new b2World(b2Vec2(0, 0), true);
        b2BodyDef d;
        b2Body *body = world->CreateBody(&d);
        b2PolygonShape shape;
        terrainOneWayPolygon(shape, Vec{10, 10}, Vec{30, 50});
        b2Fixture *fixture = body->CreateFixture(&shape, 0);
        b2Vec2 a, b;
        check(
            terrainOneWaySegment(fixture, &a, &b) && absf(a.x / metres - 10) < 0.001f &&
                absf(a.y / metres - 10) < 0.001f && absf(b.x / metres - 30) < 0.001f &&
                absf(b.y / metres - 50) < 0.001f,
            "Sloped one-way top extraction preserves both original endpoints instead of mistaking the highest two polygon vertices for its surface");
        delete world;
    }
    // Exercise the actual Tom PreSolve hook on a narrow real contact, rather
    // than only testing the standalone squared-distance predicate.
    startStage(0);
    readyTimer = 0;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex)
            entities[i].state.gone = 1;
    rebuildPhysics();
    Entity &player = entities[playerIndex];
    player.state.x = 300;
    player.state.y = 325;
    player.state.vx = 0;
    player.state.vy = -10;
    restorePhysics();
    b2BodyDef d;
    d.position = b2Vec2(300 * metres, 300 * metres);
    b2Body *needle = physicsWorld->CreateBody(&d);
    b2PolygonShape narrow;
    narrow.SetAsBox(0.25f * metres, 16 * metres);
    needle->CreateFixture(&narrow, 0);
    physicsWorld->Step(1.0f / 120.0f, 10, 10);
    bool rejected = false;
    for (b2ContactEdge *edge = bodies[playerIndex]->GetContactList(); edge; edge = edge->next)
        if (edge->other == needle && edge->contact->IsTouching() &&
            edge->contact->GetManifold()->pointCount == 2)
            rejected |= !edge->contact->IsEnabled();
    check(
        rejected,
        "Tom's runtime PreSolve disables a collapsed subpixel two-point face contact before it can become a false floor");
    startStage(0);
    readyTimer = 0;
}
