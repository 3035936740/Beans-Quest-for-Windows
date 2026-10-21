#pragma once

// Regression coverage for particle bounds native. Runs only in the isolated BQ_ENABLE_TESTS executable.
int particleInstanceLiveNative(u64 id, int instance, int domain = 0) {
    int n = 0;
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++) {
        EffectParticle &p = effectParticles[i];
        if (p.definition && p.domain == domain && p.art->id == id &&
            burstEmittersNative[domain].emitters[p.emitter].instance == instance)
            n++;
    }
    return n;
}

void particleBoundsNativeChecks() {
    bool oldInterface = interfaceEnabled, oldSound = soundEnabled, oldMusic = musicEnabled;
    interfaceEnabled = musicEnabled = soundEnabled = false;
    clearEffects(0);
    clearEffects(1);
    resetBurstRandomNative(0, 5489);
    resetBurstRandomNative(1, 17);
    u32 births = emittedParticles;
    check(nativeParticleRoundedPosition(.5f) == 1 && nativeParticleRoundedPosition(-.5f) == -1 &&
              nativeParticleRoundedPosition(.49f) == 0 && nativeParticleRoundedPosition(-.49f) == 0,
          "Native particle emitter centres round signed half positions away from zero");
    Rect b = nativeParticleEmitterBounds(Rect{240.25f, 176.75f, 272.25f, 208.75f}, .5f);
    check(
        b.l == 248 && b.r == 264 && b.b == 185 && b.t == 201,
        "Native half-scale emitter bounds use rounded position independently from scaled half-size");
    b = nativeParticleEmitterBounds(Rect{-32, -16, 32, 16}, .5f, 90);
    check(absf(b.l + 8) < .0001f && absf(b.r - 8) < .0001f && absf(b.b + 16) < .0001f &&
              absf(b.t - 16) < .0001f,
          "Non-square emitter bounds rotate both scaled half axes");
    b = nativeParticleEmitterBounds(Rect{-32, -16, 32, 16}, 1, 45);
    check(
        absf(b.r - 22.627417f) < .0001f && absf(b.t - 22.627417f) < .0001f,
        "Native rotated particle bounds take the largest axis contribution rather than a summed rectangle projection");
    Object *death = effectClass(7581709714868184ULL);
    const Props *fire = 0;
    for (int i = 0; i < death->behaviorCount; i++)
        if (eq(death->behaviors[i].props.get("animation"), "fire"))
            fire = &death->behaviors[i].props;
    check(
        fire && fire->flag("bounded") && fire->num("gravity-y") == 25 &&
            death->props.num("max-particles") == 15,
        "Original Wizard death fire requires current-emitter clipping inside its fifteen-slot effect");
    newEffectParticle(*death, *fire, 256, 192, 0, 1, 0);
    EffectParticle &p = effectParticles[0];
    p.initialX = 256;
    p.initialY = 192;
    p.vx = p.vy = p.gx = p.gy = 0;
    p.age = p.delay = 0;
    p.life = 10;
    EffectParticle born = p;
    Rect bounds = burstEmittersNative[0].emitters[p.emitter].bounds;
    bool equality = true;
    for (int edge = 0; edge < 4; edge++) {
        p = born;
        if (edge == 0)
            p.initialX = bounds.l;
        if (edge == 1)
            p.initialX = bounds.r;
        if (edge == 2)
            p.initialY = bounds.b;
        if (edge == 3)
            p.initialY = bounds.t;
        updateEffectParticleNative(p, 0);
        equality &= p.definition != 0;
    }
    check(
        equality,
        "Bounded particles retain all four exact emitter edges using inclusive native comparisons");
    bool outside = true;
    for (int edge = 0; edge < 4; edge++) {
        p = born;
        if (edge == 0)
            p.initialX = bounds.l - .001f;
        if (edge == 1)
            p.initialX = bounds.r + .001f;
        if (edge == 2)
            p.initialY = bounds.b - .001f;
        if (edge == 3)
            p.initialY = bounds.t + .001f;
        updateEffectParticleNative(p, 0);
        outside &= !p.definition;
    }
    check(outside, "Bounded particles retire immediately beyond any of the four emitter edges");
    p = born;
    p.initialY = bounds.t - 1;
    p.gy = 25;
    tickEffects(.5f);
    float deadAge = p.age;
    tickEffects(2);
    check(
        !p.definition && p.age == deadAge,
        "A bounded burst cannot resume updates or reappear after its trajectory leaves the emitter");
    p = born;
    p.delay = .5f;
    p.initialX = bounds.r + 1;
    tickEffects(.25f);
    bool delayed = p.definition != 0;
    tickEffects(.25f);
    check(delayed && !p.definition,
          "A delayed bounded particle defers clipping until its exact active-age zero frame");
    clearEffects(0);
    resetBurstRandomNative(0, 5489);
    emitEffect(death->id, 256.25f, 192.75f, 0);
    int wizard = -1;
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++)
        if (effectParticles[i].definition &&
            eq(effectParticles[i].definition->get("animation"), "wizard"))
            wizard = i;
    check(
        liveEffects(0) == 15 && wizard >= 0 && effectParticles[wizard].x == 256 &&
            effectParticles[wizard].y == 193,
        "The actual full Wizard death burst spawns its original wizard artwork at the native rounded centre");
    float oldX = effectParticles[wizard].x, oldY = effectParticles[wizard].y;
    u32 draws = hashBytes(burstRandomNative, sizeof(BurstRandomNative));
    emitEffect(death->id, 900, 193, 0);
    check(
        liveEffects(0) == 1 && effectParticles[wizard].definition &&
            effectParticles[wizard].x == oldX && effectParticles[wizard].y == oldY,
        "Moving an exhausted shared emitter clips old bounded fire but leaves its unbounded wizard particle at the original world position");
    check(
        hashBytes(burstRandomNative, sizeof(BurstRandomNative)) == draws,
        "Full-pool relocation evaluates native clipping without consuming spawn randomness or refilling deaths in that batch");
    emitEffect(death->id, 900, 193, 0);
    check(liveEffects(0) == 15,
          "The following explosion reuses slots released by the previous native clipping pass");
    clearEffects(0);
    resetBurstRandomNative(0, 5489);
    Object rotated = *effectClass(13492354840019264282ULL);
    rotated.angle = 90;
    Pair fields[] = {{"offset-min-x", "1"},    {"offset-max-x", "1"},   {"offset-min-y", "0.5"},
                     {"offset-max-y", "0.5"},  {"velocity-mean", "10"}, {"velocity-stddev", "0"},
                     {"theta-min", "0"},       {"theta-max", "0"},      {"start-angle-min", "90"},
                     {"start-angle-max", "90"}};
    Props rotatedDef = {10, fields};
    newEffectParticle(rotated, rotatedDef, 256, 192, 0, .5f, 0);
    check(
        absf(p.x - 256) < .0001f && absf(p.y - 200) < .0001f && absf(p.vx) < .0001f &&
            absf(p.vy - 5) < .0001f && absf(p.angle - 90) < .0001f,
        "Rotated burst births transform normalized offsets, launch velocity and initial sprite angle using the emitter rotation");
    clearEffects(0);
    resetBurstRandomNative(0, 5489);
    const u64 pre = 998576489116ULL;
    emitEffect(pre, 128, 192, 0, 1, 0, 1, 1);
    emitEffect(pre, 256, 192, 0, 1, 0, 1, 2);
    bool cache1 = false, cache2 = false;
    for (int i = 0; i < burstRandomNative[0].count; i++) {
        BurstDefinitionRandomNative &d = burstRandomNative[0].definitions[i];
        int instance = burstEmittersNative[0].emitters[d.emitter].instance;
        if (instance == 1)
            cache1 = d.lifetime.ready && d.velocity.ready;
        if (instance == 2)
            cache2 = d.lifetime.ready && d.velocity.ready;
    }
    check(
        cache1 && cache2,
        "Separate original prespawn instances retain separate cached lifetime and velocity normal partners");
    clearEffects(0);
    resetBurstRandomNative(0, 5489);
    for (int instance = 1; instance <= 3; instance++)
        emitEffect(pre, instance * 128.0f, 192, 0, 1, 0, 20, instance);
    check(
        liveEffects(0) == 60 && particleInstanceLiveNative(pre, 1) == 20 &&
            particleInstanceLiveNative(pre, 2) == 20 && particleInstanceLiveNative(pre, 3) == 20,
        "Three original twenty-particle Wizard prespawn instances each receive their full independent capacity");
    int first = -1;
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++)
        if (effectParticles[i].definition &&
            burstEmittersNative[0].emitters[effectParticles[i].emitter].instance == 2) {
            first = i;
            break;
        }
    EffectParticle retained = effectParticles[first];
    emitEffect(pre, 700, 192, 0, 1, 0, 20, 1);
    check(
        hashBytes(&retained, sizeof(retained)) ==
            hashBytes(effectParticles + first, sizeof(retained)),
        "Repositioning one Wizard warning emitter leaves another instance's living particles unchanged");
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    int bossIndex = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            bossIndex = i;
    Entity &boss = entities[bossIndex];
    boss.state.hp = 4;
    boss.state.active = 3;
    wizardMotion.shooting = 0;
    wizardMotion.cycleAge = .4f;
    wizardMotion.prespawnAge = 0;
    wizardMotion.pendingCount = 0;
    for (int i = 0; i < entityCount && wizardMotion.pendingCount < 3; i++)
        if (entities[i].bossSpawn == 1)
            wizardMotion.pending[wizardMotion.pendingCount++] = i;
    for (int frame = 0; frame < 60; frame++)
        wizardCombatCycle(boss, 1 / 60.0f);
    bool independent = wizardMotion.pendingCount == 3;
    for (int instance = 1; instance <= 3; instance++)
        independent &= particleInstanceLiveNative(pre, instance) >= 19 &&
                       particleInstanceLiveNative(pre, instance) <= 20;
    check(
        independent && liveEffects(0) >= 57,
        "The production Wizard combat caller distributes a whole warning interval to all three independent emitters without starving the later positions");
    emitEffect(pre, 256, 192, 0, 1, 1, 3, 1);
    wizardStopPrespawn(boss);
    check(
        liveEffects(0) == 0 && liveEffects(1) == 3,
        "Stopping Wizard warnings clears every campaign instance while retaining unrelated interface effects");
    clearEffects(0);
    clearEffects(1);
    emittedParticles = births;
    startStage(0);
    readyTimer = 0;
    interfaceEnabled = oldInterface;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
}
