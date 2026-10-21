#pragma once

// Regression coverage for water. Runs only in the isolated BQ_ENABLE_TESTS executable.

// Independent numeric fixtures plus real authored water contacts. These check
// displacement and forces, not merely the presence of a water behaviour name.
void waterChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = false;
    debugMode = true;
    musicEnabled = soundEnabled = false;
    int stageWithWater = -1, wetStages = 0, waterIndex = -1;
    for (int stage = 0; stage < stageCount; stage++) {
        startStage(stage);
        bool wet = false;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == WATER) {
                wet = true;
                waterIndex = i;
                const Props &props = *entities[i].source->behavior("WaterAreaBehaviour");
                check(
                    bodies[i] && bodies[i]->GetFixtureList()->IsSensor() &&
                        props.num("bean-density") == 15 && props.num("bean-linear-drag") == 15,
                    "Authored water pool has a sensor and the original Bean buoyancy/drag parameters");
                Vec authored = center(entities[i].source->rect);
                check(absf(entities[i].state.y - authored.y + 4) < 0.001f,
                      "Water activation lowers its sensor and surface by the original four pixels");
            }
        if (wet) {
            wetStages++;
            if (stageWithWater < 0)
                stageWithWater = stage;
        }
    }
    check(wetStages == 5, "All five authored gameplay stages containing water are covered");
    if (stageWithWater < 0)
        return;
    startStage(stageWithWater);
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == WATER) {
            waterIndex = i;
            break;
        }
    const Props &props = *entities[waterIndex].source->behavior("WaterAreaBehaviour");
    // b2World owns a large stack allocator; keep it on the heap in the
    // CRT-free build, which intentionally does not link stack-probe helpers.
    b2World &world = *new b2World(b2Vec2(0, -500 * metres), true);
    b2BodyDef definition;
    definition.type = b2_dynamicBody;
    definition.fixedRotation = true;
    b2Body *body = world.CreateBody(&definition);
    b2CircleShape circle;
    circle.m_radius = 1;
    b2FixtureDef fixture;
    fixture.shape = &circle;
    fixture.density = 10;
    b2Fixture *disc = body->CreateFixture(&fixture);
    b2Vec2 centre(0, 0);
    float area = submergedFixture(*disc, 0, centre);
    check(absf(area - b2_pi * 0.5f) < 0.00001f && absf(centre.y + 4 / (3 * b2_pi)) < 0.00001f,
          "Half-submerged circle uses segment area and its analytic centroid");
    area = submergedFixture(*disc, 2, centre);
    check(absf(area - b2_pi) < 0.00001f && centre.y == 0 &&
              submergedFixture(*disc, -2, centre) == 0,
          "Circle displacement covers dry and fully submerged cases");
    body->SetTransform(b2Vec2(20, 30), 0);
    area = submergedFixture(*disc, 30, centre);
    check(absf(area - b2_pi * 0.5f) < 0.00001f && absf(centre.x - 20) < 0.00001f &&
              absf(centre.y - (30 - 4 / (3 * b2_pi))) < 0.00001f,
          "Submerged centroid follows the actual fixture transform");
    body->SetTransform(b2Vec2(0, 0), 0);
    body->DestroyFixture(disc);
    b2PolygonShape polygon;
    polygon.SetAsBox(1, 1);
    fixture.shape = &polygon;
    b2Fixture *square = body->CreateFixture(&fixture);
    area = submergedFixture(*square, 0, centre);
    check(absf(area - 2) < 0.00001f && absf(centre.y + 0.5f) < 0.00001f,
          "Half-submerged box clips to the waterline with the correct area and centroid");
    body->SetTransform(b2Vec2(0, 0), b2_pi / 4);
    area = submergedFixture(*square, 0, centre);
    check(absf(area - 2) < 0.00001f && absf(centre.y + b2Sqrt(2.0f) / 3) < 0.00001f,
          "Rotated polygon displacement uses its clipped shape rather than its bounding box");
    body->SetTransform(b2Vec2(0, 0), 0);
    body->SetLinearVelocity(b2Vec2(100 * metres, 0));
    WaterSample full = sampleWater(*body, true, props, 10),
                half = sampleWater(*body, true, props, 0);
    check(
        absf(full.buoyancy.y / body->GetMass() / metres - 750) < 0.001f &&
            absf(full.drag.x / body->GetMass() / metres + 150) < 0.001f,
        "Fully submerged Bean receives +750 buoyancy and velocity-proportional drag, against -400 gravity");
    check(absf(half.buoyancy.y / full.buoyancy.y - 0.5f) < 0.00001f &&
              absf(half.drag.x / full.drag.x - 0.5f) < 0.00001f,
          "Partial immersion scales both forces with displaced area");
    WaterSample other = sampleWater(*body, false, props, 10);
    check(absf(other.buoyancy.y / body->GetMass() / metres - 87.5f) < 0.001f &&
              absf(other.drag.x / body->GetMass() / metres + 50) < 0.001f,
          "Ordinary dynamic objects use the separate authored water density and drag");
    body->SetLinearVelocity(b2Vec2(0, -200 * metres));
    full = sampleWater(*body, true, props, 10);
    body->ApplyForce(full.buoyancy, full.massCentre);
    body->ApplyForce(full.drag, full.areaCentre);
    world.Step(dt, 10, 10);
    check(
        absf(body->GetLinearVelocity().y / metres - (-200 + 550 * dt)) < 0.001f,
        "Water changes downward speed through force integration without an artificial -80 speed cap");
    body->SetTransform(b2Vec2(0, 0), 0);
    body->SetLinearVelocity(b2Vec2(0, 0));
    for (int i = 0; i < 120; i++) {
        full = sampleWater(*body, true, props, 100);
        body->ApplyForce(full.buoyancy, full.massCentre);
        body->ApplyForce(full.drag, full.areaCentre);
        world.Step(dt, 10, 10);
    }
    float factor = 1;
    for (int i = 0; i < 120; i++)
        factor *= 1 - 1.5f * dt;
    check(absf(body->GetLinearVelocity().y / metres - (250 / 1.5f) * (1 - factor)) < 0.005f,
          "One second of submerged motion matches the independent discrete buoyancy/drag solution");
    body->SetFixedRotation(false);
    body->SetTransform(b2Vec2(0, 0), 0);
    body->SetLinearVelocity(b2Vec2(0, 0));
    body->SetAngularVelocity(2);
    full = sampleWater(*body, false, props, 0);
    check(full.angularDrag < 0 && sampleWater(*body, true, props, 0).angularDrag == 0,
          "Angular resistance uses the authored object setting while Bean's remains zero");
    body->DestroyFixture(square);
    polygon.SetAsBox(0.5f, 1, b2Vec2(-2, 0), 0);
    fixture.density = 2;
    body->CreateFixture(&fixture);
    polygon.SetAsBox(0.5f, 1, b2Vec2(2, 0), 0);
    fixture.density = 6;
    body->CreateFixture(&fixture);
    full = sampleWater(*body, false, props, 0);
    check(absf(full.areaCentre.x) < 0.00001f && absf(full.massCentre.x - 1) < 0.00001f,
          "Compound fixtures keep geometric drag and density-weighted buoyancy centroids distinct");
    delete &world;

    readyTimer = 0;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && i != waterIndex)
            entities[i].state.gone = 1;
    Entity &bean = entities[playerIndex];
    Entity &water = entities[waterIndex];
    Rect pool = entityRect(water);
    bean.state.x = (pool.l + pool.r) * 0.5f;
    bean.state.y = (pool.b + pool.t) * 0.5f;
    bean.state.vx = bean.state.vy = 0;
    rebuildPhysics();
    tick(dt, false, false);
    check(playerInWater && !onGround,
          "Actual Box2D water sensor detects immersion without becoming a solid floor");
    float initialY = bean.state.y;
    for (int i = 0; i < 120; i++)
        tick(dt, false, false);
    check(bean.state.y > initialY + 20 && bean.state.vy > 80 && bean.state.vy < 200,
          "Real game ticks make a fully submerged Bean rise under the recovered forces");
    captureCheckpoint(true);
    float savedY = bean.state.y;
    bean.state.y = pool.t + 100;
    restoreCheckpoint();
    tick(dt, false, false);
    check(playerInWater && absf(bean.state.y - savedY) < 2,
          "Restoring a submerged checkpoint rebuilds water contact without retaining stale forces");
    checkpoint.state[waterIndex].y += 4;
    restoreCheckpoint();
    check(entityRect(water).t == pool.t,
          "Older snapshots cannot move the corrected static water surface back up four pixels");
    clearEffects();
    stopSounds();
    bean.state.y = pool.t - bean.hit.b + 3;
    bean.state.vy = -200;
    restorePhysics();
    tick(dt, false, false);
    soundEnabled = true;
    for (int i = 0; i < 7; i++)
        tick(dt, false, false);
    soundEnabled = false;
    check(
        playerInWater && liveEffects() > 0 && activeAudioVoices() > 0,
        "Crossing the actual water surface at speed emits splash particles and an original water sound");
    stopSounds();
    bean.state.y = pool.t - bean.hit.b - 2;
    bean.state.vy = 250;
    restorePhysics();
    tick(dt, false, false);
    clearEffects();
    soundEnabled = true;
    for (int i = 0; i < 6; i++)
        tick(dt, false, false);
    soundEnabled = false;
    check(!playerInWater && liveEffects() > 0 && activeAudioVoices() > 0,
          "Leaving the water surface also triggers the native speed-gated splash");
    stopSounds();
    bean.state.y = pool.t - bean.hit.b + 1;
    bean.state.vy = -10;
    restorePhysics();
    tick(dt, false, false);
    clearEffects();
    for (int i = 0; i < 12; i++)
        tick(dt, false, false);
    check(liveEffects() == 0,
          "Slow waterline contact does not repeatedly emit artificial splash particles");
    bean.state.x = pool.r + 100;
    bean.state.y = pool.t + 100;
    bean.state.vx = bean.state.vy = 0;
    restorePhysics();
    tick(dt, false, false);
    tick(dt, false, false);
    check(!playerInWater && absf(bean.state.vy + 800 * dt) < 0.001f,
          "Exiting a pool clears queued fluid forces and restores ordinary airborne gravity");
    debugMode = false;
}
