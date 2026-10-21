#pragma once

// Regression coverage for frog spin native. Runs only in the isolated BQ_ENABLE_TESTS executable.

u32 frogSpinValueBitsNative(float value) {
    u32 bits;
    memcpy(&bits, &value, 4);
    return bits;
}

void frogSpinNativeChecks() {
    interfaceEnabled = musicEnabled = soundEnabled = debugMode = false;
    startStage(stageById(5770908237787719ULL));
    readyTimer = 0;
    bool original = frogLinkCount == 1;
    int index = original ? frogLinks[0].ball : -1;
    check(
        original,
        "Original Moonlight Oasis resolves its actual cooperative frog ball for angular-speed checks");
    if (index < 0)
        return;
    Entity &ball = entities[index];
    const u32 expected[] = {0x40bdd63c, 0xc0dbe006, 0x40f4c498, 0x40ca128a};
    Vec from = {100, 200}, to = {292, 200};
    resetBurstRandomNative(0, 5489);
    bool sequence = true, trajectory = true;
    for (int i = 0; i < 4; i++) {
        ball.state.timer = .25f;
        bool ok = frogBallistic(ball, from, to, 2);
        sequence &= ok && frogSpinValueBitsNative(ball.state.aux) == expected[i];
        trajectory &=
            ok && absf(ball.state.vx - 256) < .00001f && absf(ball.state.vy - 187.5f) < .00001f;
    }
    check(
        sequence && burstRandomNative[0].engine.index == 4,
        "Campaign frog spin matches four independent seed5489 binary32 MT vectors over native minus3pi to plus3pi radians/sec");
    check(
        trajectory,
        "Correct angular sampling preserves the original remaining-half-period translational trajectory");
    resetBurstRandomNative(0, 5489);
    particleScriptWordsNative(burstRandomNative[0].engine, 0, 0xffffff00u);
    ball.state.timer = .25f;
    frogBallistic(ball, from, to, 2);
    bool low = frogSpinValueBitsNative(ball.state.aux) == 0xc116cbe4u;
    frogBallistic(ball, from, to, 2);
    bool high = frogSpinValueBitsNative(ball.state.aux) == 0x4116cbe2u;
    check(
        low && high && burstRandomNative[0].engine.index == 2,
        "Actual frog calculation includes the native negative endpoint and reaches above3radians/sec near the excluded positive3pi endpoint");
    resetBurstRandomNative(0, 5489);
    particleScriptWordsNative(burstRandomNative[0].engine, 0xffffffffu, 0x80000000u);
    frogBallistic(ball, from, to, 2);
    bool retry = ball.state.aux == 0 && burstRandomNative[0].engine.index == 2;
    particleScriptWordsNative(burstRandomNative[0].engine, 129, 0);
    frogBallistic(ball, from, to, 2);
    check(
        retry && frogSpinValueBitsNative(ball.state.aux) == 0xc116cbe3u &&
            burstRandomNative[0].engine.index == 1,
        "Frog spin retries a rounded upper endpoint and retains the original low-word precision instead of a24bit cosmetic draw");
    resetBurstRandomNative(0, 5489);
    resetBurstRandomNative(1, 19);
    u32 other = hashBytes(&burstRandomNative[1].engine, sizeof(NativeParticleRng)),
        oldEffect = effectSeed, source = hashBytes(&sourceRandom, sizeof(sourceRandom));
    frogBallistic(ball, from, to, 2);
    check(
        burstRandomNative[0].engine.index == 1 && effectSeed == oldEffect &&
            other == hashBytes(&burstRandomNative[1].engine, sizeof(NativeParticleRng)) &&
            source == hashBytes(&sourceRandom, sizeof(sourceRandom)),
        "Campaign frog rotation consumes its scene MT without a cosmetic LCG draw or a change to the independent UI domain");
    int previous = burstRandomNative[0].engine.index;
    ball.state.timer = 1;
    float retained = ball.state.aux;
    bool refused = !frogBallistic(ball, from, to, 2) &&
                   burstRandomNative[0].engine.index == previous && ball.state.aux == retained;
    check(
        refused,
        "A nonpositive remaining flight time rejects recalculation without consuming a spin sample or changing retained angular speed");
    resetBurstRandomNative(0, 5489);
    ball.state.timer = .25f;
    frogBallistic(ball, from, to, 2);
    bodies[index]->SetAngularVelocity(ball.state.aux);
    captureCheckpoint(false);
    float saved = ball.state.aux;
    frogBallistic(ball, from, to, 2);
    restoreCheckpoint();
    bool restored = ball.state.aux == saved && bodies[index]->GetAngularVelocity() == saved;
    ball.state.timer = .25f;
    frogBallistic(ball, from, to, 2);
    check(
        restored && frogSpinValueBitsNative(ball.state.aux) == expected[2] &&
            burstRandomNative[0].engine.index == 3,
        "Checkpoint restores existing frog spin while the level random stream continues live rather than rewinding to the saved draw");
    u32 campaign = hashBytes(&burstRandomNative[0].engine, sizeof(NativeParticleRng));
    interfaceEnabled = true;
    navigate(137502285691ULL);
    bool ending = creditsRuntime.frogBall >= 0;
    if (ending) {
        Entity &cast = page.playfield[creditsRuntime.frogBall];
        resetBurstRandomNative(1, 5489);
        for (int i = 0; i < 4; i++) {
            cast.state.timer = .25f;
            ending &= creditsFrogBallistic(cast, from, to, 2) &&
                      frogSpinValueBitsNative(cast.state.aux) == expected[i];
        }
        ending &= burstRandomNative[1].engine.index == 4;
    }
    check(
        ending && campaign == hashBytes(&burstRandomNative[0].engine, sizeof(NativeParticleRng)),
        "Actual ending frog uses the same native spin distribution and its own MT stream without advancing campaign randomness");
    interfaceEnabled = false;
    startStage(0);
    readyTimer = 0;
    atomicWrite("test-output/frog-spin-results.txt", testLog, lstrlenA(testLog));
}
