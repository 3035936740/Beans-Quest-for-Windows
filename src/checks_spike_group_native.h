#pragma once

// Regression coverage for spike group native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void spikeGroupNativeChecks() {
    check(
        nativeSpikeGroupAffects(true, false, true, false) &&
            nativeSpikeGroupAffects(true, false, false, true) &&
            !nativeSpikeGroupAffects(true, true, true, true) &&
            !nativeSpikeGroupAffects(false, false, true, true),
        "Native SpikeGroup BeginContact ignores sensor targets and off state while accepting player-affectors and killable enemies");
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    const float dt = 1.0f / 120.0f;
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    int spike = -1, enemy = -1;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].source->id == 1000868912191818ULL && bodies[i])
            spike = i;
        if (entities[i].bossSpawn == 2 && enemy < 0)
            enemy = i;
        if (entities[i].kind == BOSS)
            entities[i].state.gone = 1;
    }
    check(
        spike >= 0 && enemy >= 0,
        "Original second-arena SpikeGroup owns a physical full-matrix detector beside the actual beetle summon pool");
    if (spike < 0 || enemy < 0)
        return;
    Entity &group = entities[spike];
    b2Fixture *fixture = bodies[spike]->GetFixtureList();
    int bodyCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->id == group.source->id && bodies[i])
            bodyCount++;
    bool bounds = false;
    if (fixture && fixture->GetShape()->GetType() == b2Shape::e_polygon) {
        const b2PolygonShape &shape = *(const b2PolygonShape *)fixture->GetShape();
        float l = 100000, r = -100000, b = 100000, t = -100000;
        for (int n = 0; n < shape.m_vertexCount; n++) {
            b2Vec2 p = bodies[spike]->GetWorldPoint(shape.m_vertices[n]);
            l = minf(l, p.x / metres);
            r = maxf(r, p.x / metres);
            b = minf(b, p.y / metres);
            t = maxf(t, p.y / metres);
        }
        bounds = absf(l - 4290) < 0.001f && absf(r - 4606) < 0.001f && absf(b - 36) < 0.001f &&
                 absf(t - 44) < 0.001f;
    }
    check(
        bodyCount == 1 && fixture && fixture->IsSensor() &&
            bodies[spike]->GetType() == b2_kinematicBody && bounds,
        "Original320px SpikeGroup uses one kinematic sensor rectangle with whole-matrix2/-2/4/-4 adjustments applied exactly once");
    Entity &beetle = entities[enemy];
    beetle.state.gone = 0;
    beetle.state.active = 1;
    beetle.state.x = 4400;
    beetle.state.y = 46;
    beetle.state.vx = beetle.state.vy = 0;
    entities[playerIndex].state.x = 4160;
    entities[playerIndex].state.y = 340;
    entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
    alternationBits = 0;
    restorePhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    simulateBodies(dt);
    bool offAlive = !beetle.state.gone && bodies[spike]->IsActive();
    alternationBits = 16;
    simulateBodies(dt);
    check(
        offAlive && !beetle.state.gone,
        "Off SpikeGroup keeps its fixture and changing its flag during an existing overlap does not invent a second BeginContact death");
    beetle.state.x = 4700;
    simulateBodies(dt);
    beetle.state.x = 4400;
    beetle.state.y = 46;
    beetle.state.vx = beetle.state.vy = 0;
    simulateBodies(dt);
    check(
        beetle.state.gone && !bodies[enemy]->IsActive(),
        "Leaving and re-entering the now-active original spike detector kills the beetle through real BeginContact");
    startStage(0);
    readyTimer = 0;
}
