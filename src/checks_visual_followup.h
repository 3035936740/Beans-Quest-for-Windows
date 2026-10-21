#pragma once

// Regression coverage for visual followup. Runs only in the isolated BQ_ENABLE_TESTS executable.

Rect visualFootprint(Rect region) {
    int x = viewportX + (int)(region.l * viewportW / 512),
        y = viewportY + (int)(region.b * viewportH / 384),
        w = (int)((region.r - region.l) * viewportW / 512),
        h = (int)((region.t - region.b) * viewportH / 384);
    u8 *bytes = (u8 *)HeapAlloc(GetProcessHeap(), 0, w * h * 4);
    Rect result = {10000, 10000, -10000, -10000};
    if (!bytes)
        return result;
    glReadBuffer(GL_BACK);
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
    for (int py = 0; py < h; py++)
        for (int px = 0; px < w; px++) {
            u8 *p = bytes + (py * w + px) * 4;
            if (p[0] || p[1] || p[2]) {
                float sx = region.l + px * 512.0f / viewportW,
                      sy = region.b + py * 384.0f / viewportH;
                result.l = minf(result.l, sx);
                result.r = maxf(result.r, sx);
                result.b = minf(result.b, sy);
                result.t = maxf(result.t, sy);
            }
        }
    HeapFree(GetProcessHeap(), 0, bytes);
    return result;
}

