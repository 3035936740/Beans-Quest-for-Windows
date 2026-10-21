#pragma once

// Regression coverage for selector motion. Runs only in the isolated BQ_ENABLE_TESTS executable.

void selectorMotionChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = true;
    musicEnabled = soundEnabled = false;
    debugMode = false;
    progress.unlocked = stageCount - 1;
    memset(keys, 0, sizeof(keys));
    navigate(worldScenes[4]);
    int mover = -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].move) {
            mover = i;
            break;
        }
    check(mover >= 0, "World 5 selector loads its authored oscillating block");
    if (mover >= 0) {
        Entity &block = page.playfield[mover];
        float origin = block.originX;
        tickUiPlayfield(page, 1);
        bool outbound =
            absf(block.state.x - origin - 100) < 0.01f && absf(block.state.vx - 100) < 0.01f;
        tickUiPlayfield(page, 0.5f);
        bool reflected = absf(block.state.x - origin - 106) < 0.01f && block.state.aux < 0;
        tickUiPlayfield(page, 0.25f);
        reflected &= absf(block.state.x - origin - 81) < 0.01f && block.state.vx < 0;
        check(outbound && reflected,
              "Selector oscillator preserves speed and overshoot through an endpoint reversal");
        page.bean.state.x = block.state.x;
        page.bean.state.y = hitRect(block).t - page.bean.hit.b;
        page.bean.state.vy = 0;
        float beanX = page.bean.state.x;
        tickUiPlayfield(page, 0.1f);
        check(absf(page.bean.state.x - beanX + 10) < 0.01f &&
                  absf(hitRect(page.bean).b - hitRect(block).t) < 0.01f,
              "Selector moving platform carries a supported Bean with the collision surface");
    }
    navigate(worldScenes[4]);
    int spring = -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].kind == SPRING) {
            spring = i;
            break;
        }
    if (spring >= 0) {
        Entity &support = page.playfield[spring];
        page.bean.state.x = support.state.x;
        page.bean.state.y = hitRect(support).t - page.bean.hit.b + 0.05f;
        page.bean.state.vy = -20;
        page.bean.state.timer = .1f;
        jumps = 19;
        alternationBits = 12;
        tickInterface(dt);
        check(support.state.active == 2 && support.state.timer == 0 &&
                  page.specialJump.mega.pending && page.specialJump.mega.source == spring &&
                  page.bean.state.vy < 100,
              "Selector spring contact starts its original bounce animation");
        check(jumps == 19 && alternationBits == 12,
              "Selector bounce does not mutate the suspended gameplay counters");
    }
    navigate(worldScenes[2]);
    int cloud = -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].crumble) {
            cloud = i;
            break;
        }
    check(cloud >= 0, "World 3 selector loads its crumbling cloud platforms");
    if (cloud >= 0) {
        Entity &support = page.playfield[cloud];
        support.state.active = 1;
        support.state.timer = 0;
        float y = support.state.y;
        tickUiPlayfield(page, dt);
        check(support.state.active == 2 && !solid(support) && support.state.y == y,
              "Selector cloud bursts immediately in place and releases its collision surface");
        tickUiPlayfield(page, 2.9f);
        check(support.state.active == 2 && !support.state.gone,
              "Selector cloud stays hidden until its authored respawn time");
        tickUiPlayfield(page, 0.11f);
        tickUiPlayfield(page, dt);
        check(support.state.active == 0 && !support.state.gone && solid(support),
              "Selector cloud restores its visible collision surface after the authored delay");
    }
    navigate(worldScenes[4]);
    float before = page.playfield[0].state.timer, travel = 0;
    int oldJumps = jumps, oldBits = alternationBits;
    for (int step = 0; step < 120; step++) {
        tickInterface(dt);
        for (int i = 0; i < page.playfieldCount; i++)
            if (page.playfield[i].move)
                travel = maxf(travel, absf(page.playfield[i].state.x - page.playfield[i].originX));
    }
    bool clock = false, moved = travel > 50;
    for (int i = 0; i < page.playfieldCount; i++) {
        Entity &e = page.playfield[i];
        clock |= e.state.timer > before + 0.9f;
    }
    check(clock && moved && !page.minimized && jumps == oldJumps && alternationBits == oldBits,
          "Visible selector buttons leave the miniature scene animation and movement running");
    page.minimized = true;
    keys[VK_LEFT] = true;
    tickInterface(dt);
    keys[VK_LEFT] = false;
    check(page.bean.state.aux < 0, "Playable selector Bean remembers leftward facing");
    keys[VK_RIGHT] = true;
    tickInterface(dt);
    keys[VK_RIGHT] = false;
    check(page.bean.state.aux > 0, "Playable selector Bean remembers rightward facing");
    page.minimizePhase = 1;
    transitionTime = 0;
    captureCase("test-output/selector-motion-world5.bmp");
    navigate(worldScenes[4]);
    page.minimized = true;
    tickInterface(0.125f);
    float x, y;
    layerOffset(page, page.selectorLayer, &x, &y);
    check(
        absf(page.minimizePhase - 0.5f) < 0.001f && absf(selectorLayerSlide(page) + 384) < 0.01f &&
            sceneLayerVisible(page, page.selectorLayer),
        "Selector stage buttons slide through the native smoothstep midpoint after 0.125 seconds");
    transitionTime = 0;
    captureCase("test-output/selector-minimize-midpoint.bmp");
    tickInterface(0.125f);
    check(page.minimizePhase == 1 && selectorLayerSlide(page) == -768 &&
              !sceneLayerVisible(page, page.selectorLayer),
          "Selector minimization reaches twice the viewport height after 0.25 seconds");
    int controls = -1;
    for (int i = 0; i < page.level.objectCount; i++)
        if (page.nodes[i].object->has("LevelSelectMinimizeButton"))
            controls = page.nodes[i].object->layer;
    if (controls >= 0) {
        float cx, cy;
        layerOffset(page, controls, &cx, &cy);
        Vec expected = layerTranslation(page.level, controls, page.homeX, page.homeY);
        check(
            cx == expected.x && cy == expected.y && sceneLayerVisible(page, controls),
            "Native stage-layer slide leaves back and restore controls in their authored position");
    }
    page.minimized = false;
    tickInterface(0.125f);
    check(absf(selectorLayerSlide(page) + 384) < 0.01f,
          "Restoring the selector reverses the original layer transition smoothly");
    tickInterface(0.125f);
    check(page.minimizePhase == 0 && selectorLayerSlide(page) == 0,
          "Restored selector ends exactly at its authored layer origin");
}
