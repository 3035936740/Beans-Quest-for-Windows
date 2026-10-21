#pragma once

// Regression coverage for background native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void backgroundNativeChecks() {
    Object emission = *classById(528389847068ULL);
    NativeContinuousPool *pool = (NativeContinuousPool *)HeapAlloc(
        GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NativeContinuousPool));
    nativeContinuousConfigure(*pool, emission);
    nativeContinuousStep(*pool, Rect{-68, -6, 68, 6}, 0.5f, 0, 0.001f);
    check(
        pool->births == 1 && pool->budget < 0,
        "Native positive emission budget creates the first water particle immediately then debits its interval");
    for (int i = 0; i < 600; i++)
        nativeContinuousStep(*pool, Rect{-68, -6, 68, 6}, 0.5f, 0, 1 / 60.0f);
    int live = 0;
    for (int i = 0; i < pool->capacity; i++)
        if (pool->particles[i].definition >= 0)
            live++;
    check(
        live > 80 && live < 220 && pool->births > 1000 && pool->deaths > 700,
        "Original Gaussian water lifetimes recycle pool slots instead of filling all300 or drawing fixed ages");
    HeapFree(GetProcessHeap(), 0, pool->particles);
    HeapFree(GetProcessHeap(), 0, pool);
    check(absf(backgroundWrappedAxis(-959, 256, 512, 960, false) - 961) < 0.0001f,
          "Background wraps native actor centres through a 1920-unit cloud period");
    check(absf(backgroundWrappedAxis(1216, 256, 512, 960, false) + 704) < 0.0001f,
          "Background upper wrap boundary returns to the lower native boundary");
    check(absf(backgroundWrappedAxis(-2624, 256, 512, 960, false) - 1216) < 0.0001f,
          "Negative exact-period wrapping retains the native signed-fmod upper boundary");
    check(absf(backgroundWrappedAxis(1100, 256, 512, 960, false) - 1100) < 0.0001f,
          "Cloud wrap does not add the sprite width or viewport when use-camera-x is disabled");
    check(absf(backgroundWrappedAxis(1536, 256, 512, 1024, true) + 1024) < 0.0001f,
          "Camera-relative background wrap includes the original half viewport");
    startStage(stageById(899263731133ULL));
    readyTimer = 0;
    deathTimer = 0;
    clearEffects();
    int topClouds = 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->klass == 159837608269ULL && entities[i].originY > 300)
            topClouds++;
    check(topClouds == 4,
          "Real Skysawz stage contains the four authored upside-down sky-cover clouds");
    const float ages[] = {0, 80, 1000};
    bool covered = true;
    for (int t = 0; t < 3; t++) {
        cloudRenderBegin();
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        Vec shift = layerTranslation(current, 3, 0, 0);
        glPushMatrix();
        glTranslatef(shift.x, shift.y, 0);
        for (int i = 0; i < entityCount; i++)
            if (entities[i].source->klass == 159837608269ULL && entities[i].originY > 300)
                drawEntity(entities[i], &current, 0, 0, ages[t]);
        glPopMatrix();
        Rect footprint = visualFootprint(Rect{0, 366, 512, 380});
        if (footprint.l > 1 || footprint.r < 510)
            covered = false;
        // Probe every column, including the two ends and each cloud join.
        for (int x = 1; x < 512; x += 8) {
            u8 pixel[4] = {};
            glReadBuffer(GL_BACK);
            glReadPixels(viewportX + (int)(x * viewportW / 512.0f),
                         viewportY + (int)(374 * viewportH / 384.0f), 1, 1, GL_RGBA,
                         GL_UNSIGNED_BYTE, pixel);
            if (pixel[0] + pixel[1] + pixel[2] < 20)
                covered = false;
        }
        if (t == 1) {
            SwapBuffers(gameDC);
            capture("test-output/background-native-cloud-loop.bmp");
        }
    }
    check(
        covered,
        "Actual upside-down cloud artwork covers the black sky gap across multiple native wrap cycles");
    elapsed = 80;
    camX = camY = 0;
    captureCase("test-output/background-native-stage-3-07.bmp");
}
