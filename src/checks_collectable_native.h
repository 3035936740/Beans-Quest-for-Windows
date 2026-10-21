#pragma once

// Regression coverage for collectable native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void collectableNativeChecks() {
    Rect player = {-8, -10, 8, 10}, cell = {-16, -16, 16, 16};
    check(absf(nativeCollectableRadius(cell) - 16) < 0.00001f,
          "Original 32-pixel diamond has a 16-pixel circle sensor without an invented inset");
    check(
        nativeCollectableTouches(player, Vec{22, 0}, 16),
        "Bean collects a diamond at its circular side before the old shrunken AABB would overlap");
    check(!nativeCollectableTouches(player, Vec{22, 24}, 16),
          "Diamond sensor excludes square corners outside the native circle");
    check(nativeCollectableTouches(player, Vec{18, 20}, 16) &&
              !nativeCollectableTouches(player, Vec{26, 0}, 16),
          "Gem collection uses Bean's adjusted body instead of its larger hat artwork");
    b2PolygonShape box;
    box.SetAsBox(8 * metres, 10 * metres);
    b2CircleShape circle;
    circle.m_radius = 16 * metres;
    b2Transform transform;
    transform.SetIdentity();
    const Vec cases[] = {{22, 0}, {22, 24}, {18, 20}, {24.5f, 0}, {24.8f, 0}};
    bool same = true;
    for (int i = 0; i < 5; i++) {
        circle.m_p.Set(cases[i].x * metres, cases[i].y * metres);
        bool touch = b2TestOverlap(&box, 0, &circle, 0, transform, transform);
        if (touch != nativeCollectableTouches(player, cases[i], 16, box.m_radius / metres))
            same = false;
    }
    check(
        same,
        "Collectable helper agrees with actual Box2D circle-versus-main-fixture overlap including polygon skin");
    bool sideCollected = false, cornerRejected = false, fixtureCorrect = false;
    for (int scenario = 0; scenario < 2; scenario++) {
        startStage(0);
        readyTimer = deathTimer = 0;
        current.geometryCount = 0;
        for (int i = 0; i < entityCount; i++)
            if (i != playerIndex)
                entities[i].state.gone = 1;
        Vec gem = {150, scenario ? 152.0f : 128.0f};
        int index = entityCount;
        addEntity(*classById(314060601782828169ULL),
                  Rect{gem.x - 16, gem.y - 16, gem.x + 16, gem.y + 16});
        Entity &bean = entities[playerIndex];
        bean.state.x = bean.state.y = 128;
        bean.state.vx = bean.state.vy = 0;
        rebuildPhysics();
        if (bodies[index]) {
            b2Fixture *sensor = bodies[index]->GetFixtureList();
            fixtureCorrect =
                sensor && sensor->IsSensor() && sensor->GetType() == b2Shape::e_circle &&
                absf(((b2CircleShape *)sensor->GetShape())->m_radius / metres - 16) < 0.001f;
        }
        int before = coins;
        tick(1 / 120.0f, false, false);
        if (!scenario)
            sideCollected = entities[index].state.gone && coins == before + 1;
        else
            cornerRejected = !entities[index].state.gone && coins == before;
    }
    check(fixtureCorrect, "Real gem entity creates the original full-radius circle sensor fixture");
    check(sideCollected,
          "Actual gameplay collects a side-contact gem that the previous inset box missed");
    check(cornerRejected,
          "Actual gameplay refuses the AABB-corner false positive outside the original gem circle");
    startStage(0);
}
