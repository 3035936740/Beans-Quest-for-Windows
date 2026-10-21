#pragma once

// Regression coverage for burst lifecycle native. Runs only in the isolated BQ_ENABLE_TESTS executable.
// Independent scalar/GL expectations transcribed from ParticleInstance::Update
// 0x2aef80. These checks also compile against the pre-repair implementation.
void burstLifecycleNativeChecks() {
    static BurstRandomNative retainedRandom[2];
    memcpy(retainedRandom, burstRandomNative, sizeof(retainedRandom));
    static BurstEmittersNative retainedEmitters[2];
    memcpy(retainedEmitters, burstEmittersNative, sizeof(retainedEmitters));
    clearEffects(0);
    clearEffects(1);
    u32 seed = effectSeed, births = emittedParticles;
    Object *source = effectClass(1018761474584413157ULL);
    const Props *hat = 0;
    if (source)
        for (int i = 0; i < source->behaviorCount; i++)
            if (eq(source->behaviors[i].type, "ParticleDefSettings") &&
                eq(source->behaviors[i].props.get("animation"), "hat"))
                hat = &source->behaviors[i].props;
    check(hat && absf(hat->num("start-time") - .33333f) < .000001f && hat->num("gravity-y") == -400,
          "Original Bean death hat retains its authored delayed gravitational launch");
    if (!hat)
        return;
    newEffectParticle(*source, *hat, 256, 192, 0, 1, 0);
    EffectParticle &p = effectParticles[0];
    EffectParticle born = p;
    tickEffects(.125f, 0);
    check(p.definition && p.age == .125f && p.x == born.x && p.y == born.y && p.angle == born.angle,
          "A delayed burst remains alive without advancing its trajectory before start time");
    p = born;
    p.delay = .25f;
    p.life = .5f;
    tickEffects(.25f, 0);
    check(p.definition && p.age == p.delay && p.x == born.x && p.y == born.y,
          "The exact delay boundary evaluates the initial particle pose");
    tickEffects(.5f, 0);
    check(
        p.definition && p.age - p.delay == p.life,
        "A burst stays alive at exact lifetime equality as in the native strict greater-than comparison");
    check(p.definition && absf(p.y - (born.y + .5f * (born.vy + .25f * born.gy))) < .0001f,
          "The terminal retained frame reaches its complete authored gravitational trajectory");
    tickEffects(.0001f, 0);
    check(!p.definition, "The update after the exact terminal frame releases the burst slot");
    clearEffects(0);
    newEffectParticle(*source, *hat, 256, 192, 0, 1, 0);
    p.delay = 0;
    p.life = 4;
    born = p;
    tickEffects(.75f, 0);
    EffectParticle single = p;
    p = born;
    for (int i = 0; i < 192; i++)
        tickEffects(1 / 256.0f, 0);
    check(
        p.age == single.age && p.x == single.x && p.y == single.y && p.angle == single.angle,
        "Original hat position and rotation agree exactly across one update and 192 equal-total-time updates");
    check(p.vx == born.vx && p.vy == born.vy,
          "Burst launch velocity remains immutable like the native ParticleInstance birth state");
    p = born;
    p.x += 300;
    p.y -= 100;
    p.angle += 40;
    tickEffects(0, 0);
    check(
        p.x == born.x && p.y == born.y && p.angle == born.angle,
        "A zero-time native evaluation rebuilds position and rotation from the stored birth state");
    p = born;
    p.delay = .25f;
    tickEffects(1, 0);
    float t = .75f;
    float expectedX = born.x + t * (born.vx + .5f * born.gx * t),
          expectedY = born.y + t * (born.vy + .5f * born.gy * t),
          expectedAngle = born.angle + born.spin * t;
    char trajectory[256];
    wsprintfA(trajectory, "Delayed pose absolute errors in millionths: x=%d y=%d angle=%d\r\n",
              (int)(absf(p.x - expectedX) * 1000000), (int)(absf(p.y - expectedY) * 1000000),
              (int)(absf(p.angle - expectedAngle) * 1000000));
    atomicWrite("test-output/burst-lifecycle-trajectory.txt", trajectory, lstrlenA(trajectory));
    check(absf(p.x - expectedX) < .0001f && absf(p.y - expectedY) < .0001f &&
              absf(p.angle - expectedAngle) < .0001f,
          "Crossing a delayed start samples only elapsed active age in the analytic trajectory");
    clearEffects(0);
    clearEffects(1);
    newEffectParticle(*source, *hat, 256, 192, 0, 1, 0);
    newEffectParticle(*source, *hat, 256, 192, 0, 1, 1);
    EffectParticle other = effectParticles[1];
    tickEffects(.125f, 0);
    check(hashBytes(&other, sizeof(other)) == hashBytes(effectParticles + 1, sizeof(other)),
          "Campaign burst updates leave interface particle birth state and clocks unchanged");
    // Original artwork with controlled legal fade parameters isolates native
    // branch precedence and short fade intervals from random lifetime sampling.
    clearEffects(0);
    clearEffects(1);
    newEffectParticle(*source, *hat, 256, 192, 0, 1, 1);
    p = effectParticles[0];
    p.delay = 0;
    p.life = 1;
    p.age = .5f;
    p.fadeIn = .75f;
    p.fadeOut = .25f;
    p.alpha = .9f;
    p.angle = 0;
    cloudRenderBegin();
    drawEffects(1, 0);
    u32 overlap = cloudRenderPixels(Rect{224, 160, 288, 224}, false);
    cloudRenderBegin();
    drawActor(p.art, Rect{p.x - p.w * .5f, p.y - p.h * .5f, p.x + p.w * .5f, p.y + p.h * .5f},
              "hat",
              particleAnimationTime(p.art, "hat", .5f, 1, p.animationStart, p.animationScale), 0,
              false, -1, .6f);
    u32 expected = cloudRenderPixels(Rect{224, 160, 288, 224}, false);
    check(overlap == expected,
          "Actual OpenGL burst alpha gives fade-in precedence when fade ranges overlap");
    p.age = .75f;
    p.fadeOut = 1;
    cloudRenderBegin();
    drawEffects(1, 0);
    u32 boundary = cloudRenderPixels(Rect{224, 160, 288, 224}, false);
    cloudRenderBegin();
    drawActor(p.art, Rect{p.x - p.w * .5f, p.y - p.h * .5f, p.x + p.w * .5f, p.y + p.h * .5f},
              "hat", 0, 0, false, -1, .9f);
    expected = cloudRenderPixels(Rect{224, 160, 288, 224}, false);
    check(
        boundary == expected,
        "Fade-in equality renders full authored opacity instead of retaining the preceding branch");
    p.age = .99609375f;
    p.fadeIn = 0;
    p.fadeOut = .9921875f;
    cloudRenderBegin();
    drawEffects(1, 0);
    u32 shortFade = cloudRenderPixels(Rect{224, 160, 288, 224}, false);
    cloudRenderBegin();
    drawActor(p.art, Rect{p.x - p.w * .5f, p.y - p.h * .5f, p.x + p.w * .5f, p.y + p.h * .5f},
              "hat", 0, 0, false, -1, .45f);
    expected = cloudRenderPixels(Rect{224, 160, 288, 224}, false);
    check(
        shortFade == expected,
        "Actual OpenGL short fade-out uses the complete authored interval without a one-percent denominator clamp");
    p.age = 1;
    cloudRenderBegin();
    drawEffects(1, 0);
    u32 terminal = cloudRenderPixels(Rect{224, 160, 288, 224}, false);
    cloudRenderBegin();
    u32 blank = cloudRenderPixels(Rect{224, 160, 288, 224}, false);
    check(terminal == blank && p.definition,
          "A fully faded terminal frame remains allocated but contributes no pixels");
    p.delay = .25f;
    p.age = .125f;
    cloudRenderBegin();
    drawEffects(1, 0);
    u32 hidden = cloudRenderPixels(Rect{224, 160, 288, 224}, false);
    check(hidden == blank,
          "The real renderer hides particles throughout their authored start delay");
    clearEffects(0);
    clearEffects(1);
    effectSeed = seed;
    emittedParticles = births;
    memcpy(burstRandomNative, retainedRandom, sizeof(retainedRandom));
    memcpy(burstEmittersNative, retainedEmitters, sizeof(retainedEmitters));
}
