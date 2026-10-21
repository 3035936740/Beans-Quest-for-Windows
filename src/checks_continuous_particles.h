#pragma once

// Regression coverage for continuous particles. Runs only in the isolated BQ_ENABLE_TESTS executable.

void continuousParticleNativeChecks() {
    NativeContinuousPool *pool = (NativeContinuousPool *)HeapAlloc(
        GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NativeContinuousPool));
    nativeParticleSeed(pool->random, 5489);
    check(nativeParticleBits(pool->random) == 3499211612u &&
              nativeParticleBits(pool->random) == 581869302u,
          "Particle MT19937 matches the original generator's standard reference outputs");
    NativeParticleNormal normal = {};
    float sum = 0, squares = 0;
    for (int i = 0; i < 10000; i++) {
        float n = nativeParticleGaussian(pool->random, normal);
        sum += n;
        squares += n * n;
    }
    check(absf(sum / 10000) < 0.04f && absf(squares / 10000 - 1) < 0.05f,
          "Native cached Box Muller sampling supplies Gaussian lifetime and velocity variation");
    Pair props[] = {{"particles-per-second", "10"}, {"max-particles", "1"}};
    Pair fields[] = {{"lifetime-mean", "0.2"}, {"lifetime-stddev", "0"}, {"velocity-mean", "0"},
                     {"velocity-stddev", "0"}, {"offset-x", "0.5"},      {"offset-y", "0.5"}};
    Behavior def = {};
    def.type = "ParticleDefSettings";
    def.props = Props{6, fields};
    Object emitter = {};
    emitter.id = 123;
    emitter.props = Props{2, props};
    emitter.behaviorCount = 1;
    emitter.behaviors = &def;
    Rect bounds = {-2, -2, 2, 2};
    nativeContinuousConfigure(*pool, emitter);
    nativeContinuousStep(*pool, bounds, 1, 0, 0.001f);
    check(
        pool->births == 1 && absf(pool->particles[0].age - 0.001f) < 0.00001f,
        "Original pool initializes a new particle with accumulated emission age on its first positive tick");
    nativeContinuousStep(*pool, bounds, 1, 0, 0.15f);
    check(
        pool->births == 1 && pool->budget <= 0 && pool->budget >= -0.1f,
        "Saturated native pool drops overdue births rather than accumulating an artificial later burst");
    nativeContinuousStep(*pool, bounds, 1, 0, 0.1f);
    check(pool->births == 2 && pool->deaths == 1 && pool->particles[0].generation == 2,
          "Native pool reuses a dead slot in the same update pass");
    NativeContinuousParticle &particle = pool->particles[0];
    NativeContinuousDef &definition = pool->definitions[0];
    definition.bounded = true;
    particle.initial = Vec{0, 0};
    particle.velocity = Vec{0, 10};
    particle.gravity = Vec{0, -10};
    particle.age = 0.5f;
    particle.life = 10;
    bool died = !nativeContinuousAlive(*pool, particle, bounds);
    particle.age = 2;
    check(
        died && particle.definition < 0 && !nativeContinuousAlive(*pool, particle, bounds),
        "Bounded native particles remain dead after leaving the emitter even if their parabola would reenter");
    definition.bounded = false;
    particle.definition = 0;
    particle.age = 0.2f;
    particle.life = 0.2f;
    bool equality = nativeContinuousAlive(*pool, particle, bounds);
    particle.age = 0.2001f;
    check(
        equality && !nativeContinuousAlive(*pool, particle, bounds),
        "Native particle lifetime expires strictly after the authored duration rather than on its exact boundary");
    HeapFree(GetProcessHeap(), 0, pool->particles);
    HeapFree(GetProcessHeap(), 0, pool);
    navigate(799282380888ULL);
    Object *wave = 0;
    for (int i = 0; i < page.level.objectCount; i++)
        if (page.level.objects[i].id == 534211538701571ULL)
            wave = &page.level.objects[i];
    check(wave != 0, "Real underwater selector wave source is available for native spawn checks");
    if (!wave)
        return;
    nativeContinuousResetScene(page.level);
    for (int tick = 1; tick <= 120; tick++)
        nativeContinuousAdvanceScene(page.level, 0.5f, tick / 60.0f, 1 / 60.0f);
    Rect w = nativeParticleEmitterBounds(wave->rect, 0.5f);
    Vec origin = center(w);
    Rect local = shifted(w, -origin.x, -origin.y);
    NativeContinuousPool &actual = nativeContinuousFor(*wave, local, 0.5f, 2);
    bool positions = true, geometry = true, variation = false;
    float firstLife = 0;
    int live = 0;
    for (int i = 0; i < actual.capacity; i++) {
        NativeContinuousParticle &p = actual.particles[i];
        if (p.definition < 0)
            continue;
        live++;
        positions &= p.initial.y + origin.y >= 127 && p.initial.y + origin.y <= 129.401f &&
                     p.initial.x + origin.x >= 436 && p.initial.x + origin.x <= 572;
        geometry &= p.size.x == 2 && p.size.y == 7;
        if (firstLife == 0)
            firstLife = p.life;
        else
            variation |= absf(firstLife - p.life) > 0.01f;
    }
    check(
        live > 0 && positions,
        "Actual half scale waves spawn in native offset range127 to129.4 with no synthetic water clipping or Y correction");
    check(geometry && variation,
          "Actual mode1 waves retain horizontal7 by2 geometry and randomized lifetimes");
    u32 births = actual.births;
    float age = actual.particles[0].age;
    NativeContinuousPool &repeated = nativeContinuousFor(*wave, local, 0.5f, 2);
    check(
        repeated.births == births && repeated.particles[0].age == age,
        "Repeated rendering at the same scene clock cannot create or age additional continuous particles");
    check(actual.births >= 295 && actual.births <= 302 && actual.deaths > 50,
          "Offscreen scene ticks advance continuous emitters before their first rendering");
    nativeContinuousAdvanceScene(page.level, 0.5f, 2, 1 / 60.0f);
    check(actual.births == births && actual.particles[0].age == age,
          "Duplicate scene tick clock cannot emit or age particles twice");
}
