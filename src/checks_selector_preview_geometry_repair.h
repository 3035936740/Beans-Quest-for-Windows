#pragma once

// Regression coverage for selector preview geometry repair. Runs only in the isolated BQ_ENABLE_TESTS executable.
#include "selector_preview_geometry_repair.h"

void selectorPreviewGeometryPrepare(bool repaired) {
    navigate(worldScenes[4]);
    page.minimized = true;
    page.minimizePhase = 1;
    page.miniDeath = 0;
    page.miniCoins = 0;
    transitionTime = 0;
    destroyUiPhysics(page);
    // Restore the exact shipped loop for the explicit before/after baseline.
    page.level.geometry[0].count = 5;
    if (repaired)
        repairFifthSelectorGeometry(page.level);
}

u32 selectorPreviewGeometryPixels() {
    glViewport(viewportX, viewportY, viewportW, viewportH);
    nativeViewportProjection();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawOriginalScene(page);
    glFinish();
    int size = viewportW * viewportH * 4;
    u8 *pixels = (u8 *)HeapAlloc(GetProcessHeap(), 0, size);
    glReadBuffer(GL_BACK);
    glReadPixels(viewportX, viewportY, viewportW, viewportH, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    u32 hash = hashBytes(pixels, size);
    HeapFree(GetProcessHeap(), 0, pixels);
    return hash;
}

void selectorPreviewGeometryRepairChecks() {
    char oldSaveRoot[MAX_PATH];
    lstrcpyA(oldSaveRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/selector-geometry-repair-saves");
    CreateDirectoryA(saveRoot, 0);
    Progress oldProgress = progress;
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldMusic = musicEnabled,
         oldSound = soundEnabled, oldKeys[256];
    memcpy(oldKeys, keys, sizeof(keys));
    interfaceEnabled = true;
    debugMode = true;
    musicEnabled = soundEnabled = false;
    memset(keys, 0, sizeof(keys));
    const float dt = 1.0f / 120;
    navigate(worldScenes[4]);
    check(
        page.level.geometry[0].count == 0,
        "The production World5 selector loader removes its internal obstruction before constructing the independent preview physics world");
    selectorPreviewGeometryPrepare(false);
    u32 originalPixels = selectorPreviewGeometryPixels();
    int geometryCount = page.level.geometryCount;
    Geometry column = page.level.geometry[2], cap = page.level.geometry[5];
    bool applied = repairFifthSelectorGeometry(page.level);
    u32 fixedPixels = selectorPreviewGeometryPixels();
    check(
        applied && page.level.id == 669776786229ULL && page.level.geometry[0].count == 0 &&
            page.level.geometryCount == geometryCount &&
            page.level.geometry[2].count == column.count &&
            page.level.geometry[5].count == cap.count && originalPixels == fixedPixels,
        "The exact World5 internal16x16 loop is disabled while the real column, one-way cap and full original OpenGL image remain unchanged");
    page.level.geometry[0].count = 5;
    u64 id = page.level.id;
    page.level.id = worldScenes[3];
    bool other = repairFifthSelectorGeometry(page.level);
    page.level.id = id;
    page.level.geometry[0].points[0].x += 1;
    bool mismatch = repairFifthSelectorGeometry(page.level);
    page.level.geometry[0].points[0].x -= 1;
    bool guarded = repairFifthSelectorGeometry(page.level);
    check(
        !other && !mismatch && guarded && !repairFifthSelectorGeometry(page.level),
        "The repair requires the exact scene, loop coordinates and matching cap and is safe to call repeatedly");
    float sideX[2] = {}, peak[2] = {};
    int collected[2] = {}, headHits[2] = {};
    char trace[1200] =
        "mode,side_final_x1000,coins,isolated_ceiling_peak1000,isolated_ceiling_hits\r\n";
    for (int mode = 0; mode < 2; mode++) {
        selectorPreviewGeometryPrepare(mode != 0);
        page.bean.state.x = 432;
        page.bean.state.y = 85;
        page.bean.state.vx = page.bean.state.vy = 0;
        createUiPhysics(page);
        keys[VK_LEFT] = true;
        for (int step = 0; step < 180; step++)
            tickInterface(dt);
        memset(keys, 0, sizeof(keys));
        sideX[mode] = page.bean.state.x;
        collected[mode] = page.miniCoins;
        selectorPreviewGeometryPrepare(mode != 0);
        page.bean.state.x = 408;
        page.bean.state.y = 70;
        page.bean.state.vx = 0;
        page.bean.state.vy = page.bean.jumpPower;
        createUiPhysics(page);
        // The unchanged magnet occupies64..80 immediately below this loop.
        // Make only that adjacent fixture a sensor in this isolated ceiling
        // experiment so the loop's underside can be measured independently.
        for (int i = 0; i < page.playfieldCount; i++)
            if (page.bodies[i] && page.playfield[i].source->id == 8759011704404663ULL)
                for (b2Fixture *f = page.bodies[i]->GetFixtureList(); f; f = f->GetNext())
                    f->SetSensor(true);
        for (int step = 0; step < 120; step++) {
            tickInterface(dt);
            peak[mode] = maxf(peak[mode], page.bean.state.y);
            b2Body *bean = page.bodies[page.playfieldCount];
            for (b2ContactEdge *edge = bean->GetContactList(); edge; edge = edge->next) {
                b2Contact *c = edge->contact;
                if (!c->IsTouching() || !c->IsEnabled() || !c->GetManifold()->pointCount ||
                    c->GetFixtureA()->IsSensor() || c->GetFixtureB()->IsSensor())
                    continue;
                b2WorldManifold m;
                c->GetWorldManifold(&m);
                if ((c->GetFixtureA()->GetBody() == bean ? -m.normal.y : m.normal.y) < -.9f)
                    headHits[mode]++;
            }
        }
        char row[220];
        wsprintfA(row, "%d,%d,%d,%d,%d\r\n", mode, (int)(sideX[mode] * 1000), collected[mode],
                  (int)(peak[mode] * 1000), headHits[mode]);
        lstrcatA(trace, row);
    }
    atomicWrite("test-output/selector-preview-geometry-repair.csv", trace, lstrlenA(trace));
    check(
        sideX[0] > 420 && sideX[0] < 422 && collected[0] == 0 && sideX[1] > 388 && sideX[1] < 390 &&
            collected[1] == 2,
        "Actual selector movement reproduces the old side wall and unreachable coins, then collects both coins and stops only at the intact visible column after repair");
    check(
        peak[0] > 73 && peak[0] < 76 && headHits[0] > 0 && peak[1] > 105 && headHits[1] == 0,
        "With only the adjacent magnet isolated, the shipped loop causes a real underside head hit while the repaired miniature Bean completes its authored jump through that area");
    selectorPreviewGeometryPrepare(true);
    page.bean.state.x = 400;
    page.bean.state.y = 140;
    page.bean.state.vx = 0;
    page.bean.state.vy = -20;
    createUiPhysics(page);
    bool capLanding = false;
    for (int step = 0; step < 100; step++) {
        float vy = page.bean.state.vy;
        tickInterface(dt);
        if (page.ordinaryJumpNative && page.bean.state.vy > 100 &&
            absf(hitRect(page.bean).b - (112 + page.bean.state.vy * dt)) < 2)
            capLanding = true;
    }
    check(
        capLanding,
        "The original floating cap still supports and bounces a genuine downward landing after its internal obstruction is removed");
    startStage(0);
    readyTimer = 0;
    lstrcpyA(saveRoot, oldSaveRoot);
    progress = oldProgress;
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    musicEnabled = oldMusic;
    soundEnabled = oldSound;
    memcpy(keys, oldKeys, sizeof(keys));
}
