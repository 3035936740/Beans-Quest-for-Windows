#pragma once

// Regression coverage for water sound native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int waterVoiceNative() {
    const char *names[] = {"water1", "water2", "water3"};
    for (int i = 0; i < 32; i++)
        if (audioVoices[i].clip)
            for (int j = 0; j < soundClipCount; j++)
                if (audioVoices[i].clip == &soundClips[j].clip)
                    for (int k = 0; k < 3; k++)
                        if (eq(soundClips[j].name, names[k]))
                            return k + 1;
    return 0;
}

u32 waterParticleHashNative(int domain) {
    u32 h = 0;
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++)
        if (effectParticles[i].definition && effectParticles[i].domain == domain)
            h = h * 33 + hashBytes(effectParticles + i, sizeof(EffectParticle));
    return h;
}

void waterSoundNativeChecks() {
    interfaceEnabled = debugMode = musicEnabled = false;
    soundEnabled = true;
    stopMusic();
    stopSounds();
    int wi = -1;
    for (int stage = 0; stage < stageCount && wi < 0; stage++) {
        startStage(stage);
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == WATER) {
                wi = i;
                break;
            }
    }
    check(wi >= 0, "Water sound comparison uses an actual original campaign water entity");
    if (wi < 0)
        return;
    Entity &water = entities[wi];
    b2Body &body = *bodies[playerIndex];
    body.SetLinearVelocity(b2Vec2(0, -200 * metres));
    Entity empty = water;
    Object source = *water.source;
    Behavior behavior = {};
    Pair field[] = {{"splash-class", "0"}};
    behavior.type = "WaterAreaBehaviour";
    behavior.props = Props{1, field};
    source.behaviorCount = 1;
    source.behaviors = &behavior;
    empty.source = &source;
    clearEffects(0);
    clearEffects(1);
    resetBurstRandomNative(0, 5489);
    // Canonical MT19937 seed5489 words, divided into the three native buckets.
    const int expected[] = {3, 1, 3, 3, 1, 3, 3, 1, 2, 1};
    bool sequence = true;
    for (int i = 0; i < 10; i++) {
        stopSounds();
        waterSplash(empty, body);
        sequence &= waterVoiceNative() == expected[i];
    }
    check(
        sequence && burstRandomNative[0].engine.index == 10,
        "Splash variants follow the native MT seed5489 reference rather than a repeating water1-water2-water3 cycle");
    const u32 words[] = {0, 0x55555554u, 0x55555555u, 0xaaaaaaa9u, 0xaaaaaaaau, 0xfffffffeu};
    const int choices[] = {1, 1, 2, 2, 3, 3};
    bool buckets = true;
    for (int i = 0; i < 6; i++) {
        particleScriptWordsNative(burstRandomNative[0].engine, words[i], 0);
        stopSounds();
        waterSplash(empty, body);
        buckets &= waterVoiceNative() == choices[i] && burstRandomNative[0].engine.index == 1;
    }
    check(
        buckets,
        "Splash integer sampling uses the exact native unsigned bucket boundaries instead of modulo or floating conversion");
    particleScriptWordsNative(burstRandomNative[0].engine, 0xffffffffu, 0x55555555u);
    stopSounds();
    waterSplash(empty, body);
    check(waterVoiceNative() == 2 && burstRandomNative[0].engine.index == 2,
          "The native splash sampler rejects UINT32_MAX and consumes the following MT word");
    resetBurstRandomNative(0, 5489);
    stopSounds();
    body.SetLinearVelocity(b2Vec2(0, 99 * metres));
    u32 before = hashBytes(&burstRandomNative[0].engine, sizeof(NativeParticleRng));
    waterSplash(water, body);
    check(before == hashBytes(&burstRandomNative[0].engine, sizeof(NativeParticleRng)) &&
              activeAudioVoices() == 0 && liveEffects(0) == 0,
          "Water contact below100px/s consumes no sound random word and emits no voice or splash");
    bool directions = true;
    const float speeds[] = {100, -100};
    for (int i = 0; i < 2; i++) {
        resetBurstRandomNative(0, 5489);
        stopSounds();
        body.SetLinearVelocity(b2Vec2(0, speeds[i] * metres));
        waterSplash(empty, body);
        directions &= waterVoiceNative() == 3 && burstRandomNative[0].engine.index == 1;
    }
    check(
        directions,
        "Both entry and exit at exactly100px/s select a native water variant at normal playback rate");
    body.SetLinearVelocity(b2Vec2(0, -200 * metres));
    const Props &props = *water.source->behavior("WaterAreaBehaviour");
    u64 splash = idnum(props.get("splash-class"));
    clearEffects(0);
    resetBurstRandomNative(0, 5489);
    nativeParticleBits(burstRandomNative[0].engine);
    emitEffect(splash, body.GetPosition().x / metres, entityRect(water).t, water.layer, 1, 0, 16);
    u32 referenceParticles = waterParticleHashNative(0),
        referenceRandom = hashBytes(&burstRandomNative[0].engine, sizeof(NativeParticleRng));
    int referenceCount = liveEffects(0);
    clearEffects(0);
    resetBurstRandomNative(0, 5489);
    stopSounds();
    waterSplash(water, body);
    check(
        referenceCount == 16 && waterVoiceNative() == 3 &&
            waterParticleHashNative(0) == referenceParticles &&
            hashBytes(&burstRandomNative[0].engine, sizeof(NativeParticleRng)) == referenceRandom,
        "Actual splash particles use the stream after sound selection, preserving native sound-before-Explode draw order");
    clearEffects(0);
    resetBurstRandomNative(0, 5489);
    stopSounds();
    soundEnabled = false;
    waterSplash(water, body);
    check(
        activeAudioVoices() == 0 && waterParticleHashNative(0) == referenceParticles &&
            hashBytes(&burstRandomNative[0].engine, sizeof(NativeParticleRng)) == referenceRandom,
        "Disabling sound still consumes the native splash choice and preserves the same visible splash sample");
    soundEnabled = true;
    resetBurstRandomNative(0, 5489);
    resetBurstRandomNative(1, 5489);
    u32 campaign = hashBytes(burstRandomNative, sizeof(BurstRandomNative));
    stopSounds();
    waterSplash(empty, body, 1);
    check(
        waterVoiceNative() == 3 && burstRandomNative[1].engine.index == 1 &&
            hashBytes(burstRandomNative, sizeof(BurstRandomNative)) == campaign,
        "Playable-selector splashes consume their own scene stream without advancing campaign sound or particle randomness");
    stopSounds();
    waterSplash(empty, body, 0);
    captureCheckpoint(false);
    waterSplash(empty, body, 0);
    u32 retained = hashBytes(&burstRandomNative[0].engine, sizeof(NativeParticleRng));
    restoreCheckpoint();
    bool kept = retained == hashBytes(&burstRandomNative[0].engine, sizeof(NativeParticleRng));
    restartStage();
    check(
        kept && retained == hashBytes(&burstRandomNative[0].engine, sizeof(NativeParticleRng)),
        "Checkpoint and reset-state loading retain the current splash random stream instead of replaying saved sound choices");
    clearEffects(0);
    clearEffects(1);
    stopSounds();
    startStage(0);
    soundEnabled = false;
    readyTimer = 0;
}
