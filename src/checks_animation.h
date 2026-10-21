#pragma once

// Regression coverage for animation. Runs only in the isolated BQ_ENABLE_TESTS executable.

void animationChecks() {
    Animation *diamond = animation(actor(314060601782828169ULL), "diamond 1");
    check(diamond && absf(animationLength(diamond) - 1.7f) < 0.00001f &&
              animationFrame(diamond, 0.99f) == 0 && animationFrame(diamond, 1.01f) == 1,
          "Original diamond preserves its ten-tick first frame and weighted animation length");
    check(animationFrame(diamond, 1.69f) == 7 && animationFrame(diamond, 1.7f) == 0 &&
              animationFrame(diamond, -0.01f) == 7,
          "Looping animation wraps at the native end time in either playback direction");
    Animation *hat = animation(actor(1018761474584413157ULL), "hat");
    check(hat && animationFrame(hat, 0.1f) == 1 && animationFrame(hat, 0.19f) == 1 &&
              animationFrame(hat, 0.21f) == 2,
          "Death hat animation respects the original longer second frame");
    check(animationFrame(diamond, 4, ANIMATION_STOP) == 7 &&
              animationFrame(diamond, -2, ANIMATION_STOP) == 0,
          "One-shot animation clamps to its last frame after completion");
    check(animationFrame(diamond, 0.505f, ANIMATION_STOP, 2) == 1 &&
              animationFrame(diamond, 0.01f, ANIMATION_STOP, 1, true) == 7,
          "Native playback speed and reverse restart choose the correct weighted frames");
    check(animationFrame(diamond, 1.95f, ANIMATION_PING_PONG) == 5 &&
              animationFrame(diamond, 3.4f, ANIMATION_PING_PONG) == 0,
          "Optional native ping-pong action reflects at each animation endpoint");
    // Check texture output at one-shot completion, where modulo playback used
    // to restart the animation instead of holding its final image.
    Actor *marker = 0;
    for (int i = 0; i < classCount; i++)
        if (classes[i].has("CheckpointBehaviour")) {
            marker = actor(classes[i].id);
            break;
        }
    Animation *save = animation(marker, "save");
    if (save && save->count > 1) {
        Rect r = {208, 120, 304, 264};
        cloudRenderBegin();
        drawActor(marker, r, "save", animationLength(save) + 1, 0, false, -1, 1, ANIMATION_STOP);
        u32 stopped = cloudRenderPixels(r, false);
        cloudRenderBegin();
        drawActor(marker, r, "save", 0, 0, false, save->count - 1);
        u32 last = cloudRenderPixels(r, false);
        check(stopped == last, "Rendered checkpoint one-shot holds its actual final texture");
        SwapBuffers(gameDC);
        capture("test-output/animation-checkpoint-complete.bmp");
    }
    Entity block;
    initializeEntity(block, *classById(12987116682223282109ULL), Rect{240, 176, 272, 208});
    cloudRenderBegin();
    drawEntity(block);
    u32 healthy = cloudRenderPixels(Rect{240, 176, 272, 208}, false);
    cloudRenderBegin();
    block.state.timer = 0.025f;
    drawEntity(block);
    u32 later = cloudRenderPixels(Rect{240, 176, 272, 208}, false);
    check(healthy == later,
          "Healthy breakable block holds its initial artwork instead of looping damage frames");
    cloudRenderBegin();
    block.state.hp--;
    drawEntity(block);
    u32 damaged = cloudRenderPixels(Rect{240, 176, 272, 208}, false);
    check(damaged != healthy,
          "Breakable block damage selects the native frozen life animation frame");
    SwapBuffers(gameDC);
    capture("test-output/animation-breakable-damaged.bmp");
    Entity bird;
    initializeEntity(bird, *classById(937947234821ULL), Rect{232, 168, 280, 216});
    cloudRenderBegin();
    drawEntity(bird);
    u32 sleeping = cloudRenderPixels(Rect{232, 168, 280, 216}, false);
    cloudRenderBegin();
    bird.state.active = 1;
    drawEntity(bird);
    u32 flying = cloudRenderPixels(Rect{232, 168, 280, 216}, false);
    cloudRenderBegin();
    bird.state.active = 2;
    drawEntity(bird);
    u32 manic = cloudRenderPixels(Rect{232, 168, 280, 216}, false);
    check(sleeping != flying && flying != manic && sleeping != manic,
          "Helper bird displays distinct original sleeping, flying and manic textures");
    cloudRenderBegin();
    bird.state.aux = -1;
    drawEntity(bird);
    u32 facedLeft = cloudRenderPixels(Rect{232, 168, 280, 216}, false);
    check(facedLeft != manic, "Helper bird flips the selected animation when facing left");
    Entity perch;
    initializeEntity(perch, *classById(678114862604ULL), Rect{232, 160, 280, 224});
    cloudRenderBegin();
    drawEntity(perch);
    u32 idlePerch = cloudRenderPixels(Rect{232, 160, 280, 224}, false);
    cloudRenderBegin();
    perch.state.active = 2;
    perch.state.timer = 0.2f;
    drawEntity(perch);
    u32 ringPerch = cloudRenderPixels(Rect{232, 160, 280, 224}, false);
    check(idlePerch != ringPerch, "Bird perch rings using its original one-shot animation");
    SwapBuffers(gameDC);
    capture("test-output/animation-perch-ringing.bmp");
    Entity bean;
    initializeEntity(bean, *classById(11120285848841938423ULL), Rect{240, 176, 272, 208});
    bean.lastLaunchSpeed = 240;
    Animation *jump = animation(bean.art, "jump");
    bean.state.vy = 120;
    int rising = animationFrame(jump, beanAnimationTime(bean), ANIMATION_STOP);
    bean.state.vy = 0;
    int apex = animationFrame(jump, beanAnimationTime(bean), ANIMATION_STOP);
    bean.state.vy = -120;
    int falling = animationFrame(jump, beanAnimationTime(bean), ANIMATION_STOP);
    check(rising == 0 && apex == 4 && falling == 8,
          "Bean jump artwork spans the original half-launch ascent, apex and half-launch descent");
    bean.lastLaunchSpeed = 600;
    bean.state.vy = 150;
    int springFrame = animationFrame(jump, beanAnimationTime(bean), ANIMATION_STOP);
    check(springFrame == 2, "Bean animation follows the stronger spring's actual launch speed");
}
