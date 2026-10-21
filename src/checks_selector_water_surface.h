#pragma once

// Regression coverage for selector water surface. Runs only in the isolated BQ_ENABLE_TESTS executable.

double selectorWavePixelCentre(Object &wave, Rect rect, float time) {
    nativeViewportProjection();
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    drawParticles(wave, actor(wave.klass), shifted(rect, -300, 0), time, 0.5f);
    glFinish();
    u8 *pixels = (u8 *)HeapAlloc(GetProcessHeap(), 0, viewportW * viewportH * 4);
    if (!pixels)
        return -10000;
    glReadBuffer(GL_BACK);
    glReadPixels(viewportX, viewportY, viewportW, viewportH, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    double weighted = 0, total = 0;
    for (int y = 0; y < viewportH; y++)
        for (int x = 0; x < viewportW; x++) {
            u8 *p = pixels + (y * viewportW + x) * 4;
            int weight = p[0] + p[1] + p[2];
            weighted += (y + 0.5) * weight;
            total += weight;
        }
    HeapFree(GetProcessHeap(), 0, pixels);
    return total ? weighted / total / screenScale : -10000;
}

void selectorWaterSurfaceChecks() {
    bool oldInterface = interfaceEnabled;
    interfaceEnabled = true;
    musicEnabled = soundEnabled = false;
    navigate(799282380888ULL);
    UiNode *wave = 0;
    Object *water = 0;
    for (int i = 0; i < page.level.objectCount; i++) {
        if (page.nodes[i].object->id == 534211538701571ULL)
            wave = page.nodes + i;
        if (page.level.objects[i].id == 5677084289919524ULL)
            water = page.level.objects + i;
    }
    check(wave && water,
          "Second chapter playable selector retains the original water and wave identities");
    if (!wave || !water) {
        interfaceEnabled = oldInterface;
        return;
    }
    Object &source = *wave->object;
    u32 objectsBefore = hashBytes(page.level.objects, sizeof(Object) * page.level.objectCount),
        geometryBefore =
            hashBytes(page.level.geometry, sizeof(Geometry) * page.level.geometryCount);
    Rect original = source.rect, repaired = visualRect(page, *wave);
    Rect bounds = nativeParticleEmitterBounds(original, 0.5f);
    const Props &p = *source.behavior("ParticleDefSettings");
    float mean =
        bounds.b + (bounds.t - bounds.b) * (p.num("offset-min-y") + p.num("offset-max-y")) * 0.5f;
    check(
        absf(mean - 128.2f) < 0.001f &&
            absf(mean + repaired.b - original.b - water->rect.t) < 0.001f &&
            repaired.l == original.l && repaired.r == original.r && repaired.t - repaired.b == 24,
        "Actual selector drawing aligns the wave mean to WaterArea height127 while retaining wave width and vertical distribution");
    int changed = 0;
    for (int i = 0; i < page.level.objectCount; i++) {
        Object &o = page.level.objects[i];
        Rect r = selectorWaterSurfaceVisualRect(page.level, o, o.rect);
        if (r.l != o.rect.l || r.b != o.rect.b || r.r != o.rect.r || r.t != o.rect.t)
            changed++;
    }
    check(
        changed == 1 &&
            objectsBefore ==
                hashBytes(page.level.objects, sizeof(Object) * page.level.objectCount) &&
            geometryBefore ==
                hashBytes(page.level.geometry, sizeof(Geometry) * page.level.geometryCount),
        "Water alignment translates only one rendered emitter and leaves authored objects, sensor and terrain data unchanged");
    Level other = page.level;
    other.id = 669776786229ULL;
    Rect untouched = selectorWaterSurfaceVisualRect(other, source, original);
    Object altered = source;
    altered.rect.t += 1;
    Rect unrecognized = selectorWaterSurfaceVisualRect(page.level, altered, original);
    check(
        untouched.b == original.b && unrecognized.b == original.b,
        "Water alignment requires the exact affected scene and emitter bounds rather than shifting unrelated water effects");
    RECT saved;
    GetWindowRect(window, &saved);
    bool visible = true, aligned = true;
    const int sizes[][2] = {{512, 384}, {1024, 768}, {1536, 1152}};
    for (int test = 0; test < 3; test++) {
        resizeWindow(sizes[test][0], sizes[test][1]);
        nativeContinuousResetScene(page.level);
        float time = 0;
        for (int step = 0; step < 240; step++) {
            time += 1.0f / 120;
            nativeContinuousAdvanceScene(page.level, 0.5f, time, 1.0f / 120);
        }
        double before = selectorWavePixelCentre(source, original, time),
               after = selectorWavePixelCentre(source, repaired, time);
        visible &= before > 120 && before < 135 && after > 120 && after < 135;
        aligned &= after < before - 0.7 && after > before - 1.7 && after > water->rect.t - 0.5 &&
                   after < water->rect.t + 0.5;
    }
    check(
        visible && aligned,
        "Actual OpenGL wave pixels align with the physical water surface at three window scales without a detached particle band");
    SetWindowPos(window, 0, saved.left, saved.top, saved.right - saved.left,
                 saved.bottom - saved.top, SWP_NOZORDER | SWP_NOACTIVATE);
    nativeViewportProjection();
    page.age = 2;
    page.minimized = true;
    page.minimizePhase = 1;
    transitionTime = 0;
    captureCase("test-output/selector-water-surface-aligned.bmp");
    interfaceEnabled = oldInterface;
}
