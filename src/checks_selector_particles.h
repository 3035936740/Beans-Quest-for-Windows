#pragma once

// Regression coverage for selector particles. Runs only in the isolated BQ_ENABLE_TESTS executable.

void selectorParticleBoundsChecks() {
    interfaceEnabled = true;
    musicEnabled = soundEnabled = false;
    navigate(799282380888ULL);
    Object *bubble = 0;
    Object *wave = 0;
    Object *water = 0;
    for (int i = 0; i < page.level.objectCount; i++) {
        Object &o = page.level.objects[i];
        if (o.id == 9735771281483297ULL)
            bubble = &o;
        if (o.id == 534211538701571ULL)
            wave = &o;
        if (o.id == 5677084289919524ULL)
            water = &o;
    }
    check(bubble && wave && water,
          "Original underwater selector retains its authored bubble, wave, and water objects");
    if (!bubble || !wave || !water)
        return;
    Rect b = nativeParticleEmitterBounds(bubble->rect, 0.5f),
         w = nativeParticleEmitterBounds(wave->rect, 0.5f);
    const Props &def = bubble->behaviors[0].props;
    check(
        b.l == 448 && b.b == 0 && b.r == 560 && b.t == 112 && b.t < water->rect.t,
        "Native selector half-scale places bubble emitter top112 below authored water top127 without a water-surface clipping rule");
    check(w.l == 436 && w.b == 118 && w.r == 572 && w.t == 130 && w.r - w.l == 136 &&
              water->rect.r - water->rect.l == 144,
          "Native half-scale wave emitter width136 matches its authored 144-pixel water region");
    check(def.flag("bounded") && !bubble->props.flag("bounded") &&
              nativeParticleInsideBounds(Vec{504, 112}, b, true) &&
              !nativeParticleInsideBounds(Vec{504, 112.01f}, b, true),
          "Particle bounded flag belongs to the definition and uses inclusive emitter bounds");
    Vec sample = nativeParticleTrajectory(b, 0.5f, 0, Vec{0, 1}, Vec{0, 15}, 2, 0.5f);
    check(
        sample.x == 504 && sample.y == 16,
        "Native half-scale affects upward velocity and acceleration as well as emitter dimensions");
    Rect full = nativeParticleEmitterBounds(bubble->rect, 1);
    check(full.l == 392 && full.b == -56 && full.r == 616 && full.t == 168,
          "Normal stage particle bounds keep scale one rather than inheriting the selector scale");
    bool valid = true;
    int visible = 0;
    for (int i = 0; i <= 600; i++) {
        float age = i / 100.0f;
        Vec p = nativeParticleTrajectory(b, 0.5f, 0.05f, Vec{0, 1}, Vec{0, 15}, age, 0.5f);
        if (nativeParticleInsideBounds(p, b, true)) {
            visible++;
            valid &= p.y <= 112;
        }
    }
    check(
        valid && visible > 0 && visible < 601,
        "Rising bounded bubbles die at their own native emitter limit instead of continuing into air");
    // Isolate the real bubble atlas on black and inspect actual OpenGL pixels.
    // A 4px half-scaled particle may extend 2px beyond its centre boundary.
    glDisable(GL_SCISSOR_TEST);
    glViewport(viewportX, viewportY, viewportW, viewportH);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 512, 0, 384, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_BLEND);
    drawParticles(*bubble, actor(bubble->klass), shifted(bubble->rect, -300, 0), 8, 0.5f);
    glFlush();
    u32 size = viewportW * viewportH * 4;
    u8 *pixels = (u8 *)HeapAlloc(GetProcessHeap(), 0, size);
    if (pixels) {
        glReadBuffer(GL_BACK);
        glReadPixels(viewportX, viewportY, viewportW, viewportH, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        int below = 0, above = 0;
        for (int y = 0; y < viewportH; y++)
            for (int x = 0; x < viewportW; x++) {
                u8 *p = pixels + (y * viewportW + x) * 4;
                if (p[0] || p[1] || p[2]) {
                    if ((float)y * 384 / viewportH > 114)
                        above++;
                    else
                        below++;
                }
            }
        check(
            below > 0 && above == 0,
            "Actual bubble atlas renders inside native scaled bounds and leaves the air above entirely empty");
        HeapFree(GetProcessHeap(), 0, pixels);
    } else
        check(false, "Bubble GL readback memory is available");
    page.age = 8;
    page.minimized = true;
    page.minimizePhase = 1;
    transitionTime = 0;
    captureCase("test-output/selector-native-bubble-bounds.bmp");
}
