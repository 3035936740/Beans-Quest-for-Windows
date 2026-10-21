#pragma once

// Regression coverage for selector physics. Runs only in the isolated BQ_ENABLE_TESTS executable.

void selectorPhysicsChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = true;
    debugMode = true;
    musicEnabled = soundEnabled = false;
    memset(keys, 0, sizeof(keys));
    startStage(0);
    readyTimer = 0;
    State stageBean = entities[playerIndex].state;
    b2World *stageWorld = physicsWorld;
    int count = entityCount, oldJumps = jumps, oldCoins = coins;
    u32 stateHash = hashBytes(entities, sizeof(Entity) * entityCount);
    for (int w = 0; w < 5; w++) {
        navigate(worldScenes[w]);
        check(page.physics && page.physics != stageWorld && page.bodies[page.playfieldCount] &&
                  page.bodies[page.playfieldCount]->GetMass() > 0,
              "Selector owns a separate Box2D world with the authored miniature Bean fixture");
        float x = page.homeX, y = page.homeY;
        page.minimized = true;
        keys[VK_RIGHT] = true;
        for (int i = 0; i < 120; i++)
            tickInterface(dt);
        memset(keys, 0, sizeof(keys));
        check(page.cameraX == x && page.cameraY == y,
              "Playable selector preserves its fixed authored camera while Bean moves");
    }
    check(
        physicsWorld == stageWorld && entityCount == count &&
            hashBytes(entities, sizeof(Entity) * entityCount) == stateHash && jumps == oldJumps &&
            coins == oldCoins && entities[playerIndex].state.x == stageBean.x,
        "All five selector simulations leave the suspended stage bodies, counters and entity states unchanged");
    navigate(worldScenes[3]);
    int water = -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].kind == WATER) {
            water = i;
            break;
        }
    check(water >= 0, "Desert selector retains its authored water pool");
    if (water >= 0) {
        destroyUiPhysics(page);
        page.level.geometryCount = 0;
        for (int i = 0; i < page.playfieldCount; i++)
            if (i != water)
                page.playfield[i].state.gone = 1;
        Rect pool = entityRect(page.playfield[water]);
        page.bean.state.x = (pool.l + pool.r) * 0.5f;
        page.bean.state.y = pool.b + 32;
        page.bean.state.vx = page.bean.state.vy = 0;
        page.miniDeath = 0;
        createUiPhysics(page);
        page.minimized = true;
        tickInterface(dt);
        check(page.inWater, "Miniature Bean detects a real water sensor contact");
        b2Body *body = page.bodies[page.playfieldCount];
        WaterSample sample =
            sampleWater(*body, true, *page.playfield[water].source->behavior("WaterAreaBehaviour"),
                        pool.t * metres);
        check(
            absf(sample.buoyancy.y / body->GetMass() / metres - 750) < 0.001f &&
                page.bean.physics->num("gravity") == -200,
            "Selector water uses native +750 displaced-area buoyancy with the miniature Bean's authored -200 gravity");
        float y = page.bean.state.y;
        for (int i = 0; i < 30; i++)
            tickInterface(dt);
        check(page.inWater && page.bean.state.y > y + 7 && page.bean.state.vy > 70,
              "Selector Bean actually rises under water instead of falling to the bottom");
        page.bean.state.x = pool.r + 80;
        page.bean.state.y = pool.t + 60;
        page.bean.state.vx = page.bean.state.vy = 0;
        resetUiPhysics(page);
        tickInterface(dt);
        tickInterface(dt);
        check(!page.inWater && absf(page.bean.state.vy + 400 * dt) < 0.001f,
              "Leaving selector water clears fluid forces and restores miniature airborne gravity");
        page.bean.state.x = (pool.l + pool.r) * 0.5f;
        page.bean.state.y = pool.t - page.bean.hit.b + 2;
        page.bean.state.vx = 0;
        page.bean.state.vy = -180;
        resetUiPhysics(page);
        tickInterface(dt);
        clearEffects(1);
        soundEnabled = true;
        for (int i = 0; i < 5; i++)
            tickInterface(dt);
        soundEnabled = false;
        check(
            liveEffects(1) > 0 && liveEffects(0) == 0 && activeAudioVoices() > 0,
            "Selector surface crossing emits original water effects into its own particle domain");
        stopSounds();
        captureCase("test-output/selector-water-fixed-camera.bmp");
    }
    navigate(worldScenes[2]);
    int cloud = -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].crumble) {
            cloud = i;
            break;
        }
    if (cloud >= 0) {
        destroyUiPhysics(page);
        page.level.geometryCount = 0;
        for (int i = 0; i < page.playfieldCount; i++)
            if (i != cloud)
                page.playfield[i].state.gone = 1;
        Entity &c = page.playfield[cloud];
        page.bean.state.x = c.state.x;
        page.bean.state.y = hitRect(c).b - page.bean.hit.t - 2;
        page.bean.state.vx = 0;
        page.bean.state.vy = 150;
        page.miniDeath = 0;
        createUiPhysics(page);
        for (int i = 0; i < 30; i++)
            tickInterface(dt);
        check(
            page.bean.state.y > hitRect(c).t && c.state.active == 0,
            "Selector Bean passes upward through the original cloud fixture without breaking it from below");
        page.bean.state.y = hitRect(c).t - page.bean.hit.b + 0.1f;
        page.bean.state.vy = -20;
        resetUiPhysics(page);
        for (int i = 0; i < 4; i++)
            tickInterface(dt);
        check(c.state.active == 2 && !page.bodies[cloud]->IsActive() && page.bean.state.vy > 0,
              "Landing on selector cloud bounces Bean and disables only the crumbled surface");
    }
    screen = COMPLETE;
    int backing = 0;
    for (int i = 0; i < hudScene.level.objectCount; i++) {
        UiNode &n = hudScene.nodes[i];
        if (eq(n.object->props.get("name"), "Pause Button Text Background") &&
            n.object->layer == 2 && visibleNode(hudScene, n))
            backing++;
    }
    check(backing == 6, "Stage-complete captions retain all six authored black backing halves");
    captureCase("test-output/complete-caption-backing.bmp");
    interfaceEnabled = false;
    debugMode = false;
}
