#pragma once

// Regression coverage for particle distribution native. Runs only in the isolated BQ_ENABLE_TESTS executable.
u32 particleUntemperNative(u32 value) {
    u32 x = value;
    for (int i = 0; i < 6; i++)
        x = value ^ (x >> 18);
    value = x;
    for (int i = 0; i < 6; i++)
        x = value ^ ((x << 15) & 0xefc60000u);
    value = x;
    for (int i = 0; i < 6; i++)
        x = value ^ ((x << 7) & 0x9d2c5680u);
    value = x;
    for (int i = 0; i < 6; i++)
        x = value ^ (x >> 11);
    return x;
}

void particleScriptWordsNative(NativeParticleRng &r, u32 a, u32 b) {
    memset(&r, 0, sizeof(r));
    r.state[0] = particleUntemperNative(a);
    r.state[1] = particleUntemperNative(b);
}

void particleDistributionNativeChecks() {
    static BurstRandomNative retained[2];
    memcpy(retained, burstRandomNative, sizeof(retained));
    static BurstEmittersNative retainedEmitters[2];
    memcpy(retainedEmitters, burstEmittersNative, sizeof(retainedEmitters));
    u32 oldBirths = emittedParticles;
    clearEffects(0);
    clearEffects(1);
    NativeParticleRng r;
    particleScriptWordsNative(r, 0x81, 0);
    float small = nativeParticleUniform(r);
    check(
        small == 129.0f / 4294967296.0f && r.index == 1,
        "Particle uniform conversion keeps low-word precision instead of discarding the lower eight bits");
    particleScriptWordsNative(r, 0xffffffffu, 0x80000000u);
    check(nativeParticleUniform(r) == .5f && r.index == 2,
          "A uint32 word rounding to uniform one is rejected before the next particle sample");
    nativeParticleSeed(r, 5489);
    check(nativeParticleRange(r, 3, 3) == 3 && r.index == 624,
          "Equal particle range endpoints consume no random words");
    particleScriptWordsNative(r, 0xc0000000u, 0x40000000u);
    float adjacent = 1.00000011920928955078125f;
    check(nativeParticleRange(r, 1, adjacent) == 1 && r.index == 2,
          "A mapped binary32 upper endpoint is retried even when the source uniform was below one");
    particleScriptWordsNative(r, 0x40000000u, 0);
    check(nativeParticleRange(r, 4, 0) == 1 && r.index == 1,
          "Reversed particle limits are sorted before native half-open sampling");
    NativeParticleNormal normal = {};
    nativeParticleSeed(r, 5489);
    float first = nativeParticleGaussian(r, normal), second = nativeParticleGaussian(r, normal);
    // Independent Python binary32/Box-Muller seed5489 reference retained in build.
    check(
        absf(first - .2134360671f) < .00001f && absf(second - (-.495580256f)) < .00001f &&
            r.index == 2 && !normal.ready,
        "Native normal first cosine and cached sine match independent seed5489 reference values without an extra draw");
    Object *source = effectClass(13492354840019264282ULL);
    const Props *splash = source && source->behaviorCount ? &source->behaviors[0].props : 0;
    check(
        splash && absf(splash->num("depth-max") - .95f) < .000001f &&
            splash->num("velocity-mean") == 170,
        "Original water splash definition requests depth zero to0.95 and Gaussian speed170 plus30");
    if (!splash)
        return;
    resetBurstRandomNative(0, 5489);
    float sum = 0;
    int n = 6000;
    for (int i = 0; i < n; i++) {
        newEffectParticle(*source, *splash, 256, 192, 0, 1, 0);
        EffectParticle &p = effectParticles[0];
        sum += root(p.vx * p.vx + p.vy * p.vy);
        p.definition = 0;
    }
    float splashMean = sum / n;
    check(
        splashMean > 135 && splashMean < 142,
        "Actual authored water splash planar speed averages the native depth-scaled138.67 instead of the former unscaled170");
    Pair fields[] = {{"lifetime-mean", "1"},
                     {"lifetime-stddev", "0.1"},
                     {"velocity-mean", "100"},
                     {"velocity-stddev", "20"},
                     {"depth-min", "0.8"},
                     {"depth-max", "0.8"},
                     {"theta-min", "0"},
                     {"theta-max", "0"},
                     {"animation-start-min", "0"},
                     {"animation-start-max", "0"},
                     {"start-angle-min", "0"},
                     {"start-angle-max", "0"},
                     {"angular-velocity-min", "0"},
                     {"angular-velocity-max", "0"}};
    Props p = {14, fields};
    resetBurstRandomNative(0, 5489);
    float velocity = 0, velocitySquares = 0, life = 0, lifeSquares = 0;
    int tails = 0;
    for (int i = 0; i < 10000; i++) {
        newEffectParticle(*source, p, 256, 192, 0, 1, 0);
        EffectParticle &q = effectParticles[0];
        velocity += q.vx;
        velocitySquares += q.vx * q.vx;
        life += q.life;
        lifeSquares += q.life * q.life;
        if (absf(q.vx - 60) > 12)
            tails++;
        q.definition = 0;
    }
    float vm = velocity / 10000, variance = velocitySquares / 10000 - vm * vm, lm = life / 10000,
          lifeVariance = lifeSquares / 10000 - lm * lm;
    check(
        absf(vm - 60) < .6f && absf(variance - 144) < 9 && tails > 2900 && tails < 3450,
        "Burst velocity has native Gaussian variance and tails after fixed depth0.8 projects its speed by0.6");
    check(
        absf(lm - 1) < .004f && absf(lifeVariance - .01f) < .0006f,
        "Burst lifetime has its authored Gaussian variance rather than uniform one-third variance");
    resetBurstRandomNative(0, 5489);
    newEffectParticle(*source, p, 256, 192, 0, 1, 0);
    effectParticles[0].definition = 0;
    BurstRandomNative &random = burstRandomNative[0];
    bool cached = random.engine.index == 4 && random.definitions[0].lifetime.ready &&
                  random.definitions[0].velocity.ready;
    newEffectParticle(*source, p, 256, 192, 0, 1, 0);
    effectParticles[0].definition = 0;
    check(
        cached && random.engine.index == 4 && !random.definitions[0].lifetime.ready &&
            !random.definitions[0].velocity.ready,
        "Lifetime and velocity keep separate definition caches and the second birth uses both sine partners without new MT words");
    fields[0].value = (char *)"0";
    fields[1].value = (char *)"1";
    fields[2].value = (char *)"0";
    fields[3].value = (char *)"1";
    resetBurstRandomNative(0, 5489);
    int negativeLife = 0, negativeVelocity = 0, retired = 0;
    for (int i = 0; i < 1000; i++) {
        newEffectParticle(*source, p, 256, 192, 0, 1, 0);
        EffectParticle &q = effectParticles[0];
        if (q.life < 0) {
            negativeLife++;
            updateEffectParticleNative(q, 0);
            if (!q.definition)
                retired++;
        }
        if (q.vx < 0)
            negativeVelocity++;
        q.definition = 0;
    }
    check(
        negativeLife > 450 && negativeLife < 550 && retired == negativeLife,
        "Negative Gaussian lifetimes are preserved and retire on the native zero-time evaluation instead of an invented minimum lifetime");
    check(
        negativeVelocity > 450 && negativeVelocity < 550,
        "Negative Gaussian launch speeds remain signed instead of being clamped to stationary particles");
    resetBurstRandomNative(0, 5489);
    resetBurstRandomNative(1, 17);
    u32 other = hashBytes(burstRandomNative + 1, sizeof(BurstRandomNative));
    newEffectParticle(*source, p, 256, 192, 0, 1, 0);
    effectParticles[0].definition = 0;
    check(
        hashBytes(burstRandomNative + 1, sizeof(BurstRandomNative)) == other,
        "Campaign particle sampling leaves interface MT state and all cached normal partners untouched");
    u32 live = hashBytes(burstRandomNative, sizeof(BurstRandomNative));
    clearEffects(0);
    check(
        hashBytes(burstRandomNative, sizeof(BurstRandomNative)) == live,
        "Clearing active particles does not reseed the live scene stream or discard definition normal caches");
    Props emptyDefinition = {};
    resetBurstRandomNative(0, 5489);
    bool sizes = true;
    float angleMin = 900, angleMax = -900;
    for (int i = 0; i < 128; i++) {
        newEffectParticle(*source, emptyDefinition, 256, 192, 0, 1, 0);
        EffectParticle &q = effectParticles[0];
        sizes &= q.w == 16 && q.h == 16;
        angleMin = minf(angleMin, q.angle);
        angleMax = maxf(angleMax, q.angle);
        q.definition = 0;
    }
    check(
        sizes && angleMin < -80 && angleMax > 250,
        "Burst definitions without angle or size properties use native size16 and full zero-to360 initial rotation limits");
    Pair offsetField[] = {{"offset-min-x", "0.25"}};
    Props offsetDefinition = {1, offsetField};
    resetBurstRandomNative(0, 5489);
    float left = 900, right = -900;
    bool inside = true;
    for (int i = 0; i < 128; i++) {
        newEffectParticle(*source, offsetDefinition, 256, 192, 0, 1, 0);
        EffectParticle &q = effectParticles[0];
        inside &= q.x >= 248 && q.x <= 256;
        left = minf(left, q.x);
        right = maxf(right, q.x);
        q.definition = 0;
    }
    check(
        inside && left < 250 && right > 254,
        "An authored offset minimum does not override a missing maximum's independent native0.5 fallback");
    // Explode runs Update(0) after its whole batch, rather than refilling a slot
    // immediately after a sampled negative lifetime.
    Object *fire = effectClass(7073760897930888ULL);
    Props original = fire->behaviors[0].props;
    Pair negativeFields[] = {{"lifetime-mean", "-1"},
                             {"lifetime-stddev", "0"},
                             {"explosion-min-spawn", "1"},
                             {"spawn-priority", "0"}};
    fire->behaviors[0].props = Props{4, negativeFields};
    clearEffects(0);
    u32 births = emittedParticles;
    emitEffect(fire->id, 256, 192, 0);
    fire->behaviors[0].props = original;
    check(
        emittedParticles == births + 1 && liveEffects(0) == 0,
        "A real burst performs its post-batch zero-time update and does not refill a negative-lifetime particle");
    NativeContinuousPool *pool = (NativeContinuousPool *)HeapAlloc(
        GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NativeContinuousPool));
    Behavior behavior = {};
    behavior.type = "ParticleDefSettings";
    Object empty = {};
    empty.id = 12345;
    empty.behaviorCount = 1;
    empty.behaviors = &behavior;
    nativeContinuousConfigure(*pool, empty);
    check(pool->definitions[0].lifeMean == 1 && pool->definitions[0].lifeStd == .1f &&
              pool->definitions[0].velocityMean == 1 && pool->definitions[0].velocityStd == .1f &&
              pool->definitions[0].angleHi == 360,
          "Continuous definitions use native constructor mean1 and standard deviation0.1 defaults");
    HeapFree(GetProcessHeap(), 0, pool->particles);
    HeapFree(GetProcessHeap(), 0, pool);
    char report[400];
    wsprintfA(
        report,
        "samples=10000 splash_samples=%d splash_mean_milli=%d velocity_mean_milli=%d velocity_variance_milli=%d velocity_tails=%d lifetime_mean_million=%d lifetime_variance_million=%d negative_lifetime=%d negative_velocity=%d\r\n",
        n, (int)(splashMean * 1000), (int)(vm * 1000), (int)(variance * 1000), tails,
        (int)(lm * 1000000), (int)(lifeVariance * 1000000), negativeLife, negativeVelocity);
    atomicWrite("test-output/particle-distribution-statistics.txt", report, lstrlenA(report));
    clearEffects(0);
    clearEffects(1);
    memcpy(burstRandomNative, retained, sizeof(retained));
    memcpy(burstEmittersNative, retainedEmitters, sizeof(retainedEmitters));
    emittedParticles = oldBirths;
}
