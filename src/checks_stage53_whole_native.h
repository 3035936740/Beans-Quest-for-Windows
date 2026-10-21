#pragma once

// Regression coverage for stage53 whole native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void stage53WholeNativeChecks() {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    startStage(stageById(739118707375ULL));
    readyTimer = 0;
    int sources = 0, crates = 0, birds = 0, maintainers = 0;
    for (int i = 0; i < current.objectCount; i++) {
        Object &o = current.objects[i];
        sources += o.has("BulletSource");
        crates += o.klass == 2913132245228912724ULL;
        birds += o.klass == 17108215598456197958ULL;
        maintainers += o.has("TrampolineBehaviour");
    }
    check(
        current.size.x == 3008 && current.size.y == 384 && current.geometryCount == 12 &&
            sources == 6 && crates == 2 && birds == 2 && maintainers == 2 &&
            stageJumpTarget == 15 && current.props.num("bean-jump-best") == 13,
        "Complete original5-3retains3008x384terrain,12geometry outlines,6sources,2crates,2birds,2maintainers and original13/15jump targets");
    Entity &bean = entities[playerIndex];
    b2Fixture *fixture = bodies[playerIndex]->GetFixtureList();
    const b2PolygonShape *shape = (const b2PolygonShape *)fixture->GetShape();
    b2Vec2 lo = shape->m_vertices[0], hi = lo;
    for (int i = 1; i < shape->m_vertexCount; i++) {
        lo = b2Min(lo, shape->m_vertices[i]);
        hi = b2Max(hi, shape->m_vertices[i]);
    }
    check(
        shape->GetType() == b2Shape::e_polygon && absf((hi.x - lo.x) / metres - 16) < .001f &&
            absf((hi.y - lo.y) / metres - 20) < .001f && fixture->GetDensity() == 10 &&
            fixture->GetRestitution() == -1 && bodies[playerIndex]->IsBullet(),
        "Actual5-3Tom keeps native16x20core,density10,negative-restition sentinel and continuous-player body; artwork is not used as a larger hazard box");
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex)
            entities[i].state.gone = 1;
    bean.state.x = 2000;
    bean.state.y = 500;
    rebuildPhysics();
    const float starts[] = {-300, -200, -100, 0, 100, 200, 300};
    bool ordinary = true, overspeed = true;
    char trace[7000] = "fps,input,incoming1000,expected1000,actual1000\r\n";
    for (int rate = 0; rate < 3; rate++)
        for (int input = -1; input <= 1; input++)
            for (int n = 0; n < 7; n++) {
                float dt = 1.0f / (rate == 0   ? 60
                                   : rate == 1 ? 120
                                               : 240),
                      incoming = starts[n], target = input * 200.0f;
                // Native0x311864(right) /0x3123c4(left): stop acceleration only
                // when already beyond the requested speed in that direction.
                float delta = ((input > 0 && incoming > target) ||
                                       (input < 0 && incoming < target) || input == 0
                                   ? 200.0f
                                   : 500.0f) *
                              dt;
                float expected = incoming < target ? minf(target, incoming + delta)
                                                   : maxf(target, incoming - delta);
                bean.state.x = 2000;
                bean.state.y = 500;
                bean.state.vx = incoming;
                bean.state.vy = 0;
                bean.state.timer = .2f;
                bean.state.hp = 1;
                ordinaryJumpStateNative = OrdinaryJumpStateNative{};
                pinballPendingNative = PinballPendingNative{};
                trampolineStateNative.pending = 0;
                restorePhysics();
                tick(dt, input < 0, input > 0);
                bool equal = absf(bean.state.vx - expected) < .002f;
                bool excess = (input > 0 && incoming > target) || (input < 0 && incoming < target);
                if (excess)
                    overspeed &= equal;
                else
                    ordinary &= equal;
                char row[160];
                wsprintfA(row, "%d,%d,%d,%d,%d\r\n",
                          rate == 0   ? 60
                          : rate == 1 ? 120
                                      : 240,
                          input, (int)(incoming * 1000), (int)(expected * 1000),
                          (int)(bean.state.vx * 1000));
                lstrcatA(trace, row);
            }
    check(
        ordinary,
        "Actual5-3Tom matches native acceleration,release braking and opposite-direction reversal for57ordinary cases at60/120/240Hz");
    check(
        overspeed,
        "Actual5-3Tom preserves native200px/s2braking of collision-induced excess run speed in both directions instead of applying500px/s2movement acceleration");
    atomicWrite("test-output/stage53-whole-horizontal.csv", trace, lstrlenA(trace));
    atomicWrite("test-output/stage53-whole-results.txt", testLog, lstrlenA(testLog));
    startStage(0);
    readyTimer = 0;
}