void visualFollowupChecks() {
    Actor *waves = actor(528389847068ULL);
    Animation *sparkle = animation(waves, "sparkle");
    float phaseTime = particleAnimationTime(waves, "sparkle", 0.25f, 1, 0.5f, 1);
    check(animationFrame(sparkle, phaseTime) == 24,
          "Water sparkle samples its original normalized lifetime and randomized start fraction");
    check(absf(phaseTime - particleAnimationTime(waves, "sparkle", 0.5f, 2, 0.5f, 1)) < 0.00001f,
          "Particle animation is invariant when age and lifetime are scaled together");
    check(animationFrame(sparkle, particleAnimationTime(waves, "sparkle", 0.5f, 1, 0.75f, 0)) == 24,
          "Zero animation speed preserves the authored random starting frame");
    Object source = *classById(528389847068ULL);
    Pair props[64];
    props[0] = Pair{(char *)"max-particles", (char *)"1"};
    for (int i = 0; i < source.props.count; i++)
        props[i + 1] = source.props.pairs[i];
    source.props.pairs = props;
    source.props.count++;
    cloudRenderBegin();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    drawParticles(source, waves, Rect{256, 192, 256, 192}, 0.4f);
    Rect footprint = visualFootprint(Rect{224, 160, 288, 224});
    check(footprint.r > footprint.l && footprint.r - footprint.l > 3 * (footprint.t - footprint.b),
          "Actual OpenGL water surface particle is a horizontal ripple instead of a vertical bar");
    SwapBuffers(gameDC);
    capture("test-output/visual-water-ripple.bmp");
    Entity bean, magnet;
    initializeEntity(bean, *classById(11120285848841938423ULL), Rect{240, 176, 272, 208});
    initializeEntity(magnet, *classById(670936219271ULL), Rect{240, 144, 272, 176});
    cloudRenderBegin();
    bean.state.active = 1;
    drawEntity(bean);
    u32 jumping = cloudRenderPixels(Rect{240, 176, 272, 208}, false);
    cloudRenderBegin();
    bean.state.active = 2;
    bean.state.timer = 0.2f;
    drawEntity(bean);
    u32 attached = cloudRenderPixels(Rect{240, 176, 272, 208}, false);
    check(attached != jumping,
          "Attached Bean renders the original magnet animation instead of its jump pose");
    cloudRenderBegin();
    drawEntity(magnet, &current, 0, 0, 0.2f);
    u32 idle = cloudRenderPixels(Rect{240, 144, 272, 176}, false);
    cloudRenderBegin();
    magnet.state.active = 2;
    drawEntity(magnet, &current, 0, 0, 0.2f);
    u32 active = cloudRenderPixels(Rect{240, 144, 272, 176}, false);
    check(active != idle, "Contact magnet cell displays its original active animation");
    cloudRenderBegin();
    drawEntity(magnet, &current, 0, 0, 0.35f);
    u32 animated = cloudRenderPixels(Rect{240, 144, 272, 176}, false);
    check(animated != active, "Active magnet animation advances with the scene clock");
    SwapBuffers(gameDC);
    capture("test-output/visual-magnet-active.bmp");
    glBindTexture(GL_TEXTURE_2D, waves->textures[0].gl);
    GLint mag = 0, min = 0, mipWidth = 0;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &mag);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &min);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 7, GL_TEXTURE_WIDTH, &mipWidth);
    check(mag == GL_LINEAR && min == GL_LINEAR_MIPMAP_LINEAR && mipWidth == 1,
          "Original water atlas keeps its linear filtering and complete eight-level mip chain");
    Object bird = *classById(697074819893ULL);
    Pair birdProps[64];
    birdProps[0] = Pair{(char *)"max-particles", (char *)"1"};
    for (int i = 0; i < bird.props.count; i++)
        birdProps[i + 1] = bird.props.pairs[i];
    bird.props.pairs = birdProps;
    bird.props.count++;
    NativeContinuousPool &birdPool = nativeContinuousFor(bird, Rect{0, 0, 0, 0}, 1, 1.4f);
    NativeContinuousParticle *birdParticle = 0;
    for (int i = 0; i < birdPool.capacity; i++)
        if (birdPool.particles[i].definition >= 0)
            birdParticle = &birdPool.particles[i];
    check(birdParticle != 0,
          "Original background bird pool contains an emitted live actor for pose validation");
    if (birdParticle) {
        const NativeContinuousDef &d = birdPool.definitions[birdParticle->definition];
        float age = birdParticle->age - d.delay;
        Vec delta = nativeContinuousPosition(*birdParticle, d);
        float phase = age / birdParticle->life, fade = 1;
        if (d.fadeIn > 0 && phase < d.fadeIn)
            fade = phase / d.fadeIn;
        else if (d.fadeOut < 1 && phase > d.fadeOut)
            fade = maxf(0, (1 - phase) / maxf(0.00001f, 1 - d.fadeOut));
        cloudRenderBegin();
        drawParticles(bird, actor(bird.klass),
                      Rect{256 - delta.x, 192 - delta.y, 256 - delta.x, 192 - delta.y}, 1.4f);
        u32 birdActual = cloudRenderPixels(Rect{224, 160, 288, 224}, false);
        cloudRenderBegin();
        float width = birdParticle->size.x, height = birdParticle->size.y;
        drawActor(
            actor(bird.klass),
            Rect{256 - width * 0.5f, 192 - height * 0.5f, 256 + width * 0.5f, 192 + height * 0.5f},
            "Bird",
            particleAnimationTime(actor(bird.klass), "Bird", age, birdParticle->life,
                                  birdParticle->animationStart, d.animationScale),
            0, false, -1, fade * d.alpha);
        u32 birdUpright = cloudRenderPixels(Rect{224, 160, 288, 224}, false);
        check(
            absf(birdParticle->angle + birdParticle->spin * age - 90) < 0.001f &&
                birdActual == birdUpright,
            "Actual randomized background goose retains native mode1 upright horizontal sprite orientation");
        SwapBuffers(gameDC);
        capture("test-output/visual-background-goose.bmp");
    }
    const u64 stages[] = {789619714186ULL, 7138745870511843ULL, 976871008727ULL};
    const char *captures[] = {"test-output/visual-stage-5-11-background.bmp",
                              "test-output/visual-stage-5-12-background.bmp",
                              "test-output/visual-wizard-background.bmp"};
    for (int n = 0; n < 3; n++) {
        startStage(stageById(stages[n]));
        elapsed = 6;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].source->base == 5)
                entities[i].state.timer = 6;
        readyTimer = 0;
        updateCamera(true);
        captureCase(captures[n]);
    }
}
