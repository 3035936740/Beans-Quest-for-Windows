#pragma once

// Regression coverage for particle burst native. Runs only in the isolated BQ_ENABLE_TESTS executable.
int burstClassLiveNative(u64 id, int domain) {
    int n = 0;
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++) {
        const EffectParticle &p = effectParticles[i];
        if (p.definition && p.domain == domain && p.art && p.art->id == id)
            n++;
    }
    return n;
}

void particleBurstNativeChecks() {
    static BurstRandomNative retainedRandom[2];
    memcpy(retainedRandom, burstRandomNative, sizeof(retainedRandom));
    static BurstEmittersNative retainedEmitters[2];
    memcpy(retainedEmitters, burstEmittersNative, sizeof(retainedEmitters));
    clearEffects(0);
    clearEffects(1);
    u32 seed = effectSeed, emitted = emittedParticles;
    const u64 fire = 7073760897930888ULL;
    Object *source = effectClass(fire);
    check(source && source->base == 5 && source->props.num("max-particles") == 10 &&
              source->props.num("explosion-size") == 1,
          "Original fireball death effect has a shared ten-particle pool");
    for (int i = 0; i < 10; i++)
        emitEffect(fire, 100 + i * 20.0f, 160, i & 1, 1, 0);
    check(burstClassLiveNative(fire, 0) == 10,
          "Repeated fireball deaths fill one shared pool across rendering layers");
    tickEffects(.05f, 0);
    u32 full = hashBytes(effectParticles, sizeof(effectParticles)),
        rng = hashBytes(burstRandomNative, sizeof(burstRandomNative)), births = emittedParticles;
    for (int i = 0; i < 20; i++)
        emitEffect(fire, 800, 900, 7, 1, 0);
    check(
        hashBytes(effectParticles, sizeof(effectParticles)) == full &&
            hashBytes(burstRandomNative, sizeof(burstRandomNative)) == rng &&
            emittedParticles == births,
        "A full burst pool drops overflow without replacing moving particles or consuming random draws");
    for (int i = 0; i < 12; i++)
        emitEffect(fire, 200, 200, 0, 1, 1, 20);
    check(burstClassLiveNative(fire, 0) == 10 && burstClassLiveNative(fire, 1) == 10,
          "Campaign and interface keep independent burst capacities");
    u64 other = 0;
    for (int i = 0; i < classCount; i++)
        if (classes[i].base == 5 && classes[i].id != fire &&
            classes[i].props.num("explosion-size", 1) > 0) {
            for (int n = 0; n < classes[i].behaviorCount; n++)
                if (eq(classes[i].behaviors[n].type, "ParticleDefSettings") &&
                    classes[i].behaviors[n].props.num("explosion-min-spawn") > 0) {
                    other = classes[i].id;
                    break;
                }
            if (other)
                break;
        }
    if (other)
        emitEffect(other, 300, 200, 0, 1, 0);
    u32 otherBirths = emittedParticles - births - 10;
    check(other && otherBirths > 0 && burstClassLiveNative(fire, 0) == 10,
          "An exhausted effect class does not suppress another class");
    int retired = 0;
    for (int i = 0; i < MAX_EFFECT_PARTICLES && retired < 3; i++) {
        EffectParticle &p = effectParticles[i];
        if (p.definition && p.domain == 0 && p.art->id == fire) {
            p.life = p.age + .01f;
            p.delay = 0;
            retired++;
        }
    }
    tickEffects(.02f, 0);
    check(burstClassLiveNative(fire, 0) == 7,
          "Burst lifetime expiry releases only the expired slots");
    emitEffect(fire, 400, 200, 3, 1, 0, 9);
    check(
        burstClassLiveNative(fire, 0) == 8,
        "Zero random spawn priority preserves the authored one-particle minimum even with a larger requested burst");
    for (int i = 0; i < 6; i++)
        emitEffect(fire, 400, 200, 3, 1, 0, 9);
    check(burstClassLiveNative(fire, 0) == 10 && emittedParticles == births + 10 + otherBirths + 3,
          "A partial burst pool accepts exactly its available capacity");
    clearEffects(0);
    for (int i = 0; i < 10; i++)
        emitEffect(fire, 500, 200, 0, 1, 0, 30);
    check(burstClassLiveNative(fire, 0) == 10 && burstClassLiveNative(fire, 1) == 10,
          "Clearing a scene releases its entire burst capacity without clearing the other scene");
    clearEffects(0);
    clearEffects(1);
    effectSeed = seed;
    emittedParticles = emitted;
    memcpy(burstRandomNative, retainedRandom, sizeof(retainedRandom));
    memcpy(burstEmittersNative, retainedEmitters, sizeof(retainedEmitters));
}
