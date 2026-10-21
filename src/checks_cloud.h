#pragma once

// Regression coverage for cloud. Runs only in the isolated BQ_ENABLE_TESTS executable.

// Read the actual OpenGL output: a valid collision fixture alone cannot catch
// the transparent "idle" animation that previously erased cloud artwork.
u32 cloudRenderPixels(Rect r, bool countPixels) {
    int x = viewportX + (int)(r.l * viewportW / 512), y = viewportY + (int)(r.b * viewportH / 384);
    int w = (int)((r.r - r.l) * viewportW / 512), h = (int)((r.t - r.b) * viewportH / 384);
    u8 *bytes = (u8 *)HeapAlloc(GetProcessHeap(), 0, w * h * 4);
    if (!bytes)
        return 0;
    glReadBuffer(GL_BACK);
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
    u32 result = 0;
    if (countPixels) {
        for (int i = 0; i < w * h; i++)
            if (bytes[i * 4] != 255 || bytes[i * 4 + 1] != 0 || bytes[i * 4 + 2] != 255)
                result++;
    } else
        result = hashBytes(bytes, w * h * 4);
    HeapFree(GetProcessHeap(), 0, bytes);
    return result;
}

void cloudRenderBegin() {
    glViewport(viewportX, viewportY, viewportW, viewportH);
    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 512, 0, 384, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void cloudVisualChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    startStage(0);
    deathTimer = 0;
    clearEffects();
    Entity moving, stationary, fragile, seesaw;
    initializeEntity(moving, *classById(6181314796472045350ULL), Rect{80, 260, 144, 292});
    initializeEntity(stationary, *classById(14843448166857854273ULL), Rect{224, 268, 288, 284});
    initializeEntity(fragile, *classById(14815823507569313204ULL), Rect{384, 267, 416, 285});
    initializeEntity(seesaw, *classById(17963251161137489403ULL), Rect{176, 142, 336, 158});
    seesaw.state.angle = 15;
    cloudRenderBegin();
    drawEntity(moving);
    drawEntity(stationary);
    drawEntity(fragile);
    drawEntity(seesaw);
    check(cloudRenderPixels(Rect{80, 260, 144, 292}, true) > 100,
          "Happy cloud platform renders its authored textured default frame");
    check(cloudRenderPixels(Rect{224, 268, 288, 284}, true) > 100,
          "Static one-way platform retains its authored artwork");
    check(cloudRenderPixels(Rect{384, 267, 416, 285}, true) > 100,
          "Intact fragile cloud renders the original on animation");
    check(cloudRenderPixels(Rect{168, 116, 344, 184}, true) > 100,
          "Rotating seesaw retains its authored default texture");
    SwapBuffers(gameDC);
    capture("test-output/cloud-platforms-intact.bmp");
    check(absf(crumblingAnimationTime(fragile, 0) - 0.41866666f) < 0.0001f,
          "Fragile cloud initial phase uses the native (position.y-position.x)/500 formula");
    cloudRenderBegin();
    drawEntity(fragile, &current, 0, 0, 0);
    u32 first = cloudRenderPixels(Rect{384, 267, 416, 285}, false);
    cloudRenderBegin();
    drawEntity(fragile, &current, 0, 0, 0.18f);
    u32 second = cloudRenderPixels(Rect{384, 267, 416, 285}, false);
    check(first != second, "Fragile cloud cycles through its authored on animation before contact");
    fragile.state.timer = 0;
    float timeAfterReturn = crumblingAnimationTime(fragile, 3.18f);
    check(absf(timeAfterReturn - crumblingAnimationTime(fragile, 3.18f + 2 / 3.0f)) < 0.0001f &&
              absf(timeAfterReturn - crumblingAnimationTime(fragile, 0)) > 0.01f,
          "Reappearing fragile cloud keeps advancing its animation while hidden");
    SwapBuffers(gameDC);
    capture("test-output/cloud-platform-animated.bmp");
    cloudRenderBegin();
    fragile.state.active = 2;
    drawEntity(fragile);
    check(cloudRenderPixels(Rect{384, 267, 416, 285}, true) == 0,
          "Burst fragile cloud hides its main actor instead of drawing a stretched burst");
    entityEffect(fragile, "CrumblingPlatformBehaviour", "effect");
    tickEffects(0.1f);
    drawEffects(0, fragile.layer);
    check(cloudRenderPixels(Rect{368, 244, 432, 308}, true) > 100,
          "Fragile cloud disappearance displays the separate original particle animation");
    SwapBuffers(gameDC);
    capture("test-output/cloud-platform-burst.bmp");
    clearEffects();
    // A real stage capture also covers the normal layer merge and camera path.
    startStage(5);
    int found = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->has("PlatformBehaviour")) {
            found = i;
            break;
        }
    if (found >= 0) {
        entities[playerIndex].state.x = entities[found].state.x;
        entities[playerIndex].state.y = entities[found].state.y + 40;
        updateCamera(true);
        captureCase("test-output/cloud-platform-stage-1-06.bmp");
    }
    // Exercise the real landing path and the native SetOff/OnUpdate/SetOn
    // lifecycle, rather than setting only the renderer's active flag.
    startStage(0);
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex)
            entities[i].state.gone = 1;
    current.geometryCount = 0;
    int index = entityCount;
    addEntity(*classById(14815823507569313204ULL), Rect{240, 180, 272, 198});
    Entity &cloud = entities[index];
    Entity &bean = entities[playerIndex];
    bean.state.x = 256;
    bean.state.y = hitRect(cloud).t - bean.hit.b + 2;
    bean.state.vx = 0;
    bean.state.vy = -60;
    rebuildPhysics();
    const float dt = 1 / 120.0f;
    int landing = -1;
    for (int step = 0; step < 30; step++) {
        tick(dt, false, false);
        if (cloud.state.active == 2) {
            landing = step;
            break;
        }
    }
    check(landing >= 0 && landing < 10 && !bodies[index]->IsActive() && liveEffects() > 0,
          "Landing immediately bursts the original fragile cloud and disables its fixture");
    bean.state.x = 48;
    bean.state.y = 48;
    restorePhysics();
    bodies[playerIndex]->SetType(b2_staticBody);
    for (int step = 0; step < 348; step++)
        tick(dt, false, false);
    check(cloud.state.active == 2 && !cloud.state.gone && !bodies[index]->IsActive(),
          "Gameplay fragile cloud remains hidden and recoverable during its three-second delay");
    for (int step = 0; step < 15; step++)
        tick(dt, false, false);
    check(cloud.state.active == 0 && !cloud.state.gone && bodies[index]->IsActive(),
          "Gameplay fragile cloud reappears with collision after its original delay");
    startStage(0);
}
